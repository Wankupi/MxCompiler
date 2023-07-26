#include "ClassCollector.h"
#include "FunctionCollector.h"
#include "Scope.h"

void ClassCollector::init_builtin_classes() {
	scope->add_class("void");
	scope->add_class("bool");
	scope->add_class("int");
	scope->add_class("string");
}

void FunctionCollector::init_builtin_functions() {
	FuncType f;
	// void print(string)
	f.name = "print";
	f.returnType = {globalScope->query_class("void"), 0, true};
	f.args = {{globalScope->query_class("string"), 0, false}};
	globalScope->add_function(std::move(f));

	// void println(string)
	f.name = "println";
	f.returnType = {globalScope->query_class("void"), 0, true};
	f.args = {{globalScope->query_class("string"), 0, false}};
	globalScope->add_function(std::move(f));

	// void printInt(int n)
	f.name = "printInt";
	f.returnType = {globalScope->query_class("void"), 0, true};
	f.args = {{globalScope->query_class("int"), 0, false}};
	globalScope->add_function(std::move(f));

	// void printlnInt(int n)
	f.name = "printlnInt";
	f.returnType = {globalScope->query_class("void"), 0, true};
	f.args = {{globalScope->query_class("int"), 0, false}};
	globalScope->add_function(std::move(f));

	// string getString()
	f.name = "getString";
	f.returnType = {globalScope->query_class("string"), 0, true};
	f.args = {};
	globalScope->add_function(std::move(f));

	// int getInt()
	f.name = "getInt";
	f.returnType = {globalScope->query_class("int"), 0, true};
	f.args = {};
	globalScope->add_function(std::move(f));

	// string toString(int i)
	f.name = "toString";
	f.returnType = {globalScope->query_class("string"), 0, true};
	f.args = {{globalScope->query_class("int"), 0, false}};
	globalScope->add_function(std::move(f));

	auto scope = globalScope->query_class("string")->scope = globalScope->add_sub_scope();

	// int string::length()
	f.name = "length";
	f.returnType = {globalScope->query_class("int"), 0, true};
	f.args = {};
	scope->add_function(std::move(f));

	// string string::substring(int left, int right)
	f.name = "substring";
	f.returnType = {globalScope->query_class("string"), 0, true};
	f.args = {{globalScope->query_class("int"), 0, false},
			  {globalScope->query_class("int"), 0, false}};
	scope->add_function(std::move(f));

	// int string::parseInt()
	f.name = "parseInt";
	f.returnType = {globalScope->query_class("int"), 0, true};
	f.args = {};
	scope->add_function(std::move(f));

	// int string::ord(int pos)
	f.name = "ord";
	f.returnType = {globalScope->query_class("int"), 0, true};
	f.args = {{globalScope->query_class("int"), 0, false}};
	scope->add_function(std::move(f));

	// int type[]::size()
	f.name = "size";
	f.returnType = {globalScope->query_class("int"), 0, true};
	f.args = {};
	globalScope->add_function_for_array(std::move(f));
}