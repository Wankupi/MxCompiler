#pragma once
#include "IRBaseVisitor.h"
#include "Type.h"
#include "Val.h"
#include <sstream>
#include <string>
#include <vector>

namespace IR {

struct Class : public IRNode {
	ClassType type;
	std::map<std::string, size_t> name2index;
	void print(std::ostream &out) const override;
	void add_filed(PrimitiveType *type, const std::string &name);
	void accept(IRBaseVisitor *visitor) override { visitor->visitClass(this); }
};

struct Stmt : public IRNode {};

struct BasicBlock : public IRNode {
	explicit BasicBlock(std::string label) : label(std::move(label)) {}
	std::string label;
	std::vector<Stmt *> stmts;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitBasicBlock(this); }
};

struct Function : public IRNode {
	Type *type;
	std::string name;
	std::vector<std::pair<Type *, std::string>> params;
	std::vector<BasicBlock *> blocks;

	std::vector<LocalVar *> paramsVar;
	std::vector<LocalVar *> localVars;// stored for memory control

	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitFunction(this); }
};

struct AllocaStmt : public Stmt {
	Type *type = nullptr;
	Var *res = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitAllocaStmt(this); }
};

struct StoreStmt : public Stmt {
	Val *value = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitStoreStmt(this); }
};

struct LoadStmt : public Stmt {
	Var *res = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitLoadStmt(this); }
};

struct ArithmeticStmt : public Stmt {
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	std::string cmd;
	// possible cmd: add, sub, mul, sdiv, srem, shl, ashr, and, or, xor
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitArithmeticStmt(this); }
};

struct IcmpStmt : public Stmt {
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	std::string cmd;
	// possible cmd: eq, ne, SltInst, sgt, sle, sge
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitIcmpStmt(this); }
};

struct RetStmt : public Stmt {
	RetStmt() = default;
	explicit RetStmt(Val *value) : value(value) {}
	Val *value = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitRetStmt(this); }
};

struct GetElementPtrStmt : public Stmt {
	Var *res = nullptr;
	Var *pointer = nullptr;
	std::string typeName;
	std::vector<Val *> indices;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGetElementPtrStmt(this); }
};

struct CallStmt : public Stmt {
	Var *res = nullptr;
	Function *func;
	std::vector<Val *> args;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitCallStmt(this); }
};

struct BrStmt : public Stmt {};

struct DirectBrStmt : public BrStmt {
	BasicBlock *block = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitDirectBrStmt(this); }
};

struct CondBrStmt : public BrStmt {
	Val *cond = nullptr;
	BasicBlock *trueBlock = nullptr;
	BasicBlock *falseBlock = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitCondBrStmt(this); }
};

struct PhiStmt : public Stmt {
	Var *res = nullptr;
	std::map<BasicBlock *, Val *> branches;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitPhiStmt(this); }
};

struct UnreachableStmt : public Stmt {
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitUnreachableStmt(this); }
};

struct GlobalStmt : public Stmt {
	GlobalVar *var = nullptr;
	Val *value = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGlobalStmt(this); }
};

struct GlobalStringStmt : public Stmt {
	explicit GlobalStringStmt(StringLiteralVar *var) : var(var) {}
	StringLiteralVar *var = nullptr;
	void print(std::ostream &out) const override;
	void accept(IRBaseVisitor *visitor) override { visitor->visitGlobalStringStmt(this); }
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
