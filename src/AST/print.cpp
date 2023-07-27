#include "AstNode.h"
#include <iostream>

void AstBlockStmtNode::print() {
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

void AstTypeNode::print() {
	std::cout << "\033[32m" << name << "\033[0m";
	for (auto size: arraySize) {
		std::cout << "\033[32m[\033[0m";
		size->print();
		std::cout << "\033[32m]\033[0m";
	}
	for (auto i = arraySize.size(); i < dimension; ++i)
		std::cout << "\033[32m[]\033[0m";
}

void AstArrayAccessExprNode::print() {
	array->print();
	std::cout << '[';
	index->print();
	std::cout << ']';
}

void AstMemberAccessExprNode::print() {
	object->print();
	std::cout << '.' << member;
}

void AstVarStmtNode::print() {
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

void AstFunctionNode::print() {
	returnType->print();
	std::cout << ' ' << name << "(" << std::flush;
	bool first = true;
	for (auto param: params) {
		if (!first)
			std::cout << ", ";
		param.first->print();
		std::cout << " " << param.second;
		first = false;
	}
	std::cout << ") ";
	body->print();
}

void AstConstructFuncNode::print() {
	std::cout << name << "(" << std::flush;
	bool first = true;
	for (auto param: params) {
		if (!first)
			std::cout << ", ";
		param.first->print();
		std::cout << " " << param.second;
		first = false;
	}
	std::cout << ") ";
	body->print();
}

void AstClassNode::print() {
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

void AstBinaryExprNode::print() {
	lhs->print();
	std::cout << ' ' << op << ' ';
	rhs->print();
}

void AstAtomExprNode::print() {
	std::cout << name;
}


void AstExprStmtNode::print() {
	bool first = true;
	for (auto e: expr) {
		if (!first) std::cout << ", ";
		first = false;
		e->print();
	}
	std::cout << ';';
}

void AstAssignExprNode::print() {
	lhs->print();
	std::cout << " = ";
	rhs->print();
}

void AstLiterExprNode::print() {
	std::cout << value;
}

void AstReturnStmtNode::print() {
	std::cout << "return ";
	if (expr)
		expr->print();
	std::cout << ';';
}

void AstBreakStmtNode::print() {
	std::cout << "break;";
}

void AstContinueStmtNode::print() {
	std::cout << "continue;";
}

void AstFuncCallExprNode::print() {
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

void AstNewExprNode::print() {
	std::cout << "new ";
	type->print();
}

void AstForStmtNode::print() {
	std::cout << "for (";
	if (init)
		init->print();
	std::cout << (init ? " " : "; ");
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

void AstSingleExprNode::print() {
	if (!right)
		std::cout << op;
	expr->print();
	if (right)
		std::cout << op;
}

void AstWhileStmtNode::print() {
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

void AstIfStmtNode::print() {
	for (auto cur = ifStmts.begin(); cur != ifStmts.end(); ++cur) {
		if (cur == ifStmts.begin())
			std::cout << "if (";
		else
			std::cout << "\nelse if (";
		cur->first->print();
		std::cout << ") ";
		cur->second->print();
	}
	elseStmt->print();
}

void AstTernaryExprNode::print() {
	cond->print();
	std::cout << " ? ";
	trueExpr->print();
	std::cout << " : ";
	falseExpr->print();
}

void AstFileNode::print() {
	for (auto child: children) {
		child->print();
		std::cout << std::endl;
	}
}
