#pragma once

#include "AST/AstBaseVisitor.h"
#include "IR/Wrapper.h"
#include <stack>

class IRBuilder : public AstBaseVisitor {
private:
	IR::Wrapper &env;
	IR::Module *module = nullptr;
	IR::Class *currentClass = nullptr;
	IR::Function *currentFunction = nullptr;
	std::map<std::string, IR::Function *> name2function;
	std::map<AstNode *, IR::Val *> exprResult;
	std::map<std::string, IR::Var *> name2var;
	std::map<std::string, IR::Class *> name2class;
	std::map<std::string, IR::StringLiteralVar *> literalStr2var;
	std::stack<IR::BasicBlock *> loopBreakTo;
	std::stack<IR::BasicBlock *> loopContinueTo;
	std::map<std::string, int> annoyCounter;
	int loopCounter = 0;
	int ifCounter = 0;
	int continueCounter = 0;
	int breakCounter = 0;
	int returnCounter = 0;
	int andOrCounter = 0;
	int ternaryCounter = 0;
	int newCounter = 0;

	/// @brief <(GlobalStmt|GlobalStringStmt),Expr>
	std::vector<std::pair<IR::Stmt *, AstExprNode *>> globalInitList;

public:
	explicit IRBuilder(IR::Wrapper &wrapper) : env(wrapper) {}
	[[nodiscard]] IR::Module *getIR() const {
		return module;
	}

private:
	void init_builtin_function();
	void visitFileNode(AstFileNode *node) override;
	void registerClass(AstClassNode *node);
	void registerFunction(AstFunctionNode *node);
	void registerGlobalVar(AstVarStmtNode *node);
	void visitVarStmtNode(AstVarStmtNode *node) override;
	void visitClassNode(AstClassNode *node) override;
	void visitFunctionNode(AstFunctionNode *node) override;
	void visitBlockStmtNode(AstBlockStmtNode *node) override;
	void visitExprStmtNode(AstExprStmtNode *node) override;

	// expressions
	void visitArrayAccessExprNode(AstArrayAccessExprNode *node) override;
	void visitMemberAccessExprNode(AstMemberAccessExprNode *node) override;
	void visitBinaryExprNode(AstBinaryExprNode *node) override;

	void enterAndOrBinaryExprNode(AstBinaryExprNode *node);
	void enterStringBinaryExprNode(AstBinaryExprNode *node);

	void visitAtomExprNode(AstAtomExprNode *node) override;
	void visitAssignExprNode(AstAssignExprNode *node) override;
	void visitLiterExprNode(AstLiterExprNode *node) override;
	void visitFuncCallExprNode(AstFuncCallExprNode *node) override;
	void visitNewExprNode(AstNewExprNode *node) override;
	void visitSingleExprNode(AstSingleExprNode *node) override;
	void visitTernaryExprNode(AstTernaryExprNode *node) override;
	IR::Var *TransformNewToFor(std::vector<IR::Val *> const &array_size, int total_dim, std::string const &base_typename, int dep = 0);

	// statements
	void visitReturnStmtNode(AstReturnStmtNode *node) override;
	void visitBreakStmtNode(AstBreakStmtNode *node) override;
	void visitContinueStmtNode(AstContinueStmtNode *node) override;
	void visitForStmtNode(AstForStmtNode *node) override;
	void visitWhileStmtNode(AstWhileStmtNode *node) override;
	void visitIfStmtNode(AstIfStmtNode *node) override;


	IR::Val *remove_variable_pointer(IR::Val *val);

private:
	IR::PrimitiveType *toIRType(AstTypeNode *node);
	IR::PrimitiveType *toIRType(TypeInfo typeInfo);
	void add_stmt(IR::Stmt *node);
	void add_phi(IR::PhiStmt *phi);
	void add_block(IR::BasicBlock *block);
	void add_local_var(IR::LocalVar *node);
	void add_global_var(IR::GlobalVar *node);
	IR::LocalVar *register_annoy_var(IR::Type *type, std::string const &prefix = "");
	IR::PtrVar *register_annoy_ptr_var(IR::Type *obj_type, std::string const &prefix = "");
	void push_loop(IR::BasicBlock *step, IR::BasicBlock *after);
	void pop_loop();
	IR::StringLiteralVar *register_literal_str(const std::string &str);

	void init_function_params(IR::Function *func);
	void create_init_global_var_function();
	IR::Val *type_to_default_value(IR::Type *type);
	void add_terminals(IR::Function *func);
};
