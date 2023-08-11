#pragma once
#include <string>
namespace ASM {

struct Val {
	[[nodiscard]] virtual std::string to_string() const = 0;
	virtual ~Val() = default;
};

struct Imm : public Val {
	int val;
};

struct StackVal;

struct OffsetOfStackVal : public Val {
	friend struct StackVal;
	[[nodiscard]] std::string to_string() const override;
	~OffsetOfStackVal() override = default;

private:
	explicit OffsetOfStackVal(StackVal *stackVal) : stackVal(stackVal) {}
	StackVal *stackVal = nullptr;
};

struct StackVal {
	StackVal() = default;
	~StackVal() {
		delete offsetOfStackVal;
	}
	int offset = 0;
	OffsetOfStackVal *get_offset();

private:
	OffsetOfStackVal *offsetOfStackVal = nullptr;
};

}// namespace ASM
