#include "SemanticChecker.h"
#include "AST/AstNode.h"
#include <cctype>

void SemanticChecker::visit(AstNode *node) {
	auto current_scope = node->scope = scope;
	node->accept(this);
	scope = current_scope;
}

void SemanticChecker::visitFileNode(AstFileNode *node) {
	for (auto child: node->children) {
		if (auto cs = dynamic_cast<AstClassNode *>(child); cs)
			enterClassNode(cs);
		else
			visit(child);
	}
	auto main_func = scope->query_variable("main");
	auto f = dynamic_cast<FuncType *>(main_func.basicType);
	if (!f || !f->args.empty())
		throw semantic_error("main function not found");
	if (f->returnType != globalScope->query_class("int"))
		throw semantic_error("main function return type should be int");
}

void SemanticChecker::enterClassNode(AstClassNode *node) {
	// do not create a new scope, it has already created in FunctionCollector
	auto scope_back = scope;
	scope = node->scope;
	for (auto &ctor: node->constructors) {
		ctor->scope = scope;
		enterConstructFuncNode(ctor, node);
	}
	for (auto &func: node->functions)
		visit(func);
	scope = scope_back;
}

void SemanticChecker::visitFunctionNode(AstFunctionNode *node) {
	scope = node->scope = scope->add_sub_scope();
	currentFunction = node;
	node->valueType = enterTypeNode(node->returnType);

	for (auto &param: node->params) {
		auto type = enterTypeNode(param.first);
		scope->add_variable(param.second, type);
	}

	visitBlockStmtNodeWithoutNewScope(dynamic_cast<AstBlockStmtNode *>(node->body));
	currentFunction = nullptr;
}

void SemanticChecker::enterConstructFuncNode(AstConstructFuncNode *node, AstClassNode *classNode) {
	if (node->name != classNode->name)
		throw semantic_error("construct function should have the same name as class");
	scope = node->scope = classNode->scope->add_sub_scope();
	for (auto &param: node->params) {
		auto type = enterTypeNode(param.first);
		scope->add_variable(param.second, type);
	}
	visitBlockStmtNodeWithoutNewScope(dynamic_cast<AstBlockStmtNode *>(node->body));
}

TypeInfo SemanticChecker::enterTypeNode(AstTypeNode *node) {
	for (auto expr: node->arraySize) {
		visit(expr);
		if (!expr->valueType.is_int())
			throw semantic_error("array size should be int");
	}
	return TypeInfo{globalScope->query_class(node->name), node->dimension, false};
}

void SemanticChecker::visitBlockStmtNodeWithoutNewScope(AstBlockStmtNode *node) {
	for (auto stmt: node->stmts)
		visit(stmt);
}

void SemanticChecker::visitBlockStmtNode(AstBlockStmtNode *node) {
	scope = node->scope = scope->add_sub_scope();
	for (auto stmt: node->stmts)
		visit(stmt);
}

void SemanticChecker::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = enterTypeNode(node->type);
	for (const auto &var: node->vars) {
		// check init_value before add variable
		if (var.second) {
			visit(var.second);
			if (!type.assignable(var.second->valueType))
				throw semantic_error("can't assign <" + var.second->valueType.to_string() + "> to <" + type.to_string() + ">");
		}
		scope->add_variable(var.first, type);
	}
}

void SemanticChecker::visitIfStmtNode(AstIfStmtNode *node) {
	for (auto &s: node->ifStmts) {
		visit(s.first);
		if (!s.first->valueType.is_bool())
			throw semantic_error("if condition should be bool, but get <" + s.first->valueType.to_string() + ">");
		// block stmt will create a new scope
		visit(s.second);
	}
	if (node->elseStmt) visit(node->elseStmt);
}

void SemanticChecker::visitForStmtNode(AstForStmtNode *node) {
	scope = node->scope = scope->add_sub_scope();
	if (node->init) visit(node->init);
	if (node->cond) {
		visit(node->cond);
		if (!node->cond->valueType.is_bool())
			throw semantic_error("for condition should be bool, but get <" + node->cond->valueType.to_string() + ">");
	}
	if (node->step) visit(node->step);
	++loopDepth;
	for (auto stmt: node->body)
		visit(stmt);
	--loopDepth;
}

void SemanticChecker::visitWhileStmtNode(AstWhileStmtNode *node) {
	scope = node->scope = scope->add_sub_scope();
	visit(node->cond);
	if (!node->cond->valueType.is_bool())
		throw semantic_error("while condition should be bool, but get <" + node->cond->valueType.to_string() + ">");
	++loopDepth;
	for (auto stmt: node->body)
		visit(stmt);
	--loopDepth;
}

void SemanticChecker::visitBreakStmtNode(AstBreakStmtNode *node) {
	if (loopDepth == 0)
		throw semantic_error("break statement not in loop");
}

void SemanticChecker::visitContinueStmtNode(AstContinueStmtNode *node) {
	if (loopDepth == 0)
		throw semantic_error("continue statement not in loop");
}

void SemanticChecker::visitReturnStmtNode(AstReturnStmtNode *node) {
	if (node->expr) {
		visit(node->expr);
		if (!currentFunction)
			throw semantic_error("construct function should not have return value");
		if (!currentFunction->valueType.assignable(node->expr->valueType))
			throw semantic_error("return type mismatch, need " + currentFunction->valueType.to_string() + ", but give " + node->expr->valueType.to_string());
	}
	else {
		if (currentFunction && currentFunction->valueType != globalScope->query_class("void"))
			throw semantic_error("return type mismatch, need " + currentFunction->valueType.to_string() + ", but give void");
	}
}

void SemanticChecker::visitExprStmtNode(AstExprStmtNode *node) {
	for (auto expr: node->expr)
		visit(expr);
}

void SemanticChecker::visitAtomExprNode(AstAtomExprNode *node) {
	node->valueType = scope->query_variable(node->name);
}

void SemanticChecker::visitLiterExprNode(AstLiterExprNode *node) {
	if (node->value == "null")
		node->valueType = {nullptr, 0, true};
	else if (node->value == "true" || node->value == "false")
		node->valueType = {globalScope->query_class("bool"), 0, true};
	else if (node->value.front() == '"')
		node->valueType = {globalScope->query_class("string"), 0, true};
	else if (isdigit(node->value.front()))
		node->valueType = {globalScope->query_class("int"), 0, true};
	else
		throw semantic_error("unknown literal: " + node->value);
}

void SemanticChecker::visitMemberAccessExprNode(AstMemberAccessExprNode *node) {
	visit(node->object);
	node->valueType = node->object->valueType.get_member(node->member, globalScope);
}

void SemanticChecker::visitFuncCallExprNode(AstFuncCallExprNode *node) {
	visit(node->func);
	auto f = dynamic_cast<FuncType *>(node->func->valueType.basicType);
	if (!f)
		throw semantic_error("can't call non-function type");

	if (f->args.size() != node->args.size())
		throw semantic_error("function argument number mismatch");

	for (size_t i = 0; i < f->args.size(); ++i) {
		visit(node->args[i]);
		if (!f->args[i].assignable(node->args[i]->valueType))
			throw semantic_error("function argument type mismatch");
	}
	node->valueType = f->returnType;
	node->valueType.isConst = true;
}

void SemanticChecker::visitArrayAccessExprNode(AstArrayAccessExprNode *node) {
	visit(node->array);
	if (node->array->valueType.dimension == 0)
		throw semantic_error("array access on non-array type");

	visit(node->index);
	if (!node->index->valueType.is_int())
		throw semantic_error("array index should be int");

	node->valueType = node->array->valueType;
	node->valueType.dimension--;
}

void SemanticChecker::visitNewExprNode(AstNewExprNode *node) {
	node->valueType = enterTypeNode(node->type);
	if (node->valueType.is_void())
		throw semantic_error("can't new void type");
}

void SemanticChecker::visitSingleExprNode(AstSingleExprNode *node) {
	visit(node->expr);
	auto &type = node->expr->valueType;
	auto &op = node->op;
	if (op == "++" || op == "--") {
		if (!type.is_int())
			throw semantic_error("++ or -- on non-int type");
		if (type.isConst)
			throw semantic_error("++ or -- on const type");
		node->valueType = type;
		if (node->right) node->valueType.isConst = true;
	}
	else if (op == "+" || op == "-") {
		if (!type.is_int())
			throw semantic_error("+ or - on non-int type");
		node->valueType = type;
		node->valueType.isConst = true;
	}
	else if (op == "!") {
		if (!type.is_bool())
			throw semantic_error("! on non-bool type");
		node->valueType = type;
		node->valueType.isConst = true;
	}
	else if (op == "~") {
		if (!type.is_int())
			throw semantic_error("~ on non-int type");
		node->valueType = type;
		node->valueType.isConst = true;
	}
	else
		throw semantic_error("unknown unary operator: " + op);
}

void SemanticChecker::visitBinaryExprNode(AstBinaryExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	auto vl = node->lhs->valueType, vr = node->rhs->valueType;
	auto const &op = node->op;

	if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") {
		if (op == "==" || op == "!=") {
			if (!vl.is_basic() || !vr.is_basic()) {
				if (!vl.is_null() && !vr.is_null())
					throw semantic_error("operator== on non-basic type l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
			}
			else if (vl != vr)
				throw semantic_error("operator== on different basic type l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
		}
		else {
			if (vl != vr || !vl.is_basic() || !vr.is_basic())
				throw semantic_error("compare l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
			if (vl.is_bool() || vl.is_void())
				throw semantic_error("can't compare on l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
		}
		node->valueType = {globalScope->query_class("bool"), 0, true};
	}
	else {
		if (!vl.is_basic() || !vr.is_basic())
			throw semantic_error("binary operation on non-basic type, l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);

		if (vl.is_string() || vr.is_string()) {
			if (!vl.is_string() || !vr.is_string())
				throw semantic_error("string operation on non-string type, l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
			if (node->op != "+" && node->op != "==" && node->op != "!=" && node->op != "<" && node->op != ">" && node->op != "<=" && node->op != ">=")
				throw semantic_error("string operation not support " + node->op);
		}
		node->valueType = vl;
		node->valueType.isConst = true;
	}
}

void SemanticChecker::visitTernaryExprNode(AstTernaryExprNode *node) {
	visit(node->cond);
	if (!node->cond->valueType.is_bool())
		throw semantic_error("ternary condition should be bool, but get <" + node->cond->valueType.to_string() + ">");
	visit(node->trueExpr);
	visit(node->falseExpr);
	if (node->trueExpr->valueType != node->falseExpr->valueType)
		throw semantic_error("ternary expression type mismatch");
	node->valueType = node->trueExpr->valueType;
	node->valueType.isConst = true;
}

void SemanticChecker::visitAssignExprNode(AstAssignExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	if (!node->lhs->valueType.assignable(node->rhs->valueType))
		throw semantic_error("can't assign <" + node->rhs->valueType.to_string() + "> to <" + node->lhs->valueType.to_string() + ">");
	node->valueType = node->lhs->valueType;
}
