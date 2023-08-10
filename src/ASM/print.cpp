#include "Node.h"


namespace ASM {

void Module::print(std::ostream &os) const {
	for (auto f: functions) {
		f->print(os);
		os << '\n';
	}
}

void Function::print(std::ostream &os) const {
	os << "\t.text\n";
	os << "\t.globl " << name << '\n';
	// os << "\t.type " << name << ", @function\n";
	os << name << ":\n";
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



}// namespace ASM