// UTF-8 
// @ArinaMgk
// OutFileFormat

#include "inc/aasm.h"
#include "../inc/cpp/Format/Code.hpp"

void elf32_set_architecture(uni::lango::ArchitectureKind architecture);
void elf64_set_architecture(uni::lango::ArchitectureKind architecture);
void elf32_configure_target(const uni::lango::AssemblyTargetProfile& target);
void elf64_configure_target(const uni::lango::AssemblyTargetProfile& target);
void elf32_emit_fixup(const uni::CodeFixup& fixup);
void elf64_emit_fixup(const uni::CodeFixup& fixup);

outffmt* OutfileFormat::crt_format = NULL;

#ifndef OF_DEFAULT
#define OF_DEFAULT of_bin
#endif

namespace {

uni::CodeendFlag CodeendFlags(outffmt& driver)
{
	uni::CodeendFlag flags = (driver.flags & OFMT_TEXT) ?
		uni::CodeendFlag::Text : uni::CodeendFlag::None;
	if (&driver == &of_bin)
		flags = flags | uni::CodeendFlag::MultipleArchitectures;
	return flags;
}

class AasmCodeend final : public uni::CodeendInterface {
private:
	outffmt* Driver;
	uni::CodeendInfo Info;

public:
	explicit AasmCodeend(outffmt& driver)
		: Driver(&driver), Info {
			driver.fullname,
			driver.shortname,
			CodeendFlags(driver)
		}
	{
	}

	virtual const uni::CodeendInfo& DumpCodeend() const override
	{
		return Info;
	}

	virtual bool SupportsArchitecture(
		uni::lango::ArchitectureKind architecture) const override
	{
		if (architecture == uni::lango::ArchitectureKind::X86) return true;
		return architecture == uni::lango::ArchitectureKind::RISCV &&
			(Driver == &of_bin || Driver == &of_elf32 || Driver == &of_elf ||
			Driver == &of_elf64);
	}

	virtual bool SupportsBitWidth(
		uni::lango::ArchitectureKind architecture, int bits) const override
	{
		if (!SupportsArchitecture(architecture)) return false;
		if (architecture == uni::lango::ArchitectureKind::X86) return true;
		if (Driver == &of_bin) return bits == 32 || bits == 64;
		if (Driver == &of_elf32 || Driver == &of_elf) return bits == 32;
		if (Driver == &of_elf64) return bits == 64;
		return false;
	}

	virtual const unsigned char* StandardMacros() const override
	{
		return Driver->stdmac;
	}

	virtual void PrepareFilename(char* input_name, char* output_name) override
	{
		Driver->filename(input_name, output_name);
	}

	virtual void Initialize(FILE* output,
		uni::lango::ArchitectureKind architecture) override
	{
		if (Driver == &of_elf32 || Driver == &of_elf)
			elf32_set_architecture(architecture);
		else if (Driver == &of_elf64)
			elf64_set_architecture(architecture);
		Driver->init(output,
			static_cast<ldfunc>(&Labelsys::Define), eval_evaluate);
	}

	virtual void ConfigureTarget(
		const uni::lango::AssemblyTargetProfile& target) override
	{
		if (Driver == &of_elf32 || Driver == &of_elf)
			elf32_configure_target(target);
		else if (Driver == &of_elf64)
			elf64_configure_target(target);
	}

	virtual void InitializeDebug(FILE* output) override
	{
		Driver->current_dfmt->init(Driver, NULL, output);
	}

	virtual void Cleanup(bool debug_info) override
	{
		Driver->cleanup(debug_info);
	}

	virtual int32_t SelectSection(
		const char* name, int pass, int& bits) override
	{
		return Driver->section(name, pass, &bits);
	}

	virtual void DefineSymbol(char* name, int32_t section,
		int64_t value, int binding, char* attributes) override
	{
		Driver->symdef(name, section, value, binding, attributes);
	}

	virtual void DefineDebugSymbol(char* name, int32_t section,
		int64_t value, int binding, char* attributes) override
	{
		Driver->current_dfmt->debug_deflabel(
			name, section, value, binding, attributes);
	}

	virtual bool ProcessDirective(
		char* directive, char* value, int pass) override
	{
		return Driver->directive(directive, value, pass);
	}

	virtual bool ProcessOption(char** option) override
	{
		return Driver->setinfo(GI_SWITCH, option);
	}

	virtual bool SelectDebugFormat(const char* name) override
	{
		for (dbgffmt** format = Driver->debug_formats; *format; format++) {
			if (!StrCompare(name, (*format)->shortname)) {
				Driver->current_dfmt = *format;
				return true;
			}
		}
		return false;
	}

	virtual void ListDebugFormats(FILE* output) const override
	{
		for (dbgffmt** format = Driver->debug_formats; *format; format++) {
			fprintf(output, "  %c %-10s%s\n",
				*format == Driver->current_dfmt ? '*' : ' ',
				(*format)->shortname, (*format)->fullname);
		}
	}

	virtual void DisableDebug() override
	{
		Driver->current_dfmt = &null_debug_form;
	}

	virtual void ProcessDebugDirective(
		const char* directive, const char* parameters) override
	{
		Driver->current_dfmt->debug_directive(directive, parameters);
	}

	virtual void SetDebugType(int32_t type) override
	{
		Driver->current_dfmt->debug_typevalue(type);
	}

	virtual void SetDebugLine(
		const char* filename, int32_t line, int32_t section) override
	{
		Driver->current_dfmt->linenum(filename, line, section);
	}

	virtual void Emit(const uni::CodeendOutput& output) override
	{
		out_type type;

		switch (output.type) {
		case uni::CodeendOutputType::RawData:
			type = OUT_RAWDATA;
			break;
		case uni::CodeendOutputType::Reserve:
			type = OUT_RESERVE;
			break;
		default:
			aasm_log(_LOG_PANIC, "invalid Codeend output type");
			return;
		}

		Driver->output(output.section, output.data, type, output.size,
			NO_SEG, NO_SEG);
	}

	virtual void EmitFixup(const uni::CodeFixup& fixup) override
	{
		if (fixup.architecture_kind == static_cast<uint32_t>(
			uni::lango::ArchitectureKind::RISCV)) {
			if (Driver == &of_elf32 || Driver == &of_elf) {
				elf32_emit_fixup(fixup);
				return;
			}
			if (Driver == &of_elf64) {
				elf64_emit_fixup(fixup);
				return;
			}
			aasm_log(_LOG_ERROR,
				"output format `%s' does not support RISC-V relocation",
				Info.shortname);
			return;
		}
		const uni::CodeFixupEncoding& encoding = fixup.encoding;
		if (encoding.bit_offset || encoding.right_shift ||
			encoding.value_bits != encoding.storage_bits ||
			(encoding.storage_bits != 16 && encoding.storage_bits != 32 &&
			encoding.storage_bits != 64)) {
			aasm_log(_LOG_PANIC, "unsupported fixup encoding");
			return;
		}

		out_type type;
		uint64_t size;
		switch (fixup.kind) {
		case uni::CodeFixupKind::Absolute:
			type = OUT_ADDRESS;
			size = encoding.storage_bits / 8;
			break;
		case uni::CodeFixupKind::PcRelative:
		case uni::CodeFixupKind::Branch:
		case uni::CodeFixupKind::Call:
			switch (encoding.storage_bits) {
			case 16: type = OUT_REL2ADR; break;
			case 32: type = OUT_REL4ADR; break;
			case 64: type = OUT_REL8ADR; break;
			default:
				aasm_log(_LOG_PANIC, "unsupported relative fixup width");
				return;
			}
			if (fixup.pc_bias < 0) {
				aasm_log(_LOG_PANIC, "negative fixup PC bias");
				return;
			}
			size = static_cast<uint64_t>(fixup.pc_bias);
			break;
		default:
			aasm_log(_LOG_PANIC, "unsupported fixup kind");
			return;
		}

		int64_t addend = fixup.addend;
		Driver->output(fixup.section, &addend, type, size,
			fixup.target_section, fixup.legacy_wrt);
	}

	virtual int32_t ResolveSegmentBase(int32_t section) override
	{
		return Driver->segbase(section);
	}

	virtual void* NativeHandle() const override
	{
		return Driver;
	}
};

AasmCodeend Drivers[] = {
	AasmCodeend(of_bin),
	AasmCodeend(of_ith),
	AasmCodeend(of_srec),
	AasmCodeend(of_aout),
	AasmCodeend(of_aoutb),
	AasmCodeend(of_coff),
	AasmCodeend(of_elf32),
	AasmCodeend(of_elf),
	AasmCodeend(of_elf64),
	AasmCodeend(of_as86),
	AasmCodeend(of_obj),
	AasmCodeend(of_win32),
	AasmCodeend(of_win64),
	AasmCodeend(of_ieee),
	AasmCodeend(of_macho32),
	AasmCodeend(of_macho),
	AasmCodeend(of_macho64),
	AasmCodeend(of_dbg)
};

AasmCodeend& FindDefaultCodeend()
{
	for (AasmCodeend& codeend : Drivers) {
		if (codeend.NativeHandle() == &OF_DEFAULT) return codeend;
	}
	aasm_log(_LOG_PANIC, "No default output driver given at compile time");
	return Drivers[0];
}

}

namespace uni {

CodeendInterface& DefaultCodeend()
{
	return FindDefaultCodeend();
}

CodeendInterface* FindCodeend(const char* name)
{
	for (AasmCodeend& codeend : Drivers)
		if (!StrCompare(name, codeend.DumpCodeend().shortname)) return &codeend;
	return nullptr;
}

void SelectCodeend(CodeendInterface& codeend)
{
	OutfileFormat::crt_format = static_cast<outffmt*>(codeend.NativeHandle());
}

void ListCodeends(const CodeendInterface& selected, FILE* output)
{
	for (AasmCodeend& codeend : Drivers) {
		const CodeendInfo& info = codeend.DumpCodeend();
		fprintf(output, "  %c %-10s%s\n",
			&codeend == &selected ? '*' : ' ',
			info.shortname, info.fullname);
	}
}

}

uint64_t realsize(enum out_type type, uint64_t size)
{
	switch (type) {
	case OUT_REL2ADR:
		return 2;
	case OUT_REL4ADR:
		return 4;
	case OUT_REL8ADR:
		return 8;
	default:
		return size;
	}
}

struct outffmt* ofmt_find(char* name)
{
	uni::CodeendInterface* codeend = uni::FindCodeend(name);
	return codeend ? static_cast<outffmt*>(codeend->NativeHandle()) : NULL;
}
struct dbgffmt* dfmt_find(struct outffmt* ofmt, char* name)
{                               /* find driver */
	struct dbgffmt** dfmt = ofmt->debug_formats;
	while (*dfmt) {
		if (!StrCompare(name, (*dfmt)->shortname))
			return (*dfmt);
		dfmt++;
	}
	return NULL;
}

void ofmt_list(struct outffmt* deffmt, FILE* fp)
{
	uni::CodeendInterface* selected = &uni::DefaultCodeend();
	for (AasmCodeend& codeend : Drivers)
		if (codeend.NativeHandle() == deffmt) selected = &codeend;
	uni::ListCodeends(*selected, fp);
}
void dfmt_list(struct outffmt* ofmt, FILE* fp)
{
	struct dbgffmt** drivers = ofmt->debug_formats;
	while (*drivers) {
		fprintf(fp, "  %c %-10s%s\n",
			drivers[0] == ofmt->current_dfmt ? '*' : ' ',
			drivers[0]->shortname, drivers[0]->fullname);
		drivers++;
	}
}

struct outffmt* ofmt_register()
{
	uni::CodeendInterface& codeend = uni::DefaultCodeend();
	uni::SelectCodeend(codeend);
	return static_cast<outffmt*>(codeend.NativeHandle());
}


// ----

// register_output_formats = ofmt_register
void OutfileFormat::setMode() {
	crt_format = ofmt_register();
}

_ESYM_C struct outffmt** ofmt_f() {
	return &OutfileFormat::crt_format;
}
