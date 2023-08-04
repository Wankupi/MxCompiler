#pragma once

#include "AST/AstBaseVisitor.h"

class SemanticChecker : public AstBaseVisitor {
public:
	explicit SemanticChecker(GlobalScope *scope) : globalScope(scope), scope(scope) {}
	~SemanticChecker() override = default;
	void visit(AstNode *node) override;

private:
	GlobalScope *globalScope = nullptr;
	Scope *scope = nullptr;
	AstFunctionNode *currentFunction = nullptr;
	int loopDepth = 0;

private:
	TypeInfo enterTypeNode(AstTypeNode *node);
	void visitArrayAccessExprNode(AstArrayAccessExprNode *node) override;
	void visitMemberAccessExprNode(AstMemberAccessExprNode *node) override;
	void visitBinaryExprNode(AstBinaryExprNode *node) override;
	void visitAtomExprNode(AstAtomExprNode *node) override;
	void visitAssignExprNode(AstAssignExprNode *node) override;
	void visitLiterExprNode(AstLiterExprNode *node) override;
	void visitFuncCallExprNode(AstFuncCallExprNode *node) override;
	void visitNewExprNode(AstNewExprNode *node) override;
	void visitSingleExprNode(AstSingleExprNode *node) override;
	void visitTernaryExprNode(AstTernaryExprNode *node) override;
	void visitBlockStmtNode(AstBlockStmtNode *node) override;
	void visitBlockStmtNodeWithoutNewScope(AstBlockStmtNode *node);
	void visitVarStmtNode(AstVarStmtNode *node) override;
	void visitFunctionNode(AstFunctionNode *node) override;
	void enterClassNode(AstClassNode *node);
	void visitExprStmtNode(AstExprStmtNode *node) override;
	void visitReturnStmtNode(AstReturnStmtNode *node) override;
	void visitBreakStmtNode(AstBreakStmtNode *node) override;
	void visitContinueStmtNode(AstContinueStmtNode *node) override;
	void visitForStmtNode(AstForStmtNode *node) override;
	void visitWhileStmtNode(AstWhileStmtNode *node) override;
	void visitIfStmtNode(AstIfStmtNode *node) override;
	void visitFileNode(AstFileNode *node) override;
};
