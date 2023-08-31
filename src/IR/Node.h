#pragma once
#include "IRBaseVisitor.h"
#include "Type.h"
#include "Val.h"
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace IR {

struct Class : public IRNode {
	explicit Class(std::string name) { type.name = std::move(name); }
	ClassType type;
	std::map<std::string, size_t> name2index;
	void print(std::ostream &out) const override;
	void add_filed(PrimitiveType *type, const std::string &name);
	void accept(IRBaseVisitor *visitor) override { visitor->visitClass(this); }
};

struct Stmt : public IRNode {
	[[nodiscard]] virtual std::set<Val *> getUse() const { return {}; }
	[[nodiscard]] virtual Var *getDef() const { return nullptr; }
};

struct BasicBlock : public IRNode {
	explicit BasicBlock(std::string label) : label(std::move(label)) {}
	std::string label;
	std::map<Var *, PhiStmt *> phis;
	std::list<Stmt *> stmts;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitBasicBlock(this); }
};

struct Function : public IRNode {
	Function(Type *retType, std::string name) : type(retType), name(std::move(name)) {}

	Type *type = nullptr;
	std::string name;
	std::vector<std::pair<Type *, std::string>> params;
	std::vector<BasicBlock *> blocks;

	std::vector<LocalVar *> paramsVar;
	std::vector<LocalVar *> localVars;// stored for memory control

	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitFunction(this); }
};

struct AllocaStmt : public Stmt {
	explicit AllocaStmt(PtrVar *res) : res(res) {}
	PtrVar *res = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitAllocaStmt(this); }
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct StoreStmt : public Stmt {
	StoreStmt(Val *value, Var *pointer) : value(value), pointer(pointer) {}
	Val *value = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitStoreStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {pointer, value}; }
};

struct LoadStmt : public Stmt {
	LoadStmt(Var *res, Var *pointer) : res(res), pointer(pointer) {}
	Var *res = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitLoadStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {pointer}; }
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct ArithmeticStmt : public Stmt {
	ArithmeticStmt(std::string cmd, Var *res, Val *lhs, Val *rhs) : cmd(std::move(cmd)), res(res), lhs(lhs), rhs(rhs) {}
	std::string cmd;
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	// possible cmd: add, sub, mul, sdiv, srem, shl, ashr, and, or, xor
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitArithmeticStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {lhs, rhs}; }
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct IcmpStmt : public Stmt {
	IcmpStmt(std::string cmd, Var *res, Val *lhs, Val *rhs) : cmd(std::move(cmd)), res(res), lhs(lhs), rhs(rhs) {}
	std::string cmd;
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	// possible cmd: eq, ne, SltInst, sgt, sle, sge
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitIcmpStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {lhs, rhs}; }
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct RetStmt : public Stmt {
	RetStmt() = default;
	explicit RetStmt(Val *value) : value(value) {}
	Val *value = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitRetStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override {
		if (value) return {value};
		else
			return {};
	}
};

struct GetElementPtrStmt : public Stmt {
	GetElementPtrStmt(std::string typeName, Var *res, Var *pointer, std::vector<Val *> indices = {})
		: typeName(std::move(typeName)), res(res), pointer(pointer), indices(std::move(indices)) {}
	Var *res = nullptr;
	Var *pointer = nullptr;
	std::string typeName;
	std::vector<Val *> indices;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGetElementPtrStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override {
		std::set<Val *> ret;
		ret.insert(pointer);
		for (auto index: indices)
			ret.insert(index);
		return ret;
	}
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct CallStmt : public Stmt {
	explicit CallStmt(Function *func, std::vector<Val *> args = {}, Var *res = nullptr) : func(func), args(std::move(args)), res(res) {}
	Var *res = nullptr;
	Function *func;
	std::vector<Val *> args;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitCallStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override {
		std::set<Val *> ret;
		for (auto arg: args)
			ret.insert(arg);
		return ret;
	}
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct BrStmt : public Stmt {};

struct DirectBrStmt : public BrStmt {
	explicit DirectBrStmt(BasicBlock *block) : block(block) {}
	BasicBlock *block = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitDirectBrStmt(this); }
};

struct CondBrStmt : public BrStmt {
	CondBrStmt(Val *cond, BasicBlock *trueBlock, BasicBlock *falseBlock) : cond(cond), trueBlock(trueBlock), falseBlock(falseBlock) {}
	Val *cond = nullptr;
	BasicBlock *trueBlock = nullptr;
	BasicBlock *falseBlock = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitCondBrStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {cond}; }
};

struct PhiStmt : public Stmt {
	explicit PhiStmt(Var *res, std::map<BasicBlock *, Val *> branches = {}) : res(res), branches(std::move(branches)) {}
	Var *res = nullptr;
	std::map<BasicBlock *, Val *> branches;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitPhiStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override {
		std::set<Val *> ret;
		for (auto [block, val]: branches)
			ret.insert(val);
		return ret;
	}
	[[nodiscard]] Var *getDef() const override { return res; }
};

struct UnreachableStmt : public Stmt {
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitUnreachableStmt(this); }
};

struct GlobalStmt : public Stmt {
	GlobalStmt(GlobalVar *var, Val *value) : var(var), value(value) {}

	GlobalVar *var = nullptr;
	Val *value = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGlobalStmt(this); }
	[[nodiscard]] std::set<Val *> getUse() const override { return {value}; }
	[[nodiscard]] Var *getDef() const override { return var; }
};

struct GlobalStringStmt : public Stmt {
	explicit GlobalStringStmt(StringLiteralVar *var) : var(var) {}
	StringLiteralVar *var = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGlobalStringStmt(this); }
	[[nodiscard]] Var *getDef() const override { return var; }
};

struct Module : public IRNode {
	std::vector<Class *> classes;
	std::vector<Function *> functions;
	std::vector<GlobalStmt *> variables;
	std::vector<GlobalStringStmt *> stringLiterals;

	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitModule(this); }

	std::vector<Var *> globalVars;// stored for memory control
};

}// namespace IR
