#include "Wrapper.h"

IR::Wrapper::~Wrapper() {
	delete literal_null;
	for (auto &i: literal_ints)
		delete i.second;
	delete literal_bool[0];
	delete literal_bool[1];
	for (auto &i: literal_strings)
		delete i.second;
	for (auto v: vars)
		delete v;
	for (auto n: nodes)
		delete n;
}

IR::LiteralNull *IR::Wrapper::get_literal_null() {
	if (!literal_null)
		literal_null = new LiteralNull{ptrType};
	return literal_null;
}

IR::LiteralBool *IR::Wrapper::get_literal_bool(bool value) {
	if (!literal_bool[value])
		literal_bool[value] = new LiteralBool{value, boolType};
	return literal_bool[value];
}

IR::LiteralInt *IR::Wrapper::get_literal_int(int value) {
	if (!literal_ints[value])
		literal_ints[value] = new LiteralInt{value, intType};
	return literal_ints[value];
}

IR::StringLiteralVar *IR::Wrapper::get_literal_string(const std::string &value) {
	if (!literal_strings[value]) {
		auto var = new StringLiteralVar{".str." + std::to_string(literal_strings.size()), stringType, value};
		literal_strings[value] = var;
		auto *node = new GlobalStringStmt{var};
		module->stringLiterals.push_back(node);
	}
	return literal_strings[value];
}

IR::LocalVar *IR::Wrapper::create_local_var(IR::Type *type, std::string name) {
	auto *var = new LocalVar{std::move(name), type};
	vars.push_back(var);
	return var;
}

IR::PtrVar *IR::Wrapper::create_ptr_var(IR::Type *objType, std::string name) {
	auto *var = new PtrVar{std::move(name), objType, ptrType};
	vars.push_back(var);
	return var;
}

IR::GlobalVar *IR::Wrapper::create_global_var(IR::Type *type, std::string name) {
	auto *var = new GlobalVar{std::move(name), type};
	vars.push_back(var);
	return var;
}

IR::Module *IR::Wrapper::createModule() {
	if (module) throw std::runtime_error("module already exists");
	module = new Module{};
	nodes.emplace_back(module);
	return module;
}
