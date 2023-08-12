#include "RegAllocator.h"

ASM::StackVal *RegAllocator::vreg2stack(ASM::VirtualReg *reg) {
	if (auto cur = reg2stack.find(reg); cur != reg2stack.end())
		return cur->second;
	auto obj = new ASM::StackVal{};
	reg2stack[reg] = obj;
	currentFunction->stack.push_back(obj);
	return obj;
}

ASM::Reg *RegAllocator::load_reg(ASM::VirtualReg *reg, ASM::PhysicalReg *phyReg) {
	auto load = new ASM::LoadInst{};
	load->rd = phyReg;
	load->src = regs->get("sp");
	load->offset = vreg2stack(reg)->get_offset();
	add_inst(load);
	return phyReg;
}

ASM::Reg *RegAllocator::store_reg(ASM::VirtualReg *reg) {
	auto store = new ASM::StoreInst{};
	store->val = regs->get("t0");
	store->dst = regs->get("sp");
	store->offset = vreg2stack(reg)->get_offset();
	add_inst(store);
	return regs->get("t0");
}

void RegAllocator::visitBinaryInst(ASM::BinaryInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs1))
		inst->rs1 = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs2))
		inst->rs2 = load_reg(v, regs->get("t2"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitLiInst(ASM::LiInst *inst) {
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitStoreInst(ASM::StoreInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->val))
		inst->val = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->dst))
		inst->dst = load_reg(v, regs->get("t2"));
	add_inst(inst);
}

void RegAllocator::visitLoadInst(ASM::LoadInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->src))
		inst->src = load_reg(v, regs->get("t1"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitMoveInst(ASM::MoveInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs))
		inst->rs = load_reg(v, regs->get("t1"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}
