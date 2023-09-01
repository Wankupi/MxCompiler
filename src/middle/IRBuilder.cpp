#include "IRBuilder.h"
#include "AST/AstNode.h"
#include "IR/Type.h"

#include <iostream>

using namespace IR;

static constexpr const std::string empty_string;

Val *IRBuilder::type_to_default_value(IR::Type *type) {
	if (type == env.boolType)
		return env.literal(false);
	else if (type == env.intType)
		return env.literal(0);
	else if (type == env.stringType)
		return env.literal(empty_string);
	else
		return env.literal(nullptr);
}

void IRBuilder::visitFileNode(AstFileNode *node) {
	if (module)
		throw std::runtime_error("IRBuilder: module already exists");
	module = env.createModule();
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
	auto cls = env.createClass(node->name);
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
	auto func = env.createFunction(toIRType(node->returnType),
								   currentClass ? currentClass->type.name + "." + node->name : node->name);
	if (currentClass)
		func->params.emplace_back(env.ptrType, "this");
	for (auto &p: node->params)
		func->params.emplace_back(toIRType(p.first), p.second);
	module->functions.push_back(func);
	name2function[func->name] = func;
}

void IRBuilder::registerGlobalVar(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	for (auto &var: node->vars_unique_name) {
		auto gv = env.create_global_var(type, var.first);
		add_global_var(gv);

		auto globalStmt = env.createGlobalStmt(gv, type_to_default_value(type));
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
		auto var_val = env.create_local_var(p.first, p.second + SUFFIX);
		name2var[var_val->name] = var_val;
		func->paramsVar.push_back(var_val);

		auto var = env.create_ptr_var(p.first, p.second);
		name2var[var->name] = var;

		auto alloc = env.createAllocaStmt(var);
		add_stmt(alloc);

		auto st = env.createStoreStmt(var_val, var);
		add_stmt(st);

		p.second += SUFFIX;
	}
}

void IRBuilder::add_terminals(Function *func) {
	static UnreachableStmt unreachable{};
	static RetStmt ret_void{};
	static RetStmt ret_zero{env.literal(0)};
	Stmt *to_insert = nullptr;
	if (func->type != env.voidType)
		to_insert = env.createUnreachableStmt();
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
	func->blocks.push_back(env.createBasicBlock("entry"));
	currentFunction = func;
	annoyCounter.clear();
	init_function_params(func);
	visit(node->body);
	currentFunction = nullptr;
	add_terminals(func);
}

IR::PrimitiveType *IRBuilder::toIRType(AstTypeNode *node) {
	if (!node) return env.voidType;
	if (node->dimension) return env.ptrType;
	if (node->name == "void")
		return env.voidType;
	else if (node->name == "bool")
		return env.boolType;
	else if (node->name == "int")
		return env.intType;
	else if (node->name == "string")
		return env.stringType;
	else
		return env.ptrType;// string and class is ptr
}

IR::PrimitiveType *IRBuilder::toIRType(TypeInfo typeInfo) {
	if (typeInfo.dimension) return env.ptrType;
	if (typeInfo.is_void()) return env.voidType;
	else if (typeInfo.is_bool())
		return env.boolType;
	else if (typeInfo.is_int())
		return env.intType;
	else if (typeInfo.is_string())
		return env.stringType;
	else
		return env.ptrType;
}

void IRBuilder::visitVarStmtNode(AstVarStmtNode *node) {
	auto type = toIRType(node->type);
	if (!currentFunction)// global
		return;

	// local
	for (auto &var: node->vars_unique_name) {
		if (var.second)
			visit(var.second);

		auto def_var = env.create_ptr_var(type, var.first);
		add_local_var(def_var);

		auto alloc = env.createAllocaStmt(def_var);
		add_stmt(alloc);
		if (var.second) {
			auto st = env.createStoreStmt(remove_variable_pointer(exprResult[var.second]), def_var);
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

void IRBuilder::add_phi(IR::PhiStmt *phi) {
	currentFunction->blocks.back()->phis[phi->res] = phi;
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
		auto _this = remove_variable_pointer(name2var["this"]);
		if (!_this)
			throw std::runtime_error("IRBuilder: accessing member variable without this");
		auto res = register_annoy_ptr_var(currentClass->type.fields[idx], "$" + currentClass->type.name + "_" + name_to_find + ".");
		auto gep = env.createGetElementPtrStmt("%class." + currentClass->type.name,
											   res,
											   dynamic_cast<Var *>(_this),
											   std::vector<Val *>{env.literal(0), env.literal(idx)});
		add_stmt(gep);
		exprResult[node] = res;
	}
	else
		throw std::runtime_error("unknown variable: " + node->name);
}

void IRBuilder::visitAssignExprNode(AstAssignExprNode *node) {
	visit(node->lhs);
	visit(node->rhs);
	auto st = env.createStoreStmt(remove_variable_pointer(exprResult[node->rhs]),
								  dynamic_cast<Var *>(exprResult[node->lhs]));
	add_stmt(st);
}

void IRBuilder::visitLiterExprNode(AstLiterExprNode *node) {
	if (node->valueType.is_null())
		exprResult[node] = env.literal(nullptr);
	else if (node->valueType.is_bool())
		exprResult[node] = env.literal(node->value == "true");
	else if (node->valueType.is_int())
		exprResult[node] = env.literal(std::stoi(node->value));
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
	auto lhs = remove_variable_pointer(exprResult[node->lhs]);
	visit(node->rhs);
	auto rhs = remove_variable_pointer(exprResult[node->rhs]);
	if (arth.contains(node->op)) {
		auto arh = env.createArithmeticStmt(arth[node->op], nullptr, lhs, rhs);
		exprResult[node] = arh->res = register_annoy_var(env.intType, ".arith.");
		add_stmt(arh);
	}
	else if (cmp.contains(node->op)) {
		auto icmp = env.createIcmpStmt(cmp[node->op], nullptr, lhs, rhs);
		exprResult[node] = icmp->res = register_annoy_var(env.boolType, ".cmp.");
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
	auto resType = node->op == "+" ? env.stringType : env.boolType;
	auto lhs = remove_variable_pointer(exprResult[node->lhs]);
	auto rhs = remove_variable_pointer(exprResult[node->rhs]);
	auto call = env.createCallStmt(name2function[cmd[node->op]], std::vector<Val *>{lhs, rhs});
	exprResult[node] = call->res = register_annoy_var(resType, ".arith.str.");
	add_stmt(call);
}

IR::Val *IRBuilder::remove_variable_pointer(IR::Val *val) {
	auto loc = dynamic_cast<PtrVar *>(val);
	auto glo = dynamic_cast<GlobalVar *>(val);
	if (!(loc || glo)) return val;
	auto load = env.createLoadStmt(
			register_annoy_var(loc ? loc->objType : glo->type, (loc ? loc->name : glo->name) + ".val."),
			loc ? static_cast<Var *>(loc) : static_cast<Var *>(glo));
	add_stmt(load);
	return load->res;
}

void IRBuilder::visitMemberAccessExprNode(AstMemberAccessExprNode *node) {
	visit(node->object);
	if (node->valueType.is_function()) {
		std::cout << "<TODO: call Class.function>";
	}
	else {
		auto cls = name2class[node->object->valueType.to_string()];
		int index = static_cast<int>(cls->name2index[node->member]);

		auto obj = remove_variable_pointer(exprResult[node->object]);
		auto gep = env.createGetElementPtrStmt("%class." + cls->type.name,
											   register_annoy_ptr_var(cls->type.fields[index], ".gep."),
											   dynamic_cast<Var *>(obj),
											   std::vector<Val *>{env.literal(0), env.literal(index)});
		if (!gep->pointer)
			throw std::runtime_error("IRBuilder: member access on non-variable");
		add_stmt(gep);
		exprResult[node] = gep->res;
	}
}

void Class::add_filed(PrimitiveType *mem_type, const std::string &mem_name) {
	type.fields.emplace_back(mem_type);
	name2index[mem_name] = type.fields.size() - 1;
}

IR::LocalVar *IRBuilder::register_annoy_var(IR::Type *type, std::string const &prefix) {
	auto var = env.create_local_var(type, prefix + std::to_string(annoyCounter[prefix]++));
	add_local_var(var);
	return var;
}

IR::PtrVar *IRBuilder::register_annoy_ptr_var(IR::Type *obj_type, std::string const &prefix) {
	auto var = env.create_ptr_var(obj_type, prefix + std::to_string(annoyCounter[prefix]++));
	add_local_var(var);
	return var;
}

void IRBuilder::init_builtin_function() {
	// void @print(ptr str)
	auto print = env.createFunction(env.voidType, "print");
	print->params.emplace_back(env.stringType, "str");
	module->functions.push_back(print);

	// void @println(ptr str)
	auto println = env.createFunction(env.voidType, "println");
	println->params.emplace_back(env.stringType, "str");
	module->functions.push_back(println);

	// void @printInt(i32 n)
	auto printInt = env.createFunction(env.voidType, "printInt");
	printInt->params.emplace_back(env.intType, "n");
	module->functions.push_back(printInt);

	// void @printlnInt(i32 n)
	auto printlnInt = env.createFunction(env.voidType, "printlnInt");
	printlnInt->params.emplace_back(env.intType, "n");
	module->functions.push_back(printlnInt);


	// ptr @getString()
	auto getString = env.createFunction(env.stringType, "getString");
	module->functions.push_back(getString);

	// i32 @getInt()
	auto getInt = env.createFunction(env.intType, "getInt");
	module->functions.push_back(getInt);

	// ptr @toString(i32 n)
	auto toString = env.createFunction(env.stringType, "toString");
	toString->params.emplace_back(env.intType, "n");
	module->functions.push_back(toString);

	// i32 @.array.size(ptr array)
	auto array_size = env.createFunction(env.intType, "_array.size");
	array_size->params.emplace_back(env.ptrType, "array");
	module->functions.push_back(array_size);

	// i32 @string.length(ptr str)
	auto string_length = env.createFunction(env.intType, "string.length");
	string_length->params.emplace_back(env.stringType, "str");
	module->functions.push_back(string_length);

	// ptr @string.substring(ptr str, i32 left, i32 right)
	auto substring = env.createFunction(env.stringType, "string.substring");
	substring->params.emplace_back(env.stringType, "str");
	substring->params.emplace_back(env.intType, "left");
	substring->params.emplace_back(env.intType, "right");
	module->functions.push_back(substring);

	// i32 @string.parseInt(ptr str)
	auto parseInt = env.createFunction(env.intType, "string.parseInt");
	parseInt->params.emplace_back(env.stringType, "str");
	module->functions.push_back(parseInt);

	// i32 @string.ord(ptr str, i32 pos)
	auto ord = env.createFunction(env.intType, "string.ord");
	ord->params.emplace_back(env.stringType, "str");
	ord->params.emplace_back(env.intType, "pos");
	module->functions.push_back(ord);

	// ptr @string.add(ptr str1, ptr str2)
	auto str_add = env.createFunction(env.stringType, "string.add");
	str_add->params.emplace_back(env.stringType, "lhs");
	str_add->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_add);

	// i1 @string.equal(ptr str1, ptr str2)
	auto str_equal = env.createFunction(env.boolType, "string.equal");
	str_equal->params.emplace_back(env.stringType, "lhs");
	str_equal->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_equal);

	// i1 @string.notEqual(ptr str1, ptr str2)
	auto str_notEqual = env.createFunction(env.boolType, "string.notEqual");
	str_notEqual->params.emplace_back(env.stringType, "lhs");
	str_notEqual->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_notEqual);

	// i1 @string.less(ptr str1, ptr str2)
	auto str_less = env.createFunction(env.boolType, "string.less");
	str_less->params.emplace_back(env.stringType, "lhs");
	str_less->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_less);

	// i1 @string.greater(ptr str1, ptr str2)
	auto str_greater = env.createFunction(env.boolType, "string.greater");
	str_greater->params.emplace_back(env.stringType, "lhs");
	str_greater->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_greater);

	// i1 @string.lessEqual(ptr str1, ptr str2)
	auto str_lessEqual = env.createFunction(env.boolType, "string.lessEqual");
	str_lessEqual->params.emplace_back(env.stringType, "lhs");
	str_lessEqual->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_lessEqual);

	// i1 @string.greaterEqual(ptr str1, ptr str2)
	auto str_greaterEqual = env.createFunction(env.boolType, "string.greaterEqual");
	str_greaterEqual->params.emplace_back(env.stringType, "lhs");
	str_greaterEqual->params.emplace_back(env.stringType, "rhs");
	module->functions.push_back(str_greaterEqual);

	// ptr @malloc(i32 size)
	auto malloc_ = env.createFunction(env.ptrType, "malloc");
	malloc_->params.emplace_back(env.intType, "size");
	module->functions.push_back(malloc_);

	// i32 @__array.size(ptr array)
	auto array_size_ = env.createFunction(env.intType, "__array.size");
	array_size_->params.emplace_back(env.ptrType, "array");
	module->functions.push_back(array_size_);

	// ptr @__newPtrArray(i32 size)
	auto newPtrArray = env.createFunction(env.ptrType, "__newPtrArray");
	newPtrArray->params.emplace_back(env.intType, "size");
	module->functions.push_back(newPtrArray);

	// ptr @__newIntArray(i32 size)
	auto newIntArray = env.createFunction(env.ptrType, "__newIntArray");
	newIntArray->params.emplace_back(env.intType, "size");
	module->functions.push_back(newIntArray);

	// ptr @__newBoolArray(i32 size)
	auto newBoolArray = env.createFunction(env.ptrType, "__newBoolArray");
	newBoolArray->params.emplace_back(env.intType, "size");
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
	auto ret = env.createRetStmt();
	if (node->expr) {
		visit(node->expr);
		ret->value = remove_variable_pointer(exprResult[node->expr]);
	}
	add_stmt(ret);
	auto block = env.createBasicBlock("after_return_" + std::to_string(++returnCounter));
	add_block(block);
}

void IRBuilder::visitWhileStmtNode(AstWhileStmtNode *node) {
	++loopCounter;
	auto cond = env.createBasicBlock("while_cond_" + std::to_string(loopCounter));
	auto body = env.createBasicBlock("while_body_" + std::to_string(loopCounter));
	auto afterLoop = env.createBasicBlock("while_end_" + std::to_string(loopCounter));

	auto brCond = env.createDirectBrStmt(cond);
	add_stmt(brCond);

	// visit cond
	add_block(cond);
	visit(node->cond);
	auto brBody = env.createCondBrStmt(remove_variable_pointer(exprResult[node->cond]), body, afterLoop);
	add_stmt(brBody);

	// visit body
	add_block(body);
	push_loop(cond, afterLoop);
	for (auto stmt: node->body)
		visit(stmt);
	add_stmt(brCond);
	pop_loop();

	// after loop
	add_block(afterLoop);
}

void IRBuilder::add_block(IR::BasicBlock *block) {
	currentFunction->blocks.push_back(block);
}

void IRBuilder::visitForStmtNode(AstForStmtNode *node) {
	// visit init
	if (node->init) visit(node->init);

	// create blocks
	++loopCounter;
	auto body = env.createBasicBlock("for_body_" + std::to_string(loopCounter));
	auto cond = node->cond ? env.createBasicBlock("for_cond_" + std::to_string(loopCounter)) : body;
	auto step = node->step ? env.createBasicBlock("for_step_" + std::to_string(loopCounter)) : cond;
	auto afterLoop = env.createBasicBlock("for_end_" + std::to_string(loopCounter));

	auto br2cond = env.createDirectBrStmt(cond);
	add_stmt(br2cond);

	// visit cond
	if (node->cond) {
		add_block(cond);
		visit(node->cond);
		auto br2body = env.createCondBrStmt(remove_variable_pointer(exprResult[node->cond]), body, afterLoop);
		add_stmt(br2body);
	}
	// visit body
	add_block(body);
	push_loop(step, afterLoop);
	for (auto stmt: node->body)
		visit(stmt);
	auto br2step = env.createDirectBrStmt(step);
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

void IRBuilder::push_loop(IR::BasicBlock *step, IR::BasicBlock *after) {
	loopBreakTo.push(after);
	loopContinueTo.push(step);
}

void IRBuilder::pop_loop() {
	loopBreakTo.pop();
	loopContinueTo.pop();
}

void IRBuilder::visitBreakStmtNode(AstBreakStmtNode *node) {
	auto br = env.createDirectBrStmt(loopBreakTo.top());
	add_stmt(br);
	auto block = env.createBasicBlock("after_break_" + std::to_string(++breakCounter));
	add_block(block);
}

void IRBuilder::visitContinueStmtNode(AstContinueStmtNode *node) {
	auto br = env.createDirectBrStmt(loopContinueTo.top());
	add_stmt(br);
	auto block = env.createBasicBlock("after_continue_" + std::to_string(++continueCounter));
	add_block(block);
}

void IRBuilder::visitIfStmtNode(AstIfStmtNode *node) {
	auto after = env.createBasicBlock("if_end_" + std::to_string(ifCounter));

	for (auto &clause: node->ifStmts) {
		++ifCounter;
		auto true_block = env.createBasicBlock("if_true_" + std::to_string(ifCounter));
		auto false_block = (&clause != &node->ifStmts.back() || node->elseStmt) ? env.createBasicBlock("if_false_" + std::to_string(ifCounter)) : nullptr;

		visit(clause.first);
		auto br_cond = env.createCondBrStmt(remove_variable_pointer(exprResult[clause.first]),
											true_block,
											false_block ? false_block : after);
		add_stmt(br_cond);

		add_block(true_block);
		visit(clause.second);
		add_stmt(env.createDirectBrStmt(after));

		if (false_block)
			add_block(false_block);
	}
	if (node->elseStmt) {
		visit(node->elseStmt);
		add_stmt(env.createDirectBrStmt(after));
	}
	add_block(after);
}


void IRBuilder::enterAndOrBinaryExprNode(AstBinaryExprNode *node) {
	++andOrCounter;
	auto calc_right = env.createBasicBlock("short_rhs_" + std::to_string(andOrCounter));
	auto result = env.createBasicBlock("short_result_" + std::to_string(andOrCounter));

	visit(node->lhs);
	auto calc_left = currentFunction->blocks.back();
	auto br = env.createCondBrStmt(remove_variable_pointer(exprResult[node->lhs]), nullptr, nullptr);
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

	auto br2result = env.createDirectBrStmt(result);
	add_stmt(br2result);

	add_block(result);
	auto phi = env.createPhiStmt(register_annoy_var(env.boolType, ".short."));
	if (node->op == "&&")
		phi->branches = {{calc_left, env.literal(false)}, {calc_right, rhs_res}};
	else
		phi->branches = {{calc_left, env.literal(true)}, {calc_right, rhs_res}};
	exprResult[node] = phi->res;
	add_phi(phi);
}


void IRBuilder::visitSingleExprNode(AstSingleExprNode *node) {
	if (node->op == "++" || node->op == "--") {// A++, A--, ++A, --A
		visit(node->expr);
		auto add = env.createArithmeticStmt(node->op == "++" ? "add" : "sub",
											nullptr,
											remove_variable_pointer(exprResult[node->expr]),
											env.literal(1));
		add->res = register_annoy_var(env.intType, ".arith.");
		add_stmt(add);
		if (node->right)
			exprResult[node] = add->lhs;
		else
			exprResult[node] = exprResult[node->expr];
		auto store = env.createStoreStmt(add->res, dynamic_cast<Var *>(exprResult[node->expr]));
		add_stmt(store);
	}
	else if (node->op == "+") {
		visit(node->expr);
		exprResult[node] = exprResult[node->expr];
	}
	else if (node->op == "-") {
		visit(node->expr);
		auto sub = env.createArithmeticStmt("sub",
											nullptr,
											env.literal(0),
											remove_variable_pointer(exprResult[node->expr]));
		sub->res = register_annoy_var(env.intType, ".arith.");
		add_stmt(sub);
		exprResult[node] = sub->res;
	}
	else if (node->op == "!") {
		visit(node->expr);
		auto xor_ = env.createArithmeticStmt("xor",
											 nullptr,
											 remove_variable_pointer(exprResult[node->expr]),
											 env.literal(true));
		xor_->res = register_annoy_var(env.boolType, ".arith.");
		add_stmt(xor_);
		exprResult[node] = xor_->res;
	}
	else if (node->op == "~") {
		visit(node->expr);
		auto xor_ = env.createArithmeticStmt("xor",
											 nullptr,
											 remove_variable_pointer(exprResult[node->expr]),
											 env.literal(-1));
		xor_->res = register_annoy_var(env.intType, ".arith.");
		add_stmt(xor_);
		exprResult[node] = xor_->res;
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

	auto call = env.createCallStmt(p->second, std::move(args));
	call->res = (call->func->type == env.voidType ? nullptr : register_annoy_var(call->func->type, ".call."));
	add_stmt(call);
	exprResult[node] = call->res;
}

IR::StringLiteralVar *IRBuilder::register_literal_str(const std::string &str) {
	return env.literal(str);
}

void IRBuilder::visitArrayAccessExprNode(AstArrayAccessExprNode *node) {
	visit(node->array);
	visit(node->index);
	auto array = remove_variable_pointer(exprResult[node->array]);
	auto index = remove_variable_pointer(exprResult[node->index]);
	auto type = toIRType(node->valueType);
	auto gep = env.createGetElementPtrStmt(type->to_string(),
										   register_annoy_ptr_var(type, ".arr."),
										   dynamic_cast<Var *>(array),
										   std::vector<Val *>{index});
	add_stmt(gep);
	exprResult[node] = gep->res;
}

void IRBuilder::visitTernaryExprNode(AstTernaryExprNode *node) {
	++ternaryCounter;
	auto true_expr = env.createBasicBlock("ternary_true_" + std::to_string(ternaryCounter));
	auto false_expr = env.createBasicBlock("ternary_false_" + std::to_string(ternaryCounter));
	auto end = env.createBasicBlock("ternary_end_" + std::to_string(ternaryCounter));

	visit(node->cond);
	auto br_cond = env.createCondBrStmt(remove_variable_pointer(exprResult[node->cond]), true_expr, false_expr);
	add_stmt(br_cond);

	auto br_end = env.createDirectBrStmt(end);

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
		auto phi = env.createPhiStmt(register_annoy_var(toIRType(node->valueType), ".ternary_res."),
									 std::map<BasicBlock *, Val *>{{from_true, true_res}, {from_false, false_res}});
		add_phi(phi);
		exprResult[node] = phi->res;
	}
}

void IRBuilder::create_init_global_var_function() {
	if (globalInitList.empty())
		return;

	auto first_sign = env.create_global_var(env.boolType, ".global_var_inited");
	module->globalVars.push_back(first_sign);

	auto first_sign_stmt = env.createGlobalStmt(first_sign, env.literal(false));
	module->variables.push_back(first_sign_stmt);

	auto func = env.createFunction(env.voidType, "init-global-var");
	func->blocks.emplace_back(env.createBasicBlock("entry"));
	currentFunction = func;

	auto init_block = env.createBasicBlock("init");
	auto end_block = env.createBasicBlock("end");

	auto is_first = env.createCondBrStmt(remove_variable_pointer(first_sign), end_block, init_block);
	add_stmt(is_first);

	add_block(init_block);
	auto set_sign = env.createStoreStmt(env.literal(true), first_sign);
	add_stmt(set_sign);
	for (auto &init: globalInitList) {
		visit(init.second);
		auto store = env.createStoreStmt(remove_variable_pointer(exprResult[init.second]), nullptr);
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
			auto call = env.createCallStmt(func);
			auto &stmts = f->blocks[0]->stmts;
			stmts.insert(stmts.begin(), call);
			break;
		}
	module->functions.push_back(func);
}

IR::Var *IRBuilder::TransformNewToFor(std::vector<IR::Val *> const &array_size, int total_dim, std::string const &base_typename, int dep) {
	const std::map<std::string, IR::Type *> type_map = {
			{"int", env.intType},
			{"bool", env.boolType},
			{"string", env.stringType},
			{"void", env.voidType}};
	if (dep == array_size.size()) {
		IR::Type *type = nullptr;
		if (dep < total_dim) type = env.ptrType;
		else if (auto p = type_map.find(base_typename); p != type_map.end())
			type = p->second;
		else if (auto q = name2class.find(base_typename); q != name2class.end())
			type = &q->second->type;
		else
			throw std::runtime_error(std::string("unknown type") + __FILE_NAME__);

		auto make_obj = env.createCallStmt(name2function["malloc"],
										   std::vector<Val *>{env.literal(type->size())},
										   register_annoy_var(env.ptrType, ".new_obj."));
		add_stmt(make_obj);

		if (type == env.stringType) {
			auto store = env.createStoreStmt(literalStr2var[empty_string], make_obj->res);
			add_stmt(store);
		}
		else if (type != env.ptrType && type != env.intType && type != env.boolType) {
			auto con = name2function.find(base_typename + "." + base_typename);
			if (con != name2function.end()) {
				auto call = env.createCallStmt(con->second, std::vector<Val *>{make_obj->res});
				add_stmt(call);
			}
		}
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
		auto make_array = env.createCallStmt(
				name2function[functionName],
				std::vector<Val *>{array_size[dep]},
				register_annoy_var(env.ptrType, ".new_array."));
		add_stmt(make_array);

		if (dep + 1 == array_size.size())
			return make_array->res;

		std::string label = "new_" + std::to_string(newCounter) + "_for_" + std::to_string(dep);
		auto cond_block = env.createBasicBlock(label + "_cond");
		auto body_block = env.createBasicBlock(label + "_body");
		auto end_block = env.createBasicBlock(label + "_end");

		auto counter = register_annoy_var(env.intType, ".new_for_idx.");
		auto increased_counter = register_annoy_var(env.intType, ".new_for_idx_inc.");

		auto br_cond = env.createDirectBrStmt(cond_block);
		add_stmt(br_cond);

		auto current_block = currentFunction->blocks.back();
		add_block(cond_block);
		auto phi = env.createPhiStmt(counter, std::map<BasicBlock *, Val *>{{current_block, env.literal(0)}});
		// phi->branches will be pushed later
		add_phi(phi);
		auto cmp = env.createIcmpStmt("slt", register_annoy_var(env.boolType, ".new_for_cmp."),
									  counter, array_size[dep]);
		add_stmt(cmp);
		auto br = env.createCondBrStmt(cmp->res, body_block, end_block);
		add_stmt(br);

		add_block(body_block);
		auto ptr = TransformNewToFor(array_size, total_dim, base_typename, dep + 1);
		auto gep = env.createGetElementPtrStmt(gepName,
											   register_annoy_var(env.ptrType, ".new_for_gep."),
											   make_array->res,
											   std::vector<Val *>{counter});
		add_stmt(gep);

		auto store = env.createStoreStmt(ptr, gep->res);
		add_stmt(store);

		auto inc = env.createArithmeticStmt("add", increased_counter, counter, env.literal(1));
		add_stmt(inc);

		add_stmt(br_cond);
		phi->branches[currentFunction->blocks.back()] = increased_counter;
		add_block(end_block);

		return make_array->res;
	}
}
