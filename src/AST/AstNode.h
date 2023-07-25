#pragma once

#include <string>
#include <vector>

#include "AstBaseVisitor.h"
#include "Declaration.h"

struct AstExprNode : public AstNode {
	Type *valueType = nullptr;
	std::string NodeType() override {
		return "AstExprNode";
	}
	void accept(AstBaseVisitor *visitor) override { return visitor->visitExprNode(this); }
};

struct AstTypeNode : public AstNode {
	std::string name;
	std::vector<AstExprNode *> arraySize;
	size_t dimension = 0;
	std::string NodeType() override {
		return "AstTypeNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitTypeNode(this); }
};

struct AstFuncParamNode : public AstNode {
	AstTypeNode *type;
	std::string name;
	std::string NodeType() override {
		return "AstFuncParamNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitFuncParamNode(this); }
};


struct AstArrayAccessExprNode : public AstExprNode {
	AstExprNode *array = nullptr;
	AstExprNode *index = nullptr;
	std::string NodeType() override {
		return "AstArrayAccessExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitArrayAccessExprNode(this); }
};

struct AstMemberAccessExprNode : public AstExprNode {
	AstExprNode *object = nullptr;
	std::string member;
	std::string NodeType() override {
		return "AstMemberAccessExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitMemberAccessExprNode(this); }
};

struct AstBinaryExprNode : public AstExprNode {
	AstBinaryExprNode(std::string op, AstExprNode *lhs, AstExprNode *rhs)
		: AstExprNode(), op(std::move(op)), lhs(lhs), rhs(rhs) {}
	std::string op;
	AstExprNode *lhs, *rhs;
	std::string NodeType() override {
		return "AstBinaryExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitBinaryExprNode(this); }
};

struct AstAtomExprNode : public AstExprNode {
	std::string name;
	std::string NodeType() override {
		return "AstAtomExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitAtomExprNode(this); }
};

struct AstAssignExprNode : public AstExprNode {
	// only support '=' now
	// std::string op;
	AstExprNode *lhs = nullptr, *rhs = nullptr;
	std::string NodeType() override {
		return "AstAssignExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitAssignExprNode(this); }
};

struct AstLiterExprNode : public AstExprNode {
	std::string value;
	std::string NodeType() override {
		return "AstLiterExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitLiterExprNode(this); }
};

struct AstFuncCallExprNode : public AstExprNode {
	AstExprNode *func = nullptr;
	std::vector<AstExprNode *> args;
	std::string NodeType() override {
		return "AstCallExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitFuncCallExprNode(this); }
};

/**
 * @notice `new Type` will be recognized as a expr returns a construction function
 */
struct AstNewExprNode : public AstExprNode {
	AstTypeNode *type = nullptr;
	std::string NodeType() override {
		return "AstNewExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitNewExprNode(this); }
};

struct AstSingleExprNode : public AstExprNode {
	AstExprNode *expr = nullptr;
	std::string op;
	bool right = false;
	std::string NodeType() override {
		return "AstSingleExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitSingleExprNode(this); }
};

struct AstTernaryExprNode : public AstExprNode {
	AstExprNode *cond = nullptr, *trueExpr = nullptr, *falseExpr = nullptr;
	std::string NodeType() override {
		return "AstTernaryExprNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitTernaryExprNode(this); }
};

struct AstStmtNode : public AstNode {
	using AstNode::AstNode;
	std::string NodeType() override {
		return "AstStmtNode";
	}
	void accept(AstBaseVisitor *visitor) override { return visitor->visitStmtNode(this); }
};

struct AstBlockStmtNode : public AstStmtNode {
	std::vector<AstStmtNode *> stmts;
	std::string NodeType() override {
		return "AstBlockStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitBlockStmtNode(this); }
};

struct AstVarStmtNode : public AstStmtNode {
	AstTypeNode *type;
	std::vector<std::pair<std::string, AstExprNode *>> vars;
	std::string NodeType() override {
		return "AstVariableNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitVarStmtNode(this); }
};

struct AstFunctionNode : public AstNode {
	AstTypeNode *returnType;
	std::string name;
	std::vector<AstFuncParamNode *> params;
	AstStmtNode *body;
	std::string NodeType() override {
		return "AstFunctionNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitFunctionNode(this); }
};

struct AstConstructFuncNode : public AstNode {
	std::string name;
	std::vector<AstFuncParamNode *> params;
	AstStmtNode *body;
	std::string NodeType() override {
		return "AstConstructFuncNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitConstructFuncNode(this); }
};

struct AstClassNode : public AstNode {
	std::string name;
	std::vector<AstVarStmtNode *> variables;
	std::vector<AstConstructFuncNode *> constructors;
	std::vector<AstFunctionNode *> functions;
	std::string NodeType() override {
		return "AstClassNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitClassNode(this); }
};

struct AstExprStmtNode : public AstStmtNode {
	std::vector<AstExprNode *> expr;
	std::string NodeType() override {
		return "AstExprStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitExprStmtNode(this); }
};

struct AstFlowStmtNode : public AstStmtNode {
	std::string NodeType() override {
		return "AstFlowStmtNode";
	}
	void accept(AstBaseVisitor *visitor) override { return visitor->visitFlowStmtNode(this); }
};

struct AstReturnStmtNode : public AstFlowStmtNode {
	AstExprNode *expr = nullptr;
	std::string NodeType() override {
		return "AstReturnStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitReturnStmtNode(this); }
};

struct AstBreakStmtNode : public AstFlowStmtNode {
	std::string NodeType() override {
		return "AstBreakStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitBreakStmtNode(this); }
};

struct AstContinueStmtNode : public AstFlowStmtNode {
	std::string NodeType() override {
		return "AstContinueStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitContinueStmtNode(this); }
};

struct AstForStmtNode : public AstStmtNode {
	AstStmtNode *init = nullptr;
	AstExprNode *cond = nullptr, *step = nullptr;
	std::vector<AstStmtNode *> body;
	std::string NodeType() override {
		return "AstForStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitForStmtNode(this); }
};

struct AstWhileStmtNode : public AstStmtNode {
	AstExprNode *cond = nullptr;
	std::vector<AstStmtNode *> body;
	std::string NodeType() override {
		return "AstWhileStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitWhileStmtNode(this); }
};

struct AstIfStmtNode : public AstStmtNode {
	std::vector<std::pair<AstExprNode *, std::vector<AstStmtNode *>>> ifStmts;
	std::vector<AstStmtNode *> elseStmt;
	std::string NodeType() override {
		return "AstIfStmtNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitIfStmtNode(this); }
};

struct AstFileNode final : public AstNode {
	using AstNode::AstNode;
	std::vector<AstNode *> children;
	std::string NodeType() override {
		return "AstFileNode";
	}
	void print() override;
	void accept(AstBaseVisitor *visitor) override { return visitor->visitFileNode(this); }
};
