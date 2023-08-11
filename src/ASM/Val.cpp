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
