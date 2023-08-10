#pragma once

#include "ASM/Node.h"
#include "ASM/Register.h"
#include "IR/Node.h"

class InstMake {
public:
	InstMake(ASM::Module *asmModule, ASM::Registers *registers) : asmModule(asmModule), regs(registers) {}

	void visit(IR::Module *node);

private:
	void visitFunction(IR::Function *node);
	//	void visitClass(IR::Class *node);
	void visitBasicBlock(IR::BasicBlock *node);
	void visitStmt(IR::Stmt *node);


private:
	ASM::Module *asmModule = nullptr;
	ASM::Registers *regs = nullptr;

private:
	ASM::Function *currentFunction = nullptr;
	int labelCount = 0;
};
