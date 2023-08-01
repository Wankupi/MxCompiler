#include "Scope.h"

std::string TypeInfo::to_string() const {
	std::string ret = basicType ? basicType->to_string() : "null";
	for (int i = 0; i < dimension; ++i) ret += "[]";
	if (isConst) ret = "const " + ret;
	return ret;
}

TypeInfo TypeInfo::get_member(const std::string &member_name, GlobalScope *scope) {
	if (dimension == 0)
		return basicType->get_member(member_name);
	else
		return scope->query_function_for_array(member_name);
}

bool TypeInfo::is_int() const {
	return basicType && basicType->name == "int" && dimension == 0;
}

bool TypeInfo::is(const std::string &name) const {
	return basicType && basicType->name == name && dimension == 0;
}

bool TypeInfo::is_string() const {
	return basicType && basicType->name == "string" && dimension == 0;
}

bool TypeInfo::is_bool() const {
	return basicType && basicType->name == "bool" && dimension == 0;
}
bool TypeInfo::is_basic() const {
	return basicType && (basicType->name == "int" || basicType->name == "string" || basicType->name == "bool" || basicType->name == "void") && dimension == 0;
}
bool TypeInfo::is_void() const {
	return basicType && basicType->name == "void" && dimension == 0;
}

TypeInfo ClassType::get_member(const std::string &member_name) {
	if (!scope) throw semantic_error("class has no member: " + name + "." + member_name);
	return scope->query_variable_type(member_name);
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
	if (type.basicType->name == "void")
		throw semantic_error("variable type could not be void: " + name);
	vars[name] = type;
	uniqueNames[name] = name + scopeName;
}

TypeInfo Scope::query_variable_type(const std::string &name) {
	auto s = this;
	do {
		if (auto it = s->vars.find(name); it != s->vars.end())
			return it->second;
		else
			s = s->fatherScope;
	} while (s);
	throw semantic_error("variable not found: " + name);
}

std::string Scope::query_var_unique_name(const std::string &name) {
	auto s = this;
	do {
		if (auto it = s->vars.find(name); it != s->vars.end())
			return s->uniqueNames[name];
		else
			s = s->fatherScope;
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
	vars[ArrayFuncPrefix + funcP->name] = {funcP, 0, true};
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

bool GlobalScope::has_class(const std::string &name) {
	return types.contains(name);
}
