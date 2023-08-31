#include "IR/IRBaseVisitor.h"
#include "Node.h"

namespace IR {

struct RewriteLayer : public IRBaseVisitor {
	void visitModule(Module *node) override {
		for (auto &func: node->functions)
			visitFunction(func);
	}
	void visitFunction(Function *node) override {
		current_function = node;
		for (auto &block: node->blocks)
			visitBasicBlock(block);
		current_function = nullptr;
	}
	void visitBasicBlock(BasicBlock *node) override {
		current_block = node;
		decltype(node->phis) phis;
		phis.swap(node->phis);
		for (auto &phi: phis)
			visit(phi.second);
		decltype(node->stmts) stmts;
		stmts.swap(node->stmts);
		for (auto &stmt: stmts)
			visit(stmt);
		current_block = nullptr;
	}

	void visitAllocaStmt(AllocaStmt *node) override { add_stmt(node); }
	void visitStoreStmt(StoreStmt *node) override { add_stmt(node); }
	void visitLoadStmt(LoadStmt *node) override { add_stmt(node); }
	void visitArithmeticStmt(ArithmeticStmt *node) override { add_stmt(node); }
	void visitIcmpStmt(IcmpStmt *node) override { add_stmt(node); }
	void visitRetStmt(RetStmt *node) override { add_stmt(node); }
	void visitGetElementPtrStmt(GetElementPtrStmt *node) override { add_stmt(node); }
	void visitCallStmt(CallStmt *node) override { add_stmt(node); }
	void visitDirectBrStmt(DirectBrStmt *node) override { add_stmt(node); }
	void visitCondBrStmt(CondBrStmt *node) override { add_stmt(node); }
	void visitPhiStmt(PhiStmt *node) override { add_phi(node); }
	void visitUnreachableStmt(UnreachableStmt *node) override { add_stmt(node); }
	void visitGlobalStmt(GlobalStmt *node) override { add_stmt(node); }
	void visitGlobalStringStmt(GlobalStringStmt *node) override { add_stmt(node); }

protected:
	virtual void add_stmt(Stmt *stmt) {
		current_block->stmts.push_back(stmt);
	}
	virtual void add_phi(PhiStmt *phi) {
		current_block->phis[phi->res] = phi;
	}

protected:
	Function *current_function = nullptr;
	BasicBlock *current_block = nullptr;
};

};// namespace IR