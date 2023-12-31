#pragma once

#include "ASM/ASMBaseVisitor.h"
#include "ASM/Node.h"

namespace ASM {

struct RewriteLayer : public ASM::ASMBaseVisitor {
	virtual void work(ASM::Module *module) {
		visitModule(module);
	}
	virtual ~RewriteLayer() = default;

protected:
	void visitModule(ASM::Module *module) override {
		for (auto function: module->functions)
			visitFunction(function);
	}
	void visitFunction(ASM::Function *function) override {
		currentFunction = function;
		for (auto block: function->blocks)
			visitBlock(block);
		currentFunction = nullptr;
	}
	void visitBlock(ASM::Block *block) override {
		currentBlock = block;
		decltype(block->stmts) instructions;
		instructions.swap(block->stmts);
		for (auto inst: instructions)
			visit(inst);
		currentBlock = nullptr;
	}
	void visitInstruction(ASM::Instruction *inst) override { add_inst(inst); }
	void visitLuiInst(ASM::LuiInst *inst) override { add_inst(inst); }
	void visitLiInst(ASM::LiInst *inst) override { add_inst(inst); }
	void visitLaInst(ASM::LaInst *inst) override { add_inst(inst); }
	void visitSltInst(ASM::SltInst *inst) override { add_inst(inst); }
	void visitBinaryInst(ASM::BinaryInst *inst) override { add_inst(inst); }
	void visitMulDivRemInst(ASM::MulDivRemInst *inst) override { add_inst(inst); }
	void visitCallInst(ASM::CallInst *inst) override { add_inst(inst); }
	void visitMoveInst(ASM::MoveInst *inst) override { add_inst(inst); }
	void visitStoreInstBase(ASM::StoreInstBase *inst) override { add_inst(inst); }
	void visitStoreOffset(ASM::StoreOffset *inst) override { add_inst(inst); }
	void visitStoreSymbol(ASM::StoreSymbol *inst) override { add_inst(inst); }
	void visitLoadInstBase(ASM::LoadInstBase *inst) override { add_inst(inst); }
	void visitLoadOffset(ASM::LoadOffset *inst) override { add_inst(inst); }
	void visitLoadSymbol(ASM::LoadSymbol *inst) override { add_inst(inst); }
	void visitJumpInst(ASM::JumpInst *inst) override { add_inst(inst); }
	void visitBranchInst(ASM::BranchInst *inst) override { add_inst(inst); }
	void visitRetInst(ASM::RetInst *inst) override { add_inst(inst); }

protected:
	ASM::Function *currentFunction = nullptr;
	ASM::Block *currentBlock = nullptr;

protected:
	void add_inst(ASM::Instruction *inst) { currentBlock->stmts.push_back(inst); }
};

}// namespace ASM
