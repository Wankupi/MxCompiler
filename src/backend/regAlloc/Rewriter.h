#pragma once
#include "ASM/Node.h"
#include "ASM/RewriteLayer.h"
#include "BaseRegAllocator.h"

namespace ASM {

class ColorIt : public BaseRegAllocator {
	Function *func;
	std::map<Reg *, PhysicalReg *> const &color;

public:
	explicit ColorIt(Function *func, std::map<Reg *, PhysicalReg *> const &color) : func(func), color(color) {}
	void work() {
		visitFunction(func);
	}

private:
	Reg *get(ASM::Reg *reg) {
		if (!color.contains(reg))
			throw std::runtime_error("DoColor: fail to replace " + reg->to_string());
		return color.at(reg);
	}
	Reg *get_src(ASM::Reg *reg) override {
		return get(reg);
	}
	Reg *get_dst(ASM::Reg *reg) override {
		return get(reg);
	}
	void deal(Reg *&a, Reg *&b) override {
		a = get(a);
		b = get(b);
	}
	void deal(Reg *&reg, Val *&val) override {
		reg = get(reg);
		if (auto r = dynamic_cast<Reg *>(val))
			val = get(r);
	}

	void visitMoveInst(ASM::MoveInst *inst) override {
		auto rdbak = inst->rd, rsbak = inst->rs;
		deal(inst->rd, inst->rs);
		if (inst->rd != inst->rs)// special action for move
			add_inst(inst);
		if (!inst->rd || !inst->rs)
			throw std::runtime_error("DoColor: null reg : " + rdbak->to_string() + " " + rsbak->to_string());
	}
};

class PutStack : public BaseRegAllocator {
	Function *func = nullptr;
	std::map<Reg *, StackVal *> const &reg2st;
	std::map<Reg *, Reg *> const &alias;
	ValueAllocator *regs;

public:
	explicit PutStack(Function *func, std::map<Reg *, StackVal *> const &r2s, std::map<Reg *, Reg *> const &alias, ValueAllocator *regs)
		: func(func), reg2st(r2s), alias(alias), regs(regs) {}
	void work() {
		visitFunction(func);
	}

private:
	Reg *stackVirtVal(Reg *reg) {
		// use the same reg.
		// to reduce memory use
		// no need to create a new virtual reg
		return reg;
	}
	Reg *get_src(ASM::Reg *reg) override {
		if (alias.contains(reg))
			reg = alias.at(reg);
		if (!reg2st.contains(reg))
			return reg;
		auto v = stackVirtVal(reg);
		auto load = new LoadOffset{};
		load->rd = v;
		load->src = regs->get("sp");
		load->offset = reg2st.at(reg)->get_offset();
		load->comment = "load spilled reg " + reg->to_string() + " from stack";
		add_inst(load);
		return v;
	}
	Reg *get_dst(ASM::Reg *reg) override {
		if (alias.contains(reg))
			reg = alias.at(reg);
		if (!reg2st.contains(reg))
			return reg;
		auto v = stackVirtVal(reg);
		auto store = new StoreOffset{};
		store->dst = regs->get("sp");
		store->val = v;
		store->offset = reg2st.at(reg)->get_offset();
		store->comment = "store spilled reg " + reg->to_string() + " to stack";
		add_inst(store);
		return v;
	}
	void deal(Reg *&a, Reg *&b) override {
		a = get_src(a);
		b = get_src(b);
	}
	void deal(Reg *&reg, Val *&val) override {
		reg = get_src(reg);
		if (auto r = dynamic_cast<Reg *>(val))
			val = get_src(r);
	}
	void visitMoveInst(ASM::MoveInst *inst) override {
		if (alias.contains(inst->rd))
			inst->rd = alias.at(inst->rd);
		if (alias.contains(inst->rs))
			inst->rs = alias.at(inst->rs);
		bool containRd = reg2st.contains(inst->rd), containRs = reg2st.contains(inst->rs);
		if (inst->rd == inst->rs && containRd)
			return;
		if (containRd) {
			if (containRs)
				inst->rs = get_src(inst->rs);
			auto st = new StoreOffset{};
			st->dst = regs->get("sp");
			st->val = inst->rs;
			st->offset = reg2st.at(inst->rd)->get_offset();
			st->comment = "move spilled reg " + inst->rd->to_string() + " to stack";
			add_inst(st);
		}
		else if (containRs) {
			auto ld = new LoadOffset{};
			ld->rd = inst->rd;
			ld->src = regs->get("sp");
			ld->offset = reg2st.at(inst->rs)->get_offset();
			ld->comment = "move spilled reg " + inst->rs->to_string() + " from stack";
			add_inst(ld);
		}
		else
			add_inst(inst);
	}
};

}// namespace ASM
