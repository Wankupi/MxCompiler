#pragma once
#include "Type.h"

namespace IR {
struct Val {
	Type *type = nullptr;

	explicit Val(Type *type) : type(type) {}
	virtual void print(std::ostream &out) const;
	[[nodiscard]] virtual std::string get_name() const = 0;
	virtual ~Val() = default;
};

struct Var : public Val {
	std::string name;
	Var(std::string name, Type *type) : name(std::move(name)), Val(type) {}
};

struct StringLiteralVar : public Var {
	StringLiteralVar(std::string name, Type *type, std::string value) : Var(std::move(name), type), value(std::move(value)) {}
	std::string value;
	[[nodiscard]] std::string get_name() const override;
};

struct GlobalVar : public Var {
	using Var::Var;
	[[nodiscard]] std::string get_name() const override;
};

struct LocalVar : public Var {
	using Var::Var;
	[[nodiscard]] std::string get_name() const override;
};

struct PtrVar : public LocalVar {
	PtrVar(std::string name, Type *objType, Type *ptrType) : LocalVar(std::move(name), ptrType), objType(objType) {}
	Type *objType = nullptr;
};

struct Literal : public Val {
	using Val::Val;
};

struct LiteralBool : public Literal {
	explicit LiteralBool(bool value, Type *type) : value(value), Literal(type) {}
	bool value;
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};
struct LiteralInt : public Literal {
	explicit LiteralInt(int value, Type *type) : value(value), Literal(type) {}
	int value;
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};
struct LiteralNull : public Literal {
	using Literal::Literal;
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};

}// namespace IR
