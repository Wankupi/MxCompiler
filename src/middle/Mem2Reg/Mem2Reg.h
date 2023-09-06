#pragma once
#include "IR/Wrapper.h"

namespace IR {

class Mem2Reg {
public:
	explicit Mem2Reg(IR::Wrapper &env) : env(env) {}
	void work();

private:
	IR::Wrapper &env;
};

}// namespace IR
