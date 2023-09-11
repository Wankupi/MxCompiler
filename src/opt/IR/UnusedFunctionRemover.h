#include "IR/IRBaseVisitor.h"
#include "IR/Node.h"
#include "IR/Wrapper.h"
#include <map>
#include <queue>

namespace IR {
class UnusedFunctionRemover {
	IR::Wrapper &env;

public:
	explicit UnusedFunctionRemover(IR::Wrapper &env) : env(env) {}
	void work() {
		std::map<IR::Function *, int> callCnt;
		std::set<IR::Function *> vis;
		std::queue<IR::Function *> que;
		auto module = env.get_module();
		for (auto func: module->functions)
			if (func->name == "main") {
				callCnt[func] = 1;
				que.push(func);
			}
		while (!que.empty()) {
			auto func = que.front();
			que.pop();
			for (auto block: func->blocks)
				for (auto inst: block->stmts)
					if (auto call = dynamic_cast<IR::CallStmt *>(inst)) {
						++callCnt[call->func];
						if (!vis.contains(call->func)) {
							vis.insert(call->func);
							que.push(call->func);
						}
					}
		}
		decltype(module->functions) funcs;
		funcs.swap(module->functions);
		for (auto func: funcs)
			if (callCnt[func])
				module->functions.push_back(func);
	}
};
}// namespace IR
