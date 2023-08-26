#include "Node.h"
#include <format>

namespace ASM {

void Module::print(std::ostream &os) const {
	for (auto f: functions) {
		f->print(os);
		os << '\n';
	}
	for (auto g: globalVars) {
		g->print(os);
		os << '\n';
	}
	for (auto s: literalStrings) {
		s->print(os);
		os << '\n';
	}
}

void Function::print(std::ostream &os) const {
	int count = std::max(0, max_call_arg_size - 8);
	for (auto sv: stack) sv->offset = 4 * count++;
	count = 0;
	auto sp_size = get_total_stack();
	for (auto p: params) p->offset = sp_size + 4 * count++;
	os << "\t.text\n";
	os << "\t.globl " << name << '\n';
	// os << "\t.type " << name << ", @function\n";
	os << name << ":\n";
	if (sp_size > 0)
		os << "\taddi\tsp, sp, -" << sp_size << "\t\t; var=" << stack.size() << " call=" << std::max(max_call_arg_size - 8, 0) << '\n';
	if (max_call_arg_size >= 0)
		os << "\tsw\tra, " << stack.front()->offset << "(sp)\n";
	for (auto b: blocks)
		b->print(os);
}

void Block::print(std::ostream &os) const {
	os << label << ":";
	if (!comment.empty()) os << "\t\t\t# " << comment;
	os << '\n';
	for (auto s: stmts) {
		os << '\t';
		s->print(os);
		if (!s->comment.empty()) os << "\t\t\t# " << s->comment;
		os << '\n';
	}
}

void SltInst::print(std::ostream &os) const {
	os << "slt";
	if (!dynamic_cast<Reg *>(rs2)) os << 'i';
	if (isUnsigned) os << 'u';
	os << '\t' << rd->name << ", " << rs1->name << ", " << rs2->to_string();
}

void BinaryInst::print(std::ostream &os) const {
	os << op;
	if (!dynamic_cast<Reg *>(rs2)) os << 'i';
	os << '\t' << rd->name << ", " << rs1->name << ", " << rs2->to_string();
}

void MulDivRemInst::print(std::ostream &os) const {
	os << op << '\t' << rd->name << ", " << rs1->name << ", " << rs2->name;
}

void CallInst::print(std::ostream &os) const {
	os << "call\t" << funcName;
}

void MoveInst::print(std::ostream &os) const {
	os << "mv\t" << rd->name << ", " << rs->name;
}

void StoreOffset::print(std::ostream &os) const {
	char op = size == 4 ? 'w' : 'b';
	os << 's' << op << '\t' << val->name << ", " << (offset ? offset->to_string() : "0") << '(' << dst->name << ')';
}

void StoreSymbol::print(std::ostream &os) const {
	char op = size == 4 ? 'w' : 'b';
	os << 's' << op << '\t' << val->name << ", " << symbol->to_string() << ", " << rd->name;
}

void LoadOffset::print(std::ostream &os) const {
	char op = size == 4 ? 'w' : 'b';
	os << 'l' << op << '\t' << rd->name << ", " << (offset ? offset->to_string() : "0") << '(' << src->name << ')';
}

void LoadSymbol::print(std::ostream &os) const {
	char op = size == 4 ? 'w' : 'b';
	os << 'l' << op << '\t' << rd->name << ", " << symbol->to_string();
}

void JumpInst::print(std::ostream &os) const {
	os << "j\t" << dst->label;
}

void BranchInst::print(std::ostream &os) const {
	os << "b" << op << '\t' << rs1->name << ", " << rs2->name << ", " << dst->label;
}

void RetInst::print(std::ostream &os) const {
	if (func->max_call_arg_size >= 0)
		os << "lw ra, " << func->stack.front()->offset << "(sp)\n\t";
	if (int sp_size = func->get_total_stack(); sp_size > 0)
		os << "addi sp, sp, " << sp_size << "\n\t";
	os << "ret";
}

void LuiInst::print(std::ostream &os) const {
	os << "lui\t" << rd->name << ", " << imm->to_string();
}

void LiInst::print(std::ostream &os) const {
	os << "li\t" << rd->name << ", " << imm->to_string();
}

void LaInst::print(std::ostream &os) const {
	os << "la\t" << rd->name << ", " << globalVal->to_string();
}

void GlobalVarInst::print(std::ostream &os) const {
	os << std::format(R"(	.section data
	.globl {}
{}:
	.word	{}
)",
					  globalVal->name, globalVal->name, initVal ? initVal->to_string() : "");
}

void LiteralStringInst::print(std::ostream &os) const {
	std::string trans;
	for (auto c: val) {
		switch (c) {
			case '\n':
				trans += "\\n";
				break;
			case '\t':
				trans += "\\t";
				break;
			case '"':
				trans += "\\\"";
				break;
			case '\\':
				trans += "\\\\";
			case 0:
				break;
			default:
				trans += c;
		}
	}
	os << std::format(R"(	.section rodata
{}:
	.asciz "{}"
)",
					  globalVal->name, trans);
}


}// namespace ASM
