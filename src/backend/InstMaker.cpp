#include "InstMaker.h"


void InstMake::visit(IR::Module *module) {
	if (!asmModule || !regs)
		throw std::runtime_error("InstMake: asmModule or regs is nullptr");

	for (auto f: module->functions)
		visitFunction(f);
}

void InstMake::visitFunction(IR::Function *node) {
	if (node->blocks.empty()) return;
	auto func = new ASM::Function{};
	func->name = node->name;
	currentFunction = func;
	for (auto b: node->blocks)
		visitBasicBlock(b);
	//	currentFunction = nullptr;
	asmModule->functions.push_back(func);
}

void InstMake::visitBasicBlock(IR::BasicBlock *node) {
	if (!currentFunction)
		throw std::runtime_error("InstMake: currentFunction is nullptr");
	auto block = new ASM::Block{};
	block->label = ".L" + std::to_string(labelCount++);
	for (auto s: node->stmts)
		visitStmt(s);
	currentFunction->blocks.push_back(block);
}

void InstMake::visitStmt(IR::Stmt *node) {
}
