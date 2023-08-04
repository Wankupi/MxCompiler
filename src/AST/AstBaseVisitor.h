#pragma once
#include "Declaration.h"

/**
 * @brief Base class for all AST visitors.
 * @details default behavior is doing nothing.
 */
class AstBaseVisitor {
public:
	virtual ~AstBaseVisitor() = default;
	virtual void visit(AstNode *node) { node->accept(this); }
	virtual void visitExprNode(AstExprNode *node){};
	virtual void visitTypeNode(AstTypeNode *node){};
	virtual void visitArrayAccessExprNode(AstArrayAccessExprNode *node){};
	virtual void visitMemberAccessExprNode(AstMemberAccessExprNode *node){};
	virtual void visitBinaryExprNode(AstBinaryExprNode *node){};
	virtual void visitAtomExprNode(AstAtomExprNode *node){};
	virtual void visitAssignExprNode(AstAssignExprNode *node){};
	virtual void visitLiterExprNode(AstLiterExprNode *node){};
	virtual void visitFuncCallExprNode(AstFuncCallExprNode *node){};
	virtual void visitNewExprNode(AstNewExprNode *node){};
	virtual void visitSingleExprNode(AstSingleExprNode *node){};
	virtual void visitTernaryExprNode(AstTernaryExprNode *node){};
	virtual void visitStmtNode(AstStmtNode *node){};
	virtual void visitBlockStmtNode(AstBlockStmtNode *node){};
	virtual void visitVarStmtNode(AstVarStmtNode *node){};
	virtual void visitFunctionNode(AstFunctionNode *node){};
	virtual void visitClassNode(AstClassNode *node){};
	virtual void visitExprStmtNode(AstExprStmtNode *node){};
	virtual void visitFlowStmtNode(AstFlowStmtNode *node){};
	virtual void visitReturnStmtNode(AstReturnStmtNode *node){};
	virtual void visitBreakStmtNode(AstBreakStmtNode *node){};
	virtual void visitContinueStmtNode(AstContinueStmtNode *node){};
	virtual void visitForStmtNode(AstForStmtNode *node){};
	virtual void visitWhileStmtNode(AstWhileStmtNode *node){};
	virtual void visitIfStmtNode(AstIfStmtNode *node){};
	virtual void visitFileNode(AstFileNode *node) = 0;
};
