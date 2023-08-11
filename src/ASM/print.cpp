#include "Node.h"


namespace ASM {

void Module::print(std::ostream &os) const {
	for (auto f: functions) {
		f->print(os);
		os << '\n';
	}
}

void Function::print(std::ostream &os) const {
	int count = 0;
	for (auto sv: stack) sv->offset = 4 * count++;

	os << "\t.text\n";
	os << "\t.globl " << name << '\n';
	// os << "\t.type " << name << ", @function\n";
	os << name << ":\n";
	os << "\taddi\tsp, sp, -" << stack.size() * 4 << '\n';
	for (auto b: blocks)
		b->print(os);
}

void Block::print(std::ostream &os) const {
	os << label << ":\n";
	for (auto s: stmts) {
		os << '\t';
		s->print(os);
		os << '\n';
	}
}

void SltInst::print(std::ostream &os) const {
	os << "slt\t" << rd->name << ", " << rs1->name << ", " << rs2->name;
}

void BinaryInst::print(std::ostream &os) const {
	os << op;
	if (!dynamic_cast<Reg *>(rs2)) os << 'i';
	os << '\t' << rd->name << ", " << rs1->name << ", " << rs2->to_string();
}

void CallInst::print(std::ostream &os) const {
	os << "call\t" << funcName;
}

void MoveInst::print(std::ostream &os) const {
	os << "mv\t" << rd->name << ", " << rs->name;
}

void StoreInst::print(std::ostream &os) const {
	os << "sw\t" << val->name << ", " << (offset ? offset->to_string() : "0") << '(' << dst->name << ')';
}

void LoadInst::print(std::ostream &os) const {
	os << "lw\t" << rd->name << ", " << (offset ? offset->to_string() : "0") << '(' << src->name << ')';
}

}// namespace ASM
