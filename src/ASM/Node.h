#pragma once

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "ASMBaseVisitor.h"
#include "Register.h"
#include "Val.h"
#include <set>

namespace ASM {

struct Instruction : public Node {
	std::string comment;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitInstruction(this); }
	virtual std::set<Reg *> getUse() const { return {}; }
	virtual Reg *getDef() const { return nullptr; }
};

struct Block : public Node {
	explicit Block(std::string label) : label(std::move(label)) {}

	std::string label;
	std::list<Instruction *> stmts;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitBlock(this); }
};

struct Function : public Node {
	std::string name;
	std::list<Block *> blocks;

	std::list<StackVal *> params;
	std::list<StackVal *> stack;
	int max_call_arg_size = -1;// -1 means no call

	[[nodiscard]] int get_total_stack() const {
		int count = std::max(0, max_call_arg_size - 8);
		count += static_cast<int>(stack.size());
		return ((count * 4) + 15) / 16 * 16;
	}

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitFunction(this); }
};

struct GlobalVarInst : public Node {
	GlobalVal *globalVal = nullptr;
	Imm *initVal = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitGlobalVarInst(this); }
};

struct LiteralStringInst : public Node {
	GlobalVal *globalVal = nullptr;
	std::string val;// val end with '\0'
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLiteralStringInst(this); }
};

struct Module : public Node {
	std::list<Function *> functions;
	std::list<LiteralStringInst *> literalStrings;
	std::list<GlobalVarInst *> globalVars;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitModule(this); }
};

struct LuiInst : public Instruction {
	Reg *rd = nullptr;
	Imm *imm = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLuiInst(this); }
	Reg *getDef() const override { return rd; }
};

struct LiInst : public Instruction {
	LiInst(Reg *rd, ImmI32 *imm) : rd(rd), imm(imm) {}

	Reg *rd = nullptr;
	ImmI32 *imm = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLiInst(this); }
	Reg *getDef() const override { return rd; }
};

struct LaInst : public Instruction {
	LaInst(Reg *rd, GlobalPosition *globalVal) : rd(rd), globalVal(globalVal) {}
	Reg *rd = nullptr;
	GlobalPosition *globalVal = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLaInst(this); }
	Reg *getDef() const override { return rd; }
};

struct SltInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr;
	Val *rs2 = nullptr;
	bool isUnsigned = false;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitSltInst(this); }
	std::set<Reg *> getUse() const override {
		std::set<Reg *> ret = {rs1};
		if (auto reg = dynamic_cast<Reg *>(rs2)) ret.insert(reg);
		return ret;
	}
	Reg *getDef() const override { return rd; }
};

struct BinaryInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr;
	Val *rs2 = nullptr;
	std::string op;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitBinaryInst(this); }
	std::set<Reg *> getUse() const override {
		std::set<Reg *> ret;
		if (auto reg = dynamic_cast<Reg *>(rs1)) ret.insert(reg);
		if (auto reg = dynamic_cast<Reg *>(rs2)) ret.insert(reg);
		return ret;
	}
	Reg *getDef() const override { return rd; }
};

struct MulDivRemInst : public Instruction {
	Reg *rd = nullptr;
	Reg *rs1 = nullptr;
	Reg *rs2 = nullptr;
	std::string op;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitMulDivRemInst(this); }
	std::set<Reg *> getUse() const override { return {rs1, rs2}; }
	Reg *getDef() const override { return rd; }
};

struct CallInst : public Instruction {
	std::string funcName;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitCallInst(this); }
};

struct MoveInst : public Instruction {
	Reg *rd = nullptr, *rs = nullptr;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitMoveInst(this); }
	std::set<Reg *> getUse() const override { return {rs}; }
	Reg *getDef() const override { return rd; }
};

struct StoreInstBase : public Instruction {
	int size = 4;
	Reg *val = nullptr;
	Reg *getDef() const override { return nullptr; }
};

struct StoreOffset : public StoreInstBase {
	Reg *dst = nullptr;
	Imm *offset = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitStoreOffset(this); }
	std::set<Reg *> getUse() const override { return {val, dst}; }
};

struct StoreSymbol : public StoreInstBase {
	GlobalPosition *symbol = nullptr;
	PhysicalReg *rd = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitStoreSymbol(this); }
	std::set<Reg *> getUse() const override { return {val}; }
	Reg *getDef() const override { return rd; }
};

struct LoadInstBase : public Instruction {
	int size = 4;
	Reg *rd = nullptr;
	Reg *getDef() const override { return rd; }
};

struct LoadOffset : public LoadInstBase {
	Reg *src = nullptr;
	Imm *offset = nullptr;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLoadOffset(this); }
	std::set<Reg *> getUse() const override { return {src}; }
};

struct LoadSymbol : public LoadInstBase {
	GlobalPosition *symbol = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLoadSymbol(this); }
};

struct JumpInst : public Instruction {
	explicit JumpInst(Block *dst) : dst(dst) {}
	Block *dst = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitJumpInst(this); }
};

struct BranchInst : public Instruction {
	std::string op;
	Reg *rs1 = nullptr, *rs2 = nullptr;
	Block *dst = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitBranchInst(this); }
	std::set<Reg *> getUse() const override { return {rs1, rs2}; }
};

struct RetInst : public Instruction {
	explicit RetInst(Function *func) : func(func) {}
	Function *func = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitRetInst(this); }
};

}// namespace ASM
