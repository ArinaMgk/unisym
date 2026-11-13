// ASCII C++11 TAB4 LF
// Docutitle: AASM RISC-V Operand Model
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/riscv/Operand-riscv.hpp"

namespace uni::lango {

	namespace {

		bool Equal(const char* left, const char* right)
		{
			return !StrCompareInsensitive(left, right);
		}

		bool ParseNumbered(const char* text, char prefix, unsigned maximum,
			uint8_t& result)
		{
			if (ascii_tolower(text[0]) != prefix || !text[1]) return false;
			char* end;
			long number = StrTokenLong(text + 1, &end, 10);
			if (*end || number < 0 || number > static_cast<long>(maximum)) return false;
			result = static_cast<uint8_t>(number);
			return true;
		}

	}

	bool ParseRISCVRegister(const char* text, RISCVRegister& result)
	{
		uint8_t index;
		if (ParseNumbered(text, 'x', 31, index)) {
			result = { RISCVRegisterClass::Integer, index };
			return true;
		}
		if (ParseNumbered(text, 'f', 31, index)) {
			result = { RISCVRegisterClass::Float, index };
			return true;
		}
		if (ParseNumbered(text, 'v', 31, index)) {
			result = { RISCVRegisterClass::Vector, index };
			return true;
		}
		struct Entry { const char* name; RISCVRegisterClass kind; uint8_t index; };
		static const Entry entries[] = {
			{"zero", RISCVRegisterClass::Integer, 0}, {"ra", RISCVRegisterClass::Integer, 1},
			{"sp", RISCVRegisterClass::Integer, 2}, {"gp", RISCVRegisterClass::Integer, 3},
			{"tp", RISCVRegisterClass::Integer, 4}, {"t0", RISCVRegisterClass::Integer, 5},
			{"t1", RISCVRegisterClass::Integer, 6}, {"t2", RISCVRegisterClass::Integer, 7},
			{"s0", RISCVRegisterClass::Integer, 8}, {"fp", RISCVRegisterClass::Integer, 8},
			{"s1", RISCVRegisterClass::Integer, 9}, {"a0", RISCVRegisterClass::Integer, 10},
			{"a1", RISCVRegisterClass::Integer, 11}, {"a2", RISCVRegisterClass::Integer, 12},
			{"a3", RISCVRegisterClass::Integer, 13}, {"a4", RISCVRegisterClass::Integer, 14},
			{"a5", RISCVRegisterClass::Integer, 15}, {"a6", RISCVRegisterClass::Integer, 16},
			{"a7", RISCVRegisterClass::Integer, 17}, {"s2", RISCVRegisterClass::Integer, 18},
			{"s3", RISCVRegisterClass::Integer, 19}, {"s4", RISCVRegisterClass::Integer, 20},
			{"s5", RISCVRegisterClass::Integer, 21}, {"s6", RISCVRegisterClass::Integer, 22},
			{"s7", RISCVRegisterClass::Integer, 23}, {"s8", RISCVRegisterClass::Integer, 24},
			{"s9", RISCVRegisterClass::Integer, 25}, {"s10", RISCVRegisterClass::Integer, 26},
			{"s11", RISCVRegisterClass::Integer, 27}, {"t3", RISCVRegisterClass::Integer, 28},
			{"t4", RISCVRegisterClass::Integer, 29}, {"t5", RISCVRegisterClass::Integer, 30},
			{"t6", RISCVRegisterClass::Integer, 31},
			{"ft0", RISCVRegisterClass::Float, 0}, {"ft1", RISCVRegisterClass::Float, 1},
			{"ft2", RISCVRegisterClass::Float, 2}, {"ft3", RISCVRegisterClass::Float, 3},
			{"ft4", RISCVRegisterClass::Float, 4}, {"ft5", RISCVRegisterClass::Float, 5},
			{"ft6", RISCVRegisterClass::Float, 6}, {"ft7", RISCVRegisterClass::Float, 7},
			{"fs0", RISCVRegisterClass::Float, 8}, {"fs1", RISCVRegisterClass::Float, 9},
			{"fa0", RISCVRegisterClass::Float, 10}, {"fa1", RISCVRegisterClass::Float, 11},
			{"fa2", RISCVRegisterClass::Float, 12}, {"fa3", RISCVRegisterClass::Float, 13},
			{"fa4", RISCVRegisterClass::Float, 14}, {"fa5", RISCVRegisterClass::Float, 15},
			{"fa6", RISCVRegisterClass::Float, 16}, {"fa7", RISCVRegisterClass::Float, 17},
			{"fs2", RISCVRegisterClass::Float, 18}, {"fs3", RISCVRegisterClass::Float, 19},
			{"fs4", RISCVRegisterClass::Float, 20}, {"fs5", RISCVRegisterClass::Float, 21},
			{"fs6", RISCVRegisterClass::Float, 22}, {"fs7", RISCVRegisterClass::Float, 23},
			{"fs8", RISCVRegisterClass::Float, 24}, {"fs9", RISCVRegisterClass::Float, 25},
			{"fs10", RISCVRegisterClass::Float, 26}, {"fs11", RISCVRegisterClass::Float, 27},
			{"ft8", RISCVRegisterClass::Float, 28}, {"ft9", RISCVRegisterClass::Float, 29},
			{"ft10", RISCVRegisterClass::Float, 30}, {"ft11", RISCVRegisterClass::Float, 31}
		};
		for (const Entry& entry : entries)
			if (Equal(text, entry.name)) {
				result = { entry.kind, entry.index };
				return true;
			}
		return false;
	}

	bool ParseRISCVCSR(const char* text, uint16_t& result)
	{
		struct Entry { const char* name; uint16_t number; };
		static const Entry entries[] = {
			{"fflags", 0x001}, {"frm", 0x002}, {"fcsr", 0x003},
			{"vstart", 0x008}, {"vxsat", 0x009}, {"vxrm", 0x00a}, {"vcsr", 0x00f},
			{"sstatus", 0x100}, {"sie", 0x104}, {"stvec", 0x105},
			{"scounteren", 0x106}, {"senvcfg", 0x10a}, {"sscratch", 0x140},
			{"sepc", 0x141}, {"scause", 0x142}, {"stval", 0x143}, {"sip", 0x144},
			{"satp", 0x180}, {"mstatus", 0x300}, {"misa", 0x301}, {"medeleg", 0x302},
			{"mideleg", 0x303}, {"mie", 0x304}, {"mtvec", 0x305},
			{"mcounteren", 0x306}, {"menvcfg", 0x30a}, {"mstatush", 0x310},
			{"medelegh", 0x312}, {"menvcfgh", 0x31a}, {"mcountinhibit", 0x320},
			{"mscratch", 0x340},
			{"mepc", 0x341}, {"mcause", 0x342}, {"mtval", 0x343}, {"mip", 0x344},
			{"mtinst", 0x34a}, {"mtval2", 0x34b},
			{"cycle", 0xc00}, {"time", 0xc01}, {"instret", 0xc02},
			{"cycleh", 0xc80}, {"timeh", 0xc81}, {"instreth", 0xc82},
			{"dcsr", 0x7b0}, {"dpc", 0x7b1}, {"dscratch0", 0x7b2},
			{"dscratch1", 0x7b3}, {"mvendorid", 0xf11}, {"marchid", 0xf12},
			{"mimpid", 0xf13}, {"mhartid", 0xf14}, {"mconfigptr", 0xf15}
		};
		for (const Entry& entry : entries)
			if (Equal(text, entry.name)) { result = entry.number; return true; }
		struct Family { const char* prefix; unsigned first, last; uint16_t base; bool high; };
		static const Family families[] = {
			{"hpmcounter", 3, 31, 0xc00, false}, {"hpmcounter", 3, 31, 0xc80, true},
			{"mhpmcounter", 3, 31, 0xb00, false}, {"mhpmcounter", 3, 31, 0xb80, true},
			{"mhpmevent", 3, 31, 0x320, false}, {"pmpcfg", 0, 15, 0x3a0, false},
			{"pmpaddr", 0, 63, 0x3b0, false}
		};
		for (const Family& family : families) {
			size_t length = StrLength(family.prefix);
			if (StrCompareNInsensitive(text, family.prefix, length)) continue;
			char* end;
			long number = StrTokenLong(text + length, &end, 10);
			if (number < static_cast<long>(family.first) || number > static_cast<long>(family.last)) continue;
			if (family.high) { if (ascii_tolower(*end) != 'h' || end[1]) continue; }
			else if (*end) continue;
			result = static_cast<uint16_t>(family.base + number);
			return true;
		}
		return false;
	}

	bool ParseRISCVRoundingMode(const char* text, RISCVRoundingMode& result)
	{
		if (Equal(text, "rne")) result = RISCVRoundingMode::RNE;
		else if (Equal(text, "rtz")) result = RISCVRoundingMode::RTZ;
		else if (Equal(text, "rdn")) result = RISCVRoundingMode::RDN;
		else if (Equal(text, "rup")) result = RISCVRoundingMode::RUP;
		else if (Equal(text, "rmm")) result = RISCVRoundingMode::RMM;
		else if (Equal(text, "dyn")) result = RISCVRoundingMode::DYN;
		else return false;
		return true;
	}

}
