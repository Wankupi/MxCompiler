#include "Val.h"
#include "Register.h"

std::string ASM::OffsetOfStackVal::to_string() const {
	return std::to_string(stackVal->offset);
}

ASM::OffsetOfStackVal *ASM::StackVal::get_offset() {
	if (!offsetOfStackVal)
		offsetOfStackVal = new OffsetOfStackVal{this};
	return offsetOfStackVal;
}

std::string ASM::RelocationFunction::to_string() const {
	return type + '(' + globalVal->name + ')';
}

ASM::RelocationFunction *ASM::GlobalVal::get_hi() {
	if (!hi)
		hi = new RelocationFunction{"%hi", this};
	return hi;
}

ASM::RelocationFunction *ASM::GlobalVal::get_lo() {
	if (!lo)
		lo = new RelocationFunction{"%lo", this};
	return lo;
}

ASM::GlobalPosition *ASM::GlobalVal::get_pos() {
	if (!gp)
		gp = new GlobalPosition{this};
	return gp;
}

std::string ASM::GlobalPosition::to_string() const {
	return globalVal->name;
}
