#include "InstMaker.h"

void InstMake::visitModule(IR::Module *module) {
	if (!asmModule || !regs)
		throw std::runtime_error("InstMake: asmModule or regs is nullptr");

	for (auto f: module->functions)
		visitFunction(f);
}

void InstMake::visitFunction(IR::Function *node) {
	if (node->blocks.empty()) return;
	auto func = new ASM::Function{};
	func->name = node->name;
	currentFunction = func;

	for (auto b: node->blocks)
		blockLabel[b] = ".L-" + b->label;
	for (auto b: node->blocks)
		visitBasicBlock(b);

	currentFunction = nullptr;
	asmModule->functions.push_back(func);
}

void InstMake::visitBasicBlock(IR::BasicBlock *node) {
	if (!currentFunction)
		throw std::runtime_error("InstMake: currentFunction is nullptr");
	auto block = new ASM::Block{};
	block->label = blockLabel[node];
	currentFunction->blocks.push_back(block);
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
}

void InstMake::visitStoreStmt(IR::StoreStmt *node) {
	auto inst = new ASM::StoreInst{};
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
}

void InstMake::visitLoadStmt(IR::LoadStmt *node) {
	auto inst = new ASM::LoadInst{};
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
	std::map<std::string, std::string> const op2inst = {
			{"add", "add"},
			{"sub", "sub"},
			{"mul", "mul"},
			{"sdiv", "div"},
			{"srem", "rem"},
			{"and", "and"},
			{"or", "or"},
			{"xor", "xor"},
			{"shl", "sll"},
			{"ashr", "sra"}};
	auto inst = new ASM::BinaryInst{};
	inst->op = op2inst.at(node->cmd);
	inst->rs1 = getReg(node->lhs);
	inst->rs2 = getReg(node->rhs);
	inst->rd = getReg(node->res);
	add_inst(inst);
}

void InstMake::visitIcmpStmt(IR::IcmpStmt *node) {
	auto slt = new ASM::SltInst{};
	slt->rs1 = getReg(node->lhs);
	slt->rs2 = getReg(node->rhs);
	slt->rd = getReg(node->res);
	add_inst(slt);
}

void InstMake::visitCallStmt(IR::CallStmt *node) {
	auto call = new ASM::CallInst{};
	call->funcName = node->func->name;
	add_inst(call);

	auto mov = new ASM::MoveInst{};
	mov->rs = regs->get("a0");
	mov->rd = getReg(node->res);
	add_inst(mov);
}

void InstMake::add_inst(ASM::Instruction *inst) {
	currentBlock->stmts.push_back(inst);
}

ASM::Reg *InstMake::getReg(IR::Val *val) {
	auto p = val2reg.find(val);
	if (p != val2reg.end())
		return p->second;
	auto reg = regs->registerVirtualReg();
	val2reg[val] = reg;
	return reg;
}

ASM::StackVal *InstMake::add_object_to_stack() {
	auto obj = new ASM::StackVal{};
	obj->offset = -1;
	currentFunction->stack.push_back(obj);
	return obj;
}
