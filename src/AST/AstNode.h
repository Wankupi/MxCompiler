#pragma once

#include <string>
#include <vector>

#include <format>
#include <iostream>
#include <typeinfo>

using std::string;
using std::vector;

class AstNode {
public:
	AstNode() = default;
	virtual ~AstNode() = default;
	virtual void print() = 0;
	virtual std::string NodeType() {
		return "AstBaseNode";
	}
};

struct AstStmtNode : public AstNode {
	using AstNode::AstNode;
	std::string NodeType() override {
		return "AstStmtNode";
	}
};

struct AstBlockStmtNode : public AstStmtNode {
	std::vector<AstStmtNode *> stmts;
	std::string NodeType() override {
		return "AstBlockStmtNode";
	}
	void print() override {
		if (stmts.empty()) {
			std::cout << "{}";
			return;
		}
		std::cout << "{\n";
		for (auto stmt: stmts) {
			stmt->print();
			std::cout << "\n";
		}
		std::cout << "}";
	}
};
struct AstExprNode : public AstNode {
	using AstNode::AstNode;
	std::string NodeType() override {
		return "AstExprNode";
	}
};

struct AstTypeNode : public AstNode {
	std::string name;
	std::vector<AstExprNode *> arraySize;
	size_t dimension = 0;
	std::string NodeType() override {
		return "AstTypeNode";
	}
	void print() override {
		std::cout << "\033[32m" << name << "\033[0m";
		for (auto size: arraySize) {
			std::cout << "\033[32m[\033[0m";
			size->print();
			std::cout << "\033[32m]\033[0m";
		}
		for (auto i = arraySize.size(); i < dimension; ++i)
			std::cout << "\033[32m[]\033[0m";
	}
};

struct AstFuncParamNode : public AstNode {
	AstTypeNode *type;
	std::string name;
	std::string NodeType() override {
		return "AstFuncParamNode";
	}
	void print() {
		type->print();
		std::cout << ' ' << name;
	}
};


struct AstArrayAccessExprNode : public AstExprNode {
	AstExprNode *array = nullptr;
	AstExprNode *index = nullptr;
	std::string NodeType() override {
		return "AstArrayAccessExprNode";
	}
	void print() override {
		array->print();
		std::cout << '[';
		index->print();
		std::cout << ']';
	}
};

struct AstMemberAccessExprNode : public AstExprNode {
	AstExprNode *object = nullptr;
	std::string member;
	std::string NodeType() override {
		return "AstMemberAccessExprNode";
	}
	void print() override {
		object->print();
		std::cout << '.' << member;
	}
};

struct AstVarStmtNode : public AstStmtNode {
	AstTypeNode *type;
	std::vector<std::pair<std::string, AstExprNode *>> vars;
	std::string NodeType() override {
		return "AstVariableNode";
	}
	void print() override {
		type->print();
		std::cout << " ";
		bool first = true;
		for (auto &var: vars) {
			if (!first)
				std::cout << ", ";
			first = false;
			std::cout << var.first;
			if (var.second) {
				std::cout << " = ";
				var.second->print();
			}
		}
		std::cout << ";";
	}
};

struct AstFunctionNode : public AstNode {
	AstTypeNode *returnType;
	std::string name;
	std::vector<AstFuncParamNode *> params;
	AstStmtNode *body;
	std::string NodeType() override {
		return "AstFunctionNode";
	}
	void print() override {
		returnType->print();
		std::cout << ' ' << name << "(" << std::flush;
		bool first = true;
		for (auto param: params) {
			if (!first)
				std::cout << ", ";
			param->print();
			first = false;
		}
		std::cout << ") ";
		body->print();
	}
};

struct AstConstructFuncNode : public AstNode {
	std::string name;
	std::vector<AstFuncParamNode *> params;
	AstStmtNode *body;
	std::string NodeType() override {
		return "AstConstructFuncNode";
	}
	void print() override {
		std::cout << name << "(" << std::flush;
		bool first = true;
		for (auto param: params) {
			if (!first)
				std::cout << ", ";
			param->print();
			first = false;
		}
		std::cout << ") ";
		body->print();
	}
};

struct AstClassNode final : public AstNode {
	string name;
	std::vector<AstVarStmtNode *> variables;
	std::vector<AstConstructFuncNode *> constructors;
	std::vector<AstFunctionNode *> functions;
	std::string NodeType() override {
		return "AstClassNode";
	}
	void print() override {
		std::cout << "class " << name << " {\n";
		for (auto var: variables) {
			var->print();
			std::cout << "\n";
		}
		for (auto constructor: constructors) {
			constructor->print();
			std::cout << "\n";
		}
		for (auto function: functions) {
			function->print();
			std::cout << "\n";
		}
		std::cout << "};";
	}
};


struct AstBinaryExprNode : public AstExprNode {
	AstBinaryExprNode(std::string op, AstExprNode *lhs, AstExprNode *rhs)
		: AstExprNode(), op(std::move(op)), lhs(lhs), rhs(rhs) {}
	std::string op;
	AstExprNode *lhs, *rhs;
	std::string NodeType() override {
		return "AstBinaryExprNode";
	}
	void print() override {
		lhs->print();
		std::cout << ' ' << op << ' ';
		rhs->print();
	}
};

struct AstFileNode final : public AstNode {
	using AstNode::AstNode;
	std::vector<AstNode *> children;
	std::string NodeType() override {
		return "AstFileNode";
	}
	void print() override {
		for (auto child: children) {
			child->print();
			std::cout << std::endl;
		}
	}
};

struct AstAtomExprNode : public AstExprNode {
	std::string name;
	std::string NodeType() override {
		return "AstAtomExprNode";
	}
	void print() override {
		std::cout << name;
	}
};

struct AstExprStmtNode : public AstStmtNode {
	std::vector<AstExprNode *> expr;
	std::string NodeType() override {
		return "AstExprStmtNode";
	}
	void print() override {
		bool first = true;
		for (auto e: expr) {
			if (!first) std::cout << ", ";
			first = false;
			e->print();
		}
		std::cout << ';';
	}
};

struct AstAssignExprNode : public AstExprNode {
	// only support '=' now
	// std::string op;
	AstExprNode *lhs = nullptr, *rhs = nullptr;
	std::string NodeType() override {
		return "AstAssignExprNode";
	}
	void print() override {
		lhs->print();
		std::cout << " = ";
		rhs->print();
	}
};

struct AstLiterExprNode : public AstExprNode {
	std::string value;
	std::string NodeType() override {
		return "AstLiterExprNode";
	}
	void print() override {
		std::cout << value;
	}
};

struct AstFlowStmtNode : public AstStmtNode {
	std::string NodeType() override {
		return "AstFlowStmtNode";
	}
};

struct AstReturnStmtNode : public AstFlowStmtNode {
	AstExprNode *expr = nullptr;
	std::string NodeType() override {
		return "AstReturnStmtNode";
	}
	void print() override {
		std::cout << "return ";
		if (expr)
			expr->print();
		std::cout << ';';
	}
};

struct AstBreakStmtNode : public AstFlowStmtNode {
	std::string NodeType() override {
		return "AstBreakStmtNode";
	}
	void print() override {
		std::cout << "break;";
	}
};

struct AstContinueStmtNode : public AstFlowStmtNode {
	std::string NodeType() override {
		return "AstContinueStmtNode";
	}
	void print() override {
		std::cout << "continue;";
	}
};

struct AstFuncCallExprNode : public AstExprNode {
	AstExprNode *func = nullptr;
	std::vector<AstExprNode *> args;
	std::string NodeType() override {
		return "AstCallExprNode";
	}
	void print() override {
		func->print();
		std::cout << '(';
		bool first = true;
		for (auto arg: args) {
			if (!first)
				std::cout << ", ";
			first = false;
			arg->print();
		}
		std::cout << ')';
	}
};

/***
 * @notice `new Type` will be recognized as a expr returns a construction function
 */
struct AstNewExprNode : public AstExprNode {
	AstTypeNode *type = nullptr;
	std::string NodeType() override {
		return "AstNewExprNode";
	}
	void print() override {
		std::cout << "new ";
		type->print();
	}
};


struct AstForStmtNode : public AstStmtNode {
	AstExprNode *init = nullptr, *cond = nullptr, *step = nullptr;
	std::vector<AstStmtNode *> body;
	std::string NodeType() override {
		return "AstForStmtNode";
	}
	void print() override {
		std::cout << "for (";
		if (init)
			init->print();
		std::cout << "; ";
		if (cond)
			cond->print();
		std::cout << "; ";
		if (step)
			step->print();
		std::cout << ") ";
		if (body.empty())
			std::cout << ";";
		else if (body.size() == 1)
			body[0]->print();
		else {
			std::cout << "{\n";
			for (auto stmt: body) {
				stmt->print();
				std::cout << "\n";
			}
			std::cout << "}";
		}
	}
};

struct AstSingleExprNode : public AstExprNode {
	AstExprNode *expr = nullptr;
	std::string op;
	bool right = false;
	std::string NodeType() override {
		return "AstSingleExprNode";
	}
	void print() override {
		if (!right)
			std::cout << op;
		expr->print();
		if (right)
			std::cout << op;
	}
};

struct AstWhileStmtNode : public AstStmtNode {
	AstExprNode *cond = nullptr;
	std::vector<AstStmtNode *> body;
	std::string NodeType() override {
		return "AstWhileStmtNode";
	}
	void print() override {
		std::cout << "while (";
		cond->print();
		std::cout << ") ";
		if (body.empty())
			std::cout << ";";
		else if (body.size() == 1)
			body[0]->print();
		else {
			std::cout << "{\n";
			for (auto stmt: body) {
				stmt->print();
				std::cout << "\n";
			}
			std::cout << "}";
		}
	}
};


struct AstIfStmtNode : public AstStmtNode {
	std::vector<std::pair<AstExprNode *, std::vector<AstStmtNode *>>> ifStmts;
	std::vector<AstStmtNode *> elseStmt;
	std::string NodeType() override {
		return "AstIfStmtNode";
	}
	void print() override {
		for (auto cur = ifStmts.begin(); cur != ifStmts.end(); ++cur) {
			if (cur == ifStmts.begin())
				std::cout << "if (";
			else
				std::cout << "\nelse if (";
			cur->first->print();
			std::cout << ") ";
			if (cur->second.empty())
				std::cout << ";";
			else if (cur->second.size() == 1)
				cur->second[0]->print();
			else {
				std::cout << "{\n";
				for (auto stmt: cur->second) {
					stmt->print();
					std::cout << "\n";
				}
				std::cout << "}";
			}
			std::cout << "";
		}
		if (!elseStmt.empty()) {
			std::cout << "\nelse ";
			if (elseStmt.size() == 1)
				elseStmt[0]->print();
			else {
				std::cout << "{\n";
				for (auto stmt: elseStmt) {
					stmt->print();
					std::cout << "\n";
				}
				std::cout << "}";
			}
		}
	}
};

struct AstTernaryExprNode : public AstExprNode {
	AstExprNode *cond = nullptr, *trueExpr = nullptr, *falseExpr = nullptr;
	std::string NodeType() override {
		return "AstTernaryExprNode";
	}
	void print() override {
		cond->print();
		std::cout << " ? ";
		trueExpr->print();
		std::cout << " : ";
		falseExpr->print();
	}
};
