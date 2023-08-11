#pragma once

#include <iostream>
#include <list>
#include <string>
#include <vector>

#include "Register.h"
#include "Val.h"

namespace ASM {

struct Node {
	virtual void print(std::ostream &os) const = 0;
	virtual ~Node() = default;
};

struct Instruction : public Node {
};

struct Block : public Node {
	std::string label;
	std::list<Instruction *> stmts;

	void print(std::ostream &os) const override;
};

struct Function : public Node {
	std::string name;
	std::list<Block *> blocks;

	std::vector<StackVal *> stack;

	void print(std::ostream &os) const override;
};

struct Module : public Node {
	std::list<Function *> functions;

	void print(std::ostream &os) const override;
};

struct SltInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr, *rs2 = nullptr;

	void print(std::ostream &os) const override;
};

struct BinaryInst : public Instruction {
	Reg *rd = nullptr, *rs1 = nullptr;
	Val *rs2 = nullptr;
	std::string op;

	void print(std::ostream &os) const override;
};

struct CallInst : public Instruction {
	std::string funcName;
	void print(std::ostream &os) const override;
};

struct MoveInst : public Instruction {
	Reg *rd = nullptr, *rs = nullptr;
	void print(std::ostream &os) const override;
};

struct StoreInst : public Instruction {
	Reg *val = nullptr;
	Reg *dst = nullptr;
	Val *offset = nullptr;

	void print(std::ostream &os) const override;
};

struct LoadInst : public Instruction {
	Reg *rd = nullptr;
	Reg *src = nullptr;
	Val *offset = nullptr;

	void print(std::ostream &os) const override;
};

}// namespace ASM
