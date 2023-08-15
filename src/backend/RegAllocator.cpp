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
	auto load = new ASM::LoadOffset{};
	load->rd = phyReg;
	load->src = regs->get("sp");
	load->offset = vreg2stack(reg)->get_offset();
	add_inst(load);
	return phyReg;
}

ASM::Reg *RegAllocator::store_reg(ASM::VirtualReg *reg) {
	auto store = new ASM::StoreOffset{};
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

void RegAllocator::visitMulDivRemInst(ASM::MulDivRemInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs1))
		inst->rs1 = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs2))
		inst->rs2 = load_reg(v, regs->get("t2"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitLuiInst(ASM::LuiInst *inst) {
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitLiInst(ASM::LiInst *inst) {
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitLaInst(ASM::LaInst *inst) {
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitStoreInstBase(ASM::StoreInstBase *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->val))
		inst->val = load_reg(v, regs->get("t1"));
	add_inst(inst);
}

void RegAllocator::visitStoreOffset(ASM::StoreOffset *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->val))
		inst->val = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->dst))
		inst->dst = load_reg(v, regs->get("t2"));
	add_inst(inst);
}

void RegAllocator::visitStoreSymbol(ASM::StoreSymbol *inst) {
	visitStoreInstBase(inst);
}

void RegAllocator::visitLoadInstBase(ASM::LoadInstBase *inst) {
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitLoadOffset(ASM::LoadOffset *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->src))
		inst->src = load_reg(v, regs->get("t1"));
	visitLoadInstBase(inst);
}

void RegAllocator::visitLoadSymbol(ASM::LoadSymbol *inst) {
	visitLoadInstBase(inst);
}

void RegAllocator::visitMoveInst(ASM::MoveInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs))
		inst->rs = load_reg(v, regs->get("t1"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitSltInst(ASM::SltInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs1))
		inst->rs1 = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs2))
		inst->rs2 = load_reg(v, regs->get("t2"));
	add_inst(inst);
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rd))
		inst->rd = store_reg(v);
}

void RegAllocator::visitBranchInst(ASM::BranchInst *inst) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs1))
		inst->rs1 = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<ASM::VirtualReg *>(inst->rs2))
		inst->rs2 = load_reg(v, regs->get("t2"));
	add_inst(inst);
}
