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

	void enterFunctionNode(AstFunctionNode *node, Scope *class_scope, bool is_constructor = false) {
		if (!is_constructor && globalScope->has_class(node->name))
			throw semantic_error("function name conflict with class: " + node->name);
		FuncType f;
		f.name = node->name;
		f.returnType = parseType(node->returnType);
		f.args.reserve(node->params.size());
		for (auto &param: node->params)
			f.args.push_back(parseType(param.first));
		class_scope->add_function(std::move(f));
	}

	void visitClassNode(AstClassNode *node) override {
		node->scope = globalScope->add_sub_scope();
		globalScope->query_class(node->name)->scope = node->scope;
		node->scope->scopeName = "";
		node->scope->add_variable("this", {globalScope->query_class(node->name), 0, false});
		for (auto &var: node->variables) {
			enterVarStmtNode(var, node->scope);
		}
		node->scope->scopeName = "-" + node->name;
		for (auto &constructor: node->constructors) {
			if (constructor->name != node->name)
				throw semantic_error("construct function should have the same name as class");
			enterFunctionNode(constructor, node->scope, true);
		}
		for (auto &function: node->functions)
			enterFunctionNode(function, node->scope);
	}

private:
	TypeInfo parseType(AstTypeNode *node) {
		TypeInfo ret;
		ret.basicType = globalScope->query_class(node ? node->name : "void");
		ret.dimension = node ? node->dimension : 0;
		ret.isConst = false;
		return ret;
	}
};