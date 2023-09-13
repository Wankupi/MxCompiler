#pragma once
#include "Val.h"
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

namespace ASM {

struct Reg : Val {
	std::string name;
	[[nodiscard]] std::string to_string() const override { return name; }
	~Reg() override = default;
};

struct VirtualReg : public Reg {
	int id = 0;
};

struct PhysicalReg : public Reg {
	int id = 0;
};

struct ValueAllocator {
	ValueAllocator() {
		for (int i = 0; i < 32; ++i)
			regs[i].id = i;
		name2reg["zero"] = &regs[0];
		name2reg["ra"] = &regs[1];
		name2reg["sp"] = &regs[2];
		name2reg["gp"] = &regs[3];
		name2reg["tp"] = &regs[4];
		name2reg["t0"] = &regs[5];
		name2reg["t1"] = &regs[6];
		name2reg["t2"] = &regs[7];
		name2reg["s0"] = &regs[8];
		name2reg["s1"] = &regs[9];
		for (int a = 0; a < 8; ++a)
			name2reg["a" + std::to_string(a)] = &regs[10 + a];
		for (int s = 2; s < 12; ++s)
			name2reg["s" + std::to_string(s)] = &regs[16 + s];
		for (int t = 3; t < 7; ++t)
			name2reg["t" + std::to_string(t)] = &regs[25 + t];

		for (auto &p: name2reg)
			p.second->name = p.first;

		name2reg["fp"] = &regs[8];
		for (int i = 0; i < 32; ++i)
			name2reg["x" + std::to_string(i)] = &regs[i];

		// special deal: ra(x1)
		// 5-7,10-17,28-31
		CallerSave = {
				//				&regs[1],
				&regs[5], &regs[6], &regs[7],
				&regs[10], &regs[11], &regs[12], &regs[13], &regs[14], &regs[15], &regs[16], &regs[17],
				&regs[28], &regs[29], &regs[30], &regs[31]};
		// 8-9,18-27
		CalleeSave = {&regs[8], &regs[9], &regs[18], &regs[19], &regs[20], &regs[21], &regs[22], &regs[23], &regs[24], &regs[25], &regs[26], &regs[27]};
	}
	PhysicalReg *get(int id) {
		return &regs[id];
	}
	PhysicalReg *get(const std::string &name) {
		auto p = name2reg.find(name);
		if (p == name2reg.end()) throw std::runtime_error("no such register: " + name);
		return p->second;
	}
	VirtualReg *registerVirtualReg() {
		auto reg = new VirtualReg{};
		reg->id = virtualRegCount++;
		reg->name = "v" + std::to_string(reg->id);
		virtualRegs.push_back(reg);
		return reg;
	}
	ImmI32 *get_imm(int val) {
		auto p = int2imm.find(val);
		if (p != int2imm.end())
			return p->second;
		auto imm = new ImmI32{};
		imm->val = val;
		int2imm[val] = imm;
		return imm;
	}
	using PhysicalRegArray = PhysicalReg[32];
	PhysicalRegArray &getRegs() { return regs; }

private:
	PhysicalReg regs[32];
	std::map<std::string, PhysicalReg *> name2reg;
	std::map<int, ImmI32 *> int2imm;
	int virtualRegCount = 0;
	std::vector<VirtualReg *> virtualRegs;

public:
	std::vector<PhysicalReg *> CallerSave, CalleeSave;
};

}// namespace ASM
