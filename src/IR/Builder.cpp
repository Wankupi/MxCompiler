#include "Builder.h"
#include "AST/AstNode.h"
#include "IR/Type.h"

#include <iostream>

using namespace IR;

static VoidType voidType;
static IntegerType boolType(1);
static IntegerType charType(8);
static IntegerType intType(32);
static PtrType ptr;

void IRBuilder::visitFileNode(AstFileNode *node) {
	if (module)
		throw std::runtime_error("IRBuilder: module already exists");
	module = new Module{};

	for (auto c: node->children)
		visit(c);
}

void IRBuilder::visitClassNode(AstClassNode *node) {
	auto ir = new Class{};
	ir->type.name = node->name;

	for (auto vars: node->variables) {
		auto type = toIRType(vars->type);
		for (auto &var: vars->vars)
			ir->add_filed(type, var.first);
	}
	module->classes.push_back(ir);
}

void IRBuilder::visitFunctionNode(AstFunctionNode *node) {
	auto ir = new Function{};
	ir->name = node->name;
	ir->type = toIRType(node->returnType);
	for (auto &p: node->params)
		ir->params.emplace_back(toIRType(p.first), p.second);
	ir->blocks.push_back(new Block{"entry"});
	currentFunction = ir;
	annoyCounter = 0;
	visit(node->body);
	currentFunction = nullptr;

	auto ret = new RetStmt{};
	ret->value = new LiteralInt(0);
	for (auto block: ir->blocks) {
		block->stmts.push_back(ret);
	}
	module->functions.push_back(ir);
}

IR::PrimitiveType *IRBuilder::toIRType(AstTypeNode *node) {

	if (node->dimension) return &ptr;
	if (node->name == "void")
		return &voidType;
	else if (node->name == "bool")
		return &boolType;
	else if (node->name == "int")
		return &intType;
	else if (node->name == "char")
		return &charType;
	else
		return &ptr;
}


void IRBuilder::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	if (!currentFunction) {// global
		for (auto &var: node->vars_unique_name) {
			if (var.second) {
				//				visit(var.second);
				std::cerr << "TODO: global init" << std::endl;
			}
			auto ir = new GlobalVar{};
			ir->name = var.first;
			ir->type = type;
			module->globalVars.push_back(ir);
			if (var.second) {
				std::cerr << "TODO: global init" << std::endl;
			}
		}
	}
	else {// local
		for (auto &var: node->vars_unique_name) {
			if (var.second)
				visit(var.second);
			auto def_var = new LocalVar{};
			def_var->name = var.first;
			def_var->type = type;
			add_local_var(def_var);
			auto alloc = new AllocaStmt{};
			alloc->res = def_var;
			add_stmt(alloc);
			if (var.second) {
				auto st = new StoreStmt{};
				st->pointer = def_var;
				st->value = exprResult[var.second];
				add_stmt(st);
			}
		}
	}
}

void IRBuilder::visitBlockStmtNode(AstBlockStmtNode *node) {
	for (auto &stmt: node->stmts)
		visit(stmt);
}

void IRBuilder::add_stmt(Stmt *node) {
	currentFunction->blocks.back()->stmts.push_back(node);
}

void IRBuilder::add_local_var(IR::LocalVar *node) {
	currentFunction->localVars.push_back(node);
	name2var[node->name] = node;
}

void IRBuilder::visitExprStmtNode(AstExprStmtNode *node) {
	for (auto e: node->expr)
		visit(e);
}

IR::LocalVar *IRBuilder::register_annoy_var(IR::Type *type) {
	auto var = new LocalVar{};
	var->type = type;
	var->name = std::to_string(annoyCounter++);
	add_local_var(var);
	return var;
}

void IRBuilder::visitAtomExprNode(AstAtomExprNode *node) {
	exprResult[node] = name2var[node->uniqueName.empty() ? node->name : node->uniqueName];
}

void IRBuilder::visitAssignExprNode(AstAssignExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	auto st = new StoreStmt{};
	st->pointer = dynamic_cast<Var *>(exprResult[node->lhs]);
	st->value = exprResult[node->rhs];
	add_stmt(st);
}

void IRBuilder::visitLiterExprNode(AstLiterExprNode *node) {
	if (node->valueType.is_null())
		exprResult[node] = new LiteralNull{};
	else if (node->valueType.is_bool())
		exprResult[node] = new LiteralBool{node->value == "true"};
	else if (node->valueType.is_int())
		exprResult[node] = new LiteralInt{std::stoi(node->value)};
	else if (node->valueType.is_string()) {
		// TODO
		exprResult[node] = new LiteralString{};
	}
	else
		throw std::runtime_error("IRBuilder: unknown literal type");
}

void IRBuilder::visitBinaryExprNode(AstBinaryExprNode *node) {
	auto op2cmd = [](std::string const &str) {
		if (str == "+") return "add";
		if (str == "-") return "sub";
		if (str == "*") return "mul";
		if (str == "/") return "sdiv";
		if (str == "%") return "srem";
		if (str == "<<") return "shl";
		if (str == ">>") return "ashr";
		if (str == "&") return "and";
		if (str == "|") return "or";
		if (str == "^") return "xor";
		return "What?";
	};
	visit(node->lhs);
	visit(node->rhs);
	if (node->op == "+" || node->op == "-" || node->op == "*" || node->op == "/" || node->op == "%" || node->op == "<<" || node->op == ">>" || node->op == "&" || node->op == "|" || node->op == "^") {
		auto arh = new ArithmeticStmt{};
		arh->lhs = remove_variable_pointer(exprResult[node->lhs]);
		arh->rhs = remove_variable_pointer(exprResult[node->rhs]);
		arh->cmd = op2cmd(node->op);
		exprResult[node] = arh->res = register_annoy_var(&intType);
		add_stmt(arh);
	}
	else {
		std::cerr << __FUNCTION__ << " : TODO" << std::endl;
	}
}


IR::Val *IRBuilder::remove_variable_pointer(IR::Val *val) {
	if (dynamic_cast<Literal *>(val)) return val;
	auto var = dynamic_cast<Var *>(val);
	if (isdigit(var->name[0])) return val;
	auto load = new LoadStmt{};
	load->res = register_annoy_var(var->type);
	load->pointer = var;
	add_stmt(load);
	return load->res;
}


void Class::add_filed(PrimitiveType *mem_type, const std::string &mem_name) {
	type.fields.emplace_back(mem_type);
	name2index[mem_name] = type.fields.size() - 1;
}