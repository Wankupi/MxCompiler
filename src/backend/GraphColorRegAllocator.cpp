#include "GraphColorRegAllocator.h"
#include "ASM/Node.h"
#include "LiveAnalyzer.h"
#include "RewriteLayer.h"
#include "utils/Graph.h"
#include <ranges>
#include <stack>

using namespace ASM;

#define RED "\033[31m"
#define GREEN "\033[32m"
#define BLACK "\033[0m"

template<typename T, typename N>
void erase1(std::multiset<T> &s, N &&val) {
	auto p = s.find(std::forward<N>(val));
	if (p != s.end())
		s.erase(p);
}

struct ConflictGraph {
	/**
	 * Graph part
	 */
	std::map<Reg *, std::set<Reg *>> graph;
	std::map<Reg *, std::set<Reg *>> cutGraph;
	std::set<std::pair<Reg *, Reg *>> edges;
	std::set<std::pair<Reg *, Reg *>> cutEdges;
	std::map<Reg *, int> deg;// 无向图

	bool has(Reg *a, Reg *b) const {
		if (a > b) std::swap(a, b);
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
		++deg[a];
		++deg[b];
		edges.insert({a, b});
	}
	void cut(Reg *a, Reg *b) {
		if (a > b) std::swap(a, b);
		if (!has(a, b)) return;
		graph[a].erase(b);
		cutGraph[a].insert(b);
		graph[b].erase(a);
		cutGraph[b].insert(a);
		cutEdges.insert({a, b});
		--deg[a];
		--deg[b];
	}
	void clear() {
		*this = ConflictGraph();
	}
	std::set<Reg *> &operator[](Reg *reg) { return graph[reg]; }
	[[nodiscard]] std::map<Reg *, std::set<Reg *>> full() const {
		std::map<Reg *, std::set<Reg *>> ret;
		for (auto &[reg, tos]: graph)
			ret[reg].insert(tos.begin(), tos.end());
		for (auto &[reg, tos]: cutGraph)
			ret[reg].insert(tos.begin(), tos.end());
		return ret;
	}
	std::set<Reg *> full(Reg *reg) {
		std::set<Reg *> ret(graph[reg]);
		ret.insert(cutGraph[reg].begin(), cutGraph[reg].end());
		return ret;
	}
};

class Allocator {
	ValueAllocator *regs;
	Function *func;

	static constexpr int K = 27;
	std::vector<PhysicalReg *> allocatable;// 可分配的物理寄存器

	std::map<Block *, std::set<Reg *>> liveIn;
	std::map<Block *, std::set<Reg *>> liveOut;

	std::set<MoveInst *> moves;
	std::map<Reg *, std::set<MoveInst *>> moveList;

	ConflictGraph graph;

	std::set<Reg *> preColored;
	std::set<Reg *> otherRegs;

	std::set<Reg *> simplifyWorkList;
	/// @brief 待合并的 mv 指令
	/// @details 必不可行的直接加入simple; 不符合George,Briggs规则的被冻结; 否则合并
	std::set<MoveInst *> coalesceWorkList;
	std::set<Reg *> freezeWorkList;
	std::set<Reg *> spillWorkList;

	std::set<MoveInst *> frozenMoves;// 不满足规则 暂时不考虑

	std::stack<Reg *> selectStack;
	std::set<Reg *> selected;
	std::map<Reg *, Reg *> alias;
	std::map<Reg *, PhysicalReg *> color;

	std::set<Reg *> spilledNodes;

public:
	Allocator(ValueAllocator *regs, Function *func) : regs(regs), func(func) {}
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
	/// @brief reg 作为rd或rs的、未冻结的mv指令
	std::set<MoveInst *> getRelatedMoves(Reg *reg);
	bool isMoveRelated(Reg *reg);
	void decreaseDegree(Reg *reg);

	bool George(MoveInst *mv);
	bool Briggs(MoveInst *mv);

	void simplify();
	void coalesce();
	void freeze();
	void selectSpill() {}

	void freezeMoves(Reg *reg);
	Reg *getRepresent(Reg *reg);
	// try to add reg to `simplifyWorkList` if 1. need color, 2. not move related
	void tryAddToSimplify(Reg *reg);

	void assignColors();
	void rewriteProgram();
	void replaceVirToPhy();
};

void GraphColorRegAllocator::work(ASM::Module *module) {
	for (auto func: module->functions) {
		std::cout << RED "Function " << func->name << BLACK "\n";
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

		std::cout << "simplify: ";
		for (auto reg: simplifyWorkList)
			std::cout << reg->to_string() << ' ';
		std::cout << '\n';

		std::cout << "move: ";
		for (auto &[reg, _]: moveList)
			if (!_.empty())
				std::cout << reg->to_string() << ' ';
		std::cout << '\n';

		std::cout << "freeze: ";
		for (auto reg: freezeWorkList)
			std::cout << reg->to_string() << ' ';
		std::cout << '\n';

		std::cout << "spill: ";
		for (auto reg: spillWorkList)
			std::cout << reg->to_string() << ' ';
		std::cout << '\n';
		std::cout << std::flush;

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
		if (!spilledNodes.empty())
			rewriteProgram();
	} while (!spilledNodes.empty());

	std::cout << std::flush;
	replaceVirToPhy();

	for (auto [reg, phy]: color) {
		if (dynamic_cast<PhysicalReg *>(reg)) continue;
		std::cout << reg->to_string() << " : " << (phy ? phy->to_string() : "null") << '\n';
	}
}

void Allocator::clear() {
	moveList.clear();
	coalesceWorkList.clear();
	graph.clear();
	preColored.clear();
	otherRegs.clear();
	color.clear();
}

void Allocator::init() {
	for (auto &reg: regs->getRegs()) {
		preColored.insert(&reg);
		color[&reg] = &reg;
	}
	for (int i = 5; i < 32; ++i)
		allocatable.push_back(regs->get(i));

	for (auto block: func->blocks)
		for (auto inst: block->stmts) {
			auto def = inst->getDef();
			auto use = inst->getUse();
			if (def) otherRegs.insert(def);
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
		//		std::cout << "\033[32m Block " << block->label << "\033[0m\n";

		auto live = liveOut[block];
		for (auto inst: std::ranges::reverse_view(block->stmts)) {
			//			std::cout << "at {\033[32m" << inst->to_string() << "\033[0m}\t";
			//			for (auto reg: live)
			//				std::cout << reg->to_string() << ' ';
			//			std::cout << std::endl;

			if (auto mv = dynamic_cast<MoveInst *>(inst)) {
				live.erase(mv->rs);
			}
			if (auto def = inst->getDef())
				for (auto reg: live)
					graph.add(def, reg);
			live.erase(inst->getDef());
			auto use = inst->getUse();
			live.insert(use.begin(), use.end());
		}
	}
	//	for (auto &[reg, tos]: graph)
	//		deg[reg] = static_cast<int>(tos.size());
}

void Allocator::remove_from_graph(Reg *reg) {
	auto &adj = graph[reg];
	while (!adj.empty()) {
		auto to = *adj.begin();
		graph.cut(reg, to);
		tryAddToSimplify(to);
	}
}

void Allocator::decreaseDegree(Reg *reg) {
	//	--deg[reg];
	//	if (deg[reg] == K - 1)
}


void Allocator::makeWorkList() {
	// this assign must do first, because isMoveRelated rely on coalesceWorkList
	coalesceWorkList = moves;
	for (auto reg: otherRegs) {
		if (graph.deg[reg] >= K)
			spillWorkList.insert(reg);
		else if (isMoveRelated(reg))
			//			toMergeRegWorkList.insert(reg);
			;
		else
			simplifyWorkList.insert(reg);
	}
}

void Allocator::simplify() {
	while (!simplifyWorkList.empty()) {
		auto reg = *simplifyWorkList.begin();
		simplifyWorkList.erase(simplifyWorkList.begin());
		selectStack.push(reg);
		selected.insert(reg);

		std::cout << RED "simplify" BLACK " < " << reg->to_string() << " >" << std::endl;

		remove_from_graph(reg);
	}
}

std::set<MoveInst *> Allocator::getRelatedMoves(Reg *reg) {
	//	auto mvs = moveList[reg];
	auto mvs = coalesceWorkList;
	for (auto mv: frozenMoves)
		mvs.erase(mv);
	for (auto cur = mvs.begin(); cur != mvs.end();) {
		auto p = cur++;
		if (!moveList[reg].contains(*p))
			mvs.erase(p);
	}
	return mvs;
}

bool Allocator::isMoveRelated(Reg *reg) {
	// reg 是否参与到了未被冻结的 mv 中
	return !getRelatedMoves(reg).empty();
}

//void Allocator::decreaseDegree(Reg *reg) {
//	if (preColored.contains(reg))
//		return;
//	--deg[reg];
//	if (deg[reg] == K - 1) {
//		spillWorkList.erase(reg);
//		simplifyWorkList.insert(reg);
//	}
//}

bool Allocator::Briggs(MoveInst *mv) {
	std::set<Reg *> adj = graph[mv->rd];
	int cntHigh = 0;
	for (auto reg: graph[mv->rs]) {
		if (adj.contains(reg)) {
			adj.erase(reg);
			if (graph.deg[reg] - 1 >= K)
				++cntHigh;
		}
		else
			adj.insert(reg);
	}
	for (auto reg: adj)
		if (graph.deg[reg] >= K)
			++cntHigh;
	return cntHigh < K;
}

bool Allocator::George(MoveInst *mv) {
	for (auto reg: graph[mv->rs])
		if (!graph.has(reg, mv->rd) && graph.deg[reg] >= K)
			return false;
	return true;
}

void Allocator::coalesce() {
	auto mv = *coalesceWorkList.begin();
	coalesceWorkList.erase(mv);
	//	erase1(toMergeRegWorkList, mv->rd);
	//	erase1(toMergeRegWorkList, mv->rs);
	auto rd = getRepresent(mv->rd), rs = getRepresent(mv->rs);
	if (preColored.contains(rs))
		std::swap(rd, rs);
	if (rd == rs || preColored.contains(rs) || graph.has(rd, rs) ||
		rd == regs->get("zero") || rs == regs->get("zero")) {
		//		frozenMoves.insert(mv);
		tryAddToSimplify(rd);
		tryAddToSimplify(rs);
		return;
	}

	if (!George(mv) && !Briggs(mv)) {
		frozenMoves.insert(mv);
		return;
	}
	std::cout << RED "merge" BLACK " < " << rs->to_string() << " > to < " << rd->to_string() << " >" << std::endl;

	std::cout << "adj of < " << rs->to_string() << " > : ";
	for (auto x: graph[rs])
		std::cout << x->to_string() << ' ';
	std::cout << std::endl;
	std::cout << "adj of < " << rd->to_string() << " > : ";
	for (auto x: graph[rd])
		std::cout << x->to_string() << ' ';
	std::cout << std::endl;

	// merge rs to rd
	alias[rs] = rd;
	// merge mv s
	moveList[rd].merge(moveList[rs]);
	// merge edges
	for (auto to: graph[rs])
		graph.add(rd, to);
	remove_from_graph(rs);
	tryAddToSimplify(rd);

	std::cout << "adj of < " << rs->to_string() << " > : ";
	for (auto x: graph[rs])
		std::cout << x->to_string() << ' ';
	std::cout << std::endl;
	std::cout << "adj of < " << rd->to_string() << " > : ";
	for (auto x: graph[rd])
		std::cout << x->to_string() << ' ';
	std::cout << std::endl;
}

void Allocator::freezeMoves(Reg *reg) {
	for (auto mv: getRelatedMoves(reg)) {
		auto x = getRepresent(mv->rd), y = getRepresent(mv->rs);
		auto other = x == getRepresent(reg) ? y : x;

		frozenMoves.insert(mv);
		if (getRelatedMoves(other).empty() && graph.deg[other] < K) {
			freezeWorkList.erase(other);
			simplifyWorkList.insert(other);
		}
	}
}

void Allocator::freeze() {
	auto reg = *freezeWorkList.begin();
	freezeWorkList.erase(reg);
	simplifyWorkList.insert(reg);
	freezeMoves(reg);
}

Reg *Allocator::getRepresent(Reg *reg) {
	if (!alias[reg]) alias[reg] = reg;
	return alias[reg] == reg ? reg : alias[reg] = getRepresent(alias[reg]);
}

void Allocator::tryAddToSimplify(Reg *reg) {
	//	std::cout << "try add < " << reg->to_string() << " > to sim, " << preColored.contains(reg) << " " << getRelatedMoves(reg).size() << " " << graph.deg[reg] << std::endl;
	if (preColored.contains(reg)) return;
	if (isMoveRelated(reg)) return;
	if (graph.deg[reg] >= K) return;
	freezeWorkList.erase(reg);
	spilledNodes.erase(reg);
	simplifyWorkList.insert(reg);
	std::cout << GREEN "success" BLACK << std::endl;
}

void Allocator::assignColors() {
	auto G = graph.full();
	while (!selectStack.empty()) {
		auto reg = selectStack.top();
		selectStack.pop();
		std::set<Reg *> exist;
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

	for (auto [reg, phy]: color)
		if (!dynamic_cast<PhysicalReg *>(reg) || !phy)
			std::cout << reg->to_string() << " : " << (phy ? phy->to_string() : "null") << std::endl;
}

void Allocator::rewriteProgram() {
	std::cout << "rewrite: ";
	for (auto reg: spilledNodes)
		std::cout << reg->to_string() << ' ';
	std::cout << std::endl;
}

class DoColor : public ASM::RewriteLayer {
	Function *func;
	std::map<Reg *, PhysicalReg *> const &color;

public:
	explicit DoColor(Function *func, std::map<Reg *, PhysicalReg *> const &color) : func(func), color(color) {}
	void work() {
		visitFunction(func);
	}

private:
	void rep(Reg *&reg) {
		if (!color.contains(reg))
			throw std::runtime_error("DoColor: fail to replace " + reg->to_string());
		reg = color.at(reg);
	}
	void rep(Val *&val) {
		auto reg = dynamic_cast<Reg *>(val);
		if (!reg) return;
		rep(reg);
		val = reg;
	}
	void visitLuiInst(ASM::LuiInst *inst) override {
		rep(inst->rd);
		add_inst(inst);
	}
	void visitLiInst(ASM::LiInst *inst) override {
		rep(inst->rd);
		add_inst(inst);
	}
	void visitLaInst(ASM::LaInst *inst) override {
		rep(inst->rd);
		add_inst(inst);
	}
	void visitSltInst(ASM::SltInst *inst) override {
		rep(inst->rd);
		rep(inst->rs1);
		rep(inst->rs2);
		add_inst(inst);
	}
	void visitBinaryInst(ASM::BinaryInst *inst) override {
		rep(inst->rd);
		rep(inst->rs1);
		rep(inst->rs2);
		add_inst(inst);
	}
	void visitMulDivRemInst(ASM::MulDivRemInst *inst) override {
		rep(inst->rd);
		rep(inst->rs1);
		rep(inst->rs2);
		add_inst(inst);
	}
	void visitMoveInst(ASM::MoveInst *inst) override {
		rep(inst->rd);
		rep(inst->rs);
		if (inst->rd != inst->rs)
			add_inst(inst);
	}
	void visitStoreOffset(ASM::StoreOffset *inst) override {
		rep(inst->dst);
		rep(inst->val);
		add_inst(inst);
	}
	void visitStoreSymbol(ASM::StoreSymbol *inst) override {
		rep(inst->val);
		add_inst(inst);
	}
	void visitLoadOffset(ASM::LoadOffset *inst) override {
		rep(inst->rd);
		rep(inst->src);
		add_inst(inst);
	}
	void visitLoadSymbol(ASM::LoadSymbol *inst) override {
		rep(inst->rd);
		add_inst(inst);
	}
	void visitBranchInst(ASM::BranchInst *inst) override {
		rep(inst->rs1);
		rep(inst->rs2);
		add_inst(inst);
	}
};

void Allocator::replaceVirToPhy() {
	DoColor(func, color).work();
}
