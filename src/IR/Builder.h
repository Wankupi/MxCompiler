#pragma once

#include "AST/AstBaseVisitor.h"
#include "Node.h"
#include <stack>

class IRBuilder : public AstBaseVisitor {
private:
	IR::Module *module = nullptr;
	IR::Class *currentClass = nullptr;
	IR::Function *currentFunction = nullptr;
	std::map<std::string, IR::Function *> name2function;
	std::map<AstNode *, IR::Val *> exprResult;
	std::map<std::string, IR::Var *> name2var;
	std::map<std::string, IR::Class *> name2class;
	std::stack<IR::Block *> loopBreakTo;
	std::stack<IR::Block *> loopContinueTo;
	//	IR::Block *returnTo = nullptr;
	int annoyCounter = 0;
	int loopCounter = 0;
	int ifCounter = 0;
	int continueCounter = 0;
	int breakCounter = 0;
	int andOrCounter = 0;

public:
	IRBuilder() = default;
	[[nodiscard]] IR::Module *getIR() const {
		return module;
	}

private:
	void init_builtin_function();
	void visitFileNode(AstFileNode *node) override;
	void visitClassNode(AstClassNode *node) override;
	void visitFunctionNode(AstFunctionNode *node) override;
	void visitConstructFuncNode(AstConstructFuncNode *node) override;

	//	void enterGlobalVarNode(AstVarStmtNode *node);

	void visitVarStmtNode(AstVarStmtNode *node) override;
	void visitBlockStmtNode(AstBlockStmtNode *node) override;
	void visitExprStmtNode(AstExprStmtNode *node) override;

	// expressions
	//	void visitArrayAccessExprNode(AstArrayAccessExprNode *node) override;
	void visitMemberAccessExprNode(AstMemberAccessExprNode *node) override;
	void visitBinaryExprNode(AstBinaryExprNode *node) override;

	void enterAndOrBinaryExprNode(AstBinaryExprNode *node);

	void visitAtomExprNode(AstAtomExprNode *node) override;
	void visitAssignExprNode(AstAssignExprNode *node) override;
	void visitLiterExprNode(AstLiterExprNode *node) override;
	//	void visitFuncCallExprNode(AstFuncCallExprNode *node) override;
	void visitNewExprNode(AstNewExprNode *node) override;
	//	void visitSingleExprNode(AstSingleExprNode *node) override;
	//	void visitTernaryExprNode(AstTernaryExprNode *node) override;


	// statements
	//	void visitFlowStmtNode(AstFlowStmtNode *node) override;
	void visitReturnStmtNode(AstReturnStmtNode *node) override;
	void visitBreakStmtNode(AstBreakStmtNode *node) override;
	void visitContinueStmtNode(AstContinueStmtNode *node) override;
	void visitForStmtNode(AstForStmtNode *node) override;
	void visitWhileStmtNode(AstWhileStmtNode *node) override;
	void visitIfStmtNode(AstIfStmtNode *node) override;


	IR::Val *remove_variable_pointer(IR::Val *val);

private:
	static IR::PrimitiveType *toIRType(AstTypeNode *node);
	static IR::PrimitiveType *toIRType(TypeInfo typeInfo);
	void add_stmt(IR::Stmt *node);
	void add_block(IR::Block *block);
	void add_local_var(IR::LocalVar *node);
	IR::LocalVar *register_annoy_var(IR::Type *type);
	IR::PtrVar *register_annoy_ptr_var(IR::Type *obj_type);
	void push_loop(IR::Block *step, IR::Block *after);
	void pop_loop();
};
