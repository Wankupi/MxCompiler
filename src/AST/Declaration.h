#pragma once
#include "Semantic/Scope.h"
#include <string>

struct AstBaseVisitor;

struct AstNode {
public:
	AstNode() = default;
	virtual ~AstNode() = default;
	virtual void print() = 0;
	virtual std::string NodeType() {
		return "AstBaseNode";
	}
	virtual void accept(AstBaseVisitor *visitor) {}

public:
	Scope *scope = nullptr;
};

struct AstExprNode;
struct AstTypeNode;
struct AstArrayAccessExprNode;
struct AstMemberAccessExprNode;
struct AstBinaryExprNode;
struct AstAtomExprNode;
struct AstAssignExprNode;
struct AstLiterExprNode;
struct AstFuncCallExprNode;
struct AstNewExprNode;
struct AstSingleExprNode;
struct AstTernaryExprNode;
struct AstStmtNode;
struct AstBlockStmtNode;
struct AstVarStmtNode;
struct AstFunctionNode;
struct AstConstructFuncNode;
struct AstClassNode;
struct AstExprStmtNode;
struct AstFlowStmtNode;
struct AstReturnStmtNode;
struct AstBreakStmtNode;
struct AstContinueStmtNode;
struct AstForStmtNode;
struct AstWhileStmtNode;
struct AstIfStmtNode;
struct AstFileNode;
