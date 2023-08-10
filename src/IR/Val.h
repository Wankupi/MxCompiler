#pragma once
#include "Type.h"

namespace IR {
struct Val {
	Type *type = nullptr;
	virtual void print(std::ostream &out) const;
	[[nodiscard]] virtual std::string get_name() const = 0;
};

struct Var : public Val {
	std::string name;
};

struct StringLiteralVar : public Var {
	[[nodiscard]] std::string get_name() const override;
};

struct GlobalVar : public Var {
	[[nodiscard]] std::string get_name() const override;
};

struct LocalVar : public Var {
	[[nodiscard]] std::string get_name() const override;
};

struct PtrVar : public LocalVar {
	Type *objType = nullptr;
};

struct Literal : public Val {};

struct LiteralBool : public Literal {
	explicit LiteralBool(bool value) : value(value) {}
	bool value;
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};
struct LiteralInt : public Literal {
	explicit LiteralInt(int value) : value(value) {}
	int value;
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};
struct LiteralNull : public Literal {
	void print(std::ostream &out) const override;
	[[nodiscard]] std::string get_name() const override;
};

}// namespace IR
