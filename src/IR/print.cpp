#include "Node.h"
#include <format>
#include <iostream>

using namespace IR;

void Class::print(std::ostream &out) const {
	out << type.to_string() << " = type {";
	for (auto &f: type.fields) {
		out << f->to_string();
		if (&f != &type.fields.back())
			out << ", ";
	}
	out << "}";
}

void Function::print(std::ostream &out) const {
	if (blocks.empty())
		out << "declare ";
	else
		out << "define ";
	if (type)
		out << type->to_string() << " ";
	out << "@" << name << "(";
	for (auto &param: params) {
		out << param.first->to_string() << " %" << param.second;
		if (param != params.back()) out << ", ";
	}
	out << ")";
	if (!blocks.empty()) {
		out << " {\n";
		for (auto block: blocks)
			block->print(out);
		out << "}";
	}
}

void Val::print(std::ostream &out) const {
	out << type->to_string() << ' ' << get_name();
}

std::string GlobalVar::get_name() const {
	return "@" + name;
}
std::string LocalVar::get_name() const {
	return "%" + name;
}

void Module::print(std::ostream &out) const {
	/*out << R"(; ModuleID = 'test.ll'
source_filename = "test.cpp"
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128"
target triple = "riscv32-unknown-unknown-elf"

)";*/
	for (auto &c: classes) {
		c->print(out);
		out << "\n\n";
	}
	for (auto str: stringLiterals) {
		str->print(out);
		out << "\n";
	}
	out << '\n';
	for (auto &g: variables) {
		g->print(out);
		out << "\n";
	}
	out << "\n";
	for (auto &f: functions) {
		f->print(out);
		out << "\n\n";
	}
}

void Block::print(std::ostream &out) const {
	out << label << ":\n";
	for (auto stmt: stmts) {
		out << '\t';
		stmt->print(out);
		out << '\n';
	}
}

void StoreStmt::print(std::ostream &out) const {
	out << "store ";
	if (!value) out << "\033[31m%TODO\033[0m";
	else {
		value->print(out);
	}
	out << ", ptr " << pointer->get_name();
}

void AllocaStmt::print(std::ostream &out) const {
	out << res->get_name() << " = alloca " << type->to_string();
}

void LiteralBool::print(std::ostream &out) const {
	out << "i1 " << get_name();
}
std::string LiteralBool::get_name() const {
	return value ? "true" : "false";
}

void LiteralInt::print(std::ostream &out) const {
	out << "i32 " << value;
}
std::string LiteralInt::get_name() const {
	return std::to_string(value);
}

//void LiteralChar::print(std::ostream &out) const {
//	out << "i8 " << value;
//}

void LiteralNull::print(std::ostream &out) const {
	out << "ptr null";
}

std::string LiteralNull::get_name() const {
	return "null";
}


void LiteralString::print(std::ostream &out) const {
	out << "<TODO: string literal>";
}
std::string LiteralString::get_name() const {
	return "<TODO: string literal>";
}

void ArithmeticStmt::print(std::ostream &out) const {
	out << res->get_name() << " = " << cmd << " " << res->type->to_string() << " ";
	if (!lhs) out << "\033[31m%TODO\033[0m";
	else
		out << lhs->get_name();
	out << ", ";
	if (!rhs) out << "\033[31m%TODO\033[0m";
	else
		out << rhs->get_name();
}

void LoadStmt::print(std::ostream &out) const {
	out << res->get_name() << " = load " << res->type->to_string() << ", ptr " << pointer->get_name();
}

void RetStmt::print(std::ostream &out) const {
	out << "ret ";
	if (value)
		value->print(out);
	else
		out << "void";
}

void GetElementPtrStmt::print(std::ostream &out) const {
	out << res->get_name() << " = getelementptr " << typeName << ", ptr " << pointer->get_name() << ", ";
	for (auto &idx: indices) {
		idx->print(out);
		if (&idx != &indices.back())
			out << ", ";
	}
}

void CallStmt::print(std::ostream &out) const {
	if (res)
		out << res->get_name() << " = ";
	out << "call " << func->type->to_string() << " @" << func->name << "(";
	for (auto &arg: args) {
		arg->print(out);
		if (&arg != &args.back())
			out << ", ";
	}
	out << ")";
}

void DirectBrStmt::print(std::ostream &out) const {
	out << "br label %" << block->label;
}

void CondBrStmt::print(std::ostream &out) const {
	out << "br ";
	cond->print(out);
	out << ", label %" << trueBlock->label << ", label %" << falseBlock->label;
}

void IcmpStmt::print(std::ostream &out) const {
	out << res->get_name() << " = icmp " << cmd << " i32 ";
	out << lhs->get_name();
	out << ", ";
	out << rhs->get_name();
}

void PhiStmt::print(std::ostream &out) const {
	out << res->get_name() << " = phi " << res->type->to_string() << " ";
	for (auto &val: branches) {
		out << "[ ";
		out << val.first->get_name();
		out << ", %" << val.second->label << " ]";
		if (&val != &branches.back())
			out << ", ";
	}
}

void UnreachableStmt::print(std::ostream &out) const {
	out << "unreachable";
}

void GlobalStmt::print(std::ostream &out) const {
	out << var->get_name() << " = global ";
	value->print(out);
}

void GlobalStringStmt::print(std::ostream &out) const {
	out << var->get_name() << " = private unnamed_addr constant [" << value.size() << " x i8] c\"";
	for (auto c: value) {
		if (c == '\n')
			out << "\\0A";

		else if (c == '\\')
			out << "\\\\";
		else if (c == '"')
			out << "\\22";
		else
			out << c;
	}
	out << '"';
}

std::string StringLiteralVar::get_name() const {
	return "@" + name;
}