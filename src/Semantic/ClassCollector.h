#pragma once

#include "AST/AstNode.h"
#include "Scope.h"

class ClassCollector : public AstBaseVisitor {
private:
	GlobalScope *scope;

public:
	explicit ClassCollector(GlobalScope *scope) : scope(scope) {}
	~ClassCollector() override = default;

	void init_builtin_classes();
private:
	void visitFileNode(AstFileNode *node) override {
		for (auto &child: node->children)
			visit(child);
	}
	void visitClassNode(AstClassNode *node) override {
		scope->add_class(node->name);
	}
};
