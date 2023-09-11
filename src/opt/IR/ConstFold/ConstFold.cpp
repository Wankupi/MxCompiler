#include "ConstFold.h"
#include <map>
#include <set>

using namespace IR;

class Folder : private IRBaseVisitor {
	Wrapper &env;
	Function *func;

public:
	Folder(Wrapper &wrapper, Function *function) : env(wrapper), func(function) {}
	void work();

private:
	std::map<BasicBlock *, std::set<BasicBlock *>> successors;
	std::map<BasicBlock *, std::set<BasicBlock *>> predecessors;
	std::map<Var *, std::set<Stmt *>> usage;
	std::map<Var *, Stmt *> def;
	std::map<Stmt *, BasicBlock *> belong;

	std::map<Val *, Val *> substitute;
	std::set<BasicBlock *> removedBlock;
	std::set<Stmt *> removedStmt;
	//	std::map<CondBrStmt *, DirectBrStmt *> condBrToDirectBr;

	std::set<BasicBlock *> blockQueue;
	std::set<Stmt *> stmtQueue;

private:
	void init();
	void cut_edge(BasicBlock *from, BasicBlock *to);
	Val *find(Val *val);
	void change(Val *&val, Stmt *at);
	void add_queue(Var *var);
	void check_block(BasicBlock *block);
	void remove_block(BasicBlock *block);    // // 无入或者无出，直接删除
	void substitute_block(BasicBlock *block);// B 仅有一个 DirectBrStmt 指令
	void merge_block(BasicBlock *block);     // A 仅有后继 B 且 B 仅有前驱 A

	bool block_deletable(BasicBlock *block);
	static bool block_substitutable(BasicBlock *block);
	bool block_merge_able(BasicBlock *block);

private:
	void visitArithmeticStmt(IR::ArithmeticStmt *node) override;
	void visitIcmpStmt(IR::IcmpStmt *node) override;
	void visitPhiStmt(IR::PhiStmt *node) override;
	void visitCondBrStmt(IR::CondBrStmt *node) override;
	void visitRetStmt(IR::RetStmt *node) override;
	void visitGetElementPtrStmt(IR::GetElementPtrStmt *node) override;
	void visitCallStmt(IR::CallStmt *node) override;
	void visitStoreStmt(IR::StoreStmt *node) override;
};

void ConstFold::work() {
	auto module = env.get_module();
	for (auto function: module->functions)
		Folder(env, function).work();
}

void Folder::work() {
	init();
	for (auto [var, inst]: def)
		visit(inst);
	for (auto block: func->blocks)
		check_block(block);
	while (!blockQueue.empty() || !stmtQueue.empty()) {
		while (!stmtQueue.empty()) {
			auto stmt = *stmtQueue.begin();
			stmtQueue.erase(stmt);
			visit(stmt);
		}
		while (!blockQueue.empty()) {
			auto block = *blockQueue.begin();
			blockQueue.erase(block);
			remove_block(block);
		}
	}
	decltype(func->blocks) oldBlocks;
	oldBlocks.swap(func->blocks);
	for (auto block: oldBlocks)
		if (!removedBlock.contains(block))
			func->blocks.push_back(block);
	for (auto block: func->blocks) {
		decltype(block->phis) oldPhis;
		oldPhis.swap(block->phis);
		for (auto [res, phi]: oldPhis)
			if (!removedStmt.contains(phi))
				block->phis.emplace(phi->res, phi);

		decltype(block->stmts) oldStmts;
		oldStmts.swap(block->stmts);
		for (auto inst: oldStmts)
			if (!removedStmt.contains(inst))
				block->stmts.push_back(inst);
	}
}

void Folder::init() {
	auto deal_stmt = [&](Stmt *stmt, BasicBlock *block) {
		belong[stmt] = block;
		for (auto use: stmt->getUse())
			if (auto var = dynamic_cast<Var *>(use))
				usage[var].insert(stmt);
		if (auto d = stmt->getDef())
			def[d] = stmt;
	};
	for (auto block: func->blocks) {
		for (auto [var, phi]: block->phis)
			deal_stmt(phi, block);
		for (auto inst: block->stmts)
			deal_stmt(inst, block);
		auto back = block->stmts.back();
		if (auto cond = dynamic_cast<CondBrStmt *>(back))
			successors[block].insert(cond->trueBlock), successors[block].insert(cond->falseBlock);
		else if (auto direct = dynamic_cast<DirectBrStmt *>(back))
			successors[block].insert(direct->block);
		else if (auto unreach = dynamic_cast<UnreachableStmt *>(back))
			blockQueue.insert(block);
		// otherwise back is RetStmt.
	}
	for (auto &[block, successor]: successors)
		for (auto to: successor)
			predecessors[to].insert(block);
	for (auto block: func->blocks)
		if (predecessors[block].empty())
			blockQueue.insert(block);
}

void Folder::remove_block(BasicBlock *block) {
	if (removedBlock.contains(block) || block->label == "entry")
		return;
	if (block_merge_able(block)) {
		merge_block(block);
		return;
	}
	if (block_substitutable(block)) {
		substitute_block(block);
		return;
	}
	if (!block_deletable(block))
		return;

	removedBlock.insert(block);
	auto pre = predecessors[block];
	for (auto p: pre)
		cut_edge(p, block);
	auto suc = successors[block];
	for (auto s: suc)
		cut_edge(block, s);
}

void Folder::substitute_block(BasicBlock *block) {
	// block 没有 phi 指令
	auto to = *successors[block].begin();
	std::set<BasicBlock *> pres;
	pres.swap(predecessors[block]);
	for (auto pre: pres) {
		bool ok = true;
		for (auto [res, phi]: to->phis)
			if (phi->branches.contains(pre) &&
				phi->branches[pre] != phi->branches[block])
				ok = false;
		if (!ok) {
			predecessors[block].insert(pre);
			continue;
		}
		auto bak = pre->stmts.back();
		if (auto cond = dynamic_cast<CondBrStmt *>(bak)) {
			if (cond->trueBlock == block)
				cond->trueBlock = to;
			else if (cond->falseBlock == block)
				cond->falseBlock = to;
			else
				throw std::runtime_error("ConstFold: substitute block fail, condbr not validate.");
			successors[pre] = {cond->trueBlock, cond->falseBlock};
			if (cond->trueBlock == cond->falseBlock) {
				pre->stmts.back() = env.createDirectBrStmt(cond->trueBlock);
				if (auto var = dynamic_cast<Var *>(cond->cond))
					usage[var].erase(cond);
			}
		}
		else if (auto direct = dynamic_cast<DirectBrStmt *>(bak)) {
			if (direct->block != block)
				throw std::runtime_error("ConstFold: substitute block fail, direct br not validate." + pre->label + " " + block->label + " " + direct->block->label);
			direct->block = to;
			successors[pre] = {to};
		}
		else
			throw std::runtime_error("ConstFold: substitute block fail, last stmt not validate.");

		predecessors[to].insert(pre);
		for (auto [res, phi]: to->phis)
			if (phi->branches.contains(block)) {
				phi->branches[pre] = phi->branches[block];
				stmtQueue.insert(phi);
			}
	}
	for (auto pre: pres)
		check_block(pre);
	if (predecessors[block].empty()) {
		cut_edge(block, to);
		removedBlock.insert(block);
	}
	check_block(to);
}

void Folder::merge_block(BasicBlock *block) {
	// try merge 'block' to 'from'
	auto from = *predecessors[block].cbegin();
	// check is able to merge
	auto back = from->stmts.back();
	auto dir = dynamic_cast<DirectBrStmt *>(back);
	if (!dir || dir->block != block)
		throw std::runtime_error("ConstFold: omit middle block fail, direct jump not validate. from=" + from->label + ", but to=" + (dir ? dir->block->label : "null") + "\n back : " + back->to_string());
	// merge
	from->stmts.pop_back();// remove direct jump

	for (auto stmt: block->stmts)
		from->stmts.push_back(stmt);

	successors[from].erase(block);
	for (auto to: successors[block]) {
		successors[from].insert(to);
		predecessors[to].erase(block);
		predecessors[to].insert(from);
		for (auto [res, phi]: to->phis)
			if (phi->branches.contains(block)) {
				phi->branches[from] = phi->branches[block];
				phi->branches.erase(block);
				stmtQueue.insert(phi);
			}
	}
	removedBlock.insert(block);
	check_block(from);
	for (auto to: successors[block])
		check_block(to);
}

void Folder::cut_edge(BasicBlock *from, BasicBlock *to) {
	successors[from].erase(to);
	predecessors[to].erase(from);
	auto jump = from->stmts.back();
	if (auto direct = dynamic_cast<DirectBrStmt *>(jump)) {
		if (direct->block != to)
			throw std::runtime_error("ConstFold: cut edge fail");
		from->stmts.back() = env.createUnreachableStmt();
	}
	else if (auto cond = dynamic_cast<CondBrStmt *>(jump)) {
		auto other = cond->trueBlock == to ? cond->falseBlock : cond->trueBlock;
		auto dir = env.createDirectBrStmt(other);
		/// @attention replace directly
		from->stmts.back() = dir;
	}
	else
		throw std::runtime_error("ConstFold: cut edge fail");
	for (auto [var, phi]: to->phis) {
		phi->branches.erase(from);
		stmtQueue.insert(phi);
	}

	check_block(from);
	check_block(to);
}

bool Folder::block_deletable(IR::BasicBlock *block) {
	if (predecessors[block].empty()) return true;
	auto bak = block->stmts.back();
	if (dynamic_cast<UnreachableStmt *>(bak))
		return true;
	return false;
}

bool Folder::block_merge_able(IR::BasicBlock *block) {
	return predecessors[block].size() == 1 && successors[*predecessors[block].begin()].size() == 1;
}

bool Folder::block_substitutable(IR::BasicBlock *block) {
	return block->phis.empty() && block->stmts.size() == 1 && dynamic_cast<DirectBrStmt *>(block->stmts.back());
}

void Folder::check_block(IR::BasicBlock *block) {
	if (removedBlock.contains(block))
		return;
	if (block_deletable(block) || block_merge_able(block) || block_substitutable(block))
		blockQueue.insert(block);
}

Val *Folder::find(IR::Val *val) {
	while (substitute.contains(val))
		val = substitute[val];
	return val;
}

void Folder::change(IR::Val *&val, IR::Stmt *at) {
	auto n = find(val);
	if (n == val) return;
	if (auto var = dynamic_cast<Var *>(val))
		usage[var].erase(at);
	val = n;
	if (auto var = dynamic_cast<Var *>(val))
		usage[var].insert(at);
}

static int get_literal(Val *val, bool &ok) {
	if (!dynamic_cast<Literal *>(val)) {
		ok = false;
		return 0;
	}
	if (auto lit = dynamic_cast<LiteralInt *>(val))
		return lit->value;
	if (auto lit = dynamic_cast<LiteralBool *>(val))
		return lit->value;
	return 0;
}

void Folder::add_queue(Var *var) {
	stmtQueue.insert(usage[var].begin(), usage[var].end());
}

void Folder::visitArithmeticStmt(IR::ArithmeticStmt *node) {
	change(node->lhs, node);
	change(node->rhs, node);
	bool ok = true;
	int lhs = get_literal(node->lhs, ok), rhs = get_literal(node->rhs, ok);
	if (!ok) return;
	int res = 0;
	if (node->cmd == "add")
		res = lhs + rhs;
	else if (node->cmd == "sub")
		res = lhs - rhs;
	else if (node->cmd == "mul")
		res = lhs * rhs;
	else if (node->cmd == "sdiv")
		res = rhs ? lhs / rhs : 0;
	else if (node->cmd == "srem")
		res = rhs ? lhs % rhs : 0;
	else if (node->cmd == "shl")
		res = lhs << rhs;
	else if (node->cmd == "ashr")
		res = lhs >> rhs;
	else if (node->cmd == "and")
		res = lhs & rhs;
	else if (node->cmd == "or")
		res = lhs | rhs;
	else if (node->cmd == "xor")
		res = lhs ^ rhs;
	else
		throw std::runtime_error("ConstFold: unknown arithmetic cmd " + node->cmd);
	if (dynamic_cast<LiteralInt *>(node->lhs))
		substitute[node->res] = env.get_literal_int(res);
	else if (dynamic_cast<LiteralBool *>(node->lhs))
		substitute[node->res] = env.get_literal_bool(res);
	else
		throw std::runtime_error("ConstFold: unknown arithmetic lhs " + node->lhs->to_string());
	add_queue(node->res);
	removedStmt.insert(node);
}

void Folder::visitIcmpStmt(IR::IcmpStmt *node) {
	change(node->lhs, node);
	change(node->rhs, node);
	bool ok = true;
	int lhs = get_literal(node->lhs, ok), rhs = get_literal(node->rhs, ok);
	if (!ok) return;
	bool res;
	if (node->cmd == "eq")
		res = lhs == rhs;
	else if (node->cmd == "ne")
		res = lhs != rhs;
	else if (node->cmd == "slt")
		res = lhs < rhs;
	else if (node->cmd == "sgt")
		res = lhs > rhs;
	else if (node->cmd == "sle")
		res = lhs <= rhs;
	else if (node->cmd == "sge")
		res = lhs >= rhs;
	else
		throw std::runtime_error("ConstFold: unknown icmp cmd " + node->cmd);
	substitute[node->res] = env.get_literal_bool(res);
	add_queue(node->res);
	removedStmt.insert(node);
}

void Folder::visitPhiStmt(IR::PhiStmt *node) {
	for (auto &[block, val]: node->branches)
		change(val, node);
	bool all_same = true;
	Val *res = nullptr;
	for (auto &[block, val]: node->branches) {
		if (res == nullptr)
			res = val;
		else if (res != val) {
			all_same = false;
			break;
		}
	}
	if (all_same) {
		substitute[node->res] = res;
		add_queue(node->res);
		removedStmt.insert(node);
	}
}

void Folder::visitCondBrStmt(IR::CondBrStmt *node) {
	change(node->cond, node);
	bool ok = true;
	int cond = get_literal(node->cond, ok);
	if (!ok) return;
	auto other = cond ? node->falseBlock : node->trueBlock;
	cut_edge(belong[node], other);
}

void Folder::visitRetStmt(IR::RetStmt *node) {
	change(node->value, node);
}

void Folder::visitGetElementPtrStmt(IR::GetElementPtrStmt *node) {
	for (auto &index: node->indices)
		change(index, node);
}

void Folder::visitCallStmt(IR::CallStmt *node) {
	for (auto &arg: node->args)
		change(arg, node);
}

void Folder::visitStoreStmt(IR::StoreStmt *node) {
	change(node->value, node);
}
