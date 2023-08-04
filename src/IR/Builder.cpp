#include "Builder.h"
#include "AST/AstNode.h"
#include "IR/Type.h"

#include <iostream>

using namespace IR;

static VoidType voidType;
static IntegerType boolType(1);
static IntegerType intType(32);
static PtrType ptrType;
static LiteralInt literal_zero(0), literal_one(1), literal_minus_one(-1);
static LiteralBool literal_true(true), literal_false(false);
static LiteralNull literal_null;

void IRBuilder::visitFileNode(AstFileNode *node) {
	if (module)
		throw std::runtime_error("IRBuilder: module already exists");
	module = new Module{};
	init_builtin_function();
	for (auto c: node->children) {
		if (auto func = dynamic_cast<AstFunctionNode *>(c))
			registerFunction(func);
		else if (auto cls = dynamic_cast<AstClassNode *>(c))
			registerClass(cls);
	}
	for (auto c: node->children)
		visit(c);
}

void IRBuilder::registerClass(AstClassNode *node) {
	auto cls = new Class{};
	cls->type.name = node->name;
	for (auto vars: node->variables) {
		auto type = toIRType(vars->type);
		for (auto &var: vars->vars)
			cls->add_filed(type, var.first);
	}
	module->classes.push_back(cls);
	name2class[node->name] = cls;
	currentClass = cls;
	for (auto constructor: node->constructors)
		registerConstructFunc(constructor);
	for (auto func: node->functions)
		registerFunction(func);
	currentClass = nullptr;
}

void IRBuilder::registerFunction(AstFunctionNode *node) {
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
	module->functions.push_back(func);
	name2function[func->name] = func;
}

void IRBuilder::registerConstructFunc(AstConstructFuncNode *node) {
	auto func = new Function{};
	func->name = currentClass->type.name + "." + node->name;
	func->type = &voidType;
	func->params.emplace_back(&ptrType, "this");
	for (auto &p: node->params)
		func->params.emplace_back(toIRType(p.first), p.second);
}

void IRBuilder::visitClassNode(AstClassNode *node) {
	auto cls = name2class[node->name];
	currentClass = cls;
	for (auto constructor: node->constructors)
		visit(constructor);
	for (auto func: node->functions)
		visit(func);
	currentClass = nullptr;
}

void add_terminals(Function *func) {
	static UnreachableStmt unreachable{};
	static RetStmt ret_void{};
	static RetStmt ret_one{&literal_one};
	Stmt *to_insert = nullptr;
	if (func->type != &voidType)
		to_insert = &unreachable;
	else
		to_insert = &ret_void;
	if (func->name == "main")
		to_insert = &ret_one;
	for (auto block: func->blocks) {
		auto bak = block->stmts.empty() ? nullptr : block->stmts.back();
		if (!(bak && (dynamic_cast<BrStmt *>(bak) || dynamic_cast<RetStmt *>(bak))))
			block->stmts.push_back(to_insert);
	}
}

void IRBuilder::visitFunctionNode(AstFunctionNode *node) {
	std::string name = (currentClass ? currentClass->type.name + "." : "") + node->name;
	auto func = name2function[name];
	for (auto &p: func->params) {
		auto var = new LocalVar{};
		var->type = p.first;
		var->name = p.second;
		name2var[p.second] = var;
	}
	func->blocks.push_back(new Block{"entry"});
	currentFunction = func;
	annoyCounter = 0;
	visit(node->body);
	currentFunction = nullptr;
	add_terminals(func);
}

void IRBuilder::visitConstructFuncNode(AstConstructFuncNode *node) {
	auto func = name2function[currentClass->type.name + "." + node->name];
	for (auto &p: func->params) {
		auto var = new LocalVar{};
		var->type = p.first;
		var->name = p.second;
		name2var[p.second] = var;
	}
	func->blocks.push_back(new Block{"entry"});
	currentFunction = func;
	annoyCounter = 0;
	visit(node->body);
	currentFunction = nullptr;
	add_terminals(func);
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
	else
		return &ptrType;// string and class is ptr
}

IR::PrimitiveType *IRBuilder::toIRType(TypeInfo typeInfo) {
	if (typeInfo.dimension) return &ptrType;
	if (typeInfo.is_void()) return &voidType;
	else if (typeInfo.is_bool())
		return &boolType;
	else if (typeInfo.is_int())
		return &intType;
	else
		return &ptrType;
}

Val *type_to_dafault_value(IR::Type *type) {
	if (type == &boolType)
		return &literal_false;
	else if (type == &intType)
		return &literal_zero;
	else
		return &literal_null;
}

void IRBuilder::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	//	auto type = &ptrType;
	if (!currentFunction) {// global
		for (auto &var: node->vars_unique_name) {
			auto gv = new GlobalVar{};
			gv->name = var.first;
			gv->type = type;
			add_global_var(gv);

			auto globalStmt = new GlobalStmt{};
			globalStmt->var = gv;
			globalStmt->value = type_to_dafault_value(type);
			module->variables.push_back(globalStmt);

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
				st->value = remove_variable_pointer(exprResult[var.second]);
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

void IRBuilder::add_global_var(IR::GlobalVar *node) {
	module->globalVars.push_back(node);
	name2var[node->name] = node;
}


void IRBuilder::visitExprStmtNode(AstExprStmtNode *node) {
	for (auto e: node->expr)
		visit(e);
}


void IRBuilder::visitAtomExprNode(AstAtomExprNode *node) {
	auto name_to_find = node->uniqueName.empty() ? node->name : node->uniqueName;
	if (auto p = name2var.find(name_to_find); p != name2var.end())
		exprResult[node] = p->second;
	else if (currentClass) {// accessing class member
		auto idx = static_cast<int>(currentClass->name2index[name_to_find]);
		auto gep = new GetElementPtrStmt{};
		gep->pointer = name2var["this"];
		if (!gep->pointer)
			throw std::runtime_error("this is not defined");
		gep->indices.push_back(&literal_zero);
		gep->indices.push_back(new LiteralInt(idx));
		gep->typeName = "%class." + currentClass->type.name;
		gep->res = register_annoy_ptr_var(currentClass->type.fields[idx]);
		add_stmt(gep);
		exprResult[node] = gep->res;
	}
	else
		throw std::runtime_error("unknown variable: " + node->name);
}

void IRBuilder::visitAssignExprNode(AstAssignExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	auto st = new StoreStmt{};
	st->pointer = dynamic_cast<Var *>(exprResult[node->lhs]);
	st->value = remove_variable_pointer(exprResult[node->rhs]);
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
		std::string str;
		str.reserve(node->value.size() - 2);
		for (size_t i = 1, siz = node->value.size(); i + 1 < siz; ++i) {
			if (node->value[i] != '\\')
				str += node->value[i];
			else {
				++i;
				if (node->value[i] == 'n') str += '\n';
				else if (node->value[i] == '"')
					str += '"';
				else if (node->value[i] == '\\')
					str += '\\';
				else
					throw std::runtime_error("unknown escape sequence");
			}
		}
		exprResult[node] = register_literal_str(node->value);
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
	auto loc = dynamic_cast<PtrVar *>(val);
	auto glo = dynamic_cast<GlobalVar *>(val);
	if (!(loc || glo)) return val;
	auto load = new LoadStmt{};
	load->res = register_annoy_var(loc ? loc->objType : glo->type);
	load->pointer = loc ? static_cast<Var *>(loc) : static_cast<Var *>(glo);
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

		getEP->indices.push_back(&literal_zero);
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
	// ptr @malloc(i32 size)
	auto malloc_ = new Function{};
	malloc_->name = "malloc";
	malloc_->type = &ptrType;
	malloc_->params.emplace_back(&intType, "size");
	module->functions.push_back(malloc_);
	// i32 @getInt()
	auto getInt = new Function{};
	getInt->name = "getInt";
	getInt->type = &intType;
	module->functions.push_back(getInt);
	// void @print(ptr str)
	auto print = new Function{};
	print->name = "print";
	print->type = &voidType;
	print->params.emplace_back(&ptrType, "str");
	module->functions.push_back(print);
	// void @printInt(i32 n)
	auto printInt = new Function{};
	printInt->name = "printInt";
	printInt->type = &voidType;
	printInt->params.emplace_back(&intType, "n");
	module->functions.push_back(printInt);
	// ptr @toString(i32 n)
	auto toString = new Function{};
	toString->name = "toString";
	toString->type = &ptrType;
	toString->params.emplace_back(&intType, "n");
	module->functions.push_back(toString);

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
	auto afterLoop = new Block{"while_end_" + std::to_string(loopCounter)};
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
	auto afterLoop = new Block{"for_end_" + std::to_string(loopCounter)};

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
	auto after = new Block{"if_end_" + std::to_string(ifCounter)};
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
	// load must do in this block
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


void IRBuilder::visitSingleExprNode(AstSingleExprNode *node) {
	if (node->op == "++" || node->op == "--") {// A++, A--, ++A, --A
		visit(node->expr);
		auto add = new ArithmeticStmt{};
		add->cmd = node->op == "++" ? "add" : "sub";
		add->lhs = remove_variable_pointer(exprResult[node->expr]);
		add->rhs = &literal_one;
		add->res = register_annoy_var(&intType);
		auto store = new StoreStmt{};
		store->value = add->res;
		store->pointer = dynamic_cast<Var *>(exprResult[node->expr]);
		add_stmt(add);
		add_stmt(store);
		if (node->right)
			exprResult[node] = add->res;
		else
			exprResult[node] = add->lhs;
	}
	else if (node->op == "+") {
		visit(node->expr);
		exprResult[node] = exprResult[node->expr];
	}
	else if (node->op == "-") {
		visit(node->expr);
		auto sub = new ArithmeticStmt{};
		sub->cmd = "sub";
		sub->lhs = &literal_zero;
		sub->rhs = remove_variable_pointer(exprResult[node->expr]);
		sub->res = register_annoy_var(&intType);
		add_stmt(sub);
		exprResult[node] = sub->res;
	}
	else if (node->op == "!") {
		visit(node->expr);
		auto xor_ = new ArithmeticStmt{};
		xor_->cmd = "xor";
		xor_->lhs = remove_variable_pointer(exprResult[node->expr]);
		xor_->rhs = &literal_true;
		xor_->res = register_annoy_var(&boolType);
		add_stmt(xor_);
		exprResult[node] = xor_->res;
	}
	else if (node->op == "~") {
		visit(node->expr);
		auto xor_ = new ArithmeticStmt{};
		xor_->cmd = "xor";
		xor_->lhs = remove_variable_pointer(exprResult[node->expr]);
		xor_->rhs = &literal_minus_one;
		xor_->res = register_annoy_var(&intType);
		add_stmt(xor_);
	}
	else
		throw std::runtime_error("unknown single expr op");
}

void IRBuilder::visitFuncCallExprNode(AstFuncCallExprNode *node) {
	std::string func_name;
	std::vector<Val *> args;
	if (auto acc = dynamic_cast<AstMemberAccessExprNode *>(node->func)) {
		func_name = acc->object->valueType.basicType->to_string() + "." + acc->member;
		visit(acc->object);
		args.push_back(remove_variable_pointer(exprResult[acc->object]));
	}
	else if (auto id = dynamic_cast<AstAtomExprNode *>(node->func))
		func_name = id->name;
	for (auto &arg: node->args) {
		visit(arg);
		args.push_back(remove_variable_pointer(exprResult[arg]));
	}
	auto call = new CallStmt{};
	auto p = currentClass ? name2function.find(currentClass->type.name + "." + func_name) : name2function.find(func_name);
	if (p == name2function.end()) p = name2function.find(func_name);
	call->func = p->second;
	call->args = std::move(args);
	call->res = (call->func->type == &voidType ? nullptr : register_annoy_var(call->func->type));
	add_stmt(call);
	exprResult[node] = call->res;
}

IR::StringLiteralVar *IRBuilder::register_literal_str(const std::string &str) {
	auto p = literalStr2var.find(str);
	if (p != literalStr2var.end())
		return p->second;
	auto var = new StringLiteralVar{};
	var->type = &ptrType;
	var->name = ".str-" + std::to_string(literalStr2var.size() + 1);
	auto str_assign = new GlobalStringStmt{};
	str_assign->var = var;
	str_assign->value = str;
	module->globalVars.push_back(var);
	module->stringLiterals.push_back(str_assign);
	return literalStr2var[str] = var;
}

void IRBuilder::visitArrayAccessExprNode(AstArrayAccessExprNode *node) {
	visit(node->array);
	visit(node->index);
	auto array = remove_variable_pointer(exprResult[node->array]);
	auto index = remove_variable_pointer(exprResult[node->index]);
	auto type = toIRType(node->valueType);
	auto gep = new GetElementPtrStmt{};
	gep->pointer = dynamic_cast<Var *>(array);
	gep->indices.push_back(index);
	gep->typeName = type->to_string();
	gep->res = register_annoy_ptr_var(type);
	add_stmt(gep);
	exprResult[node] = gep->res;
}

void IRBuilder::visitTernaryExprNode(AstTernaryExprNode *node) {
	++ternaryCounter;
	auto true_expr = new Block{"ternary_true_" + std::to_string(ternaryCounter)};
	auto false_expr = new Block{"ternary_false_" + std::to_string(ternaryCounter)};
	auto end = new Block{"ternary_end_" + std::to_string(ternaryCounter)};

	visit(node->cond);
	auto br_cond = new CondBrStmt{};
	br_cond->cond = remove_variable_pointer(exprResult[node->cond]);
	br_cond->trueBlock = true_expr;
	br_cond->falseBlock = false_expr;
	add_stmt(br_cond);

	auto br_end = new DirectBrStmt{};
	br_end->block = end;

	add_block(true_expr);
	visit(node->trueExpr);
	auto true_res = remove_variable_pointer(exprResult[node->trueExpr]);
	add_stmt(br_end);

	add_block(false_expr);
	visit(node->falseExpr);
	auto false_res = remove_variable_pointer(exprResult[node->falseExpr]);
	add_stmt(br_end);

	add_block(end);
	if (!node->valueType.is_void()) {
		auto phi = new PhiStmt{};
		phi->branches.emplace_back(true_res, true_expr);
		phi->branches.emplace_back(false_res, false_expr);
		phi->res = register_annoy_var(toIRType(node->valueType));
		add_stmt(phi);
		exprResult[node] = phi->res;
	}
}
