#pragma once

#include "AST/AstNode.h"
#include "Scope.h"

class ClassCollector : public AstBaseVisitor {
private:
	GlobalScope *scope;

public:
	explicit ClassCollector(GlobalScope *scope) : scope(scope) {}
	~ClassCollector() override = default;

private:
	void visitFileNode(AstFileNode *node) override {
		scope->add_class("void", new ObjectType("void"));
		scope->add_class("bool", new ObjectType("bool"));
		scope->add_class("int", new ObjectType("int"));
		scope->add_class("string", new ObjectType("string"));
		for (auto &child: node->children)
			visit(child);
	}
	void visitClassNode(AstClassNode *node) override {
		auto c = new ObjectType(node->name);
		scope->add_class(node->name, c);
	}
};
