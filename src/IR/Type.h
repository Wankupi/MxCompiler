#pragma once
#include <map>
#include <string>
#include <vector>

namespace IR {

struct Type {
	[[nodiscard]] virtual std::string to_string() const = 0;
	[[nodiscard]] virtual int size() const = 0;
	virtual ~Type() = default;
};

struct PrimitiveType : public Type {
	std::string name;
	int bits = 0;
	PrimitiveType(std::string name, int bits) : name(std::move(name)), bits(bits) {}
	[[nodiscard]] std::string to_string() const override {
		return name;
	}
	[[nodiscard]] int size() const override { return (bits + 7) / 8; }
};

/*
struct VoidType : public PrimitiveType {
	[[nodiscard]] std::string to_string() const override {
		return "void";
	}
	[[nodiscard]] int size() const override { return 0; }
};

struct IntegerType : public PrimitiveType {
	int bits;

	explicit IntegerType(int bit_width) : bits(bit_width) {}
	[[nodiscard]] std::string to_string() const override {
		return "i" + std::to_string(bits);
	}
	[[nodiscard]] int size() const override { return (bits + 7) / 8; }
};

struct PtrType : public PrimitiveType {
	[[nodiscard]] std::string to_string() const override {
		return "ptr";
	}
	[[nodiscard]] int size() const override { return 4; }
};

struct StringType : public PtrType {};
*/

struct ClassType : public Type {
	std::string name;
	std::vector<PrimitiveType *> fields;
	[[nodiscard]] std::string to_string() const override {
		return "%class." + name;
	}
	[[nodiscard]] int size() const override {
		int ret = 0;
		for (auto &f: fields)
			ret += f->size();
		return ret;
	}
};


}// namespace IR
