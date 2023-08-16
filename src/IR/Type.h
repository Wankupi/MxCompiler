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

struct ClassType : public Type {
	std::string name;
	std::vector<PrimitiveType *> fields;
	[[nodiscard]] std::string to_string() const override {
		return "%class." + name;
	}
	[[nodiscard]] int size() const override {
		return static_cast<int>(fields.size()) * 4;
	}
};

}// namespace IR
