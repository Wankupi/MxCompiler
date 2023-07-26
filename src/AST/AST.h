#pragma once

#include "AstNode.h"

struct AST {
	explicit AST(AstNode *root) : root(root) {}
	~AST() { delete root; }
	AstNode *root;
};
