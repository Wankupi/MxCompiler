#pragma once
#include <map>
#include <string>
#include <vector>

namespace IR {

struct Type {
	[[nodiscard]] virtual std::string to_string() const = 0;
	virtual ~Type() = default;
};

struct PrimitiveType : public Type {};

struct VoidType : public PrimitiveType {
	[[nodiscard]] std::string to_string() const override {
		return "void";
	}
};

struct IntegerType : public PrimitiveType {
	int bits;

	explicit IntegerType(int bit_width) : bits(bit_width) {}
	[[nodiscard]] std::string to_string() const override {
		return "i" + std::to_string(bits);
	}
};

struct PtrType : public PrimitiveType {
	[[nodiscard]] std::string to_string() const override {
		return "ptr";
	}
};

struct ClassType : public Type {
	std::string name;
	std::vector<PrimitiveType *> fields;
	[[nodiscard]] std::string to_string() const override {
		return "%class." + name;
	}
};

}// namespace IR