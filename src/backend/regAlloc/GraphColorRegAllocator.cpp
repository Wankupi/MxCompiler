#include "GraphColorRegAllocator.h"
#include "ASM/Node.h"
#include "backend/RewriteLayer.h"
#include "backend/regAlloc/LiveAnalyzer.h"
#include "utils/Graph.h"
#include <ranges>
#include <stack>

using namespace ASM;

struct RegPtrCmp {
	bool operator()(const Reg *lhs, const Reg *rhs) const {
		if (!lhs || !rhs)
			return lhs < rhs;
		return lhs->name < rhs->name;
	}
};

using RegSet = std::set<Reg *, RegPtrCmp>;

struct MoveInstCmp {
	bool operator()(const MoveInst *lhs, const MoveInst *rhs) const {
		if (!lhs || !rhs)
			return lhs < rhs;
		return lhs->rd->name < rhs->rd->name || (lhs->rd->name == rhs->rd->name && lhs->rs->name < rhs->rs->name);
	}
};

struct ConflictGraph {
	/**
	 * Graph part
	 */
	std::map<Reg *, RegSet> graph;
	std::map<Reg *, RegSet> cutGraph;
	std::set<std::pair<Reg *, Reg *>> edges;
	std::set<std::pair<Reg *, Reg *>> cutEdges;

	bool has(Reg *a, Reg *b) const {
		if (a > b) std::swap(a, b);
		//		bool x = edges.contains({a, b});
		//		bool y = cutEdges.contains({a, b});
		return edges.contains({a, b}) && !cutEdges.contains({a, b});
	}
	bool isConflict(Reg *a, Reg *b) const {
		if (a > b) std::swap(a, b);
		return edges.contains({a, b});
	}
	void add(Reg *a, Reg *b) {
		if (a == b) return;
		if (a > b) std::swap(a, b);
		if (has(a, b)) return;
		graph[a].insert(b);
		graph[b].insert(a);
		cutGraph[a].erase(b);
		cutGraph[b].erase(a);
		edges.insert({a, b});
		cutEdges.erase({a, b});
	}
	bool cut(Reg *a, Reg *b) {
		if (a > b) std::swap(a, b);
		if (!has(a, b)) return false;
		graph[a].erase(b);
		cutGraph[a].insert(b);
		graph[b].erase(a);
		cutGraph[b].insert(a);
		cutEdges.insert({a, b});
		return true;
	}
	void remove(Reg *a, Reg *b) {
		if (a > b) std::swap(a, b);
		graph[a].erase(b);
		graph[b].erase(a);
		cutGraph[a].erase(b);
		cutGraph[b].erase(a);
		edges.erase({a, b});
		cutEdges.erase({a, b});
	}
	void merge_to(Reg *a, Reg *b) {
		if (a == b)
			return;
		auto &g = graph[a];
		while (!g.empty()) {
			Reg *to = *g.begin();
			remove(a, to);
			add(b, to);
		}
		auto &gg = cutGraph[a];
		while (!gg.empty()) {
			Reg *to = *gg.begin();
			remove(a, to);
			if (has(b, to)) continue;
			add(b, to);
			cut(b, to);
		}
	}
	int deg(Reg *a) {
		return static_cast<int>(graph[a].size());
	}
	void clear() {
		*this = ConflictGraph();
	}
	RegSet &operator[](Reg *reg) { return graph[reg]; }
	[[nodiscard]] std::map<Reg *, RegSet> full() const {
		std::map<Reg *, RegSet> ret;
		for (auto &[reg, tos]: graph)
			ret[reg].insert(tos.begin(), tos.end());
		for (auto &[reg, tos]: cutGraph)
			ret[reg].insert(tos.begin(), tos.end());
		return ret;
	}
	RegSet full(Reg *reg) {
		RegSet ret(graph[reg]);
		ret.insert(cutGraph[reg].begin(), cutGraph[reg].end());
		return ret;
	}
};


using MvInstSet = std::set<MoveInst *, MoveInstCmp>;

class Allocator {
	/// @attention 以下成员在构造时初始化

	ValueAllocator *regs;
	Function *func;

	static constexpr int K = 27;

	std::vector<PhysicalReg *> allocatable;// 可分配的物理寄存器
	std::vector<PhysicalReg *> callerSave;

	/// @attention 以下成员在每次循环时都应清空

	std::map<Block *, std::set<Reg *>> liveIn;
	std::map<Block *, std::set<Reg *>> liveOut;

	MvInstSet moves;
	std::map<Reg *, MvInstSet> moveList;

	ConflictGraph graph;

	RegSet preColored;
	RegSet otherRegs;

	RegSet simplifyWorkList;
	/// @brief 待合并的 mv 指令
	/// @details 必不可行的直接加入simple; 不符合George,Briggs规则的被冻结; 否则合并
	MvInstSet coalesceWorkList;
	RegSet freezeWorkList;
	RegSet spillWorkList;

	MvInstSet frozenMoves;// 不满足规则 暂时不考虑

	std::stack<Reg *> selectStack;
	RegSet selected;
	std::map<Reg *, Reg *> alias;
	std::map<Reg *, PhysicalReg *> color;

	RegSet spilledNodes;

public:
	Allocator(ValueAllocator *regs, Function *func) : regs(regs), func(func) {
		for (int i = 5; i < 32; ++i)
			allocatable.push_back(regs->get(i));
		for (int i = 5; i <= 7; ++i)
			callerSave.push_back(regs->get(i));
		for (int i = 10; i <= 17; ++i)
			callerSave.push_back(regs->get(i));
		for (int i = 28; i <= 31; ++i)
			callerSave.push_back(regs->get(i));
	}
	void work();

private:
	void clear();
	void init();
	void liveAnalyze();
	void buildGraph();
	//	int deg(Reg *reg) { return graph[reg].size(); }

	//	void add_edge(Reg *a, Reg *b);
	//	void remove_edge(Reg *a, Reg *b);
	void remove_from_graph(Reg *reg);
	void makeWorkList();
	void remove_from_all_worklist(Reg *reg);
	/// @brief reg 作为rd或rs的 仍有coalesce希望(未处理和冻结)的 mv指令
	std::set<MoveInst *> getRelatedMoves(Reg *reg);
	bool isMoveRelated(Reg *reg);
	void checkDegree(Reg *reg);

	bool George(MoveInst *mv);
	bool Briggs(MoveInst *mv);

	void simplify();
	void coalesce();
	void freeze();
	void selectSpill();

	void freezeMoves(Reg *reg);
	Reg *getRepresent(Reg *reg);
	// try to add reg to `simplifyWorkList` if 1. need color, 2. not move related
	void tryAddToSimplify(Reg *reg);
	void recoverMove(Reg *reg);
	void recoverMove(MoveInst *mv);

	void assignColors();
	void rewriteProgram();
	void replaceVirToPhy();
};

void GraphColorRegAllocator::work(ASM::Module *module) {
	for (auto func: module->functions) {
		Allocator(regs, func).work();
	}
}

void Allocator::work() {
	do {
		clear();
		init();
		liveAnalyze();
		buildGraph();
		makeWorkList();

		while (!simplifyWorkList.empty() || !coalesceWorkList.empty() || !freezeWorkList.empty() || !spillWorkList.empty()) {
			if (!simplifyWorkList.empty())
				simplify();// 1. 尝试简化
			else if (!coalesceWorkList.empty())
				coalesce();// 2. 尝试合并
			else if (!freezeWorkList.empty())
				freeze();// 3. 尝试冻结mv变量
			else if (!spillWorkList.empty())
				selectSpill();// 4. 尝试存栈
		}

		assignColors();
		if (!spilledNodes.empty()) {
			rewriteProgram();
		}
	} while (!spilledNodes.empty());

	replaceVirToPhy();
}

void Allocator::clear() {
	liveIn.clear();
	liveOut.clear();
	moves.clear();
	moveList.clear();
	graph.clear();
	preColored.clear();
	otherRegs.clear();
	simplifyWorkList.clear();
	coalesceWorkList.clear();
	freezeWorkList.clear();
	spillWorkList.clear();
	frozenMoves.clear();
	selectStack = std::stack<Reg *>();
	selected.clear();
	alias.clear();
	color.clear();
	spilledNodes.clear();
}

void Allocator::init() {
	for (auto &reg: regs->getRegs()) {
		preColored.insert(&reg);
		color[&reg] = &reg;
	}

	for (auto block: func->blocks)
		for (auto inst: block->stmts) {
			auto def = inst->getDef();
			auto use = inst->getUse();
			otherRegs.insert(def.begin(), def.end());
			otherRegs.insert(use.begin(), use.end());
		}
	for (auto reg: preColored) otherRegs.erase(reg);

	for (auto block: func->blocks)
		for (auto inst: block->stmts)
			if (auto mv = dynamic_cast<MoveInst *>(inst)) {
				moveList[mv->rd].insert(mv);
				moveList[mv->rs].insert(mv);
				moves.insert(mv);
			}
}

void Allocator::liveAnalyze() {
	LiveAnalyzer analyzer(func);
	analyzer.work();
	liveIn.swap(analyzer.liveIn);
	liveOut.swap(analyzer.liveOut);
}

void Allocator::buildGraph() {
	for (auto block: func->blocks) {
		auto live = liveOut[block];
		for (auto inst: std::ranges::reverse_view(block->stmts)) {
			if (auto mv = dynamic_cast<MoveInst *>(inst))
				live.erase(mv->rs);

			for (auto def: inst->getDef())
				for (auto reg: live)
					graph.add(def, reg);
			if (auto call = dynamic_cast<CallInst *>(inst)) {
				for (auto def: regs->CallerSave)
					for (auto reg: live)
						graph.add(def, reg);
			}
			auto def = inst->getDef();
			auto use = inst->getUse();
			erase_if(live, [&def](auto reg) { return def.contains(reg); });
			live.insert(use.begin(), use.end());
		}
	}
}

void Allocator::remove_from_graph(Reg *reg) {
	auto &adj = graph[reg];
	while (!adj.empty()) {
		auto to = *adj.begin();
		if (graph.cut(reg, to)) {
			checkDegree(to);
			checkDegree(reg);
		}
	}
}

void Allocator::remove_from_all_worklist(Reg *reg) {
	simplifyWorkList.erase(reg);
	freezeWorkList.erase(reg);
	spillWorkList.erase(reg);
}

void Allocator::checkDegree(Reg *reg) {
	// 刚刚由 K 变为 K - 1
	if (graph.deg(reg) != K - 1) return;
	spillWorkList.erase(reg);
	if (isMoveRelated(reg))
		recoverMove(reg);
	else
		simplifyWorkList.insert(reg);
}

void Allocator::makeWorkList() {
	// this assign must do first, because isMoveRelated rely on coalesceWorkList
	coalesceWorkList = moves;
	for (auto reg: otherRegs) {
		if (graph.deg(reg) >= K)
			spillWorkList.insert(reg);
		else if (isMoveRelated(reg))
			;
		else
			simplifyWorkList.insert(reg);
	}
}

void Allocator::simplify() {
	while (!simplifyWorkList.empty()) {
		auto reg = *simplifyWorkList.begin();
		simplifyWorkList.erase(simplifyWorkList.begin());

		if (preColored.contains(reg)) continue;
		if (selected.contains(reg)) continue;
		if (getRepresent(reg) != reg) continue;// 跳过已经被合并的点

		selectStack.push(reg);
		selected.insert(reg);

		remove_from_graph(reg);
	}
}

std::set<MoveInst *> Allocator::getRelatedMoves(Reg *reg) {
	std::set<MoveInst *> mvs;
	for (auto mv: moveList[reg])
		if (coalesceWorkList.contains(mv) || frozenMoves.contains(mv))
			mvs.insert(mv);
	return mvs;
}

bool Allocator::isMoveRelated(Reg *reg) {
	// reg 是否参与到了未被冻结的 mv 中
	return !getRelatedMoves(reg).empty();
}

bool Allocator::Briggs(MoveInst *mv) {
	RegSet adj = graph[mv->rd];
	int cntHigh = 0;
	for (auto reg: graph[mv->rs]) {
		if (adj.contains(reg)) {
			adj.erase(reg);
			if (graph.deg(reg) - 1 >= K)
				++cntHigh;
		}
		else
			adj.insert(reg);
	}
	for (auto reg: adj)
		if (graph.deg(reg) >= K)
			++cntHigh;
	return cntHigh < K;
}

bool Allocator::George(MoveInst *mv) {
	for (auto reg: graph[mv->rs])
		if (!graph.has(reg, mv->rd) && graph.deg(reg) >= K)
			return false;
	return true;
}

void Allocator::coalesce() {
	auto mv = *coalesceWorkList.begin();
	coalesceWorkList.erase(mv);

	auto rd = getRepresent(mv->rd), rs = getRepresent(mv->rs);
	if (preColored.contains(rs))
		std::swap(rd, rs);
	if (rd == rs || preColored.contains(rs) || graph.has(rd, rs) ||
		rd == regs->get("zero") || rs == regs->get("zero")) {
		tryAddToSimplify(rd);
		tryAddToSimplify(rs);
		return;
	}

	if (!George(mv) && !Briggs(mv)) {
		frozenMoves.insert(mv);
		freezeWorkList.insert(rd);
		freezeWorkList.insert(rs);
		return;
	}

	// merge rs to rd
	alias[rs] = rd;
	// merge mv s
	moveList[rd].merge(moveList[rs]);
	// merge edges
	graph.merge_to(rs, rd);
	tryAddToSimplify(rd);
	remove_from_all_worklist(rs);
}

void Allocator::freezeMoves(Reg *reg) {
	for (auto mv: getRelatedMoves(reg)) {
		auto x = getRepresent(mv->rd), y = getRepresent(mv->rs);
		auto other = x == getRepresent(reg) ? y : x;

		frozenMoves.insert(mv);
		// coalesceWorkList.erase(mv);  // coalesceWorkList must be empty now.
		if (getRelatedMoves(other).empty() && graph.deg(other) < K) {
			// other mustn't in spillWorkList
			freezeWorkList.erase(other);
			simplifyWorkList.insert(other);
		}
	}
}

void Allocator::recoverMove(MoveInst *mv) {
	frozenMoves.erase(mv);
	coalesceWorkList.insert(mv);
	freezeWorkList.erase(mv->rs);
	freezeWorkList.erase(mv->rd);
}

void Allocator::recoverMove(Reg *reg) {
	auto mvs = getRelatedMoves(reg);
	for (auto mv: mvs)
		recoverMove(mv);
}

void Allocator::freeze() {
	auto reg = *freezeWorkList.begin();
	freezeWorkList.erase(reg);
	freezeMoves(reg);
	simplifyWorkList.insert(reg);
}

Reg *Allocator::getRepresent(Reg *reg) {
	if (!alias[reg]) alias[reg] = reg;
	return alias[reg] == reg ? reg : alias[reg] = getRepresent(alias[reg]);
}

void Allocator::tryAddToSimplify(Reg *reg) {
	if (preColored.contains(reg)) return;
	if (isMoveRelated(reg)) return;
	if (graph.deg(reg) >= K) {
		spillWorkList.insert(reg);
		return;
	}
	if (getRepresent(reg) != reg)
		std::cerr << "[[ warning : add " << reg->to_string() << " ]]" << std::endl;
	freezeWorkList.erase(reg);
	spilledNodes.erase(reg);
	simplifyWorkList.insert(reg);
}

void Allocator::selectSpill() {
	Reg *reg = nullptr;
	for (auto r: spillWorkList)
		if (!reg || graph.deg(r) > graph.deg(reg))
			reg = r;
	spillWorkList.erase(reg);
	freezeMoves(reg);
	simplifyWorkList.insert(reg);
}

void Allocator::assignColors() {
	auto G = graph.full();
	while (!selectStack.empty()) {
		auto reg = selectStack.top();
		selectStack.pop();
		if (preColored.contains(reg)) continue;
		RegSet exist;
		for (auto y: G[reg])
			exist.insert(color[y]);// color[y] might be nullptr, but no influence
		for (auto phy: allocatable)
			if (!exist.contains(phy)) {
				color[reg] = phy;
				break;
			}
		if (!color[reg])
			spilledNodes.insert(reg);
	}
	for (auto [reg, _]: alias)
		getRepresent(reg);

	for (auto [reg, rep]: alias)
		color[reg] = color[rep];
}

#include "Rewriter.h"

void Allocator::rewriteProgram() {
	std::map<Reg *, StackVal *> reg2st;
	for (auto reg: spilledNodes) {
		auto st = new StackVal{};
		func->stack.push_back(st);
		reg2st[reg] = st;
	}
	PutStack(func, reg2st, alias, regs).work();
}

void Allocator::replaceVirToPhy() {
	ColorIt(func, color).work();
}
