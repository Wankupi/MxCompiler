#include "InstMaker.h"
#include <set>

void InstMake::visitModule(IR::Module *module) {
	if (!asmModule || !regs)
		throw std::runtime_error("InstMake: asmModule or regs is nullptr");

	for (auto s: module->stringLiterals)
		visitGlobalStringStmt(s);
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
	val2reg.clear();
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
		auto load = new ASM::LoadOffset{};
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
	currentIRBlock = node;
	for (auto s: node->stmts)
		visit(s);
	currentIRBlock = nullptr;
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
	if (auto g = dynamic_cast<IR::GlobalVar *>(node->pointer)) {
		auto gv = globalVar2globalVal[g];
		auto st = new ASM::StoreSymbol{};
		st->size = node->value->type->size();
		st->val = getReg(node->value);
		st->symbol = gv->get_pos();
		st->rd = regs->get("t6");
		add_inst(st);
		st->comment = node->to_string();
		return;
	}
	auto inst = new ASM::StoreOffset{};
	inst->size = node->value->type->size();
	//	if (inst->size != 1 && inst->size != 4)
	//		throw std::runtime_error("InstMake(store): <TODO: size not supported>" + node->to_string());
	inst->val = getReg(node->value);
	if (auto cur = ptr2stack.find(node->pointer); cur != ptr2stack.end()) {
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
	if (auto g = dynamic_cast<IR::GlobalVar *>(node->pointer)) {
		auto gv = globalVar2globalVal[g];
		auto ld = new ASM::LoadSymbol{};
		ld->size = node->res->type->size();
		ld->rd = getReg(node->res);
		ld->symbol = gv->get_pos();
		add_inst(ld);
		ld->comment = node->to_string();
		return;
	}
	//	if (inst->size != 1 && inst->size != 4)
	//		throw std::runtime_error("InstMake(load): <TODO: size not supported>" + node->to_string());
	auto inst = new ASM::LoadOffset{};
	inst->size = node->res->type->size();
	inst->rd = getReg(node->res);
	if (auto cur = ptr2stack.find(node->pointer); cur != ptr2stack.end()) {
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
		if (node->cmd == "sub" && dynamic_cast<ASM::ImmI32 *>(inst->rs2)) {
			inst->op = "add";
			inst->rs2 = regs->get_imm(-dynamic_cast<ASM::ImmI32 *>(inst->rs2)->val);
		}
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
			shl->rd = regs->registerVirtualReg();
			add_inst(shl);
			idx = shl->rd;
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
		auto store = new ASM::StoreOffset{};
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
	auto st = block_phi_val(node->block, currentIRBlock);
	if (st.second)
		toExpectReg(st.second, getReg(st.first));
	auto br = new ASM::JumpInst{block2block[node->block]};
	add_inst(br);
}

void InstMake::visitCondBrStmt(IR::CondBrStmt *node) {
	auto trueBlock = node->trueBlock, falseBlock = node->falseBlock;
	auto st_true = block_phi_val(trueBlock, currentIRBlock);
	auto st_false = block_phi_val(falseBlock, currentIRBlock);
	std::string cmd = "ne";
	if (st_true.second && !st_false.second) {
		cmd = "eq";
		std::swap(st_true, st_false);
		std::swap(trueBlock, falseBlock);
	}
	if (st_true.second)
		toExpectReg(st_true.second, getReg(st_true.first));
	auto br = new ASM::BranchInst{};
	br->op = cmd;
	br->rs1 = getReg(node->cond);
	br->rs2 = regs->get("zero");
	br->dst = block2block[trueBlock];
	add_inst(br);
	if (st_false.second)
		toExpectReg(st_false.second, getReg(st_false.first));
	auto j = new ASM::JumpInst{block2block[falseBlock]};
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

void InstMake::visitGlobalStringStmt(IR::GlobalStringStmt *node) {
	// TODO: use add_global_val directly
	auto val = new ASM::GlobalVal{'"' + node->var->name + '"'};
	globalVar2globalVal[node->var] = val;
	auto var = new ASM::LiteralStringInst{};
	var->globalVal = val;
	var->val = node->var->value;
	asmModule->literalStrings.push_back(var);
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
		return reg;
	}
	else if (auto s = dynamic_cast<IR::StringLiteralVar *>(val)) {
		auto la = new ASM::LaInst{reg, globalVar2globalVal[s]};
		add_inst(la);
	}
	val2reg[val] = reg;
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

ASM::Reg *InstMake::toExpectReg(IR::Val *val, ASM::Reg *expected) {
	if (auto s = dynamic_cast<IR::StringLiteralVar *>(val)) {
		auto lui = new ASM::LuiInst{};
		lui->rd = expected;
		lui->imm = globalVar2globalVal[s]->get_hi();
		add_inst(lui);
		auto add = new ASM::BinaryInst{};
		add->op = "add";
		add->rs1 = expected;
		add->rs2 = globalVar2globalVal[s]->get_lo();
		add->rd = expected;
		add_inst(add);
		return expected;
		//		auto la = new ASM::LaInst{expected, globalVar2globalVal[s]};
		//		add_inst(la);
		//		return expected;
	}
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
	else if (auto var = dynamic_cast<IR::StringLiteralVar *>(val))
		return globalVar2globalVal[var]->get_pos();
	else
		throw std::runtime_error("InstMake: getImm: unknown val type: " + val->get_name());
}

std::pair<IR::Var *, IR::Val *> InstMake::block_phi_val(IR::BasicBlock *dst, IR::BasicBlock *src) {
	if (dst->stmts.empty()) return {};
	auto phi = dynamic_cast<IR::PhiStmt *>(dst->stmts.front());
	if (!phi) return {};
	auto p = phi->branches.find(src);
	if (p == phi->branches.end()) return {};
	return {phi->res, p->second};
}
