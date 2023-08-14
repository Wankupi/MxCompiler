#pragma once

namespace ASM {

struct ASMBaseVisitor;

struct Node {
	virtual void print(std::ostream &os) const = 0;
	virtual void accept(ASMBaseVisitor *visitor) = 0;
	virtual ~Node() = default;
};

struct Instruction;
struct Block;
struct Function;
struct GlobalVarInst;
struct LiteralStringInst;
struct Module;
struct LuiInst;
struct LiInst;
struct LaInst;
struct SltInst;
struct BinaryInst;
struct MulDivRemInst;
struct CallInst;
struct MoveInst;
struct StoreInst;
struct LoadInst;
struct JumpInst;
struct BranchInst;
struct RetInst;

struct ASMBaseVisitor {
	virtual void visit(Node *node) { node->accept(this); }
	virtual void visitModule(Module *module) {}
	virtual void visitFunction(Function *function) {}
	virtual void visitGlobalVarInst(GlobalVarInst *globalVarInst) {}
	virtual void visitBlock(Block *block) {}
	virtual void visitInstruction(Instruction *inst) {}
	virtual void visitLuiInst(LuiInst *inst) {}
	virtual void visitLiInst(LiInst *inst) {}
	virtual void visitLaInst(LaInst *inst) {}
	virtual void visitSltInst(SltInst *inst) {}
	virtual void visitBinaryInst(BinaryInst *inst) {}
	virtual void visitMulDivRemInst(MulDivRemInst *inst) {}
	virtual void visitCallInst(CallInst *inst) {}
	virtual void visitMoveInst(MoveInst *inst) {}
	virtual void visitStoreInst(StoreInst *inst) {}
	virtual void visitLoadInst(LoadInst *inst) {}
	virtual void visitJumpInst(JumpInst *inst) {}
	virtual void visitBranchInst(BranchInst *inst) {}
	virtual void visitRetInst(RetInst *inst) {}
	virtual void visitLiteralStringInst(LiteralStringInst *inst) {}
};

}// namespace ASM
