#include "NaiveRegAllocator.h"

using namespace ASM;

ASM::StackVal *NaiveRegAllocator::vreg2stack(ASM::VirtualReg *reg) {
	if (auto cur = reg2stack.find(reg); cur != reg2stack.end())
		return cur->second;
	auto obj = new ASM::StackVal{};
	reg2stack[reg] = obj;
	currentFunction->stack.push_back(obj);
	return obj;
}

ASM::Reg *NaiveRegAllocator::load_reg(ASM::VirtualReg *reg, ASM::PhysicalReg *phyReg) {
	auto load = new ASM::LoadOffset{};
	load->rd = phyReg;
	load->src = regs->get("sp");
	load->offset = vreg2stack(reg)->get_offset();
	add_inst(load);
	return phyReg;
}

ASM::Reg *NaiveRegAllocator::store_reg(ASM::VirtualReg *reg) {
	auto store = new ASM::StoreOffset{};
	store->val = regs->get("t0");
	store->dst = regs->get("sp");
	store->offset = vreg2stack(reg)->get_offset();
	add_inst(store);
	return regs->get("t0");
}

ASM::Reg *NaiveRegAllocator::get_src(ASM::Reg *reg) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(reg))
		return load_reg(v, regs->get("t1"));
	return reg;
}

ASM::Reg *NaiveRegAllocator::get_dst(ASM::Reg *reg) {
	if (auto v = dynamic_cast<ASM::VirtualReg *>(reg))
		return store_reg(v);
	return reg;
}

void NaiveRegAllocator::deal(Reg *&a, Reg *&b) {
	if (auto v = dynamic_cast<VirtualReg *>(a))
		a = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<VirtualReg *>(b))
		b = load_reg(v, regs->get("t2"));
}

void NaiveRegAllocator::deal(Reg *&reg, Val *&val) {
	if (auto v = dynamic_cast<VirtualReg *>(reg))
		reg = load_reg(v, regs->get("t1"));
	if (auto v = dynamic_cast<VirtualReg *>(val))
		val = load_reg(v, regs->get("t2"));
}
