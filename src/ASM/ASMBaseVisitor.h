#pragma once

#include <ostream>
#include <sstream>
namespace ASM {

struct ASMBaseVisitor;

struct Node {
	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		print(ss);
		return ss.str();
	}
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
struct StoreInstBase;
struct StoreOffset;
struct StoreSymbol;
struct LoadInstBase;
struct LoadOffset;
struct LoadSymbol;
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
	virtual void visitStoreInstBase(StoreInstBase *inst) {}
	virtual void visitStoreOffset(StoreOffset *inst) {}
	virtual void visitStoreSymbol(StoreSymbol *inst) {}
	virtual void visitLoadInstBase(LoadInstBase *inst) {}
	virtual void visitLoadOffset(LoadOffset *inst) {}
	virtual void visitLoadSymbol(LoadSymbol *inst) {}
	virtual void visitJumpInst(JumpInst *inst) {}
	virtual void visitBranchInst(BranchInst *inst) {}
	virtual void visitRetInst(RetInst *inst) {}
	virtual void visitLiteralStringInst(LiteralStringInst *inst) {}
};

}// namespace ASM
