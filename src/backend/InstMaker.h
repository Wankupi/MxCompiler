#pragma once

#include "ASM/Node.h"
#include "ASM/Register.h"
#include "IR/Node.h"

class InstMake : public IR::IRBaseVisitor {
public:
	InstMake(ASM::Module *asmModule, ASM::Registers *registers) : asmModule(asmModule), regs(registers) {}

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
	void visitCallStmt(IR::CallStmt *node) override;

private:
	ASM::Module *asmModule = nullptr;
	ASM::Registers *regs = nullptr;

private:
	ASM::Function *currentFunction = nullptr;
	ASM::Block *currentBlock = nullptr;
	std::map<IR::BasicBlock *, std::string> blockLabel;
	std::map<IR::Val *, ASM::Reg *> val2reg;
	std::map<IR::Var *, ASM::StackVal *> ptr2stack;

private:
	ASM::Reg *getReg(IR::Val *val);
	void add_inst(ASM::Instruction *inst);
	ASM::StackVal *add_object_to_stack();
};
