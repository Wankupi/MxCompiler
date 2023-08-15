#pragma once

#include "ASM/Register.h"
#include "RewriteLayer.h"

struct RegAllocator : public RewriteLayer {
	explicit RegAllocator(ASM::ValueAllocator *allocator) : regs(allocator) {}

private:
	void visitBinaryInst(ASM::BinaryInst *inst) override;
	void visitMulDivRemInst(ASM::MulDivRemInst *inst) override;
	void visitLuiInst(ASM::LuiInst *inst) override;
	void visitLiInst(ASM::LiInst *inst) override;
	void visitLaInst(ASM::LaInst *inst) override;
	void visitStoreInstBase(ASM::StoreInstBase *inst) override;
	void visitStoreOffset(ASM::StoreOffset *inst) override;
	void visitStoreSymbol(ASM::StoreSymbol *inst) override;
	void visitLoadInstBase(ASM::LoadInstBase *inst) override;
	void visitLoadOffset(ASM::LoadOffset *inst) override;
	void visitLoadSymbol(ASM::LoadSymbol *inst) override;
	void visitMoveInst(ASM::MoveInst *inst) override;
	void visitSltInst(ASM::SltInst *inst) override;
	void visitBranchInst(ASM::BranchInst *inst) override;

private:
	ASM::ValueAllocator *regs = nullptr;

private:
	std::map<ASM::VirtualReg *, ASM::StackVal *> reg2stack;

private:
	ASM::StackVal *vreg2stack(ASM::VirtualReg *reg);
	ASM::Reg *load_reg(ASM::VirtualReg *reg, ASM::PhysicalReg *phyReg);
	ASM::Reg *store_reg(ASM::VirtualReg *reg);
};