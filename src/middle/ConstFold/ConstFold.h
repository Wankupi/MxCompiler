#pragma once
#include "IR/RewriteLayer.h"
#include "IR/Wrapper.h"

namespace IR {

class ConstFold {
public:
	explicit ConstFold(Wrapper &env) : env(env) {}
	void work();

private:
	Wrapper &env;
};

}// namespace IR
