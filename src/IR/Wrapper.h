#pragma once
#include "Node.h"
#include "Type.h"
#include "Val.h"


namespace IR {

class Wrapper {
public:
	Wrapper() = default;
	~Wrapper();

	[[nodiscard]] Module *get_module() const { return module; }

	LiteralNull *get_literal_null();
	LiteralBool *get_literal_bool(bool value);
	LiteralInt *get_literal_int(int value);
	StringLiteralVar *get_literal_string(const std::string &value);

	LiteralInt *literal(int value) { return get_literal_int(value); }
	LiteralNull *literal(nullptr_t) { return get_literal_null(); }
	LiteralBool *literal(bool value) { return get_literal_bool(value); }
	StringLiteralVar *literal(const std::string &value) { return get_literal_string(value); }

	Val *default_value(Type *type);

	LocalVar *create_local_var(Type *type, std::string name);
	PtrVar *create_ptr_var(Type *objType, std::string name);
	GlobalVar *create_global_var(Type *type, std::string name);

	Module *createModule();

	template<typename... Args>
	Class *createClass(Args &&...args) {
		auto node = new Class{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	BasicBlock *createBasicBlock(Args &&...args) {
		auto node = new BasicBlock{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	Function *createFunction(Args &&...args) {
		auto node = new Function{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	AllocaStmt *createAllocaStmt(Args &&...args) {
		auto node = new AllocaStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	StoreStmt *createStoreStmt(Args &&...args) {
		auto node = new StoreStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	LoadStmt *createLoadStmt(Args &&...args) {
		auto node = new LoadStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	ArithmeticStmt *createArithmeticStmt(Args &&...args) {
		auto node = new ArithmeticStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	IcmpStmt *createIcmpStmt(Args &&...args) {
		auto node = new IcmpStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	RetStmt *createRetStmt(Args &&...args) {
		auto node = new RetStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	GetElementPtrStmt *createGetElementPtrStmt(Args &&...args) {
		auto node = new GetElementPtrStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	CallStmt *createCallStmt(Args &&...args) {
		auto node = new CallStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	DirectBrStmt *createDirectBrStmt(Args &&...args) {
		auto node = new DirectBrStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	CondBrStmt *createCondBrStmt(Args &&...args) {
		auto node = new CondBrStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	PhiStmt *createPhiStmt(Args &&...args) {
		auto node = new PhiStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	UnreachableStmt *createUnreachableStmt(Args &&...args) {
		if (!unreachableStmt) {
			unreachableStmt = new UnreachableStmt{std::forward<Args>(args)...};
			nodes.emplace_back(unreachableStmt);
		}
		return unreachableStmt;
	}
	template<typename... Args>
	GlobalStmt *createGlobalStmt(Args &&...args) {
		auto node = new GlobalStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}
	template<typename... Args>
	GlobalStringStmt *createGlobalStringStmt(Args &&...args) {
		auto node = new GlobalStringStmt{std::forward<Args>(args)...};
		nodes.emplace_back(node);
		return node;
	}

public:
	PrimitiveType *voidType = new PrimitiveType{"void", 0};
	PrimitiveType *intType = new PrimitiveType{"i32", 32};
	PrimitiveType *boolType = new PrimitiveType{"i1", 1};
	PrimitiveType *ptrType = new PrimitiveType{"ptr", 32};
	PrimitiveType *stringType = new PrimitiveType{"ptr", 32};

private:// delete at nodes
	Module *module = nullptr;
	UnreachableStmt *unreachableStmt = nullptr;

private:
	LiteralNull *literal_null = nullptr;
	std::map<int, LiteralInt *> literal_ints;
	LiteralBool *literal_bool[2]{nullptr, nullptr};
	std::map<std::string, StringLiteralVar *> literal_strings;

	std::vector<Var *> vars;
	std::vector<IRNode *> nodes;
};

}// namespace IR
