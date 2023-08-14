#pragma once
#include <string>
namespace ASM {

struct Val {
	[[nodiscard]] virtual std::string to_string() const = 0;
	virtual ~Val() = default;
};

struct Imm : public Val {};

struct ImmI32 : public Imm {
	int val = 0;
	[[nodiscard]] std::string to_string() const override {
		return std::to_string(val);
	}
};

struct StackVal;

struct OffsetOfStackVal : public Imm {
	friend struct StackVal;
	[[nodiscard]] std::string to_string() const override;
	~OffsetOfStackVal() override = default;

private:
	explicit OffsetOfStackVal(StackVal *stackVal) : stackVal(stackVal) {}
	StackVal *stackVal = nullptr;
};

struct StackVal : public Val {
	StackVal() = default;
	~StackVal() override {
		delete offsetOfStackVal;
	}
	int offset = 0;
	OffsetOfStackVal *get_offset();
	[[nodiscard]] std::string to_string() const override {
		// this function may not be used
		return "stack[" + std::to_string(offset) + ']';
	}

private:
	OffsetOfStackVal *offsetOfStackVal = nullptr;
};

struct GlobalVal;

struct RelocationFunction : public Imm {
	friend struct GlobalVal;
	[[nodiscard]] std::string to_string() const override;
	~RelocationFunction() override = default;

private:
	explicit RelocationFunction(std::string type, GlobalVal *globalVal) : type(std::move(type)), globalVal(globalVal) {}
	std::string type;
	GlobalVal *globalVal = nullptr;
};

struct GlobalPosition : public Imm {
	friend class GlobalVal;
	[[nodiscard]] std::string to_string() const override;
	~GlobalPosition() override = default;

private:
	explicit GlobalPosition(GlobalVal *globalVal) : globalVal(globalVal) {}
	GlobalVal *globalVal = nullptr;
};

struct GlobalVal : public Val {
	explicit GlobalVal(std::string name) : name(std::move(name)) {}
	~GlobalVal() override {
		delete hi;
		delete lo;
		delete gp;
	}
	[[nodiscard]] std::string to_string() const override {
		return name;
	}
	RelocationFunction *get_hi();
	RelocationFunction *get_lo();
	GlobalPosition *get_pos();

public:
	std::string name;

private:
	RelocationFunction *hi = nullptr, *lo = nullptr;
	GlobalPosition *gp = nullptr;
};

}// namespace ASM
