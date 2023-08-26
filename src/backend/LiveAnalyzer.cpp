#include "LiveAnalyzer.h"
#include <queue>

namespace ASM {

std::vector<ASM::Block *> get_successor(ASM::Block *block, ASM::Block *nextBlock) {
	std::vector<ASM::Block *> ret;
	for (auto inst: block->stmts) {
		if (auto br = dynamic_cast<BranchInst *>(inst))
			ret.push_back(br->dst);
		else if (auto jump = dynamic_cast<JumpInst *>(inst))
			ret.push_back(jump->dst);
	}
	// deal with fall through
	auto last = block->stmts.back();
	if (nextBlock && dynamic_cast<JumpInst *>(last) == nullptr && dynamic_cast<RetInst *>(last) == nullptr)
		ret.push_back(nextBlock);
	return ret;
}

void ASM::LiveAnalyzer::work() {
	for (auto cur = func->blocks.begin(); cur != func->blocks.end(); ++cur) {
		auto block = *cur;
		auto p = cur;
		p++;
		auto next = p != func->blocks.end() ? *p : nullptr;
		successor[block] = get_successor(block, next);
		for (auto succ: successor[block])
			predecessor[succ].push_back(block);
	}
	//	std::cout << "successor: " << std::endl;
	//	for (auto &[block, suc] : successor) {
	//		std::cout << block->label << " : ";
	//		for (auto to : suc)
	//			std::cout << to->label << " ";
	//		std::cout << std::endl;
	//	}

	for (auto block: func->blocks) {
		auto &Use = this->use[block];
		auto &Def = this->def[block];

		for (auto inst: block->stmts) {
			for (auto reg: inst->getUse())
				if (!Def.contains(reg))
					Use.insert(reg);
			if (auto reg = inst->getDef())
				Def.insert(reg);
		}
	}

	//	for (auto block: func->blocks) {
	//		auto &Use = this->use[block];
	//		auto &Def = this->def[block];
	//		std::cout << "\033[32m" << block->label << "\033[0m" << std::endl;
	//		std::cout << "Use:\t";
	//		for (auto reg: Use)
	//			std::cout << reg->to_string() << ' ';
	//		std::cout << std::endl;
	//		std::cout << "Def:\t";
	//		for (auto reg: Def)
	//			std::cout << reg->to_string() << ' ';
	//		std::cout << std::endl;
	//	}

	std::queue<Block *> q;
	std::set<Block *> inQ;
	std::set<Block *> visited;
	for (auto block: func->blocks) {
		auto bak = block->stmts.back();
		if (dynamic_cast<ASM::RetInst *>(bak)) {
			q.push(block);
			inQ.insert(block);
		}
	}
	while (!q.empty()) {
		Block *block = q.front();
		q.pop();
		inQ.erase(block);
		auto UseOld = this->use[block];
		auto DefOld = this->def[block];
		std::set<Reg *> newLiveIn, newLiveOut;
		for (auto succ: successor[block])
			newLiveOut.insert(liveIn[succ].begin(), liveIn[succ].end());
		newLiveIn.insert(newLiveOut.begin(), newLiveOut.end());
		for (auto reg: def[block])
			newLiveIn.erase(reg);
		newLiveIn.insert(use[block].begin(), use[block].end());
		if (!visited.contains(block) || newLiveIn != liveIn[block] || newLiveOut != liveOut[block]) {
			visited.insert(block);
			liveIn[block].swap(newLiveIn);
			liveOut[block].swap(newLiveOut);
			for (auto pred: predecessor[block])
				if (!inQ.contains(pred)) {
					q.push(pred);
					inQ.insert(pred);
				}
		}
	}
}

}// namespace ASM
