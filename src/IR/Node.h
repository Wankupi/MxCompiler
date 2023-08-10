#pragma once
#include "Type.h"
#include "Val.h"
#include <sstream>
#include <string>
#include <vector>

struct IRNode {
	virtual ~IRNode() = default;
	virtual void print(std::ostream &out) const = 0;
	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		print(ss);
		return ss.str();
	}
};

namespace IR {

struct Class : public IRNode {
	ClassType type;
	std::map<std::string, size_t> name2index;
	void print(std::ostream &out) const override;
	void add_filed(PrimitiveType *type, const std::string &name);
};

struct Stmt : public IRNode {};

struct Block : public IRNode {
	explicit Block(std::string label) : label(std::move(label)) {}
	std::string label;
	std::vector<Stmt *> stmts;
	void print(std::ostream &out) const override;
};

struct Function : public IRNode {
	Type *type;
	std::string name;
	std::vector<std::pair<Type *, std::string>> params;
	std::vector<Block *> blocks;

	std::vector<LocalVar *> localVars;// stored for memory control

	void print(std::ostream &out) const override;
};

struct AllocaStmt : public Stmt {
	Type *type = nullptr;
	Var *res = nullptr;
	void print(std::ostream &out) const override;
};

struct StoreStmt : public Stmt {
	Val *value = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
};

struct LoadStmt : public Stmt {
	Var *res = nullptr;
	Var *pointer = nullptr;
	void print(std::ostream &out) const override;
};

struct ArithmeticStmt : public Stmt {
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	std::string cmd;
	void print(std::ostream &out) const override;
};

struct IcmpStmt : public Stmt {
	Var *res = nullptr;
	Val *lhs = nullptr;
	Val *rhs = nullptr;
	std::string cmd;
	void print(std::ostream &out) const override;
};

struct RetStmt : public Stmt {
	RetStmt() = default;
	explicit RetStmt(Val *value) : value(value) {}
	Val *value = nullptr;
	void print(std::ostream &out) const override;
};

struct GetElementPtrStmt : public Stmt {
	Var *res = nullptr;
	Var *pointer = nullptr;
	std::string typeName;
	std::vector<Val *> indices;
	void print(std::ostream &out) const override;
};

struct CallStmt : public Stmt {
	Var *res = nullptr;
	Function *func;
	std::vector<Val *> args;
	void print(std::ostream &out) const override;
};

struct BrStmt : public Stmt {};

struct DirectBrStmt : public BrStmt {
	Block *block = nullptr;
	void print(std::ostream &out) const override;
};

struct CondBrStmt : public BrStmt {
	Val *cond = nullptr;
	Block *trueBlock = nullptr;
	Block *falseBlock = nullptr;
	void print(std::ostream &out) const override;
};

struct PhiStmt : public Stmt {
	Var *res = nullptr;
	std::vector<std::pair<Val *, Block *>> branches;
	void print(std::ostream &out) const override;
};

struct UnreachableStmt : public Stmt {
	void print(std::ostream &out) const override;
};

struct GlobalStmt : public Stmt {
	GlobalVar *var = nullptr;
	Val *value = nullptr;
	void print(std::ostream &out) const override;
};

struct GlobalStringStmt : public Stmt {
	StringLiteralVar *var = nullptr;
	std::string value;
	void print(std::ostream &out) const override;
};

struct Module : public IRNode {
	std::vector<Class *> classes;
	std::vector<Function *> functions;
	std::vector<GlobalStmt *> variables;
	std::vector<GlobalStringStmt *> stringLiterals;
	void print(std::ostream &out) const override;

	std::vector<Var *> globalVars;// stored for memory control
};

}// namespace IR
