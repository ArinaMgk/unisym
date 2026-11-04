// ASCII C TAB4 CRLF
// Docutitle: (Module) 
// Copyright: UNISYM, under Apache License 2.0
/*
	Copyright 2023 ArinaMgk

	Licensed under the Apache License, Version 2.0 (the "License");
	you may not use this file except in compliance with the License.
	You may obtain a copy of the License at

	http://www.apache.org/licenses/LICENSE-2.0
	http://unisym.org/license.html

	Unless required by applicable law or agreed to in writing, software
	distributed under the License is distributed on an "AS IS" BASIS,
	WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
	See the License for the specific language governing permissions and
	limitations under the License.
*/

#include "../../../../../asm/inc/aasm.h"
#include "../../../../../inc/cpp/Lango/Archend/x86/Instruction-x86.hpp"
#include "../../../../../inc/c/stdinc.h"



static int i;
static struct tokenval tokval;

static int prefix_slot(enum prefixes prefix)
{
	switch (static_cast<int>(prefix)) {
	case P_WAIT:
		return PPS_WAIT;
	case R_CS:
	case R_DS:
	case R_SS:
	case R_ES:
	case R_FS:
	case R_GS:
		return PPS_SEG;
	case P_LOCK:
	case P_REP:
	case P_REPE:
	case P_REPZ:
	case P_REPNE:
	case P_REPNZ:
		return PPS_LREP;
	case P_O16:
	case P_O32:
	case P_O64:
	case P_OSP:
		return PPS_OSIZE;
	case P_A16:
	case P_A32:
	case P_A64:
	case P_ASP:
		return PPS_ASIZE;
	default:
		//--auto clean flag
		aasm_log(_LOG_PANIC, "Invalid value %d passed to prefix_slot()", prefix);
		return -1;
	}
}

static void process_size_override(insn* result, int operand)
{
	if (tasm_compatible_mode) {
		switch ((int)tokval.t_integer) {
			/* For TASM compatibility a size override inside the
			 * brackets changes the size of the operand, not the
			 * address type of the operand as it does in the default
			 * syntax. Hence:
			 *
			 *  mov     eax,[DWORD val]
			 *
			 * is valid syntax in TASM compatibility mode. Note that
			 * you lose the ability to override the default address
			 * type for the instruction, but we never use anything
			 * but 32-bit flat model addressing in our code.
			 */
		case S_BYTE:
			result->oprs[operand].type |= BITS8;
			break;
		case S_WORD:
			result->oprs[operand].type |= BITS16;
			break;
		case S_DWORD:
		case S_LONG:
			result->oprs[operand].type |= BITS32;
			break;
		case S_QWORD:
			result->oprs[operand].type |= BITS64;
			break;
		case S_TWORD:
			result->oprs[operand].type |= BITS80;
			break;
		case S_OWORD:
			result->oprs[operand].type |= BITS128;
			break;
		default:
			//--auto clean flag
			aasm_log(_LOG_ERROR, "invalid operand size specification");
			break;
		}
	}
	else {
		/* Default addressing syntax */
		switch ((int)tokval.t_integer) {
		case S_NOSPLIT:
			result->oprs[operand].eaflags |= EAF_TIMESTWO;
			break;
		case S_REL:
			result->oprs[operand].eaflags |= EAF_REL;
			break;
		case S_ABS:
			result->oprs[operand].eaflags |= EAF_ABS;
			break;
		case S_BYTE:
			result->oprs[operand].disp_size = 8;
			result->oprs[operand].eaflags |= EAF_BYTEOFFS;
			break;
		case P_A16:
		case P_A32:
		case P_A64:
			if (result->prefixes[PPS_ASIZE] &&
				result->prefixes[PPS_ASIZE] != tokval.t_integer)
			{
				//--auto clean flag
				aasm_log(_LOG_ERROR, "conflicting address size specifications");
			}
			else
				result->prefixes[PPS_ASIZE] = (prefixes)tokval.t_integer;
			break;
		case S_WORD:
			result->oprs[operand].disp_size = 16;
			result->oprs[operand].eaflags |= EAF_WORDOFFS;
			break;
		case S_DWORD:
		case S_LONG:
			result->oprs[operand].disp_size = 32;
			result->oprs[operand].eaflags |= EAF_WORDOFFS;
			break;
		case S_QWORD:
			result->oprs[operand].disp_size = 64;
			result->oprs[operand].eaflags |= EAF_WORDOFFS;
			break;
		default:
			//--auto clean flag
			aasm_log(_LOG_ERROR, "invalid size specification in effective address");
			break;
		}
	}
}

X86Instruction* X86Parser::ParseLine(int pass, char* buffer,
	X86Instruction* result, evalfunc evaluate)
{
	int operand;
	int critical;
	struct eval_hints hints;
	int j;
	bool recover;

	result->forw_ref = false;
	result->prefix_only = false;

	stdscan_reset();
	stdscan_bufptr = buffer;
	i = stdscan(NULL, &tokval);

	result->operands = 0;       /* must initialize this */

	if (i == 0) {               /* blank line - ignore */
		result->opcode = I_none;    /* and no instruction either */
		return result;
	}
	if (i != TOKEN_INSN && i != TOKEN_PREFIX &&
		(i != TOKEN_REG || (REG_SREG & ~aasm_reg_flags[tokval.t_integer]))) {
		//--auto clean flag
		aasm_log(_LOG_ERROR, "instruction expected at start of line");
		result->opcode = I_none;
		return result;
	}

	for (j = 0; j < MAXPREFIX; j++)
		result->prefixes[j] = P_none;
	result->times = 1L;

	while (i == TOKEN_PREFIX ||
		(i == TOKEN_REG && !(REG_SREG & ~aasm_reg_flags[tokval.t_integer])))
	{
		int slot = prefix_slot((prefixes)tokval.t_integer);
		if (result->prefixes[slot]) {
			if (result->prefixes[slot] == tokval.t_integer)
			{
				//--auto clean flag
				aasm_log(_LOG_WARN, "instruction has redundant prefixes");
			}
			else
			{
				//--auto clean flag
				aasm_log(_LOG_ERROR, "instruction has conflicting prefixes");
			}
		}
		result->prefixes[slot] = (prefixes)tokval.t_integer;
		i = stdscan(NULL, &tokval);
	}

	if (i != TOKEN_INSN) {
		int j;
		enum prefixes pfx;

		for (j = 0; j < MAXPREFIX; j++)
			if ((pfx = result->prefixes[j]) != P_none)
				break;

		if (i == 0 && pfx != P_none) {
			result->opcode = I_none;
			result->prefix_only = true;
			return result;
		}
		else {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "parser: instruction expected");
			result->opcode = I_none;
			return result;
		}
	}

	result->opcode = (_asm_opcode)tokval.t_integer;
	result->condition = (ccode)tokval.t_inttwo;

	critical = (pass == 2 ? 2 : 0);

	/* right. Now we begin to parse the operands. There may be up to four
	 * of these, separated by commas, and terminated by a zero token. */

	for (operand = 0; operand < MAX_OPERANDS; operand++) {
		expr* value;		/* used most of the time */
		int mref;               /* is this going to be a memory ref? */
		int bracket;            /* is it a [] mref, or a & mref? */
		int setsize = 0;

		result->oprs[operand].disp_size = 0;    /* have to zero this whatever */
		result->oprs[operand].eaflags = 0;      /* and this */
		result->oprs[operand].opflags = 0;

		i = stdscan(NULL, &tokval);
		if (i == 0)
			break;              /* end of operands: get out of here */
		result->oprs[operand].type = 0; /* so far, no override */
		while (i == TOKEN_SPECIAL) {    /* size specifiers */
			switch ((int)tokval.t_integer) {
			case S_BYTE:
				if (!setsize)   /* we want to use only the first */
					result->oprs[operand].type |= BITS8;
				setsize = 1;
				break;
			case S_WORD:
				if (!setsize)
					result->oprs[operand].type |= BITS16;
				setsize = 1;
				break;
			case S_DWORD:
			case S_LONG:
				if (!setsize)
					result->oprs[operand].type |= BITS32;
				setsize = 1;
				break;
			case S_QWORD:
				if (!setsize)
					result->oprs[operand].type |= BITS64;
				setsize = 1;
				break;
			case S_TWORD:
				if (!setsize)
					result->oprs[operand].type |= BITS80;
				setsize = 1;
				break;
			case S_OWORD:
				if (!setsize)
					result->oprs[operand].type |= BITS128;
				setsize = 1;
				break;
			case S_YWORD:
				if (!setsize)
					result->oprs[operand].type |= BITS256;
				setsize = 1;
				break;
			case S_TO:
				result->oprs[operand].type |= TO;
				break;
			case S_STRICT:
				result->oprs[operand].type |= STRICT;
				break;
			case S_FAR:
				result->oprs[operand].type |= FAR;
				break;
			case S_NEAR:
				result->oprs[operand].type |= NEAR;
				break;
			case S_SHORT:
				result->oprs[operand].type |= SHORT;
				break;
			default:
				//--auto clean flag
				aasm_log(_LOG_ERROR, "invalid operand size specification");
			}
			i = stdscan(NULL, &tokval);
		}

		if (i == '[' || i == '&') {     /* memory reference */
			mref = true;
			bracket = (i == '[');
			i = stdscan(NULL, &tokval); /* then skip the colon */
			while (i == TOKEN_SPECIAL || i == TOKEN_PREFIX) {
				process_size_override(result, operand);
				i = stdscan(NULL, &tokval);
			}
		}
		else {                /* immediate operand, or register */
			mref = false;
			bracket = false;    /* placate optimisers */
		}

		if ((result->oprs[operand].type & FAR) && !mref &&
			result->opcode != I_JMP && result->opcode != I_CALL) {
			//--auto clean flag
			aasm_log(_LOG_ERROR, "invalid use of FAR operand specifier");
		}

		value = evaluate(stdscan, NULL, &tokval,
			&result->oprs[operand].opflags,
			critical, &hints);
		i = tokval.t_type;
		if (result->oprs[operand].opflags & OPFLAG_FORWARD) {
			result->forw_ref = true;
		}
		if (!value) {           /* error in evaluator */
			result->opcode = I_none;        /* unrecoverable parse error: */
			return result;      /* ignore this instruction */
		}
		if (i == ':' && mref) { /* it was seg:offset */
			/*
			 * Process the segment override.
			 */
			if (value[1].type != 0 || value->value != 1 ||
				REG_SREG & ~aasm_reg_flags[value->type]) {
				//--auto clean flag
				aasm_log(_LOG_ERROR, "invalid segment override");
			}
			else if (result->prefixes[PPS_SEG]) {
				//--auto clean flag
				aasm_log(_LOG_ERROR, "instruction has conflicting segment overrides");
			}
			else {
				result->prefixes[PPS_SEG] = (prefixes)value->type;
				if (!(REG_FSGS & ~aasm_reg_flags[value->type]))
					result->oprs[operand].eaflags |= EAF_FSGS;
			}

			i = stdscan(NULL, &tokval); /* then skip the colon */
			while (i == TOKEN_SPECIAL || i == TOKEN_PREFIX) {
				process_size_override(result, operand);
				i = stdscan(NULL, &tokval);
			}
			value = evaluate(stdscan, NULL, &tokval,
				&result->oprs[operand].opflags,
				critical, &hints);
			i = tokval.t_type;
			if (result->oprs[operand].opflags & OPFLAG_FORWARD) {
				result->forw_ref = true;
			}
			/* and get the offset */
			if (!value) {       /* but, error in evaluator */
				result->opcode = I_none;    /* unrecoverable parse error: */
				return result;  /* ignore this instruction */
			}
		}

		recover = false;
		if (mref && bracket) {  /* find ] at the end */
			if (i != ']') {
				//--auto clean flag
				aasm_log(_LOG_ERROR, "parser: expecting ]");
				recover = true;
			}
			else {            /* we got the required ] */
				i = stdscan(NULL, &tokval);
				if (i != 0 && i != ',') {
					//--auto clean flag
					aasm_log(_LOG_ERROR, "comma or end of line expected");
					recover = true;
				}
			}
		}
		else {                /* immediate operand */
			if (i != 0 && i != ',' && i != ':') {
				//--auto clean flag
				aasm_log(_LOG_ERROR, "comma, colon or end of line expected");
				recover = true;
			}
			else if (i == ':') {
				result->oprs[operand].type |= COLON;
			}
		}
		if (recover) {
			do {                /* error recovery */
				i = stdscan(NULL, &tokval);
			} while (i != 0 && i != ',');
		}

		/* now convert the exprs returned from evaluate() into operand
		 * descriptions... */

		if (mref) {             /* it's a memory reference */
			expr* e = value;
			int b, i, s;        /* basereg, indexreg, scale */
			int64_t o;             /* offset */

			b = i = -1, o = s = 0;
			result->oprs[operand].hintbase = hints.base;
			result->oprs[operand].hinttype = (eval_hint)hints.type;

			if (e->type && e->type <= EXPR_REG_END) {   /* this bit's a register */
				if (e->value == 1)      /* in fact it can be basereg */
					b = e->type;
				else            /* no, it has to be indexreg */
					i = e->type, s = e->value;
				e++;
			}
			if (e->type && e->type <= EXPR_REG_END) {   /* it's a 2nd register */
				if (b != -1)    /* If the first was the base, ... */
					i = e->type, s = e->value;  /* second has to be indexreg */

				else if (e->value != 1) {       /* If both want to be index */
					//--auto clean flag
					aasm_log(_LOG_ERROR, "beroset-p-592-invalid effective address");
					result->opcode = I_none;
					return result;
				}
				else
					b = e->type;
				e++;
			}
			if (e->type != 0) { /* is there an offset? */
				if (e->type <= EXPR_REG_END) {  /* in fact, is there an error? */
					//--auto clean flag
					aasm_log(_LOG_ERROR, "beroset-p-603-invalid effective address");
					result->opcode = I_none;
					return result;
				}
				else {
					if (e->type == EXPR_UNKNOWN) {
						result->oprs[operand].opflags |= OPFLAG_UNKNOWN;
						o = 0;  /* doesn't matter what */
						result->oprs[operand].wrt = NO_SEG;     /* nor this */
						result->oprs[operand].segment = NO_SEG; /* or this */
						while (e->type)
							e++;        /* go to the end of the line */
					}
					else {
						if (e->type == EXPR_SIMPLE) {
							o = e->value;
							e++;
						}
						if (e->type == EXPR_WRT) {
							result->oprs[operand].wrt = e->value;
							e++;
						}
						else
							result->oprs[operand].wrt = NO_SEG;
						/*
						 * Look for a segment base type.
						 */
						if (e->type && e->type < EXPR_SEGBASE) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "beroset-p-630-invalid effective address");
							result->opcode = I_none;
							return result;
						}
						while (e->type && e->value == 0)
							e++;
						if (e->type && e->value != 1) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "beroset-p-637-invalid effective address");
							result->opcode = I_none;
							return result;
						}
						if (e->type) {
							result->oprs[operand].segment =
								e->type - EXPR_SEGBASE;
							e++;
						}
						else
							result->oprs[operand].segment = NO_SEG;
						while (e->type && e->value == 0)
							e++;
						if (e->type) {
							//--auto clean flag
							aasm_log(_LOG_ERROR, "beroset-p-650-invalid effective address");
							result->opcode = I_none;
							return result;
						}
					}
				}
			}
			else {
				o = 0;
				result->oprs[operand].wrt = NO_SEG;
				result->oprs[operand].segment = NO_SEG;
			}

			if (e->type != 0) { /* there'd better be nothing left! */
				//--auto clean flag
				aasm_log(_LOG_ERROR, "beroset-p-663-invalid effective address");
				result->opcode = I_none;
				return result;
			}

		/* It is memory, but it can match any r/m operand */
			result->oprs[operand].type |= MEMORY_ANY;

			if (b == -1 && (i == -1 || s == 0)) {
				int is_rel = globalbits == 64 &&
					!(result->oprs[operand].eaflags & EAF_ABS) &&
					((globalrel &&
						!(result->oprs[operand].eaflags & EAF_FSGS)) ||
						(result->oprs[operand].eaflags & EAF_REL));

				result->oprs[operand].type |= is_rel ? IP_REL : MEM_OFFS;
			}
			result->oprs[operand].basereg = (reg_enum)b;
			result->oprs[operand].indexreg = (reg_enum)i;
			result->oprs[operand].scale = s;
			result->oprs[operand].offset = o;
		}
		else {                /* it's not a memory reference */
			if (is_just_unknown(value)) {       /* it's immediate but unknown */
				result->oprs[operand].type |= IMMEDIATE;
				result->oprs[operand].opflags |= OPFLAG_UNKNOWN;
				result->oprs[operand].offset = 0;       /* don't care */
				result->oprs[operand].segment = NO_SEG; /* don't care again */
				result->oprs[operand].wrt = NO_SEG;     /* still don't care */

				if (optimizing >= 0 && !(result->oprs[operand].type & STRICT))
				{
					/* Be optimistic */
					result->oprs[operand].type |= SBYTE16 | SBYTE32 | SBYTE64;
				}
			}
			else if (is_reloc(value)) {       /* it's immediate */
				result->oprs[operand].type |= IMMEDIATE;
				result->oprs[operand].offset = reloc_value(value);
				result->oprs[operand].segment = reloc_seg(value);
				result->oprs[operand].wrt = reloc_wrt(value);
				if (is_simple(value)) {
					if (reloc_value(value) == 1)
						result->oprs[operand].type |= UNITY;
					if (optimizing >= 0 &&
						!(result->oprs[operand].type & STRICT)) {
						int64_t v64 = reloc_value(value);
						int32_t v32 = (int32_t)v64;
						int16_t v16 = (int16_t)v32;

						if (v64 >= -128 && v64 <= 127)
							result->oprs[operand].type |= SBYTE64;
						if (v32 >= -128 && v32 <= 127)
							result->oprs[operand].type |= SBYTE32;
						if (v16 >= -128 && v16 <= 127)
							result->oprs[operand].type |= SBYTE16;
					}
				}
			}
			else {            /* it's a register */
				unsigned int rs;

				if (value->type >= EXPR_SIMPLE || value->value != 1) {
					//--auto clean flag
					aasm_log(_LOG_ERROR, "invalid operand type");
					result->opcode = I_none;
					return result;
				}

						/*
						 * check that its only 1 register, not an expression...
						 */
				for (i = 1; value[i].type; i++)
					if (value[i].value) {
						//--auto clean flag
						aasm_log(_LOG_ERROR, "invalid operand type");
						result->opcode = I_none;
						return result;
					}

				/* clear overrides, except TO which applies to FPU regs */
				if (result->oprs[operand].type & ~TO) {
					/*
					 * we want to produce a warning iff the specified size
					 * is different from the register size
					 */
					rs = result->oprs[operand].type & SIZE_MASK;
				}
				else
					rs = 0;

				result->oprs[operand].type &= TO;
				result->oprs[operand].type |= REGISTER;
				result->oprs[operand].type |= aasm_reg_flags[value->type];
				result->oprs[operand].basereg = (reg_enum)value->type;

				if (rs && (result->oprs[operand].type & SIZE_MASK) != rs) {
					//--auto clean flag
					log_pass1 = 1;
					aasm_log(_LOG_WARN, "register size specification ignored");
				}
			}
		}
	}

	result->operands = operand; /* set operand count */

	/* clear remaining operands */
	while (operand < MAX_OPERANDS)
		result->oprs[operand++].type = 0;

	return result;
}


