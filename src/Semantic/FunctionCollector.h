#pragma once

#include "AST/AstNode.h"
#include "Scope.h"

class FunctionCollector : public AstBaseVisitor {
private:
	GlobalScope *globalScope;

public:
	explicit FunctionCollector(GlobalScope *scope) : globalScope(scope) {}
	~FunctionCollector() override = default;

	void init_builtin_functions();

private:
	void visitFileNode(AstFileNode *node) override {
		for (auto &child: node->children)
			visit(child);
	}

	void visitFunctionNode(AstFunctionNode *node) override {
		enterFunctionNode(node, globalScope);
	}

	void enterVarStmtNode(AstVarStmtNode *node, Scope *class_scope) {
		auto type = parseType(node->type);
		for (auto &var: node->vars)
			class_scope->add_variable(var.first, type);
	}

	void enterFunctionNode(AstFunctionNode *node, Scope *class_scope) {
		FuncType f;
		f.name = node->name;
		f.returnType = parseType(node->returnType);
		f.args.reserve(node->params.size());
		for (auto &param: node->params)
			f.args.push_back(parseType(param.first));
		class_scope->add_function(std::move(f));
	}

	void enterConstructFuncNode(AstConstructFuncNode *node, Scope *class_scope) {
		FuncType f;
		f.name = node->name;
		f.returnType = {nullptr, 0, false};
		f.args.reserve(node->params.size());
		for (auto &param: node->params)
			f.args.push_back(parseType(param.first));
		class_scope->add_function(std::move(f));
	}

	void visitClassNode(AstClassNode *node) override {
		node->scope = globalScope->add_sub_scope();
		globalScope->query_class(node->name)->scope = node->scope;
		node->scope->add_variable("this", {globalScope->query_class(node->name), 0, false});
		for (auto &var: node->variables)
			enterVarStmtNode(var, node->scope);
		for (auto &constructor: node->constructors)
			enterConstructFuncNode(constructor, node->scope);
		for (auto &function: node->functions)
			enterFunctionNode(function, node->scope);
	}

private:
	TypeInfo parseType(AstTypeNode *node) {
		TypeInfo ret;
		ret.basicType = globalScope->query_class(node->name);
		ret.dimension = node->dimension;
		ret.isConst = false;
		return ret;
	}
};