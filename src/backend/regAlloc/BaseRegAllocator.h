#pragma once
#include "ASM/Node.h"
#include "ASM/RewriteLayer.h"

namespace ASM {

struct BaseRegAllocator : public ASM::RewriteLayer {
protected:
	virtual Reg *get_src(Reg *reg) = 0;
	virtual Reg *get_dst(Reg *reg) = 0;

	virtual void deal(Reg *&a, Reg *&b) = 0;
	virtual void deal(Reg *&reg, Val *&val) = 0;

protected:
	void visitBinaryInst(ASM::BinaryInst *inst) override {
		deal(inst->rs1, inst->rs2);
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitMulDivRemInst(ASM::MulDivRemInst *inst) override {
		deal(inst->rs1, inst->rs2);
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitLuiInst(ASM::LuiInst *inst) override {
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitLiInst(ASM::LiInst *inst) override {
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitLaInst(ASM::LaInst *inst) override {
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitStoreInstBase(ASM::StoreInstBase *inst) override {
		inst->val = get_src(inst->val);
		add_inst(inst);
	}
	void visitStoreOffset(ASM::StoreOffset *inst) override {
		deal(inst->val, inst->dst);
		// do not deal with offset
		add_inst(inst);
	}
	void visitStoreSymbol(ASM::StoreSymbol *inst) override {
		visitStoreInstBase(inst);
	}
	void visitLoadInstBase(ASM::LoadInstBase *inst) override {
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitLoadOffset(ASM::LoadOffset *inst) override {
		inst->src = get_src(inst->src);
		// do not deal with offset
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitLoadSymbol(ASM::LoadSymbol *inst) override {
		visitLoadInstBase(inst);
	}
	void visitMoveInst(ASM::MoveInst *inst) override {
		inst->rs = get_src(inst->rs);
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitSltInst(ASM::SltInst *inst) override {
		deal(inst->rs1, inst->rs2);
		add_inst(inst);
		inst->rd = get_dst(inst->rd);
	}
	void visitBranchInst(ASM::BranchInst *inst) override {
		deal(inst->rs1, inst->rs2);
		add_inst(inst);
	}
};

}// namespace ASM
