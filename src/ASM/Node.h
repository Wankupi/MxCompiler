#pragma once

#include <list>
#include <string>
#include <vector>

#include "ASMBaseVisitor.h"
#include "Register.h"
#include "Val.h"

namespace ASM {

struct Instruction : public Node {
	std::string comment;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitInstruction(this); }
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

struct Module : public Node {
	std::list<Function *> functions;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitModule(this); }
};

struct LiInst : public Instruction {
	LiInst(Reg *rd, Imm *imm) : rd(rd), imm(imm) {}

	Reg *rd = nullptr;
	Imm *imm = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLiInst(this); }
};

struct SltInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr;
	Val *rs2 = nullptr;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitSltInst(this); }
};

struct BinaryInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr;
	Val *rs2 = nullptr;
	std::string op;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitBinaryInst(this); }
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
};

struct StoreInst : public Instruction {
	Reg *val = nullptr;
	Reg *dst = nullptr;
	Val *offset = nullptr;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitStoreInst(this); }
};

struct LoadInst : public Instruction {
	Reg *rd = nullptr;
	Reg *src = nullptr;
	Val *offset = nullptr;

	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitLoadInst(this); }
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
};

struct RetInst : public Instruction {
	explicit RetInst(Function *func) : func(func) {}
	Function *func = nullptr;
	void print(std::ostream &os) const override;
	void accept(ASM::ASMBaseVisitor *visitor) override { visitor->visitRetInst(this); }
};

}// namespace ASM
