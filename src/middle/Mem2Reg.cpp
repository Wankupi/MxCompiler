#include "Mem2Reg.h"
#include "IR/RewriteLayer.h"
#include "utils/Graph.h"
#include <fstream>
#include <set>

namespace IR {

class DefUseCollector : private RewriteLayer {
public:
	std::set<Var *> use;
	std::map<Var *, Val *> def;

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
				if (auto p = def.find(ld->pointer); p != def.end()) {
					substitute[ld->res] = p->second;
				}
			}
			else if (auto st = dynamic_cast<StoreStmt *>(stmt)) {
				auto is_alloca = allocaVars.contains(dynamic_cast<PtrVar *>(st->pointer));
				if (is_alloca) {
					def[st->pointer] = st->value;
					change(def[st->pointer]);
				}
			}
		}
	}
	void work() {
		visitBasicBlock(workBlock);
	}
	void set_var(Var *var, Val *val) {
		std::cerr << "set_va(" << workBlock->label << "): " << var->name << " = " << val->to_string() << "\n";
		def[var] = val;
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
			def[node->pointer] = node->value;
			return;
		}
		add_stmt(node);
	}
	void visitLoadStmt(IR::LoadStmt *node) override {
		if (auto p = def.find(node->pointer); p != def.end())
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
	std::map<BasicBlock *, int> ptr2id;
	std::vector<BasicBlock *> id2ptr;
	Graph cfg;

	std::set<PtrVar *> vars;
	std::map<Val *, Val *> substitute;
	std::map<BasicBlock *, DefUseCollector> trans;

	std::map<BasicBlock *, std::map<Var *, PhiStmt>> def_inherit;
	std::map<Var *, int> phi_counter;

public:
	explicit Mem2RegFunc(Wrapper &wrapper, Function *func)
		: env(wrapper), func(func), n(func->blocks.size()), id2ptr(n + 1), cfg(n + 1) {
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
			}
			else if (auto dir = dynamic_cast<DirectBrStmt *>(back))
				cfg.add_edge(ptr2id[block], ptr2id[dir->block]);
		}
	}
	void work();

private:
	void collect_variables();
	void collect_variables(BasicBlock *block);

	void transfer_def(BasicBlock *block, BasicBlock *from, Var *var, Val *val);
	void change_to_phi_res(BasicBlock *block, BasicBlock *from, Var *var, Val *val);
	std::pair<bool, Val *> need_phi(PhiStmt &phi);
};

void Mem2Reg::work() {
	auto module = env.get_module();
	for (auto func: module->functions) {
		if (func->blocks.empty()) continue;
		Mem2RegFunc(env, func).work();
	}
}

std::pair<bool, Val *> Mem2RegFunc::need_phi(PhiStmt &phi) {
	if (phi.branches.size() <= 1) return {false, phi.branches.begin()->second};
	auto &branches = phi.branches;
	bool all_same = true;
	Val *val = nullptr;
	for (auto &[_, v]: branches)
		if (val == nullptr)
			val = v;
		else if (v != val) {
			all_same = false;
			break;
		}
	if (all_same && phi.res) {
		substitute.emplace(phi.res, val);
		std::cerr << "substitute: " << phi.res->to_string() << " -> " << val->to_string() << "\n";
		return {true, phi.res};
	}
	else if (all_same)
		return {false, val};
	else
		return {true, phi.res};
}

void Mem2RegFunc::work() {
	collect_variables();
	for (auto block: func->blocks) {
		trans.emplace(block, DefUseCollector(block, vars, substitute));
		trans.at(block).calc_def();
	}

	for (auto block: func->blocks)
		for (auto def: trans.at(block).def)
			for (auto y: cfg[ptr2id[block]])
				transfer_def(id2ptr[y], block, def.first, def.second);

	for (auto block: func->blocks) {
		if (!def_inherit.contains(block)) continue;
		auto &opt = trans.at(block);
		auto &phis = def_inherit.at(block);
		for (auto &[res, phi]: phis) {
			auto [need, val] = need_phi(phi);
			if (need) block->phis[phi.res] = env.createPhiStmt(phi);
			opt.set_var(res, val);
		}
	}

	for (auto block: func->blocks)
		trans.at(block).calc_def();

	for (auto block: func->blocks) {
		if (!def_inherit.contains(block)) continue;
		auto &opt = trans.at(block);
		auto &phis = def_inherit.at(block);
		for (auto &[res, phi]: phis) {
			auto [need, val] = need_phi(phi);
			opt.set_var(res, val);
		}
	}

	for (auto block: func->blocks)
		trans.at(block).work();

	for (auto block : func->blocks) {
		for (auto y : cfg[ptr2id[block]]) {
			auto &phis = id2ptr[y]->phis;
			for (auto &[var, phi] : phis) {
				if (phi->branches.contains(block)) continue;
				phi->branches[block] = env.default_value(var->type);
			}
		}
	}
}

void Mem2RegFunc::collect_variables() {
	for (auto block: func->blocks)
		collect_variables(block);
}

void Mem2RegFunc::collect_variables(BasicBlock *block) {
	for (auto stmt: block->stmts)
		if (auto alloca = dynamic_cast<AllocaStmt *>(stmt))
			vars.insert(alloca->res);
}

void Mem2RegFunc::transfer_def(BasicBlock *block, BasicBlock *from, Var *var, Val *val) {
	auto &phis = def_inherit[block];
	if (!phis.contains(var)) {
		phis.emplace(var, PhiStmt(nullptr, {{from, val}}));
		if (trans.at(block).def.contains(var))
			return;
		for (auto y: cfg[ptr2id[block]])
			transfer_def(id2ptr[y], block, var, val);
		return;
	}
	auto &phi = phis.at(var);
	phi.branches[from] = val;
	bool all_same = true;
	for (auto &[_, v]: phi.branches)
		if (v != val) {
			all_same = false;
			break;
		}
	if (all_same)
		return;
	if (!phi.res)
		phi.res = env.create_local_var(val->type, var->name + ".phi." + std::to_string(++phi_counter[var]));
	if (trans.at(block).def.contains(var))
		return;
	for (auto y: cfg[ptr2id[block]])
		change_to_phi_res(id2ptr[y], block, var, phi.res);
}

void Mem2RegFunc::change_to_phi_res(BasicBlock *block, BasicBlock *from, Var *var, Val *val) {
	auto &phis = def_inherit[block];
	if (!phis.contains(var)) throw std::runtime_error("Mem2Reg: change_to_phi_res failed, no var.");
	auto &phi = phis.at(var);
	if (phi.branches[from] == val) return;
	phi.branches[from] = val;

	bool all_same = true;
	for (auto &[_, v]: phi.branches)
		if (v != val) {
			all_same = false;
			break;
		}
	if (!all_same && !phi.res)
		phi.res = env.create_local_var(val->type, var->name + ".phi." + std::to_string(++phi_counter[var]));

	if (trans.at(block).def.contains(var))
		return;
	for (auto y: cfg[ptr2id[block]])
		change_to_phi_res(id2ptr[y], block, var, phi.res ?: val);
}


};// namespace IR
