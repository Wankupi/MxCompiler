#include "Builder.h"
#include "AST/AstNode.h"
#include "IR/Type.h"

#include <iostream>

using namespace IR;

static VoidType voidType;
static IntegerType boolType(1);
static IntegerType intType(32);
static PtrType ptrType;
static StringType stringType;
static LiteralInt literal_zero(0), literal_one(1), literal_minus_one(-1);
static LiteralBool literal_true(true), literal_false(false);
static LiteralNull literal_null;
static const std::string empty_string{'\0'};


Val *IRBuilder::type_to_default_value(IR::Type *type) {
	if (type == &boolType)
		return &literal_false;
	else if (type == &intType)
		return &literal_zero;
	else if (type == &stringType)
		return literalStr2var[empty_string];
	else
		return &literal_null;
}

void IRBuilder::visitFileNode(AstFileNode *node) {
	if (module)
		throw std::runtime_error("IRBuilder: module already exists");
	module = new Module{};
	init_builtin_function();
	register_literal_str(empty_string);
	for (auto c: node->children) {
		if (auto func = dynamic_cast<AstFunctionNode *>(c))
			registerFunction(func);
		else if (auto cls = dynamic_cast<AstClassNode *>(c))
			registerClass(cls);
		else if (auto var = dynamic_cast<AstVarStmtNode *>(c))
			registerGlobalVar(var);
	}
	for (auto c: node->children) {
		if (dynamic_cast<AstVarStmtNode *>(c))
			continue;
		visit(c);
	}
	create_init_global_var_function();
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
		registerFunction(constructor);
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

void IRBuilder::registerGlobalVar(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	for (auto &var: node->vars_unique_name) {
		auto gv = new GlobalVar{};
		gv->name = var.first;
		gv->type = type;
		add_global_var(gv);

		auto globalStmt = new GlobalStmt{};
		globalStmt->var = gv;
		globalStmt->value = type_to_default_value(type);
		module->variables.push_back(globalStmt);

		if (var.second) {
			if (auto constant = dynamic_cast<AstLiterExprNode *>(var.second)) {
				visit(constant);
				globalStmt->value = exprResult[constant];
			}
			else
				globalInitList.emplace_back(globalStmt, var.second);
		}
	}
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

void IRBuilder::init_function_params(Function *func) {
	constexpr char const *const SUFFIX = ".param";
	for (auto &p: func->params) {
		auto var_val = new LocalVar{};
		var_val->type = p.first;
		var_val->name = p.second + SUFFIX;
		name2var[var_val->name] = var_val;

		auto var = new PtrVar{};
		var->type = &ptrType;
		var->objType = p.first;
		var->name = p.second;
		name2var[var->name] = var;

		auto alloc = new AllocaStmt{};
		alloc->res = var;
		alloc->type = p.first;
		add_stmt(alloc);

		auto st = new StoreStmt{};
		st->value = var_val;
		st->pointer = var;
		add_stmt(st);

		p.second += SUFFIX;
	}
}

void add_terminals(Function *func) {
	static UnreachableStmt unreachable{};
	static RetStmt ret_void{};
	static RetStmt ret_zero{&literal_zero};
	Stmt *to_insert = nullptr;
	if (func->type != &voidType)
		to_insert = &unreachable;
	else
		to_insert = &ret_void;
	if (func->name == "main")
		to_insert = &ret_zero;
	for (auto block: func->blocks) {
		auto bak = block->stmts.empty() ? nullptr : block->stmts.back();
		if (!(bak && (dynamic_cast<BrStmt *>(bak) || dynamic_cast<RetStmt *>(bak))))
			block->stmts.push_back(to_insert);
	}
}

void IRBuilder::visitFunctionNode(AstFunctionNode *node) {
	std::string name = (currentClass ? currentClass->type.name + "." : "") + node->name;
	auto func = name2function[name];
	func->blocks.push_back(new Block{"entry"});
	currentFunction = func;
	annoyCounter.clear();
	init_function_params(func);
	visit(node->body);
	currentFunction = nullptr;
	add_terminals(func);
}

IR::PrimitiveType *IRBuilder::toIRType(AstTypeNode *node) {
	if (!node) return &voidType;
	if (node->dimension) return &ptrType;
	if (node->name == "void")
		return &voidType;
	else if (node->name == "bool")
		return &boolType;
	else if (node->name == "int")
		return &intType;
	else if (node->name == "string")
		return &stringType;
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
	else if (typeInfo.is_string())
		return &stringType;
	else
		return &ptrType;
}

void IRBuilder::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	if (!currentFunction)// global
		return;

	// local
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
		auto t = remove_variable_pointer(name2var["this"]);
		gep->pointer = dynamic_cast<Var *>(t);
		if (!gep->pointer)
			throw std::runtime_error("this is not defined");
		gep->indices.push_back(&literal_zero);
		gep->indices.push_back(new LiteralInt(idx));
		gep->typeName = "%class." + currentClass->type.name;
		gep->res = register_annoy_ptr_var(currentClass->type.fields[idx], "$" + currentClass->type.name + "_" + name_to_find + ".");
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
		str += '\0';
		exprResult[node] = register_literal_str(str);
	}
	else
		throw std::runtime_error("IRBuilder: unknown literal type");
}

void IRBuilder::visitBinaryExprNode(AstBinaryExprNode *node) {
	if (node->op == "&&" || node->op == "||") {
		enterAndOrBinaryExprNode(node);
		return;
	}
	if (node->lhs->valueType.is_string()) {
		enterStringBinaryExprNode(node);
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
		exprResult[node] = arh->res = register_annoy_var(&intType, ".arith.");
		add_stmt(arh);
	}
	else if (cmp.contains(node->op)) {
		auto icmp = new IcmpStmt{};
		icmp->lhs = remove_variable_pointer(exprResult[node->lhs]);
		icmp->rhs = remove_variable_pointer(exprResult[node->rhs]);
		icmp->cmd = cmp[node->op];
		exprResult[node] = icmp->res = register_annoy_var(&boolType, ".cmp.");
		add_stmt(icmp);
	}
	else
		throw std::runtime_error("IRBuilder: unknown binary operator: " + node->op);
}

void IRBuilder::enterStringBinaryExprNode(AstBinaryExprNode *node) {
	std::map<std::string, std::string> cmd = {
			{"+", "string.add"},
			{"==", "string.equal"},
			{"!=", "string.notEqual"},
			{"<", "string.less"},
			{">", "string.greater"},
			{"<=", "string.lessEqual"},
			{">=", "string.greaterEqual"},
	};
	visit(node->lhs);
	visit(node->rhs);
	auto resType = node->op == "+" ? static_cast<IR::Type *>(&stringType) : &boolType;
	auto call = new CallStmt{};
	call->func = name2function[cmd[node->op]];
	call->args.emplace_back(remove_variable_pointer(exprResult[node->lhs]));
	call->args.emplace_back(remove_variable_pointer(exprResult[node->rhs]));
	exprResult[node] = call->res = register_annoy_var(resType, ".arith.str.");
	add_stmt(call);
}

IR::Val *IRBuilder::remove_variable_pointer(IR::Val *val) {
	auto loc = dynamic_cast<PtrVar *>(val);
	auto glo = dynamic_cast<GlobalVar *>(val);
	if (!(loc || glo)) return val;
	auto load = new LoadStmt{};
	load->res = register_annoy_var(loc ? loc->objType : glo->type, (loc ? loc->name : glo->name) + ".val.");
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
		auto gep = new GetElementPtrStmt{};
		gep->pointer = dynamic_cast<Var *>(obj);
		if (!gep->pointer)
			throw std::runtime_error("IRBuilder: member access on non-variable");

		auto cls = name2class[node->object->valueType.to_string()];
		int index = static_cast<int>(cls->name2index[node->member]);

		gep->res = register_annoy_ptr_var(cls->type.fields[index], ".gep.");
		gep->typeName = "%class." + cls->type.name;

		gep->indices.push_back(&literal_zero);
		gep->indices.push_back(new LiteralInt{index});

		add_stmt(gep);
		exprResult[node] = gep->res;
	}
}

void Class::add_filed(PrimitiveType *mem_type, const std::string &mem_name) {
	type.fields.emplace_back(mem_type);
	name2index[mem_name] = type.fields.size() - 1;
}

IR::LocalVar *IRBuilder::register_annoy_var(IR::Type *type, std::string const &prefix) {
	auto var = new LocalVar{};
	var->type = type;
	var->name = prefix + std::to_string(annoyCounter[prefix]++);
	add_local_var(var);
	return var;
}

IR::PtrVar *IRBuilder::register_annoy_ptr_var(IR::Type *obj_type, std::string const &prefix) {
	auto var = new PtrVar{};
	var->type = &ptrType;
	var->objType = obj_type;
	var->name = prefix + std::to_string(annoyCounter[prefix]++);
	add_local_var(var);
	return var;
}

void IRBuilder::init_builtin_function() {
	// void @print(ptr str)
	auto print = new Function{};
	print->name = "print";
	print->type = &voidType;
	print->params.emplace_back(&stringType, "str");
	module->functions.push_back(print);

	// void @println(ptr str)
	auto println = new Function{};
	println->name = "println";
	println->type = &voidType;
	println->params.emplace_back(&stringType, "str");
	module->functions.push_back(println);

	// void @printInt(i32 n)
	auto printInt = new Function{};
	printInt->name = "printInt";
	printInt->type = &voidType;
	printInt->params.emplace_back(&intType, "n");
	module->functions.push_back(printInt);

	// void @printlnInt(i32 n)
	auto printlnInt = new Function{};
	printlnInt->name = "printlnInt";
	printlnInt->type = &voidType;
	printlnInt->params.emplace_back(&intType, "n");
	module->functions.push_back(printlnInt);


	// ptr @getString()
	auto getString = new Function{};
	getString->name = "getString";
	getString->type = &stringType;
	module->functions.push_back(getString);

	// i32 @getInt()
	auto getInt = new Function{};
	getInt->name = "getInt";
	getInt->type = &intType;
	module->functions.push_back(getInt);

	// ptr @toString(i32 n)
	auto toString = new Function{};
	toString->name = "toString";
	toString->type = &stringType;
	toString->params.emplace_back(&intType, "n");
	module->functions.push_back(toString);

	// i32 @.array.size(ptr array)
	auto array_size = new Function{};
	array_size->name = "_array.size";
	array_size->type = &intType;
	array_size->params.emplace_back(&ptrType, "array");
	module->functions.push_back(array_size);

	// i32 @string.length(ptr str)
	auto string_length = new Function{};
	string_length->name = "string.length";
	string_length->type = &intType;
	string_length->params.emplace_back(&stringType, "str");
	module->functions.push_back(string_length);

	// ptr @string.substring(ptr str, i32 left, i32 right)
	auto substring = new Function{};
	substring->name = "string.substring";
	substring->type = &stringType;
	substring->params.emplace_back(&stringType, "str");
	substring->params.emplace_back(&intType, "left");
	substring->params.emplace_back(&intType, "right");
	module->functions.push_back(substring);

	// i32 @string.parseInt(ptr str)
	auto parseInt = new Function{};
	parseInt->name = "string.parseInt";
	parseInt->type = &intType;
	parseInt->params.emplace_back(&stringType, "str");
	module->functions.push_back(parseInt);

	// i32 @string.ord(ptr str, i32 pos)
	auto ord = new Function{};
	ord->name = "string.ord";
	ord->type = &intType;
	ord->params.emplace_back(&stringType, "str");
	ord->params.emplace_back(&intType, "pos");
	module->functions.push_back(ord);

	// ptr @string.add(ptr str1, ptr str2)
	auto str_add = new Function{};
	str_add->name = "string.add";
	str_add->type = &stringType;
	str_add->params.emplace_back(&stringType, "lhs");
	str_add->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_add);

	// i1 @string.equal(ptr str1, ptr str2)
	auto str_equal = new Function{};
	str_equal->name = "string.equal";
	str_equal->type = &boolType;
	str_equal->params.emplace_back(&stringType, "lhs");
	str_equal->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_equal);

	// i1 @string.notEqual(ptr str1, ptr str2)
	auto str_notEqual = new Function{};
	str_notEqual->name = "string.notEqual";
	str_notEqual->type = &boolType;
	str_notEqual->params.emplace_back(&stringType, "lhs");
	str_notEqual->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_notEqual);

	// i1 @string.less(ptr str1, ptr str2)
	auto str_less = new Function{};
	str_less->name = "string.less";
	str_less->type = &boolType;
	str_less->params.emplace_back(&stringType, "lhs");
	str_less->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_less);

	// i1 @string.greater(ptr str1, ptr str2)
	auto str_greater = new Function{};
	str_greater->name = "string.greater";
	str_greater->type = &boolType;
	str_greater->params.emplace_back(&stringType, "lhs");
	str_greater->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_greater);

	// i1 @string.lessEqual(ptr str1, ptr str2)
	auto str_lessEqual = new Function{};
	str_lessEqual->name = "string.lessEqual";
	str_lessEqual->type = &boolType;
	str_lessEqual->params.emplace_back(&stringType, "lhs");
	str_lessEqual->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_lessEqual);

	// i1 @string.greaterEqual(ptr str1, ptr str2)
	auto str_greaterEqual = new Function{};
	str_greaterEqual->name = "string.greaterEqual";
	str_greaterEqual->type = &boolType;
	str_greaterEqual->params.emplace_back(&stringType, "lhs");
	str_greaterEqual->params.emplace_back(&stringType, "rhs");
	module->functions.push_back(str_greaterEqual);

	// ptr @malloc(i32 size)
	auto malloc_ = new Function{};
	malloc_->name = "malloc";
	malloc_->type = &ptrType;
	malloc_->params.emplace_back(&intType, "size");
	module->functions.push_back(malloc_);

	// i32 @__array.size(ptr array)
	auto array_size_ = new Function{};
	array_size_->name = "__array.size";
	array_size_->type = &intType;
	array_size_->params.emplace_back(&ptrType, "array");
	module->functions.push_back(array_size_);

	// ptr @__newPtrArray(i32 size)
	auto newPtrArray = new Function{};
	newPtrArray->name = "__newPtrArray";
	newPtrArray->type = &ptrType;
	newPtrArray->params.emplace_back(&intType, "size");
	module->functions.push_back(newPtrArray);

	// ptr @__newIntArray(i32 size)
	auto newIntArray = new Function{};
	newIntArray->name = "__newIntArray";
	newIntArray->type = &ptrType;
	newIntArray->params.emplace_back(&intType, "size");
	module->functions.push_back(newIntArray);

	// ptr @__newBoolArray(i32 size)
	auto newBoolArray = new Function{};
	newBoolArray->name = "__newBoolArray";
	newBoolArray->type = &ptrType;
	newBoolArray->params.emplace_back(&intType, "size");
	module->functions.push_back(newBoolArray);

	for (auto func: module->functions)
		name2function[func->name] = func;
}

void IRBuilder::visitNewExprNode(AstNewExprNode *node) {
	std::vector<Val *> array_size;
	for (auto expr: node->type->arraySize) {
		visit(expr);
		auto v = remove_variable_pointer(exprResult[expr]);
		array_size.push_back(v);
	}
	++newCounter;
	exprResult[node] = TransformNewToFor(array_size, static_cast<int>(node->type->dimension), node->type->name);
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
	if (node->init) visit(node->init);

	// create blocks
	++loopCounter;
	auto body = new Block{"for_body_" + std::to_string(loopCounter)};
	auto cond = node->cond ? new Block{"for_cond_" + std::to_string(loopCounter)} : body;
	auto step = node->step ? new Block{"for_step_" + std::to_string(loopCounter)} : cond;
	auto afterLoop = new Block{"for_end_" + std::to_string(loopCounter)};

	auto br2cond = new DirectBrStmt{};
	br2cond->block = cond;
	add_stmt(br2cond);

	// visit cond
	if (node->cond) {
		add_block(cond);
		visit(node->cond);
		auto br2body = new CondBrStmt{};
		br2body->cond = remove_variable_pointer(exprResult[node->cond]);
		br2body->trueBlock = body;
		br2body->falseBlock = afterLoop;
		add_stmt(br2body);
	}
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
	if (node->step) {
		add_block(step);
		visit(node->step);
		add_stmt(br2cond);
	}
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
	exprResult[node] = phi->res = register_annoy_var(&boolType, ".short.");
	add_stmt(phi);
}


void IRBuilder::visitSingleExprNode(AstSingleExprNode *node) {
	if (node->op == "++" || node->op == "--") {// A++, A--, ++A, --A
		visit(node->expr);
		auto add = new ArithmeticStmt{};
		add->cmd = node->op == "++" ? "add" : "sub";
		add->lhs = remove_variable_pointer(exprResult[node->expr]);
		add->rhs = &literal_one;
		add->res = register_annoy_var(&intType, ".arith.");
		auto store = new StoreStmt{};
		store->value = add->res;
		store->pointer = dynamic_cast<Var *>(exprResult[node->expr]);
		add_stmt(add);
		if (node->right)
			exprResult[node] = add->lhs;
		else
			exprResult[node] = add->res;
		add_stmt(store);
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
		sub->res = register_annoy_var(&intType, ".arith.");
		add_stmt(sub);
		exprResult[node] = sub->res;
	}
	else if (node->op == "!") {
		visit(node->expr);
		auto xor_ = new ArithmeticStmt{};
		xor_->cmd = "xor";
		xor_->lhs = remove_variable_pointer(exprResult[node->expr]);
		xor_->rhs = &literal_true;
		xor_->res = register_annoy_var(&boolType, ".arith.");
		add_stmt(xor_);
		exprResult[node] = xor_->res;
	}
	else if (node->op == "~") {
		visit(node->expr);
		auto xor_ = new ArithmeticStmt{};
		xor_->cmd = "xor";
		xor_->lhs = remove_variable_pointer(exprResult[node->expr]);
		xor_->rhs = &literal_minus_one;
		xor_->res = register_annoy_var(&intType, ".arith.");
		add_stmt(xor_);
	}
	else
		throw std::runtime_error("unknown single expr op");
}

void IRBuilder::visitFuncCallExprNode(AstFuncCallExprNode *node) {
	std::string func_name;
	std::vector<Val *> args;
	if (auto acc = dynamic_cast<AstMemberAccessExprNode *>(node->func)) {
		func_name = acc->object->valueType.dimension ? "__array.size" : acc->object->valueType.basicType->to_string() + "." + acc->member;
		visit(acc->object);
		args.push_back(remove_variable_pointer(exprResult[acc->object]));
	}
	else if (auto id = dynamic_cast<AstAtomExprNode *>(node->func))
		func_name = id->name;

	auto p = currentClass ? name2function.find(currentClass->type.name + "." + func_name) : name2function.find(func_name);
	if (p == name2function.end()) p = name2function.find(func_name);
	else if (currentClass)
		args.push_back(remove_variable_pointer(name2var["this"]));
	for (auto &arg: node->args) {
		visit(arg);
		args.push_back(remove_variable_pointer(exprResult[arg]));
	}

	auto call = new CallStmt{};
	call->func = p->second;
	call->args = std::move(args);
	call->res = (call->func->type == &voidType ? nullptr : register_annoy_var(call->func->type, ".call."));
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
	gep->res = register_annoy_ptr_var(type, ".arr.");
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
	auto from_true = currentFunction->blocks.back();

	add_block(false_expr);
	visit(node->falseExpr);
	auto false_res = remove_variable_pointer(exprResult[node->falseExpr]);
	add_stmt(br_end);
	auto from_false = currentFunction->blocks.back();

	add_block(end);
	if (!node->valueType.is_void()) {
		auto phi = new PhiStmt{};
		phi->branches.emplace_back(true_res, from_true);
		phi->branches.emplace_back(false_res, from_false);
		phi->res = register_annoy_var(toIRType(node->valueType), ".ternary_res.");
		add_stmt(phi);
		exprResult[node] = phi->res;
	}
}

void IRBuilder::create_init_global_var_function() {
	if (globalInitList.empty())
		return;

	auto first_sign = new GlobalVar{};
	first_sign->name = ".global_var_inited";
	first_sign->type = &boolType;
	module->globalVars.push_back(first_sign);

	auto first_sign_stmt = new GlobalStmt{};
	first_sign_stmt->var = first_sign;
	first_sign_stmt->value = &literal_false;
	module->variables.push_back(first_sign_stmt);

	auto func = new Function{};
	func->name = ".init_global_var";
	func->type = &voidType;
	func->blocks.emplace_back(new Block{"entry"});
	currentFunction = func;

	auto init_block = new Block{"init"};
	auto end_block = new Block{"end"};

	auto is_first = new CondBrStmt{};
	is_first->cond = remove_variable_pointer(first_sign);
	is_first->trueBlock = end_block;
	is_first->falseBlock = init_block;
	add_stmt(is_first);

	add_block(init_block);
	auto set_sign = new StoreStmt{};
	set_sign->pointer = first_sign;
	set_sign->value = &literal_true;
	add_stmt(set_sign);
	for (auto &init: globalInitList) {
		visit(init.second);
		auto store = new StoreStmt{};
		store->value = remove_variable_pointer(exprResult[init.second]);
		if (auto gs = dynamic_cast<GlobalStmt *>(init.first))
			store->pointer = gs->var;
		else if (auto gss = dynamic_cast<GlobalStringStmt *>(init.first))
			store->pointer = gss->var;
		add_stmt(store);
	}
	add_block(end_block);
	add_terminals(func);
	currentFunction = nullptr;
	for (auto f: module->functions)
		if (f->name == "main") {
			auto call = new CallStmt{};
			call->func = func;
			call->res = nullptr;
			auto &stmts = f->blocks[0]->stmts;
			stmts.insert(stmts.begin(), call);
			break;
		}
	module->functions.push_back(func);
}


IR::Var *IRBuilder::TransformNewToFor(std::vector<IR::Val *> const &array_size, int total_dim, std::string const &base_typename, int dep) {
	const std::map<std::string, IR::Type *> type_map = {
			{"int", &intType},
			{"bool", &boolType},
			{"string", &stringType},
			{"void", &voidType}};
	if (dep == array_size.size()) {
		IR::Type *type = nullptr;
		if (dep < total_dim) type = &ptrType;
		else if (auto p = type_map.find(base_typename); p != type_map.end())
			type = p->second;
		else if (auto q = name2class.find(base_typename); q != name2class.end())
			type = &q->second->type;
		else
			throw std::runtime_error(std::string("unknown type") + __FILE_NAME__);

		auto make_obj = new CallStmt{};
		make_obj->func = name2function["malloc"];
		make_obj->args.push_back(new LiteralInt{type->size()});
		make_obj->res = register_annoy_var(&ptrType, ".new_obj.");
		add_stmt(make_obj);

		if (type == &stringType) {
			auto store = new StoreStmt{};
			store->pointer = make_obj->res;
			store->value = literalStr2var[empty_string];
			add_stmt(store);
		}
		else if (type != &ptrType && type != &intType && type != &boolType) {
			auto con = name2function.find(base_typename + "." + base_typename);
			if (con != name2function.end()) {
				auto call = new CallStmt{};
				call->func = con->second;
				call->args.push_back(make_obj->res);
				add_stmt(call);
			}
		}
		//		auto debug_info = new CallStmt{};
		//		debug_info->func = name2function["printInt"];
		//		debug_info->args.push_back(&literal_zero);
		//		add_stmt(debug_info);

		return make_obj->res;
	}
	else {
		std::string functionName;
		std::string gepName;
		if (dep + 1 < total_dim) {
			functionName = "__newPtrArray";
			gepName = "ptr";
		}
		else {// dep + 1 == total_dim
			if (base_typename == "int") {
				functionName = "__newIntArray";
				gepName = "i32";
			}
			else if (base_typename == "bool") {
				functionName = "__newBoolArray";
				gepName = "i1";
			}
			else {
				functionName = "__newPtrArray";
				gepName = "ptr";
			}
		}
		auto make_array = new CallStmt{};
		make_array->func = name2function[functionName];
		make_array->args.push_back(array_size[dep]);
		make_array->res = register_annoy_var(&ptrType, ".new_array.");
		add_stmt(make_array);

		if (dep + 1 == array_size.size() && (total_dim > array_size.size() || (base_typename == "int" || base_typename == "bool")))
			return make_array->res;


		std::string label = "new_" + std::to_string(newCounter) + "_for_" + std::to_string(dep);
		auto cond_block = new Block{label + "_cond"};
		auto body_block = new Block{label + "_body"};
		auto end_block = new Block{label + "_end"};

		auto counter = register_annoy_var(&intType, ".new_for_idx.");
		auto increased_counter = register_annoy_var(&intType, ".new_for_idx_inc.");

		auto br_cond = new DirectBrStmt{};
		br_cond->block = cond_block;
		add_stmt(br_cond);

		auto current_block = currentFunction->blocks.back();
		add_block(cond_block);
		auto phi = new PhiStmt{};
		phi->branches.emplace_back(&literal_zero, current_block);
		// phi->branches will be pushed later
		phi->res = counter;
		add_stmt(phi);
		auto cmp = new IcmpStmt{};
		cmp->res = register_annoy_var(&boolType, ".new_for_cmp.");
		cmp->cmd = "slt";
		cmp->lhs = counter;
		cmp->rhs = array_size[dep];
		add_stmt(cmp);
		auto br = new CondBrStmt{};
		br->cond = cmp->res;
		br->trueBlock = body_block;
		br->falseBlock = end_block;
		add_stmt(br);

		add_block(body_block);
		auto ptr = TransformNewToFor(array_size, total_dim, base_typename, dep + 1);

		auto gep = new GetElementPtrStmt{};
		gep->res = register_annoy_var(&ptrType, ".new_for_gep.");
		gep->pointer = make_array->res;
		gep->typeName = gepName;
		gep->indices.push_back(counter);
		add_stmt(gep);

		auto store = new StoreStmt{};
		store->pointer = gep->res;
		store->value = ptr;
		add_stmt(store);

		auto inc = new ArithmeticStmt{};
		inc->res = increased_counter;
		inc->lhs = counter;
		inc->rhs = &literal_one;
		inc->cmd = "add";
		add_stmt(inc);

		add_stmt(br_cond);
		phi->branches.emplace_back(increased_counter, currentFunction->blocks.back());
		add_block(end_block);

		return make_array->res;
	}
}