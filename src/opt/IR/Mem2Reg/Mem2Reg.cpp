#include "Mem2Reg.h"
#include "IR/RewriteLayer.h"
#include "utils/Graph.h"
#include <fstream>
#include <queue>
#include <set>

#include "opt/IR/ConstFold/ConstFold.h"

namespace IR {

class DefUseCollector : private RewriteLayer {
public:
	std::set<Var *> use;
	std::map<PtrVar *, Val *> def;

private:
	std::map<Val *, Val *> &substitute;
	BasicBlock *workBlock = nullptr;
	std::set<PtrVar *> const &allocaVars;

public:
	explicit DefUseCollector(BasicBlock *basicBlock, std::set<PtrVar *> const &allocaVars, std::map<Val *, Val *> &globalSub)
		: workBlock(basicBlock), allocaVars(allocaVars), substitute(globalSub) {}
	void calc_def() {
		for (auto stmt: workBlock->stmts) {
			if (auto ld = dynamic_cast<LoadStmt *>(stmt)) {
				auto ptr = dynamic_cast<PtrVar *>(ld->pointer);
				if (auto p = def.find(ptr); p != def.end())
					substitute[ld->res] = p->second;
			}
			else if (auto st = dynamic_cast<StoreStmt *>(stmt)) {
				auto is_alloca = allocaVars.contains(dynamic_cast<PtrVar *>(st->pointer));
				if (is_alloca) {
					auto ptr = dynamic_cast<PtrVar *>(st->pointer);
					def[ptr] = st->value;
				}
			}
		}
	}
	void work() {
		visitBasicBlock(workBlock);
	}

private:
	void change(Val *&val) {
		while (substitute.count(val)) {
			val = substitute[val];
		}
	}
	void change_ptr(Var *&var) {
		while (substitute.count(var)) {
			var = dynamic_cast<Var *>(substitute[var]);
			if (!var) throw std::runtime_error("Mem2Reg: change_ptr failed");
		}
	}

	void visitAllocaStmt(IR::AllocaStmt *node) override {
		// discard this stmt
	}
	void visitStoreStmt(IR::StoreStmt *node) override {
		// GlobalVar: remember, not remove
		// PtrVar(alloca): remember, remove
		// LocalVar: not remember, not remove
		// PtrVar(non-alloca): not remember, not remove
		// StringLiteralVar: not remember, not remove // should not be here
		auto is_alloca = allocaVars.contains(dynamic_cast<PtrVar *>(node->pointer));
		change(node->value);
		if (is_alloca) {
			def[dynamic_cast<PtrVar *>(node->pointer)] = node->value;
			return;
		}
		add_stmt(node);
	}
	void visitLoadStmt(IR::LoadStmt *node) override {
		if (auto p = def.find(dynamic_cast<PtrVar *>(node->pointer)); p != def.end())
			substitute[node->res] = p->second;
		else
			add_stmt(node);
	}
	void visitArithmeticStmt(ArithmeticStmt *node) override {
		change(node->lhs);
		change(node->rhs);
		add_stmt(node);
	}
	void visitIcmpStmt(IcmpStmt *node) override {
		change(node->lhs);
		change(node->rhs);
		add_stmt(node);
	}
	void visitRetStmt(RetStmt *node) override {
		change(node->value);
		add_stmt(node);
	}
	void visitGetElementPtrStmt(GetElementPtrStmt *node) override {
		change_ptr(node->pointer);
		for (auto &index: node->indices)
			change(index);
		add_stmt(node);
	}
	void visitCallStmt(CallStmt *node) override {
		for (auto &arg: node->args)
			change(arg);
		add_stmt(node);
	}
	void visitCondBrStmt(CondBrStmt *node) override {
		change(node->cond);
		add_stmt(node);
	}
	void visitPhiStmt(IR::PhiStmt *node) override {
		if (substitute.contains(node->res))
			return;
		for (auto &[block, val]: node->branches)
			change(val);
		add_phi(node);
	}
};

class Mem2RegFunc {
	Wrapper &env;
	Function *func;
	int n = 0;
	std::map<BasicBlock *, std::vector<BasicBlock *>> predecessors, successors;
	std::map<BasicBlock *, int> ptr2id;
	std::vector<BasicBlock *> id2ptr;
	Graph cfg;
	std::map<BasicBlock *, BasicBlock *> idom;
	std::map<BasicBlock *, std::vector<BasicBlock *>> dominates, frontiers;

	std::set<PtrVar *> vars;
	std::map<Val *, Val *> substitute;
	std::map<BasicBlock *, DefUseCollector> trans;

	std::map<BasicBlock *, std::map<PtrVar *, PhiStmt>> def_inherit;
	std::map<PtrVar *, int> phi_counter;

public:
	explicit Mem2RegFunc(Wrapper &wrapper, Function *func)
		: env(wrapper), func(func), n(func->blocks.size()), id2ptr(n + 1), cfg(n + 1) {
	}
	void work();

private:
	void build_dom_tree();
	void collect_variables();
	void transfer_def(BasicBlock *block, PtrVar *var);
	void complete_phi(std::map<PtrVar *, Val *> const &def, BasicBlock *block, BasicBlock *from);
	void simplify_phi();

	Val *get_substitute(Val *val) {
		if (!substitute.contains(val))
			return val;
		return substitute[val] = get_substitute(substitute[val]);
	}
};

void Mem2Reg::work() {
	IR::ConstFold(env).work();
	auto module = env.get_module();
	for (auto func: module->functions) {
		if (func->blocks.empty()) continue;
		Mem2RegFunc(env, func).work();
	}
}

void Mem2RegFunc::work() {
	build_dom_tree();
	collect_variables();
	for (auto block: func->blocks) {
		trans.emplace(block, DefUseCollector(block, vars, substitute));
		trans.at(block).calc_def();
	}

	for (auto block: func->blocks)
		for (auto def: trans.at(block).def)
			for (auto y: frontiers[block])
				transfer_def(y, def.first);

	for (auto &[block, opt]: trans)
		opt.def.clear();


	std::queue<BasicBlock *> q;
	for (auto block: func->blocks)
		if (!idom[block]) {
			q.push(block);
		}
	while (!q.empty()) {
		auto block = q.front();
		q.pop();

		auto &opt = trans.at(block);
		for (auto &[var, phi]: def_inherit[block])
			opt.def[var] = phi.res;
		opt.work();
		auto &def = opt.def;
		for (auto succ: successors[block])
			complete_phi(def, succ, block);
		for (auto son: dominates[block]) {
			auto &son_def = trans.at(son).def;
			son_def = def;
			q.push(son);
		}
	}

	simplify_phi();
}

void Mem2RegFunc::build_dom_tree() {
	for (auto block: func->blocks) {
		int siz = static_cast<int>(ptr2id.size()) + 1;
		ptr2id[block] = siz;
		id2ptr[siz] = block;
	}
	for (auto block: func->blocks) {
		auto back = block->stmts.back();
		if (auto br = dynamic_cast<CondBrStmt *>(back)) {
			cfg.add_edge(ptr2id[block], ptr2id[br->trueBlock]);
			cfg.add_edge(ptr2id[block], ptr2id[br->falseBlock]);
			predecessors[br->trueBlock].push_back(block);
			predecessors[br->falseBlock].push_back(block);
			successors[block].push_back(br->trueBlock);
			successors[block].push_back(br->falseBlock);
		}
		else if (auto dir = dynamic_cast<DirectBrStmt *>(back)) {
			cfg.add_edge(ptr2id[block], ptr2id[dir->block]);
			predecessors[dir->block].push_back(block);
			successors[block].push_back(dir->block);
		}
	}

	DominateTree dom_tree(cfg);
	dom_tree.LengauerTarjan(ptr2id[func->blocks.front()]);

	for (auto block: func->blocks) {
		int id = ptr2id[block];
		idom[block] = id2ptr[dom_tree.idom[id]];
		if (dom_tree.idom[id] != id)
			dominates[id2ptr[dom_tree.idom[id]]].push_back(block);
	}

	DominanceFrontier dom_front(cfg, dom_tree);
	dom_front.work();

	for (auto block: func->blocks) {
		auto &front = dom_front.out[ptr2id[block]];
		auto &frontier = frontiers[block];
		frontier.reserve(front.size());
		for (auto y: front)
			frontier.push_back(id2ptr[y]);
	}
}

void Mem2RegFunc::collect_variables() {
	for (auto block: func->blocks)
		for (auto stmt: block->stmts)
			if (auto alloca = dynamic_cast<AllocaStmt *>(stmt))
				vars.insert(alloca->res);
}

void Mem2RegFunc::transfer_def(IR::BasicBlock *block, IR::PtrVar *var) {
	auto &phis = def_inherit[block];
	if (phis.contains(var))
		return;
	phis.emplace(var,
				 PhiStmt(env.create_local_var(var->objType, var->name + ".phi." + std::to_string(++phi_counter[var])),
						 {}));
	for (auto y: frontiers[block])
		transfer_def(y, var);
}

void Mem2RegFunc::complete_phi(const std::map<PtrVar *, Val *> &def, BasicBlock *block, BasicBlock *from) {
	auto &phis = def_inherit[block];
	for (auto &[var, phi]: phis) {
		if (def.contains(var))
			phi.branches[from] = def.at(var);
		else
			phi.branches[from] = env.default_value(var->objType);
	}
}

void Mem2RegFunc::simplify_phi() {
	std::map<Var *, std::vector<PhiStmt *>> uses;
	auto add_phi_use = [&](PhiStmt *phi) {
		for (auto &[block, val]: phi->branches)
			if (auto var = dynamic_cast<Var *>(val))
				uses[var].push_back(phi);
	};

	std::queue<Var *> que;
	auto check_phi = [&](PhiStmt *phi) {
		if (substitute.contains(phi->res)) return;
		for (auto &[block, val]: phi->branches)
			val = get_substitute(val);
		bool all_same = true;
		Val *same_val = nullptr;
		for (auto [block, val]: phi->branches) {
			if (val == phi->res) continue;
			if (same_val == nullptr)
				same_val = val;
			else if (same_val != val) {
				all_same = false;
				break;
			}
		}
		if (all_same) {
			substitute[phi->res] = same_val;
			que.push(phi->res);
		}
	};
	for (auto &[block, phis]: def_inherit)
		for (auto &[var, phi]: phis) {
			add_phi_use(&phi);
			check_phi(&phi);
		}
	for (auto block: func->blocks)
		for (auto [res, phi]: block->phis) {
			add_phi_use(phi);
			check_phi(phi);
		}

	while (!que.empty()) {
		auto var = que.front();
		que.pop();
		for (auto phi: uses[var])
			check_phi(phi);
	}

	// add phis
	for (auto &[block, phis]: def_inherit)
		for (auto &[var, phi]: phis)
			if (!substitute.contains(phi.res))
				block->phis[phi.res] = env.createPhiStmt(phi);
	for (auto &[block, opt]: trans)
		opt.work();
}

}// namespace IR
