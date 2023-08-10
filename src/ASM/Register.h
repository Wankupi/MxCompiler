#pragma once
#include <map>
#include <stdexcept>
#include <string>

namespace ASM {

struct Reg {
	virtual ~Reg() = default;
};

struct VirtualReg : public Reg {
	int id = 0;
};

struct PhysicalReg : public Reg {
	int id = 0;
};

struct Registers {
	Registers() {
		for (int i = 0; i < 32; ++i)
			regs[i].id = i;
		for (int i = 0; i < 32; ++i)
			name2reg["x" + std::to_string(i)] = &regs[i];
		name2reg["zero"] = &regs[0];
		name2reg["ra"] = &regs[1];
		name2reg["sp"] = &regs[2];
		name2reg["gp"] = &regs[3];
		name2reg["tp"] = &regs[4];
		name2reg["t0"] = &regs[5];
		name2reg["t1"] = &regs[6];
		name2reg["t2"] = &regs[7];
		name2reg["s0"] = name2reg["fp"] = &regs[8];
		name2reg["s1"] = &regs[9];
		for (int a = 0; a < 8; ++a)
			name2reg["a" + std::to_string(a)] = &regs[10 + a];
		for (int s = 2; s < 12; ++s)
			name2reg["s" + std::to_string(s)] = &regs[16 + s];
		for (int t = 3; t < 7; ++t)
			name2reg["t" + std::to_string(t)] = &regs[25 + t];
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
		return reg;
	}

private:
	PhysicalReg regs[32];
	std::map<std::string, PhysicalReg *> name2reg;
	int virtualRegCount = 0;
};

}// namespace ASM
