#pragma once

#include "ASM/ASMBaseVisitor.h"
#include "ASM/Node.h"
#include <map>
#include <set>


namespace ASM {

class LiveAnalyzer : public ASM::ASMBaseVisitor {
private:
	Function *func = nullptr;

public:
	std::map<Block *, std::vector<Block *>> successor, predecessor;
	std::map<Block *, std::set<Reg *>> liveIn;
	std::map<Block *, std::set<Reg *>> liveOut;
	std::map<Block *, std::set<Reg *>> def;
	std::map<Block *, std::set<Reg *>> use;

public:
	explicit LiveAnalyzer(Function *func) : func(func) {}
	void work();
};

}// namespace ASM
