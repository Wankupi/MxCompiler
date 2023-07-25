#pragma once

#include "MxException.h"
#include <map>
#include <utility>
#include <vector>

struct Type;

struct ScopeInterface {
	virtual Type *query(std::string const &name) = 0;
};

struct Type {
	explicit Type(std::string name) : name{std::move(name)} {}
	std::string name;
	bool isConst = false;
	virtual Type *get_member(std::string const &name) = 0;
	virtual bool operator==(Type const &rhs) const = 0;
	[[nodiscard]] virtual bool assignable(Type const &rhs) const = 0;
};

struct ObjectType : public Type {
	explicit ObjectType(std::string name) : Type{std::move(name)} {}
	std::map<std::string, Type *> vars;// include functions

	Type *get_member(std::string const &member) override {
		if (auto it = vars.find(member); it != vars.end())
			return it->second;
		else
			throw semantic_error("no such member in class(" + this->name + "): " + member);
	}
	bool operator==(Type const &rhs) const override {
		auto r = dynamic_cast<ObjectType const *>(&rhs);
		if (r == nullptr) return false;
		return this->name == r->name;
	}
	[[nodiscard]] bool assignable(Type const &rhs) const override {
		auto r = dynamic_cast<ObjectType const *>(&rhs);
		if (r == nullptr) return false;
		return this->name == r->name;
	}
};

struct FuncType : public Type {
	FuncType(std::string name, Type *returnType, std::vector<Type *> args)
		: Type{std::move(name)}, returnType{returnType}, args{std::move(args)} {}
	Type *returnType = nullptr;
	std::vector<Type *> args;
	Type *get_member(std::string const &) override {
		throw semantic_error("function has no members.");
	}
};

struct ArrayType : public Type {
	static FuncType *const func_size;
	Type *baseType = nullptr;
	int dimension = 0;
	Type *get_member(std::string const &member) override {
		if (member == "size")
			return func_size;
		else
			throw semantic_error("array can only access member size.");
	}
};

struct Scope {
	Scope *fatherScope;
	std::map<std::string, Type *> vars;
};

struct GlobalScope : public Scope {
	void add_class(std::string const &name, ObjectType *type) {
		if (types.contains(name))
			throw semantic_error("class redefinition: " + name);
		types[name] = type;
	}
	void add_function(std::string const &name, FuncType *type) {
		if (types.contains(name))
			throw semantic_error("function redefinition: " + name);
		vars[name] = type;
	}
	std::map<std::string, Type *> types;
};
