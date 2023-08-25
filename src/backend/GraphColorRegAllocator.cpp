#include "GraphColorRegAllocator.h"
#include "ASM/Node.h"
#include "LiveAnalyzer.h"
#include "RewriteLayer.h"
#include "utils/Graph.h"
#include <ranges>
#include <stack>

using namespace ASM;

class Allocator {
	ValueAllocator *regs;
	Function *func;

	static constexpr int K = 27;
	std::vector<PhysicalReg *> allocatable;// 可分配的物理寄存器

	std::map<Block *, std::set<Reg *>> liveIn;
	std::map<Block *, std::set<Reg *>> liveOut;

	std::set<MoveInst *> moves;
	std::map<Reg *, std::set<MoveInst *>> moveList;


	std::map<Reg *, std::set<Reg *>> graph;
	std::set<std::pair<Reg *, Reg *>> edges;
	//	std::map<Reg *, int> deg;// out degree

	std::set<Reg *> preColored;
	std::set<Reg *> otherRegs;

	std::set<Reg *> simplifyWorkList;
	std::set<MoveInst *> coalesceWorkList;// 待合并的 mv 指令
	std::set<Reg *> freezeWorkList;
	std::set<Reg *> spillWorkList;

	std::set<MoveInst *> frozenMoves;
	std::set<MoveInst *> pauseMoves;// 不满足规则 暂时不考虑

	std::stack<Reg *> selectStack;
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
	int deg(Reg *reg) { return graph[reg].size(); }
	bool has_edge(Reg *a, Reg *b) { return edges.contains({a, b}) || edges.contains({b, a}); }
	void add_edge(Reg *a, Reg *b);
	void remove_edge(Reg *a, Reg *b);
	void remove_from_graph(Reg *reg);
	void makeWorkList();
	/// @brief reg 作为rd或rs的、未冻结的mv指令
	std::set<MoveInst *> getRelatedMoves(Reg *reg);
	bool isMoveRelated(Reg *reg);
	//	void decreaseDegree(Reg *reg);

	bool George(MoveInst *mv);
	bool Briggs(MoveInst *mv);

	void simplify();
	void coalesce();
	void freeze();
	void selectSpill() {}

	Reg *getRepresent(Reg *reg);
	// try to add reg to `simplifyWorkList` if 1. need color, 2. not move related
	void tryAddToSimplify(Reg *reg);

	void assignColors();
	void rewriteProgram();
	void replaceVirToPhy();
};

void GraphColorRegAllocator::work(ASM::Module *module) {
	for (auto func: module->functions) {
		std::cout << "\033[31m Function " << func->name << "\033[0m\n";
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
	edges.clear();
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
					add_edge(def, reg);
			live.erase(inst->getDef());
			auto use = inst->getUse();
			live.insert(use.begin(), use.end());
		}
	}
}

void Allocator::add_edge(Reg *a, Reg *b) {
	if (a == b) return;
	if (has_edge(a, b)) return;
	edges.insert({a, b});
	if (!preColored.contains(a))
		graph[a].insert(b);
	if (!preColored.contains(b))
		graph[b].insert(a);
	std::cout << "\t( " << a->to_string() << ", " << b->to_string() << " )" << '\n';
}

void Allocator::remove_edge(Reg *a, Reg *b) {
	graph[a].erase(b);
	graph[b].erase(a);
	edges.erase({a, b});
	edges.erase({b, a});
}

void Allocator::remove_from_graph(Reg *reg) {
	auto &adj = graph[reg];
	while (!adj.empty()) {
		auto to = *adj.begin();
		remove_edge(reg, to);
		tryAddToSimplify(to);
	}
}

void Allocator::makeWorkList() {
	for (auto reg: otherRegs) {
		if (deg(reg) >= K)
			spillWorkList.insert(reg);
		else if (isMoveRelated(reg))
			;
		else
			simplifyWorkList.insert(reg);
	}
	coalesceWorkList = moves;
}

void Allocator::simplify() {
	while (!simplifyWorkList.empty()) {
		auto reg = *simplifyWorkList.begin();
		simplifyWorkList.erase(simplifyWorkList.begin());
		selectStack.push(reg);

		std::cout << "\033[31msimplify\033[0m < " << reg->to_string() << " >" << std::endl;

		remove_from_graph(reg);
	}
}

std::set<MoveInst *> Allocator::getRelatedMoves(Reg *reg) {
	auto mvs = moveList[reg];
	for (auto mv: frozenMoves)
		mvs.erase(mv);
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
			if (deg(reg) - 1 >= K)
				++cntHigh;
		}
		else
			adj.insert(reg);
	}
	for (auto reg: adj)
		if (deg(reg) >= K)
			++cntHigh;
	return cntHigh < K;
}

bool Allocator::George(MoveInst *mv) {
	for (auto reg: graph[mv->rs])
		if (!has_edge(reg, mv->rd) && deg(reg) >= K)
			return false;
	return true;
}

void Allocator::coalesce() {
	auto mv = *coalesceWorkList.begin();
	coalesceWorkList.erase(mv);
	auto rd = getRepresent(mv->rd), rs = getRepresent(mv->rs);
	if (preColored.contains(rs))
		std::swap(rd, rs);
	if (rd == rs || preColored.contains(rs) || has_edge(rd, rs) || rd == regs->get("zero") || rs == regs->get("zero")) {
		frozenMoves.insert(mv);
		tryAddToSimplify(rd);
		tryAddToSimplify(rs);
		return;
	}

	if (!George(mv) && !Briggs(mv)) {
		pauseMoves.insert(mv);
		return;
	}
	std::cout << "\033[31mmerge\033[0m < " << rs->to_string() << " > to < " << rd->to_string() << " >" << std::endl;
	// merge rs to rd
	alias[rs] = rd;
	// merge edges
	for (auto to: graph[rs])
		add_edge(rs, to);
	remove_from_graph(rs);
	tryAddToSimplify(rd);
}

void Allocator::freeze() {
	auto reg = *freezeWorkList.begin();
	freezeWorkList.erase(reg);
	simplifyWorkList.insert(reg);
	// TODO
}

Reg *Allocator::getRepresent(Reg *reg) {
	if (!alias.contains(reg)) alias[reg] = reg;
	return alias[reg] == reg ? reg : alias[reg] = getRepresent(alias[reg]);
}

void Allocator::tryAddToSimplify(Reg *reg) {
	if (preColored.contains(reg)) return;
	if (isMoveRelated(reg)) return;
	if (deg(reg) >= K) return;
	freezeWorkList.erase(reg);
	spilledNodes.erase(reg);
	simplifyWorkList.insert(reg);
}

void Allocator::assignColors() {
	edges.clear();
	graph.clear();
	buildGraph();
	while (!selectStack.empty()) {
		auto reg = selectStack.top();
		selectStack.pop();
		std::set<Reg *> exist;
		for (auto y: graph[reg])
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
		if (reg != rep)
			color[reg] = color[rep];
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
