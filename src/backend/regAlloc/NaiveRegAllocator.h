#pragma once

#include "ASM/Register.h"
#include "ASM/RewriteLayer.h"
#include "BaseRegAllocator.h"

namespace ASM {
struct NaiveRegAllocator : public ASM::BaseRegAllocator {
	explicit NaiveRegAllocator(ASM::ValueAllocator *allocator) : regs(allocator) {}

private:
	ASM::ValueAllocator *regs = nullptr;

private:
	std::map<ASM::VirtualReg *, ASM::StackVal *> reg2stack;

private:
	ASM::StackVal *vreg2stack(ASM::VirtualReg *reg);
	ASM::Reg *load_reg(ASM::VirtualReg *reg, ASM::PhysicalReg *phyReg);
	ASM::Reg *store_reg(ASM::VirtualReg *reg);

	ASM::Reg *get_src(ASM::Reg *reg) override;
	ASM::Reg *get_dst(ASM::Reg *reg) override;
	void deal(ASM::Reg *&a, ASM::Reg *&b) override;
	void deal(ASM::Reg *&reg, ASM::Val *&val) override;
};

}// namespace ASM
