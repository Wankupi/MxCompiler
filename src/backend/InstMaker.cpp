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

	for (auto b: node->blocks) {
		auto block = new ASM::Block{".L-" + std::to_string(block2block.size())};
		block2block[b] = block;
		currentFunction->blocks.push_back(block);
	}
	for (auto b: node->blocks)
		visitBasicBlock(b);

	currentFunction = nullptr;
	asmModule->functions.push_back(func);
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
	auto call = new ASM::CallInst{};
	call->funcName = node->func->name;
	add_inst(call);

	auto mov = new ASM::MoveInst{};
	mov->rs = regs->get("a0");
	mov->rd = getReg(node->res);
	add_inst(mov);
}

void InstMake::visitDirectBrStmt(IR::DirectBrStmt *node) {
	auto br = new ASM::JumpInst{block2block[node->block]};
	add_inst(br);
}

void InstMake::visitCondBrStmt(IR::CondBrStmt *node) {
	auto br = new ASM::BranchInst{};
	br->op = "neq";
	br->rs1 = getReg(node->cond);
	br->rs2 = regs->get("zero");
	br->dst = block2block[node->trueBlock];
	add_inst(br);
	auto j = new ASM::JumpInst{block2block[node->falseBlock]};
	add_inst(j);
}

void InstMake::visitRetStmt(IR::RetStmt *node) {
	if (node->value) {
		auto mov = new ASM::MoveInst{};
		mov->rs = getReg(node->value);
		mov->rd = regs->get("a0");
		add_inst(mov);
	}
	add_inst(new ASM::RetInst{});
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
		ASM::Imm *imm = nullptr;
		if (auto num = dynamic_cast<IR::LiteralInt *>(val))
			imm = regs->get_imm(num->value);
		else if (auto cond = dynamic_cast<IR::LiteralBool *>(val))
			imm = regs->get_imm(cond->value ? 1 : 0);
		else if (auto n = dynamic_cast<IR::LiteralNull *>(val))
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
	else if (auto n = dynamic_cast<IR::LiteralNull *>(val))
		return regs->get_imm(0);
	else
		return getReg(val);
}


ASM::StackVal *InstMake::add_object_to_stack() {
	auto obj = new ASM::StackVal{};
	obj->offset = -1;
	currentFunction->stack.push_back(obj);
	return obj;
}
