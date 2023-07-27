#include "SemanticChecker.h"
#include "AST/AstNode.h"
#include <cctype>

void SemanticChecker::visit(AstNode *node) {
	auto current_scopt = node->scope = scope;
	node->accept(this);
	scope = current_scopt;
}

void SemanticChecker::visitFileNode(AstFileNode *node) {
	for (auto child: node->children)
		visit(child);
	auto main_func = scope->query_variable("main");
	auto f = dynamic_cast<FuncType *>(main_func.basicType);
	if (!f || !f->args.empty())
		throw semantic_error("main function not found");
	if (f->returnType != globalScope->query_class("int"))
		throw semantic_error("main function return type should be int");
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
		scope->add_variable(var.first, type);
		if (var.second) visit(var.second);
	}
}

TypeInfo SemanticChecker::enterTypeNode(AstTypeNode *node) {
	return TypeInfo{globalScope->query_class(node->name), node->dimension, false};
}

void SemanticChecker::visitAssignExprNode(AstAssignExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	if (!node->lhs->valueType.assignable(node->rhs->valueType))
		throw semantic_error("can't assign <" + node->rhs->valueType.to_string() + "> to <" + node->lhs->valueType.to_string() + ">");
	node->valueType = node->lhs->valueType;
}

void SemanticChecker::visitAtomExprNode(AstAtomExprNode *node) {
	node->valueType = scope->query_variable(node->name);
}

void SemanticChecker::visitExprStmtNode(AstExprStmtNode *node) {
	for (auto expr: node->expr)
		visit(expr);
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
}

void SemanticChecker::visitReturnStmtNode(AstReturnStmtNode *node) {
	if (node->expr) {
		visit(node->expr);
		if (!currentFunction->valueType.assignable(node->expr->valueType))
			throw semantic_error("return type mismatch, need " + currentFunction->valueType.to_string() + ", but give " + node->expr->valueType.to_string());
	}
	else {
		if (currentFunction->valueType != globalScope->query_class("void"))
			throw semantic_error("return type mismatch, need " + currentFunction->valueType.to_string() + ", but give void");
	}
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

void SemanticChecker::visitBinaryExprNode(AstBinaryExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	auto vl = node->lhs->valueType, vr = node->rhs->valueType;
	if (vl.is_string() || vr.is_string()) {
		if (!vl.is_string() || !vr.is_string())
			throw semantic_error("string operation on non-string type, l = <" + vl.to_string() + ">, r = <" + vr.to_string() + ">, op = " + node->op);
		if (node->op != "+" && node->op != "==" && node->op != "!=" && node->op != "<" && node->op != ">" && node->op != "<=" && node->op != ">=")
			throw semantic_error("string operation not support " + node->op);
	}
	node->valueType = vl;
	node->valueType.isConst = true;
}
