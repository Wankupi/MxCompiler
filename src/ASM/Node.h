#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <list>

namespace ASM {

struct Node {
	virtual void print(std::ostream &os) const = 0;
	virtual ~Node() = default;
};

struct Stmt : public Node {
};

struct Block : public Node {
	std::string label;
	std::list<Stmt *> stmts;

	void print(std::ostream &os) const override;
};

struct Function : public Node {
	std::string name;
	std::list<Block *> blocks;

	void print(std::ostream &os) const override;
};

struct Module : public Node {
	std::list<Function *> functions;

	void print(std::ostream &os) const override;
};

}// namespace ASM