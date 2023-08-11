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
	bool operator==(Type *rhs) const {
		return basicType == rhs && dimension == 0;
	}
	bool operator!=(TypeInfo const &rhs) const {
		return !(*this == rhs);
	}
	bool operator!=(Type *rhs) const {
		return !(*this == rhs);
	}
	[[nodiscard]] bool assignable(TypeInfo const &rhs) const {
		if (isConst) return false;
		return convertible(rhs);
	}
	[[nodiscard]] bool convertible(TypeInfo const &rhs) const {
		if (rhs.is_null()) return dimension > 0 || !is_basic();
		return (basicType == rhs.basicType) && dimension == rhs.dimension;
	}
	[[nodiscard]] std::string to_string_full() const;
	[[nodiscard]] std::string to_string() const;
	TypeInfo get_member(std::string const &member_name, GlobalScope *scope) const;
	[[nodiscard]] bool is_null() const { return !basicType; }
	[[nodiscard]] bool is_void() const;
	[[nodiscard]] bool is_int() const;
	[[nodiscard]] bool is_string() const;
	[[nodiscard]] bool is_bool() const;
	[[nodiscard]] bool is_basic() const;
	[[nodiscard]] bool is(std::string const &name) const;
	[[nodiscard]] bool is_function() const;
};

struct Type {
	std::string name;

public:
	~Type() = default;
	[[nodiscard]] virtual std::string to_string() const = 0;
	virtual TypeInfo get_member(std::string const &member_name) = 0;
};

struct ClassType : public Type {
	Scope *scope = nullptr;

public:
	[[nodiscard]] std::string to_string() const override { return name; }
	TypeInfo get_member(const std::string &member_name) override;
};

struct FuncType : public Type {
	TypeInfo returnType;
	std::vector<TypeInfo> args;

public:
	[[nodiscard]] std::string to_string() const override;
	TypeInfo get_member(const std::string &member_name) override;
};

class Scope {
public:
	std::string scopeName;

private:
	Scope *fatherScope = nullptr;
	std::vector<Scope *> subScopes;

protected:
	std::map<std::string, TypeInfo> vars;
	std::map<std::string, FuncType *> funcs;
	std::map<std::string, std::string> uniqueNames;

	static constexpr const char *const ArrayFuncPrefix = "__array__";

public:
	virtual ~Scope() {
		for (auto &scope: subScopes)
			delete scope;
		subScopes.clear();
		for (auto &func: funcs)
			delete func.second;
		funcs.clear();
	}
	Scope *add_sub_scope() {
		auto *scope = new Scope;
		scope->fatherScope = this;
		subScopes.push_back(scope);
		scope->scopeName = scopeName + "-" + std::to_string(subScopes.size());
		return scope;
	}

	void add_variable(std::string const &name, TypeInfo const &type);

	TypeInfo query_variable_type(std::string const &name);
	std::string query_var_unique_name(std::string const &name);

	void add_function(FuncType &&func);
	void add_function_for_array(FuncType &&func);
	TypeInfo query_function_for_array(std::string const &func_name);
};

class GlobalScope : public Scope {
	std::map<std::string, ClassType *> types;

public:
	~GlobalScope() override {
		for (auto &type: types)
			delete type.second;
		types.clear();
	}

	Type *add_class(std::string const &name);
	ClassType *query_class(std::string const &name);
	bool has_class(std::string const &name);
};
