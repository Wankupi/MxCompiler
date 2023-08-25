#pragma once
#include "ASM/ASMBaseVisitor.h"
#include "ASM/Register.h"


class GraphColorRegAllocator {
private:
	ASM::ValueAllocator *regs;

public:
	explicit GraphColorRegAllocator(ASM::ValueAllocator *regs) : regs(regs){};
	void work(ASM::Module *module);
};
