#include "InstMaker.h"
#include <set>

void InstMake::visitModule(IR::Module *module) {
	if (!asmModule || !regs)
		throw std::runtime_error("InstMake: asmModule or regs is nullptr");

	for (auto g: module->variables)
		visitGlobalStmt(g);
	for (auto f: module->functions)
		visitFunction(f);
}

void InstMake::visitFunction(IR::Function *node) {
	if (node->blocks.empty()) return;
	auto func = new ASM::Function{};
	func->name = node->name;
	currentFunction = func;
	initFunctionParams(func, node);
	for (auto b: node->blocks) {
		auto block = new ASM::Block{".L-" + node->name + "-" + std::to_string(block2block.size())};
		block2block[b] = block;
		func->blocks.push_back(block);
	}
	for (auto b: node->blocks)
		visitBasicBlock(b);
	if (func->max_call_arg_size >= 0) {
		auto st = add_object_to_stack_front();
		st->offset = 0;
	}
	currentFunction = nullptr;
	asmModule->functions.push_back(func);
}

void InstMake::initFunctionParams(ASM::Function *func, IR::Function *node) {
	if (node->params.empty())
		return;
	auto init_block = new ASM::Block{".L-" + node->name + "-init"};
	func->blocks.push_back(init_block);
	for (int i = 0; i < 8 && i < node->params.size(); ++i) {
		auto p = getReg(node->paramsVar[i]);
		auto mv = new ASM::MoveInst{};
		mv->rs = regs->get(10 + i);
		mv->rd = p;
		init_block->stmts.push_back(mv);
	}
	for (int i = 8; i < node->params.size(); ++i) {
		auto obj = new ASM::StackVal{};
		func->params.push_back(obj);
		ptr2stack[node->paramsVar[i]] = obj;
		auto p = getReg(node->paramsVar[i]);
		auto load = new ASM::LoadInst{};
		load->rd = p;
		load->src = regs->get("sp");
		load->offset = obj->get_offset();
		init_block->stmts.push_back(load);
	}
}


void InstMake::visitBasicBlock(IR::BasicBlock *node) {
	if (!currentFunction)
		throw std::runtime_error("InstMake: currentFunction is nullptr");
	auto block = block2block[node];
	currentBlock = block;
	for (auto s: node->stmts)
		visit(s);
	currentBlock = nullptr;
}

void InstMake::visitAllocaStmt(IR::AllocaStmt *node) {
	auto obj = add_object_to_stack();
	ptr2stack[node->res] = obj;
	//	auto inst = new ASM::BinaryInst{};
	//	inst->op = "add";
	//	inst->rd = getReg(node->res);
	//	inst->rs1 = regs->get("sp");
	//	inst->rs2 = obj->get_offset();
	//	add_inst(inst);
	//	inst->comment = node->to_string();
}

void InstMake::visitStoreStmt(IR::StoreStmt *node) {
	auto inst = new ASM::StoreInst{};
	inst->val = getReg(node->value);
	if (auto g = dynamic_cast<IR::GlobalVar *>(node->pointer)) {
		auto gv = globalVar2globalVal[g];
		auto lui = new ASM::LuiInst{};
		lui->rd = regs->get("t6");
		lui->imm = gv->get_hi();
		add_inst(lui);
		inst->dst = regs->get("t6");
		inst->offset = gv->get_lo();
	}
	else if (auto cur = ptr2stack.find(node->pointer); cur != ptr2stack.end()) {
		inst->dst = regs->get("sp");
		inst->offset = cur->second->get_offset();
	}
	else {
		inst->dst = getReg(node->pointer);
		inst->offset = nullptr;
	}
	add_inst(inst);

	inst->comment = node->to_string();
}

void InstMake::visitLoadStmt(IR::LoadStmt *node) {
	auto inst = new ASM::LoadInst{};
	inst->rd = getReg(node->res);
	if (auto g = dynamic_cast<IR::GlobalVar *>(node->pointer)) {
		auto gv = globalVar2globalVal[g];
		auto lui = new ASM::LuiInst{};
		lui->rd = regs->get("t6");
		lui->imm = gv->get_hi();
		add_inst(lui);
		inst->src = regs->get("t6");
		inst->offset = gv->get_lo();
	}
	else if (auto cur = ptr2stack.find(node->pointer); cur != ptr2stack.end()) {
		inst->src = regs->get("sp");
		inst->offset = cur->second->get_offset();
	}
	else {
		inst->src = getReg(node->pointer);
		inst->offset = nullptr;
	}
	add_inst(inst);
}

void InstMake::visitArithmeticStmt(IR::ArithmeticStmt *node) {
	std::map<std::string, std::string> const op2inst_basic{
			{"add", "add"},
			{"sub", "sub"},
			{"and", "and"},
			{"or", "or"},
			{"xor", "xor"},
			{"shl", "sll"},
			{"ashr", "sra"}};
	std::map<std::string, std::string> const op2inst_mul{
			{"mul", "mul"},
			{"sdiv", "div"},
			{"srem", "rem"}};
	if (op2inst_basic.contains(node->cmd)) {
		auto inst = new ASM::BinaryInst{};
		inst->op = op2inst_basic.at(node->cmd);
		inst->rs1 = getReg(node->lhs);
		inst->rs2 = getVal(node->rhs);
		inst->rd = getReg(node->res);
		add_inst(inst);
	}
	else {
		auto inst = new ASM::MulDivRemInst{};
		inst->op = op2inst_mul.at(node->cmd);
		inst->rs1 = getReg(node->lhs);
		inst->rs2 = getReg(node->rhs);
		inst->rd = getReg(node->res);
		add_inst(inst);
	}
}

void InstMake::visitIcmpStmt(IR::IcmpStmt *node) {
	std::set<std::string> const need_swap = {"sle", "sgt"};
	std::set<std::string> const need_not = {"sle", "sge", "eq"};
	ASM::Reg *res = nullptr;
	if (node->cmd == "eq" || node->cmd == "ne") {
		auto xor_inst = new ASM::BinaryInst{};
		xor_inst->op = "xor";
		xor_inst->rs1 = getReg(node->lhs);
		xor_inst->rs2 = getVal(node->rhs);
		res = xor_inst->rd = getReg(node->res);
		add_inst(xor_inst);
	}
	else {
		auto slt = new ASM::SltInst{};
		auto lhs = node->lhs, rhs = node->rhs;
		if (need_swap.contains(node->cmd))
			std::swap(lhs, rhs);
		slt->rs1 = getReg(lhs);
		slt->rs2 = getVal(rhs);
		res = slt->rd = getReg(node->res);
		add_inst(slt);
	}
	if (need_not.contains(node->cmd)) {
		auto not_inst = new ASM::SltInst{};
		not_inst->rd = not_inst->rs1 = res;
		not_inst->rs2 = regs->get_imm(1);
		not_inst->isUnsigned = true;
		add_inst(not_inst);
	}
}

void InstMake::visitGetElementPtrStmt(IR::GetElementPtrStmt *node) {
	if (node->indices.size() > 2) throw std::runtime_error("InstMake(getElementPtr): <TODO: too many indices>");
	auto ptr = getReg(node->pointer);
	auto rd = getReg(node->res);
	bool firstTime = true;
	for (auto index: node->indices) {
		if (auto num = dynamic_cast<IR::LiteralInt *>(index); num && num->value == 0)
			continue;
		auto idx = getReg(index);
		if (node->typeName != "i1") {
			auto shl = new ASM::BinaryInst{};
			shl->op = "sll";
			shl->rs1 = idx;
			shl->rs2 = regs->get_imm(2);
			shl->rd = idx;
			add_inst(shl);
		}
		auto add = new ASM::BinaryInst{};
		add->op = "add";
		add->rs1 = firstTime ? ptr : rd;
		add->rs2 = idx;
		add->rd = rd;
		add_inst(add);
		firstTime = false;

		add->comment = node->to_string();
	}
	if (firstTime) {
		auto mov = new ASM::MoveInst{};
		mov->rs = ptr;
		mov->rd = rd;
		add_inst(mov);

		mov->comment = node->to_string();
	}
}


void InstMake::visitCallStmt(IR::CallStmt *node) {
	currentFunction->max_call_arg_size = std::max(currentFunction->max_call_arg_size, int(node->args.size()));
	for (int i = 8; i < node->args.size(); ++i) {
		auto store = new ASM::StoreInst{};
		store->val = getReg(node->args[i]);
		store->dst = regs->get("sp");
		store->offset = regs->get_imm((i - 8) * 4);
		add_inst(store);
	}
	for (int i = 0; i < 8 && i < node->args.size(); ++i)
		toExpectReg(node->args[i], regs->get(10 + i));

	auto call = new ASM::CallInst{};
	call->funcName = node->func->name;
	add_inst(call);

	if (node->res) {
		auto mov = new ASM::MoveInst{};
		mov->rs = regs->get("a0");
		mov->rd = getReg(node->res);
		add_inst(mov);
	}
}

void InstMake::visitDirectBrStmt(IR::DirectBrStmt *node) {
	auto br = new ASM::JumpInst{block2block[node->block]};
	add_inst(br);
}

void InstMake::visitCondBrStmt(IR::CondBrStmt *node) {
	auto br = new ASM::BranchInst{};
	br->op = "ne";
	br->rs1 = getReg(node->cond);
	br->rs2 = regs->get("zero");
	br->dst = block2block[node->trueBlock];
	add_inst(br);
	auto j = new ASM::JumpInst{block2block[node->falseBlock]};
	add_inst(j);
}

void InstMake::visitRetStmt(IR::RetStmt *node) {
	if (node->value)
		toExpectReg(node->value, regs->get("a0"));
	add_inst(new ASM::RetInst{currentFunction});
}

void InstMake::visitGlobalStmt(IR::GlobalStmt *node) {
	auto val = add_global_val(node->var);
	auto var = new ASM::GlobalVarInst{};
	var->globalVal = val;
	var->initVal = getImm(node->value);
	asmModule->globalVars.push_back(var);
}

void InstMake::add_inst(ASM::Instruction *inst) {
	currentBlock->stmts.push_back(inst);
}

ASM::Reg *InstMake::getReg(IR::Val *val) {
	auto p = val2reg.find(val);
	if (p != val2reg.end()) {
		return p->second;
	}
	auto reg = regs->registerVirtualReg();
	val2reg[val] = reg;
	if (dynamic_cast<IR::Literal *>(val)) {
		ASM::ImmI32 *imm = nullptr;
		if (auto num = dynamic_cast<IR::LiteralInt *>(val))
			imm = regs->get_imm(num->value);
		else if (auto cond = dynamic_cast<IR::LiteralBool *>(val))
			imm = regs->get_imm(cond->value ? 1 : 0);
		else if (dynamic_cast<IR::LiteralNull *>(val) != nullptr)
			imm = regs->get_imm(0);
		auto li = new ASM::LiInst{reg, imm};
		add_inst(li);
	}
	return reg;
}

ASM::Val *InstMake::getVal(IR::Val *val) {
	if (auto num = dynamic_cast<IR::LiteralInt *>(val))
		return regs->get_imm(num->value);
	else if (auto cond = dynamic_cast<IR::LiteralBool *>(val))
		return regs->get_imm(cond->value ? 1 : 0);
	else if (dynamic_cast<IR::LiteralNull *>(val))
		return regs->get_imm(0);
	else
		return getReg(val);
}

ASM::PhysicalReg *InstMake::toExpectReg(IR::Val *val, ASM::PhysicalReg *expected) {
	auto v = getVal(val);
	if (auto imm = dynamic_cast<ASM::ImmI32 *>(v)) {
		auto li = new ASM::LiInst{expected, imm};
		add_inst(li);
	}
	else {
		auto mv = new ASM::MoveInst{};
		mv->rs = dynamic_cast<ASM::Reg *>(v);
		mv->rd = expected;
		add_inst(mv);
	}
	return expected;
}

ASM::StackVal *InstMake::add_object_to_stack() {
	auto obj = new ASM::StackVal{};
	obj->offset = -114514;
	currentFunction->stack.push_back(obj);
	return obj;
}

ASM::StackVal *InstMake::add_object_to_stack_front() {
	auto obj = new ASM::StackVal{};
	obj->offset = -114514;
	currentFunction->stack.push_front(obj);
	return obj;
}

ASM::GlobalVal *InstMake::add_global_val(IR::GlobalVar *ir_var) {
	auto val = new ASM::GlobalVal{ir_var->name};
	globalVar2globalVal[ir_var] = val;
	return val;
}

ASM::Imm *InstMake::getImm(IR::Val *val) {
	if (auto num = dynamic_cast<IR::LiteralInt *>(val))
		return regs->get_imm(num->value);
	else if (auto cond = dynamic_cast<IR::LiteralBool *>(val))
		return regs->get_imm(cond->value ? 1 : 0);
	else if (dynamic_cast<IR::LiteralNull *>(val))
		return regs->get_imm(0);
	else
		return nullptr;// TODO: global static string ptr
}
