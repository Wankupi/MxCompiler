#pragma once
#include <iostream>
#include <sstream>

namespace IR {

struct IRBaseVisitor;

struct IRNode {
	virtual ~IRNode() = default;
	virtual void print(std::ostream &out) const = 0;
	[[nodiscard]] std::string to_string() const {
		std::stringstream ss;
		print(ss);
		return ss.str();
	}
	virtual void accept(IRBaseVisitor *visitor) = 0;
};

struct Class;
struct Stmt;
struct BasicBlock;
struct Function;
struct AllocaStmt;
struct StoreStmt;
struct LoadStmt;
struct ArithmeticStmt;
struct IcmpStmt;
struct RetStmt;
struct GetElementPtrStmt;
struct CallStmt;
struct BrStmt;
struct DirectBrStmt;
struct CondBrStmt;
struct PhiStmt;
struct UnreachableStmt;
struct GlobalStmt;
struct GlobalStringStmt;
struct Module;

}// namespace IR
