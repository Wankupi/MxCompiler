#pragma once

#include "ASM/Node.h"
#include "ASM/Register.h"
#include "IR/Node.h"

class InstMake : public IR::IRBaseVisitor {
public:
	InstMake(ASM::Module *asmModule, ASM::ValueAllocator *registers) : asmModule(asmModule), regs(registers) {}

private:
	void visitModule(IR::Module *node) override;
	void visitFunction(IR::Function *node) override;
	//	void visitClass(IR::Class *node);
	void visitBasicBlock(IR::BasicBlock *node) override;

	void visitAllocaStmt(IR::AllocaStmt *node) override;
	void visitStoreStmt(IR::StoreStmt *node) override;
	void visitLoadStmt(IR::LoadStmt *node) override;
	void visitArithmeticStmt(IR::ArithmeticStmt *node) override;
	void visitIcmpStmt(IR::IcmpStmt *node) override;
	void visitGetElementPtrStmt(IR::GetElementPtrStmt *node) override;
	void visitCallStmt(IR::CallStmt *node) override;

	void visitDirectBrStmt(IR::DirectBrStmt *node) override;
	void visitCondBrStmt(IR::CondBrStmt *node) override;

	void visitRetStmt(IR::RetStmt *node) override;

private:
	ASM::Module *asmModule = nullptr;
	ASM::ValueAllocator *regs = nullptr;

private:
	ASM::Function *currentFunction = nullptr;
	ASM::Block *currentBlock = nullptr;
	std::map<IR::BasicBlock *, ASM::Block *> block2block;
	std::map<IR::Val *, ASM::Reg *> val2reg;
	std::map<IR::Var *, ASM::StackVal *> ptr2stack;

private:
	ASM::Reg *getReg(IR::Val *val);
	ASM::Val *getVal(IR::Val *val);
	ASM::PhysicalReg *toExpectReg(IR::Val *val, ASM::PhysicalReg *expected);
	void add_inst(ASM::Instruction *inst);
	ASM::StackVal *add_object_to_stack();
	ASM::StackVal *add_object_to_stack_front();
};
