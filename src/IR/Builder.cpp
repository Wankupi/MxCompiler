#include "Builder.h"
#include "AST/AstNode.h"
#include "IR/Type.h"

#include <iostream>

using namespace IR;

static VoidType voidType;
static IntegerType boolType(1);
//static IntegerType charType(8);
static IntegerType intType(32);
static PtrType ptrType;
static LiteralInt zero(0);
static LiteralBool literal_true(true), literal_false(false);

void IRBuilder::visitFileNode(AstFileNode *node) {
	if (module)
		throw std::runtime_error("IRBuilder: module already exists");
	module = new Module{};
	init_builtin_function();
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
	name2class[node->name] = ir;

	currentClass = ir;
	for (auto constructor: node->constructors)
		visit(constructor);
	for (auto func: node->functions)
		visit(func);
	currentClass = nullptr;
}

void IRBuilder::visitFunctionNode(AstFunctionNode *node) {
	auto func = new Function{};
	if (currentClass) {
		func->name = currentClass->type.name + "." + node->name;
		func->params.emplace_back(&ptrType, "this");
	}
	else
		func->name = node->name;
	func->type = toIRType(node->returnType);

	for (auto &p: node->params)
		func->params.emplace_back(toIRType(p.first), p.second);
	func->blocks.push_back(new Block{"entry"});
	currentFunction = func;
	annoyCounter = 0;
	visit(node->body);
	currentFunction = nullptr;
	module->functions.push_back(func);
}

void IRBuilder::visitConstructFuncNode(AstConstructFuncNode *node) {
	auto func = new Function{};
	func->name = currentClass->type.name + "." + node->name;
	func->type = &voidType;
	func->params.emplace_back(&ptrType, "this");
	for (auto &p: node->params)
		func->params.emplace_back(toIRType(p.first), p.second);
	func->blocks.push_back(new Block{"entry"});
	currentFunction = func;
	annoyCounter = 0;
	visit(node->body);
	currentFunction = nullptr;
	module->functions.push_back(func);
}


IR::PrimitiveType *IRBuilder::toIRType(AstTypeNode *node) {
	if (node->dimension) return &ptrType;
	if (node->name == "void")
		return &voidType;
	else if (node->name == "bool")
		return &boolType;
	else if (node->name == "int")
		return &intType;
	//	else if (node->name == "string")
	//		return &charType;
	else
		return &ptrType;
}
IR::PrimitiveType *IRBuilder::toIRType(TypeInfo typeInfo) {
	if (typeInfo.dimension) return &ptrType;
	if (typeInfo.is_void()) return &voidType;
	else if (typeInfo.is_bool())
		return &boolType;
	else if (typeInfo.is_int())
		return &intType;
	//	else if (typeInfo.is_char())
	//		return &charType;
	else
		return &ptrType;
}

void IRBuilder::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	//	auto type = &ptrType;
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
			auto def_var = new PtrVar{};
			def_var->name = var.first;
			def_var->type = &ptrType;
			def_var->objType = type;
			add_local_var(def_var);

			auto alloc = new AllocaStmt{};
			alloc->res = def_var;
			alloc->type = type;
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
	if (node->op == "&&" || node->op == "||") {
		enterAndOrBinaryExprNode(node);
		return;
	}
	std::map<std::string, std::string> arth = {
			{"+", "add"},
			{"-", "sub"},
			{"*", "mul"},
			{"/", "sdiv"},
			{"%", "srem"},
			{"<<", "shl"},
			{">>", "ashr"},
			{"&", "and"},
			{"|", "or"},
			{"^", "xor"},
	};
	std::map<std::string, std::string> cmp = {
			{"==", "eq"},
			{"!=", "ne"},
			{"<", "slt"},
			{">", "sgt"},
			{"<=", "sle"},
			{">=", "sge"},
	};
	visit(node->lhs);
	visit(node->rhs);
	if (arth.contains(node->op)) {
		auto arh = new ArithmeticStmt{};
		arh->lhs = remove_variable_pointer(exprResult[node->lhs]);
		arh->rhs = remove_variable_pointer(exprResult[node->rhs]);
		arh->cmd = arth[node->op];
		exprResult[node] = arh->res = register_annoy_var(&intType);
		add_stmt(arh);
	}
	else if (cmp.contains(node->op)) {
		auto icmp = new IcmpStmt{};
		icmp->lhs = remove_variable_pointer(exprResult[node->lhs]);
		icmp->rhs = remove_variable_pointer(exprResult[node->rhs]);
		icmp->cmd = cmp[node->op];
		exprResult[node] = icmp->res = register_annoy_var(&boolType);
		add_stmt(icmp);
	}
	else
		throw std::runtime_error("IRBuilder: unknown binary operator: " + node->op);
}


IR::Val *IRBuilder::remove_variable_pointer(IR::Val *val) {
	//	if (dynamic_cast<Literal *>(val)) return val;
	auto var = dynamic_cast<PtrVar *>(val);
	if (!var) return val;
	//	if (isdigit(var->name[0]))
	//		return val;
	auto load = new LoadStmt{};
	load->res = register_annoy_var(var->objType);
	load->pointer = var;
	add_stmt(load);
	return load->res;
}

void IRBuilder::visitMemberAccessExprNode(AstMemberAccessExprNode *node) {
	visit(node->object);
	if (node->valueType.is_function()) {
		std::cout << "<TODO: call Class.function>";
	}
	else {
		auto obj = remove_variable_pointer(exprResult[node->object]);
		auto getEP = new GetElementPtrStmt{};
		getEP->pointer = dynamic_cast<Var *>(obj);
		if (!getEP->pointer)
			throw std::runtime_error("IRBuilder: member access on non-variable");

		auto cls = name2class[node->object->valueType.to_string()];
		int index = static_cast<int>(cls->name2index[node->member]);

		getEP->res = register_annoy_ptr_var(cls->type.fields[index]);
		getEP->typeName = "%class." + cls->type.name;

		getEP->indices.push_back(&zero);
		getEP->indices.push_back(new LiteralInt{index});

		add_stmt(getEP);
		exprResult[node] = getEP->res;
	}
}

void Class::add_filed(PrimitiveType *mem_type, const std::string &mem_name) {
	type.fields.emplace_back(mem_type);
	name2index[mem_name] = type.fields.size() - 1;
}

IR::LocalVar *IRBuilder::register_annoy_var(IR::Type *type) {
	auto var = new LocalVar{};
	var->type = type;
	var->name = std::to_string(annoyCounter++);
	add_local_var(var);
	return var;
}

IR::PtrVar *IRBuilder::register_annoy_ptr_var(IR::Type *obj_type) {
	auto var = new PtrVar{};
	var->type = &ptrType;
	var->objType = obj_type;
	var->name = std::to_string(annoyCounter++);
	add_local_var(var);
	return var;
}


void IRBuilder::init_builtin_function() {
	auto print = new Function{};
	print->name = "malloc";
	print->type = &ptrType;
	print->params.emplace_back(&intType, "size");
	module->functions.push_back(print);

	for (auto func: module->functions)
		name2function[func->name] = func;
}

void IRBuilder::visitNewExprNode(AstNewExprNode *node) {
	if (node->type->dimension > 0) {
		std::cerr << "<TODO: new array type>";
	}
	auto type = toIRType(node->type);
	auto call = new CallStmt{};
	call->func = name2function["malloc"];
	call->args.push_back(new LiteralInt{type->size()});
	call->res = register_annoy_var(&ptrType);
	add_stmt(call);
	exprResult[node] = call->res;
}

void IRBuilder::visitReturnStmtNode(AstReturnStmtNode *node) {
	auto ret = new RetStmt{};
	if (node->expr) {
		visit(node->expr);
		ret->value = remove_variable_pointer(exprResult[node->expr]);
	}
	add_stmt(ret);
}

void IRBuilder::visitWhileStmtNode(AstWhileStmtNode *node) {
	++loopCounter;
	auto cond = new Block{"while_cond_" + std::to_string(loopCounter)};
	auto body = new Block{"while_body_" + std::to_string(loopCounter)};
	auto afterLoop = new Block{"while_after_" + std::to_string(loopCounter)};
	auto br = new DirectBrStmt{};
	br->block = cond;
	add_stmt(br);


	// visit cond
	add_block(cond);
	visit(node->cond);
	auto brCond = new CondBrStmt{};
	brCond->cond = remove_variable_pointer(exprResult[node->cond]);
	brCond->trueBlock = body;
	brCond->falseBlock = afterLoop;
	add_stmt(brCond);

	// visit body
	add_block(body);
	push_loop(cond, afterLoop);
	for (auto stmt: node->body)
		visit(stmt);
	add_stmt(br);
	pop_loop();

	// after loop
	add_block(afterLoop);
}

void IRBuilder::add_block(IR::Block *block) {
	currentFunction->blocks.push_back(block);
}

void IRBuilder::visitForStmtNode(AstForStmtNode *node) {
	// visit init
	visit(node->init);

	// create blocks
	++loopCounter;
	auto cond = new Block{"for_cond_" + std::to_string(loopCounter)};
	auto body = new Block{"for_body_" + std::to_string(loopCounter)};
	auto step = new Block{"for_step_" + std::to_string(loopCounter)};
	auto afterLoop = new Block{"for_after_" + std::to_string(loopCounter)};

	auto br2cond = new DirectBrStmt{};
	br2cond->block = cond;
	add_stmt(br2cond);

	// visit cond
	add_block(cond);
	visit(node->cond);
	auto br2body = new CondBrStmt{};
	br2body->cond = remove_variable_pointer(exprResult[node->cond]);
	br2body->trueBlock = body;
	br2body->falseBlock = afterLoop;
	add_stmt(br2body);

	// visit body
	add_block(body);
	push_loop(step, afterLoop);
	for (auto stmt: node->body)
		visit(stmt);
	auto br2step = new DirectBrStmt{};
	br2step->block = step;
	add_stmt(br2step);
	pop_loop();

	// visit step
	add_block(step);
	visit(node->step);
	add_stmt(br2cond);

	// after loop
	add_block(afterLoop);
}

void IRBuilder::push_loop(IR::Block *step, IR::Block *after) {
	loopBreakTo.push(after);
	loopContinueTo.push(step);
}

void IRBuilder::pop_loop() {
	loopBreakTo.pop();
	loopContinueTo.pop();
}

void IRBuilder::visitBreakStmtNode(AstBreakStmtNode *node) {
	auto br = new DirectBrStmt{};
	br->block = loopBreakTo.top();
	add_stmt(br);
	auto block = new Block{"after_break_" + std::to_string(++breakCounter)};
	add_block(block);
}

void IRBuilder::visitContinueStmtNode(AstContinueStmtNode *node) {
	auto br = new DirectBrStmt{};
	br->block = loopContinueTo.top();
	add_stmt(br);
	auto block = new Block{"after_continue_" + std::to_string(++continueCounter)};
	add_block(block);
}

void IRBuilder::visitIfStmtNode(AstIfStmtNode *node) {
	auto after = new Block{"if_after_" + std::to_string(ifCounter)};
	auto br_after = new DirectBrStmt{};
	br_after->block = after;

	for (auto &clause: node->ifStmts) {
		++ifCounter;
		auto true_block = new Block{"if_true_" + std::to_string(ifCounter)};
		auto false_block = (&clause != &node->ifStmts.back() || node->elseStmt) ? new Block{"if_false_" + std::to_string(ifCounter)} : nullptr;

		visit(clause.first);
		auto br_cond = new CondBrStmt{};
		br_cond->cond = remove_variable_pointer(exprResult[clause.first]);
		br_cond->trueBlock = true_block;
		br_cond->falseBlock = false_block ? false_block : after;
		add_stmt(br_cond);

		add_block(true_block);
		visit(clause.second);
		add_stmt(br_after);

		if (false_block)
			add_block(false_block);
	}
	if (node->elseStmt) {
		visit(node->elseStmt);
		add_stmt(br_after);
	}
	add_block(after);
}


void IRBuilder::enterAndOrBinaryExprNode(AstBinaryExprNode *node) {
	++andOrCounter;
	auto calc_right = new Block{"short_rhs_" + std::to_string(andOrCounter)};
	auto result = new Block{"short_result_" + std::to_string(andOrCounter)};

	visit(node->lhs);
	auto calc_left = currentFunction->blocks.back();
	auto br = new CondBrStmt{};
	br->cond = remove_variable_pointer(exprResult[node->lhs]);
	if (node->op == "&&") {
		br->trueBlock = calc_right;
		br->falseBlock = result;
	}
	else {// op == "||"
		br->trueBlock = result;
		br->falseBlock = calc_right;
	}
	add_stmt(br);

	add_block(calc_right);
	visit(node->rhs);
	// load must done in this block
	auto rhs_res = remove_variable_pointer(exprResult[node->rhs]);

	auto br2result = new DirectBrStmt{};
	br2result->block = result;
	add_stmt(br2result);

	add_block(result);
	auto phi = new PhiStmt{};
	if (node->op == "&&")
		phi->branches = {{&literal_false, calc_left}, {rhs_res, calc_right}};
	else
		phi->branches = {{&literal_true, calc_left}, {rhs_res, calc_right}};
	exprResult[node] = phi->res = register_annoy_var(&boolType);
	add_stmt(phi);
}
