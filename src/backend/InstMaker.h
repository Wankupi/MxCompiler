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
	void initFunctionParams(ASM::Function *func, IR::Function *node);
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

	void visitGlobalStmt(IR::GlobalStmt *node) override;
	void visitGlobalStringStmt(IR::GlobalStringStmt *node) override;

private:
	ASM::Module *asmModule = nullptr;
	ASM::ValueAllocator *regs = nullptr;

private:
	ASM::Function *currentFunction = nullptr;
	ASM::Block *currentBlock = nullptr;
	IR::BasicBlock *currentIRBlock = nullptr;
	std::map<IR::BasicBlock *, ASM::Block *> block2block;
	std::map<IR::Val *, ASM::Reg *> val2reg;
	std::map<IR::Var *, ASM::StackVal *> ptr2stack;
	// TODO: use global var to unify global val and string literal val
	std::map<IR::Var *, ASM::GlobalVal *> globalVar2globalVal;
	std::map<ASM::PhysicalReg *, ASM::VirtualReg *> calleeSaveTo;
	int middle_block_count = 0;

private:
	ASM::Reg *getReg(IR::Val *val);
	ASM::Val *getVal(IR::Val *val);
	ASM::Imm *getImm(IR::Val *val);
	ASM::Val *tryGetImm(IR::Val *val);
	ASM::Reg *toExpectReg(IR::Val *val, ASM::Reg *expected);
	void add_inst(ASM::Instruction *inst);
	ASM::StackVal *add_object_to_stack();
	ASM::StackVal *add_object_to_stack_front();
	ASM::GlobalVal *add_global_val(IR::Var *ir_var);
	static std::vector<std::pair<IR::Var *, IR::Val *>> block_phi_val(IR::BasicBlock *dst, IR::BasicBlock *src);

	void phi2mv(const std::vector<std::pair<IR::Var *, IR::Val *>> &phis);
};
