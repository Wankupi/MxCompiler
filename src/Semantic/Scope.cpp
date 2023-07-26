#include "Scope.h"

std::string TypeInfo::to_string() const {
	std::string ret = basicType->to_string();
	for (int i = 0; i < dimension; ++i) ret += "[]";
	return ret;
}

TypeInfo TypeInfo::get_member(const std::string &member_name, GlobalScope *scope) {
	if (dimension == 0)
		return basicType->get_member(member_name);
	else
		return scope->query_function_for_array(member_name);
}

TypeInfo ClassType::get_member(const std::string &member_name) {
	if (!scope) throw semantic_error("class has no member: " + name + "." + member_name);
	return scope->query_variable(member_name);
}

std::string FuncType::to_string() const {
	std::string ret = name + "(";
	bool first = true;
	for (auto &arg: args) {
		if (!first) ret += ',';
		first = false;
		ret += arg.to_string();
	}
	ret += ")";
	return ret;
}

TypeInfo FuncType::get_member(const std::string &member_name) {
	throw semantic_error("function has no member: " + name + "." + member_name);
}

void Scope::add_variable(const std::string &name, const TypeInfo &type) {
	if (vars.contains(name))
		throw semantic_error("variable redefinition: " + name);
	vars[name] = type;
}

TypeInfo Scope::query_variable(const std::string &name) {
	auto s = this;
	do {
		if (auto it = s->vars.find(name); it != s->vars.end())
			return it->second;
		else
			s = fatherScope;
	} while (s);
	throw semantic_error("variable not found: " + name);
}

void Scope::add_function(FuncType &&func) {
	if (funcs.contains(func.to_string()))
		throw semantic_error("function redefinition: " + func.to_string());
	auto funcP = new FuncType(func);
	funcs[func.to_string()] = funcP;
	/// @attention 当前禁止重载
	vars[func.name] = {funcP, 0, true};
}

void Scope::add_function_for_array(FuncType &&func) {
	auto id = ArrayFuncPrefix + func.to_string();
	if (funcs.contains(id))
		throw semantic_error("function redefinition: " + id);
	auto funcP = new FuncType(std::move(func));
	funcs[id] = funcP;
	vars[ArrayFuncPrefix + funcP->name] = {funcP, 1, true};
}

TypeInfo Scope::query_function_for_array(const std::string &func_name) {
	if (auto it = vars.find(ArrayFuncPrefix + func_name); it != vars.end())
		return it->second;
	else
		throw semantic_error("function not found: __array__" + func_name);
}

Type *GlobalScope::add_class(const std::string &name) {
	if (types.contains(name))
		throw semantic_error("class redefinition: " + name);
	auto *type = new ClassType;
	type->name = name;
	types.emplace(name, type);
	return type;
}

ClassType *GlobalScope::query_class(const std::string &name) {
	if (auto it = types.find(name); it != types.end())
		return it->second;
	else
		throw semantic_error("class not found: " + name);
}
