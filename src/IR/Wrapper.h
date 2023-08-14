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

	LocalVar *create_local_var(Type *type, std::string name);
	PtrVar *create_ptr_var(Type *objType, std::string name);
	GlobalVar *create_global_var(Type *type, std::string name);

	Module *create_module();

public:
	PrimitiveType *voidType = new PrimitiveType{"void", 0};
	PrimitiveType *intType = new PrimitiveType{"i32", 32};
	PrimitiveType *boolType = new PrimitiveType{"i1", 1};
	PrimitiveType *ptrType = new PrimitiveType{"ptr", 32};
	PrimitiveType *stringType = new PrimitiveType{"ptr", 32};

private:
	Module *module = nullptr;

private:
	LiteralNull *literal_null = nullptr;
	std::map<int, LiteralInt *> literal_ints;
	LiteralBool *literal_bool[2]{nullptr, nullptr};
	std::map<std::string, StringLiteralVar *> literal_strings;

	std::vector<Var *> vars;
	std::vector<IRNode *> nodes;
};

}// namespace IR
