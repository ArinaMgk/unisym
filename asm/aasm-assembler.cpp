// ASCII C++ TAB4 CRLF
// Docutitle: AASM Assembly Driver
// Copyright: UNISYM, under Apache License 2.0

#include "../inc/c/stdinc.h"
#include "../inc/c/strpage.h"
#include "inc/aasm.h"
#include "../inc/cpp/Format/Code.hpp"
#include "../inc/cpp/Lango/Archend.hpp"

_ESYM_C {
extern bool terminate_after_phase;
extern bool warning_on[ERR_WARN_MAX + 1];
extern bool warning_on_global[ERR_WARN_MAX + 1];
extern const struct warning warnings[ERR_WARN_MAX + 1];
extern char listname[AASM_FILENAME_MAX];
extern Preproc* preproc;
extern int opt_verbose_info;
extern int64_t global_offset_changed;
extern int in_abs_seg;
extern int32_t abs_seg;
extern int32_t abs_offset;
}

Assembler::Assembler(AssemblySession& session)
	: Session(session)
{
	if (!Session.Offsets) Session.Offsets = StrpageNew();
}

int64_t Assembler::CurrentOffset() const
{
	if (in_abs_seg) return abs_offset;
	return StrpageGet(Session.Offsets, Session.Location.segment);
}

void Assembler::SetCurrentOffset(int64_t offset)
{
	if (in_abs_seg) {
		abs_offset = (int32_t)offset;
		Session.AbsoluteOffset = abs_offset;
	}
	else {
		Session.Offsets = StrpageSet(
			Session.Offsets, Session.Location.segment, offset);
	}
}

void Assembler::AssembleFile(char* fname, StrList** depend_ptr)
{
	char* directive, * value, * p, * q, * special, * line, debugid[80];
	int i, validid;
	bool rn_error;
	int32_t seg;
	int64_t offs;
	struct tokenval tokval;
	expr* e;
	int pass_max;

	AssemblerOptions& options = *Session.Options;
	AssemblyTarget& target = options.Target;
	if (!target.IsValid()) {
		//--auto clean flag
		aasm_log(_LOG_FATAL,
			"command line: default segment size is not supported by the selected architecture or cpu");
	}
	uni::CodeendInterface& codeend = target.Codeend();
	AssemblyStatementParser statement_parser(Session);
	uni::lango::ArchendInterface* initial_archend = &target.Archend();
	struct ArchendState {
		uni::lango::ArchendInterface* Driver;
		int LastPass;
	};
	ArchendState archend_states[8] = {};
	size_t archend_count = 0;
	auto ActivateArchend = [&](uni::lango::ArchendInterface& driver,
		int parse_pass, int output_pass) {
		ArchendState* state = nullptr;
		for (size_t index = 0; index < archend_count; index++) {
			if (archend_states[index].Driver == &driver) {
				state = &archend_states[index];
				break;
			}
		}
		if (!state) {
			if (archend_count >= numsof(archend_states)) {
				aasm_log(_LOG_FATAL, "too many architectures used in one assembly");
				return;
			}
			state = &archend_states[archend_count++];
			state->Driver = &driver;
			state->LastPass = 0;
			driver.Initialize(Session);
		}
		if (state->LastPass != passn) {
			driver.BeginPass(Session, parse_pass, output_pass);
			state->LastPass = passn;
		}
		Session.ActiveArchend = &driver;
	};

	pass_max = Session.PreviousOffsetChanged = (INT_MAX >> 1) + 2; /* Almost unlimited */
	for (passn = 1; pass0 <= 2; passn++) {
		int pass1, pass2;
		pass1 = pass0 == 2 ? 2 : 1;	/* 1, 1, 1, ..., 1, 2 */
		pass2 = passn > 1 ? 2 : 1;     /* 1, 2, 2, ..., 2, 2 */
		/* pass0                           0, 0, 0, ..., 1, 2 */

		globalbits = Session.SectionBits = target.DefaultBits();
		Session.CurrentCpu = target.Cpu();
		Session.ActiveArchend = initial_archend;
		Session.PassPhase = pass0;
		Session.PassNumber = passn;
		Session.CurrentBits = globalbits;
		if (pass0 == 2) {
			if (*listname)
				Session.Listing->Initialize(listname);
		}
		in_abs_seg = false;
		Session.InAbsoluteSection = false;
		global_offset_changed = 0;  /* Set when a label offset changes. */
		Session.Location.segment = codeend.SelectSection(
			NULL, pass2, Session.SectionBits);
		globalbits = Session.SectionBits;
		Session.CurrentBits = globalbits;
		if (passn > 1) {
			StrpageFree(Session.Offsets);
			Session.Offsets = StrpageNew();
		}
		ActivateArchend(*initial_archend, pass1, pass2);
		preproc->reset(fname, pass1, eval_evaluate,
			&Session.Listing->Legacy(),
			pass1 == 2 ? depend_ptr : NULL);
		MemCopyN(warning_on, warning_on_global, (ERR_WARN_MAX + 1) * sizeof(bool));

		Session.GlobalLineNumber = 0;
		if (passn == 1)
			Session.Location.known = true;
		Session.Location.offset = offs = CurrentOffset();

		while ((line = preproc->getline())) {
			Directive d;
			Session.GlobalLineNumber++;

			/*
		 * Here we parse our directives; this is not handled by the
		 * 'real' parser.  This really should be a separate function.
		 */
			directive = line;
			d = ParseDirective(&directive, &value);
			if (d != Directive::None) {
				int err = 0;

				switch (d) {
				case Directive::Arch:
				{
					char* end = value + StrLength(value);
					while (end > value && ascii_isspace(end[-1])) *--end = '\0';
					uni::lango::ArchendInterface* selected =
						uni::lango::FindArchend(value);
					if (!selected) {
						aasm_log(_LOG_ERROR, "unknown architecture `%s'", value);
						break;
					}
					if (!codeend.SupportsArchitecture(
						selected->DumpArchend().architecture)) {
						aasm_log(_LOG_ERROR,
							"output format `%s' does not support architecture `%s'",
							codeend.DumpCodeend().shortname,
							selected->DumpArchend().shortname);
						break;
					}
					ActivateArchend(*selected, pass1, pass2);
					Session.CurrentCpu = selected->DefaultCpu();
					globalbits = Session.SectionBits =
						selected->DumpArchend().default_bits;
					Session.CurrentBits = globalbits;
					break;
				}
				case Directive::Segment:		/* [SEGMENT n] */
				case Directive::Section:
					seg = codeend.SelectSection(
						value, pass2, Session.SectionBits);
					if (seg == NO_SEG) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "segment name `%s' not recognized", value);
					}
					else {
						in_abs_seg = false;
						Session.InAbsoluteSection = false;
						Session.Location.segment = seg;
					}
					break;
				case Directive::Extern:		/* [EXTERN label:special] */
					if (*value == '$')
						value++;        /* skip initial $ if present */
					if (pass0 == 2) {
						q = value;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							codeend.DefineSymbol(value, 0L, 0L, 3, q);
						}
					}
					else if (passn == 1) {
						q = value;
						validid = true;
						if (!isidstart(*q))
							validid = false;
						while (*q && *q != ':') {
							if (!isidchar(*q))
								validid = false;
							q++;
						}
						if (!validid) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "identifier expected after EXTERN");
							break;
						}
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else
							special = NULL;
						if (!Labelsys::IsExternal(value)) { /* Allow repeated EXTERN. */
							int temp = pass0;
							pass0 = 1;  /* fake pass 1 in labels.c */
							Labelsys::DeclareGlobal(value, special
							);
							Labelsys::Define(value, SegAlloc(), 0L, NULL,
								false, true, codeend);
							pass0 = temp;
						}
					}           /* else  pass0 == 1 */
					break;
				case Directive::Bits:		/* [BITS bits] */
					globalbits = Session.SectionBits = ParseBits(value);
					Session.CurrentBits = globalbits;
					break;
				case Directive::Global:		/* [GLOBAL symbol:special] */
					if (*value == '$')
						value++;        /* skip initial $ if present */
					if (pass0 == 2) {   /* pass 2 */
						q = value;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							codeend.DefineSymbol(value, 0L, 0L, 3, q);
						}
					}
					else if (pass2 == 1) {    /* pass == 1 */
						q = value;
						validid = true;
						if (!isidstart(*q))
							validid = false;
						while (*q && *q != ':') {
							if (!isidchar(*q))
								validid = false;
							q++;
						}
						if (!validid) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "identifier expected after GLOBAL");
							break;
						}
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else
							special = NULL;
						Labelsys::DeclareGlobal(value, special);
					}           /* pass == 1 */
					break;
				case Directive::Common:		/* [COMMON symbol size:special] */
				{
					int64_t size;

					if (*value == '$')
						value++;        /* skip initial $ if present */
					p = value;
					validid = true;
					if (!isidstart(*p))
						validid = false;
					while (*p && !ascii_isspace(*p)) {
						if (!isidchar(*p))
							validid = false;
						p++;
					}
					if (!validid) {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "identifier expected after COMMON");
						break;
					}
					if (*p) {
						while (*p && ascii_isspace(*p))
							*p++ = '\0';
						q = p;
						while (*q && *q != ':')
							q++;
						if (*q == ':') {
							*q++ = '\0';
							special = q;
						}
						else {
							special = NULL;
						}
						size = readnum(p, &rn_error);
						if (rn_error) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "invalid size specified in COMMON declaration");
							break;
						}
					}
					else {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "no size specified in COMMON declaration");
						break;
					}

					if (pass0 < 2) {
					Labelsys::Common(value, SegAlloc(), size,
						special, codeend);
					}
					else if (pass0 == 2) {
						if (special)
							codeend.DefineSymbol(value, 0L, 0L, 3, special);
					}
					break;
				}
				case Directive::Absolute:		/* [ABSOLUTE address] */
					stdscan_reset();
					stdscan_bufptr = value;
					tokval.t_type = TOKEN_INVALID;
					e = eval_evaluate(stdscan, NULL, &tokval, NULL, pass2
						, NULL);
					if (e) {
						if (!is_reloc(e)) {
							//--auto clean flag
							aasm_log(pass0 == 1 ? _LOG_ERROR : _LOG_PANIC, "cannot use non-relocatable expression as ABSOLUTE address");
						}
						else {
							abs_seg = reloc_seg(e);
							abs_offset = reloc_value(e);
						}
					}
					else if (passn == 1)
						abs_offset = 0x100;     /* don't go near zero in case of / */
					else {
						//--auto clean flag
						aasm_log(_LOG_PANIC, "invalid ABSOLUTE address in pass two");
					}
					in_abs_seg = true;
					Session.InAbsoluteSection = true;
					Session.AbsoluteSection = abs_seg;
					Session.AbsoluteOffset = abs_offset;
					Session.Location.segment = NO_SEG;
					break;
				case Directive::Debug:		/* [DEBUG] */
					p = value;
					q = debugid;
					validid = true;
					if (!isidstart(*p))
						validid = false;
					while (*p && !ascii_isspace(*p)) {
						if (!isidchar(*p))
							validid = false;
						*q++ = *p++;
					}
					*q++ = 0;
					if (!validid) {
						//--auto clean flag
						aasm_log(passn == 1 ? _LOG_ERROR : _LOG_PANIC, "identifier expected after DEBUG");
						break;
					}
					while (*p && ascii_isspace(*p))
						p++;
					if (pass0 == 2)
					codeend.ProcessDebugDirective(debugid, p);
					break;
				case Directive::Warning:		/* [WARNING {+|-|*}warn-name] */
					while (*value && ascii_isspace(*value))
						value++;

					switch (*value) {
					case '-': validid = 0; value++; break;
					case '+': validid = 1; value++; break;
					case '*': validid = 2; value++; break;
					default:  validid = 1; break;
					}

					for (i = 1; i <= ERR_WARN_MAX; i++)
						if (!StrCompareInsensitive(value, warnings[i].name))
							break;
					if (i <= ERR_WARN_MAX) {
						switch (validid) {
						case 0:
							warning_on[i] = false;
							break;
						case 1:
							warning_on[i] = true;
							break;
						case 2:
							warning_on[i] = warning_on_global[i];
							break;
						}
					}
					else {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "invalid warning id in WARNING directive");
					}
					break;
				case Directive::Cpu:		/* [CPU] */
				{
					uint32_t cpu = ParseCpu(value);
					if (Session.ActiveArchend->ValidateBits(
						Session.CurrentBits, cpu)) Session.CurrentCpu = cpu;
					else aasm_log(_LOG_ERROR,
						"cpu is incompatible with the current architecture or bit width");
					break;
				}
				case Directive::List:		/* [LIST {+|-}] */
					while (*value && ascii_isspace(*value))
						value++;

					if (*value == '+') {
						user_nolist = 0;
					}
					else {
						if (*value == '-') {
							user_nolist = 1;
						}
						else {
							err = 1;
						}
					}
					break;
				case Directive::Default:		/* [DEFAULT] */
					if (!Session.ActiveArchend->ProcessDefault(value)) err = 1;
					break;
				case Directive::Float:
					if (float_option(value)) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "unknown 'float' directive: %s", value);
					}
					break;
				default:
					if (!codeend.ProcessDirective(directive, value, pass2)) {
						//--auto clean flag
						aasm_log(pass1 == 1 ? _LOG_ERROR : _LOG_PANIC, "unrecognised directive [%s]", directive);
					}
				}
				if (err) {
					//--auto clean flag
					aasm_log(_LOG_ERROR, "invalid parameter to [%s] directive", directive);
				}
			}
			else {
				uni::lango::ArchendLineResult result =
					statement_parser.ProcessLine(line, pass1, pass2);
				if (result.valid) {
					offs += result.size;
					SetCurrentOffset(offs);
				}
			}
			memf(line);
			Session.Location.offset = offs = CurrentOffset();
		}                       /* end while (line = preproc->getline... */
		for (size_t index = 0; index < archend_count; index++) {
			if (archend_states[index].LastPass == passn)
				archend_states[index].Driver->EndPass(Session);
		}

		if (pass0 == 2 && global_offset_changed && !terminate_after_phase) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "phase error detected at end of assembly.");
		}

		if (pass1 == 1)
			preproc->cleanup(1);

		if ((passn > 1 && !global_offset_changed) || pass0 == 2) {
			pass0++;
		}
		else if (global_offset_changed &&
			global_offset_changed < Session.PreviousOffsetChanged) {
			Session.PreviousOffsetChanged = global_offset_changed;
			Session.StallCount = 0;
		}
		else {
			Session.StallCount++;
		}

		if (terminate_after_phase)
			break;

		if ((Session.StallCount > 997) || (passn >= pass_max)) {
			/* We get here if the labels don't converge
			 * Example: FOO equ FOO + 1
			 */
			 //--auto clean flag
			aasm_log(_LOG_ERROR, "Can't find valid values for all labels after %d passes, giving up.", passn);
			aasm_log(_LOG_ERROR, "Possible causes: recursive EQUs, macro abuse.");
			break;
		}
	}
	Session.PassPhase = pass0;
	Session.PassNumber = passn;
	Session.CurrentBits = globalbits;
	Session.GlobalOffsetChanged = global_offset_changed;
	Session.InAbsoluteSection = in_abs_seg;
	Session.AbsoluteSection = abs_seg;
	Session.AbsoluteOffset = abs_offset;
	Session.TerminateAfterPhase = terminate_after_phase;
	for (size_t index = 0; index < archend_count; index++)
		archend_states[index].Driver->Cleanup(Session);

	preproc->cleanup(0);
	Session.Listing->Cleanup();
	if (!terminate_after_phase && opt_verbose_info) {
		/*  -On and -Ov switches */
		fprintf(stdout, "info: assembly required 1+%d+1 passes\n", passn - 3);
	}
}

Assembler::Directive Assembler::ParseDirective(char** directive, char** value)
{
	struct DirectiveEntry {
		const char* Name;
		Directive Value;
	};
	static const DirectiveEntry Entries[] = {
		{"absolute", Directive::Absolute},
		{"arch", Directive::Arch},
		{"bits", Directive::Bits},
		{"common", Directive::Common},
		{"cpu", Directive::Cpu},
		{"debug", Directive::Debug},
		{"default", Directive::Default},
		{"extern", Directive::Extern},
		{"float", Directive::Float},
		{"global", Directive::Global},
		{"inst", Directive::Cpu},
		{"list", Directive::List},
		{"section", Directive::Section},
		{"segment", Directive::Segment},
		{"warning", Directive::Warning}
	};
	char* p, * q, * buf;

	buf = *directive;

	/*  allow leading spaces or tabs */
	while (*buf == ' ' || *buf == '\t')
		buf++;

	if (*buf != '[')
		return Directive::None;

	p = buf;

	while (*p && *p != ']')
		p++;

	if (!*p)
		return Directive::None;

	q = p++;

	while (*p && *p != ';') {
		if (!ascii_isspace(*p))
			return Directive::None;
		p++;
	}
	q[1] = '\0';

	*directive = p = buf + 1;
	while (*buf && *buf != ' ' && *buf != ']' && *buf != '\t')
		buf++;
	if (*buf == ']') {
		*buf = '\0';
		*value = buf;
	}
	else {
		*buf++ = '\0';
		while (ascii_isspace(*buf))
			buf++;              /* beppu - skip leading whitespace */
		*value = buf;
		while (*buf != ']')
			buf++;
		*buf++ = '\0';
	}

	for0(index, numsof(Entries))
		if (!StrCompareInsensitive(*directive, Entries[index].Name))
			return Entries[index].Value;
	return Directive::Codeend;
}

uint32_t Assembler::ParseCpu(char* value)
{
	uint32_t cpu = Session.CurrentCpu;
	if (!Session.ActiveArchend->ParseCpu(value, cpu)) {
		//--auto clean flag
		aasm_log(pass0 < 2 ? _LOG_ERROR : _LOG_FATAL, "unknown 'cpu' type");
	}
	return cpu;
}

int Assembler::ParseBits(char* value)
{
	int i;

	i = atoins(value);
	if (i != 16 && i != 32 && i != 64) {
		aasm_log(pass0 < 2 ? _LOG_ERROR : _LOG_FATAL,
			"`%s' is not a valid segment size; must be 16, 32 or 64", value);
		return Session.CurrentBits;
	}
	if (!Session.ActiveArchend->ValidateBits(i, Session.CurrentCpu)) {
		aasm_log(_LOG_ERROR,
			"cannot specify %d-bit mode for the selected architecture or cpu", i);
		return Session.CurrentBits;
	}
	if (i == 64) {
		if (i != maxbits) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "%s output format does not support 64-bit code",
				Session.Options->Target.Codeend().DumpCodeend().shortname);
			return Session.CurrentBits;
		}
	}
	return i;
}
