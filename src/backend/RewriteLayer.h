#pragma once

#include "ASM/ASMBaseVisitor.h"
#include "ASM/Node.h"

struct RewriteLayer : public ASM::ASMBaseVisitor {
	virtual void work(ASM::Module *module) {
		visitModule(module);
	}
	virtual ~RewriteLayer() = default;

private:
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
	void visitLiInst(ASM::LiInst *inst) override { add_inst(inst); }
	void visitSltInst(ASM::SltInst *inst) override { add_inst(inst); }
	void visitBinaryInst(ASM::BinaryInst *inst) override { add_inst(inst); }
	void visitCallInst(ASM::CallInst *inst) override { add_inst(inst); }
	void visitMoveInst(ASM::MoveInst *inst) override { add_inst(inst); }
	void visitStoreInst(ASM::StoreInst *inst) override { add_inst(inst); }
	void visitLoadInst(ASM::LoadInst *inst) override { add_inst(inst); }
	void visitJumpInst(ASM::JumpInst *inst) override { add_inst(inst); }
	void visitBranchInst(ASM::BranchInst *inst) override { add_inst(inst); }
	void visitRetInst(ASM::RetInst *inst) override { add_inst(inst); }

protected:
	ASM::Function *currentFunction = nullptr;
	ASM::Block *currentBlock = nullptr;

protected:
	void add_inst(ASM::Instruction *inst) { currentBlock->stmts.push_back(inst); }
};
