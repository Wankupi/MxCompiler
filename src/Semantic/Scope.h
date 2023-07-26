#pragma once

#include "MxException.h"
#include <map>
#include <string>
#include <vector>

struct Type;
struct Scope;
struct GlobalScope;

struct TypeInfo {
public:
	Type *basicType = nullptr;
	size_t dimension;
	bool isConst;

public:
	bool operator==(TypeInfo const &rhs) const {
		return basicType == rhs.basicType && dimension == rhs.dimension;
	}
	bool assignable(TypeInfo const &rhs) const {
		if (isConst) return false;
		return *this == rhs;
	}
	std::string to_string() const;
	TypeInfo get_member(std::string const &member_name, GlobalScope *scope);
};

struct Type {
	std::string name;

public:
	~Type() = default;
	virtual std::string to_string() const = 0;
	virtual TypeInfo get_member(std::string const &member_name) = 0;
};

struct ClassType : public Type {
	Scope *scope = nullptr;

public:
	std::string to_string() const override { return name; }
	TypeInfo get_member(const std::string &member_name) override;
};

struct FuncType : public Type {
	TypeInfo returnType;
	std::vector<TypeInfo> args;

public:
	std::string to_string() const override;
	TypeInfo get_member(const std::string &member_name) override;
};

class Scope {
	friend int main();
private:
	Scope *fatherScope = nullptr;
	std::vector<Scope *> subScopes;

protected:
	std::map<std::string, TypeInfo> vars;
	std::map<std::string, FuncType *> funcs;
	static constexpr const char *const ArrayFuncPrefix = "__array__";

public:
	~Scope() {
		for (auto &scope: subScopes)
			delete scope;
		subScopes.clear();
	}
	Scope *add_sub_scope() {
		auto *scope = new Scope;
		scope->fatherScope = this;
		subScopes.push_back(scope);
		return scope;
	}

	void add_variable(std::string const &name, TypeInfo const &type) ;

	TypeInfo query_variable(std::string const &name);

	void add_function(FuncType &&func);
	void add_function_for_array(FuncType &&func);
	TypeInfo query_function_for_array(std::string const &func_name);
};

class GlobalScope : public Scope {
	friend int main();
	std::map<std::string, ClassType *> types;

public:
	~GlobalScope() {
		for (auto &type: types)
			delete type.second;
		types.clear();
		for (auto &func: funcs)
			delete func.second;
		funcs.clear();
	}

	Type *add_class(std::string const &name);
	ClassType *query_class(std::string const &name);
};
