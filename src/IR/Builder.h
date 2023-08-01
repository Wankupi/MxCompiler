#pragma once

#include "AST/AstBaseVisitor.h"
#include "Node.h"


class IRBuilder : public AstBaseVisitor {
private:
	IR::Module *module = nullptr;
	IR::Function *currentFunction = nullptr;
	std::map<std::string, IR::Function *> name2function;
	std::map<AstNode *, IR::Val *> exprResult;
	std::map<std::string, IR::Var *> name2var;
	int annoyCounter = 0;

public:
	IRBuilder() = default;
	[[nodiscard]] IR::Module *getIR() const {
		return module;
	}

private:
	void visitFileNode(AstFileNode *node) override;
	void visitClassNode(AstClassNode *node) override;
	void visitFunctionNode(AstFunctionNode *node) override;

	//	void enterGlobalVarNode(AstVarStmtNode *node);

	void visitVarStmtNode(AstVarStmtNode *node) override;
	void visitBlockStmtNode(AstBlockStmtNode *node) override;
	void visitExprStmtNode(AstExprStmtNode *node) override;

	// expressions
	//	void visitArrayAccessExprNode(AstArrayAccessExprNode *node) override;
	//	void visitMemberAccessExprNode(AstMemberAccessExprNode *node) override;
	void visitBinaryExprNode(AstBinaryExprNode *node) override;
	void visitAtomExprNode(AstAtomExprNode *node) override;
	void visitAssignExprNode(AstAssignExprNode *node) override;
	void visitLiterExprNode(AstLiterExprNode *node) override;
	//	void visitFuncCallExprNode(AstFuncCallExprNode *node) override;
	//	void visitNewExprNode(AstNewExprNode *node) override;
	//	void visitSingleExprNode(AstSingleExprNode *node) override;
	//	void visitTernaryExprNode(AstTernaryExprNode *node) override;

	IR::Val *remove_variable_pointer(IR::Val *val);

private:
	static IR::PrimitiveType *toIRType(AstTypeNode *node);
	void add_stmt(IR::Stmt *node);
	void add_local_var(IR::LocalVar *node);
	IR::LocalVar *register_annoy_var(IR::Type *type);
};
