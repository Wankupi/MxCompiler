#pragma once

#include "Declaration.h"

namespace IR {

struct IRBaseVisitor {
	virtual void visit(IRNode *node) { node->accept(this); }
	virtual void visitClass(Class *node) {}
	virtual void visitBasicBlock(BasicBlock *node) {}
	virtual void visitFunction(Function *node) {}
	virtual void visitAllocaStmt(AllocaStmt *node) {}
	virtual void visitStoreStmt(StoreStmt *node) {}
	virtual void visitLoadStmt(LoadStmt *node) {}
	virtual void visitArithmeticStmt(ArithmeticStmt *node) {}
	virtual void visitIcmpStmt(IcmpStmt *node) {}
	virtual void visitRetStmt(RetStmt *node) {}
	virtual void visitGetElementPtrStmt(GetElementPtrStmt *node) {}
	virtual void visitCallStmt(CallStmt *node) {}
	virtual void visitDirectBrStmt(DirectBrStmt *node) {}
	virtual void visitCondBrStmt(CondBrStmt *node) {}
	virtual void visitPhiStmt(PhiStmt *node) {}
	virtual void visitUnreachableStmt(UnreachableStmt *node) {}
	virtual void visitGlobalStmt(GlobalStmt *node) {}
	virtual void visitGlobalStringStmt(GlobalStringStmt *node) {}
	virtual void visitModule(Module *node) {}
};

}// namespace IR
