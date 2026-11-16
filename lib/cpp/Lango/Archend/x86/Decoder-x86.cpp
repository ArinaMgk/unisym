// ASCII C++11 TAB4 LF
// Docutitle: x86 Instruction Decoder
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../../inc/c/stdinc.h"
#include "../../../../../inc/cpp/Lango/Archend/Archend-x86.hpp"
#include "../../../../../inc/cpp/Lango/Archend/x86/Decoder-x86.hpp"
extern "C" {
#include "../../../../../inc/cpp/Lango/Archend/x86/data/regdis.h"
}

namespace {

#define getu8(x) (*(uint8_t *)(x))
#if X86_MEMORY
/* Littleendian CPU which can handle unaligned references */
#define getu16(x) (*(uint16_t *)(x))
#define getu32(x) (*(uint32_t *)(x))
#define getu64(x) (*(uint64_t *)(x))
#else
static uint16_t getu16(uint8_t *data)
{
	return (uint16_t)data[0] + ((uint16_t)data[1] << 8);
}
static uint32_t getu32(uint8_t *data)
{
	return (uint32_t)getu16(data) + ((uint32_t)getu16(data+2) << 16);
}
static uint64_t getu64(uint8_t *data)
{
	return (uint64_t)getu32(data) + ((uint64_t)getu32(data+4) << 32);
}
#endif

#define gets8(x) ((int8_t)getu8(x))
#define gets16(x) ((int16_t)getu16(x))
#define gets32(x) ((int32_t)getu32(x))
#define gets64(x) ((int64_t)getu64(x))

/*
 * Determine whether the instruction template in t corresponds to the data
 * stream in data. Return the number of bytes matched if so.
 */
#define case4(x) case (x): case (x)+1: case (x)+2: case (x)+3

static int matches(const struct itemplate *t, uint8_t *data,
		   const X86DecodePrefixes *prefix, int segsize, insn *ins)
{
	uint8_t *r = (uint8_t *)(t->code);
	uint8_t *origdata = data;
	bool a_used = false, o_used = false;
	enum prefixes drep = P_none;
	enum prefixes dwait = P_none;
	uint8_t lock = prefix->lock;
	int osize = prefix->osize;
	int asize = prefix->asize;
	int i, c;
	int op1, op2;
	struct operand *opx, *opy;
	uint8_t opex = 0;
	int s_field_for = -1;	/* No 144/154 series code encountered */
	bool vex_ok = false;
	int regmask = (segsize == 64) ? 15 : 7;

	for (i = 0; i < MAX_OPERANDS; i++) {
	ins->oprs[i].segment = ins->oprs[i].disp_size =
		(segsize == 64 ? SEG_64BIT : segsize == 32 ? SEG_32BIT : 0);
	}
	ins->condition = C_none;
	ins->rex = prefix->rex;
	MemSet(ins->prefixes, 0, sizeof ins->prefixes);

	if (t->flags & (segsize == 64 ? IF_NOLONG : IF_LONG))
		return false;

	if (prefix->rep == 0xF2)
		drep = P_REPNE;
	else if (prefix->rep == 0xF3)
		drep = P_REP;

	dwait = prefix->wait ? P_WAIT : P_none;

	while ((c = *r++) != 0) {
	op1 = (c & 3) + ((opex & 1) << 2);
	op2 = ((c >> 3) & 3) + ((opex & 2) << 1);
	opx = &ins->oprs[op1];
	opy = &ins->oprs[op2];
	opex = 0;

	switch (c) {
	case 01:
	case 02:
	case 03:
	case 04:
			while (c--)
				if (*r++ != *data++)
					return false;
		break;

	case 05:
	case 06:
	case 07:
		opex = c;
		break;

	case4(010):
	{
			int t = *r++, d = *data++;
			if (d < t || d > t + 7)
				return false;
			else {
				opx->basereg = (reg_enum)((d - t) + (ins->rex & REX_B ? 8 : 0));
				opx->segment |= SEG_RMREG;
			}
		break;
	}

	case4(014):
	case4(0274):
			opx->offset = (int8_t)*data++;
			opx->segment |= SEG_SIGNED;
		break;

	case4(020):
			opx->offset = *data++;
		break;

	case4(024):
			opx->offset = *data++;
		break;

	case4(030):
			opx->offset = getu16(data);
		data += 2;
		break;

	case4(034):
		if (osize == 32) {
		opx->offset = getu32(data);
		data += 4;
		} else {
		opx->offset = getu16(data);
		data += 2;
		}
			if (segsize != asize)
				opx->disp_size = asize;
		break;

	case4(040):
	case4(0254):
			opx->offset = getu32(data);
		data += 4;
		break;

	case4(044):
		switch (asize) {
		case 16:
		opx->offset = getu16(data);
		data += 2;
		if (segsize != 16)
			opx->disp_size = 16;
		break;
		case 32:
		opx->offset = getu32(data);
		data += 4;
		if (segsize == 16)
			opx->disp_size = 32;
		break;
		case 64:
		opx->offset = getu64(data);
		opx->disp_size = 64;
		data += 8;
		break;
		}
		break;

	case4(050):
			opx->offset = gets8(data++);
			opx->segment |= SEG_RELATIVE;
		break;

	case4(054):
		opx->offset = getu64(data);
		data += 8;
		break;

	case4(060):
			opx->offset = gets16(data);
		data += 2;
			opx->segment |= SEG_RELATIVE;
			opx->segment &= ~SEG_32BIT;
		break;

	case4(064):
			opx->segment |= SEG_RELATIVE;
		if (osize == 16) {
		opx->offset = gets16(data);
		data += 2;
				opx->segment &= ~(SEG_32BIT|SEG_64BIT);
		} else if (osize == 32) {
		opx->offset = gets32(data);
		data += 4;
				opx->segment &= ~SEG_64BIT;
				opx->segment |= SEG_32BIT;
		}
			if (segsize != osize) {
				opx->type =
					(opx->type & ~SIZE_MASK)
					| ((osize == 16) ? BITS16 : BITS32);
			}
		break;

	case4(070):
			opx->offset = gets32(data);
		data += 4;
			opx->segment |= SEG_32BIT | SEG_RELATIVE;
		break;

	case4(0100):
	case4(0110):
	case4(0120):
	case4(0130):
	{
		int modrm = *data++;
			opx->segment |= SEG_RMREG;
			data = X86DecodeEffectiveAddress(
				data, modrm, asize, segsize, opy, ins);
		if (!data)
		return false;
		opx->basereg = (reg_enum)(((modrm >> 3) & 7) + (ins->rex & REX_R ? 8 : 0));
		break;
	}

	case4(0140):
		if (s_field_for == op1) {
		opx->offset = gets8(data);
		data++;
		} else {
		opx->offset = getu16(data);
		data += 2;
		}
		break;

	case4(0144):
	case4(0154):
		s_field_for = (*data & 0x02) ? op1 : -1;
		if ((*data++ & ~0x02) != *r++)
		return false;
		break;

	case4(0150):
		if (s_field_for == op1) {
		opx->offset = gets8(data);
		data++;
		} else {
		opx->offset = getu32(data);
		data += 4;
		}
		break;

	case4(0160):
		ins->rex |= REX_D;
		ins->drexdst = op1;
		break;

	case4(0164):
		ins->rex |= REX_D|REX_OC;
		ins->drexdst = op1;
		break;

	case 0171:
		data = X86DecodeDrex(data, ins);
		if (!data)
		return false;
		break;

	case 0172:
	{
		uint8_t ximm = *data++;
		c = *r++;
		ins->oprs[c >> 3].basereg = (reg_enum)((ximm >> 4) & regmask);
		ins->oprs[c >> 3].segment |= SEG_RMREG;
		ins->oprs[c & 7].offset = ximm & 15;
	}
	break;

	case 0173:
	{
		uint8_t ximm = *data++;
		c = *r++;

		if ((c ^ ximm) & 15)
		return false;

		ins->oprs[c >> 4].basereg = (reg_enum)((ximm >> 4) & regmask);
		ins->oprs[c >> 4].segment |= SEG_RMREG;
	}
	break;

	case 0174:
	{
		uint8_t ximm = *data++;
		c = *r++;

		ins->oprs[c].basereg = (reg_enum)((ximm >> 4) & regmask);
		ins->oprs[c].segment |= SEG_RMREG;
	}
	break;

	case4(0200):
	case4(0204):
	case4(0210):
	case4(0214):
	case4(0220):
	case4(0224):
	case4(0230):
	case4(0234):
	{
			int modrm = *data++;
			if (((modrm >> 3) & 07) != (c & 07))
				return false;   /* spare field doesn't match up */
			data = X86DecodeEffectiveAddress(
				data, modrm, asize, segsize, opy, ins);
		if (!data)
		return false;
		break;
	}

	case4(0260):
	{
		int vexm   = *r++;
		int vexwlp = *r++;
		ins->rex |= REX_V;
		if ((prefix->rex & (REX_V|REX_D|REX_P)) != REX_V)
		return false;

		if ((vexm & 0x1f) != prefix->vex_m)
		return false;

		switch (vexwlp & 030) {
		case 000:
		if (prefix->rex & REX_W)
			return false;
		break;
		case 010:
		if (!(prefix->rex & REX_W))
			return false;
		ins->rex &= ~REX_W;
		break;
		case 020:		/* VEX.W is a don't care */
		ins->rex &= ~REX_W;
		break;
		case 030:
		break;
		}

		if ((vexwlp & 007) != prefix->vex_lp)
		return false;

		opx->segment |= SEG_RMREG;
		opx->basereg = (reg_enum)prefix->vex_v;
		vex_ok = true;
		break;
	}

	case 0270:
	{
		int vexm   = *r++;
		int vexwlp = *r++;
		ins->rex |= REX_V;
		if ((prefix->rex & (REX_V|REX_D|REX_P)) != REX_V)
		return false;

		if ((vexm & 0x1f) != prefix->vex_m)
		return false;

		switch (vexwlp & 030) {
		case 000:
		if (ins->rex & REX_W)
			return false;
		break;
		case 010:
		if (!(ins->rex & REX_W))
			return false;
		break;
		default:
		break;		/* Need to do anything special here? */
		}

		if ((vexwlp & 007) != prefix->vex_lp)
		return false;

		if (prefix->vex_v != 0)
		return false;

		vex_ok = true;
		break;
	}

	case 0310:
			if (asize != 16)
				return false;
			else
				a_used = true;
		break;

	case 0311:
			if (asize != 32)
				return false;
			else
				a_used = true;
		break;

	case 0312:
			if (asize != segsize)
				return false;
			else
				a_used = true;
		break;

	case 0313:
		if (asize != 64)
		return false;
		else
		a_used = true;
		break;

	case 0314:
		if (prefix->rex & REX_B)
		return false;
		break;

	case 0315:
		if (prefix->rex & REX_X)
		return false;
		break;

	case 0316:
		if (prefix->rex & REX_R)
		return false;
		break;

	case 0317:
		if (prefix->rex & REX_W)
		return false;
		break;

	case 0320:
			if (osize != 16)
				return false;
			else
				o_used = true;
		break;

	case 0321:
			if (osize != 32)
				return false;
			else
				o_used = true;
		break;

	case 0322:
			if (osize != (segsize == 16) ? 16 : 32)
				return false;
			else
				o_used = true;
		break;

	case 0323:
		ins->rex |= REX_W;	/* 64-bit only instruction */
		osize = 64;
		o_used = true;
		break;

	case 0324:
		if (!(ins->rex & (REX_P|REX_W)) || osize != 64)
		return false;
		o_used = true;
		break;

	case 0325:
		ins->rex |= REX_NH;
		break;

	case 0330:
	{
			int t = *r++, d = *data++;
			if (d < t || d > t + 15)
				return false;
			else
				ins->condition = ccode(d - t);
		break;
	}

	case 0331:
			if (prefix->rep)
				return false;
		break;

	case 0332:
		if (prefix->rep != 0xF2)
		return false;
		drep = P_none;
		break;

	case 0333:
			if (prefix->rep != 0xF3)
				return false;
			drep = P_none;
		break;

	case 0334:
		if (lock) {
		ins->rex |= REX_R;
		lock = 0;
		}
		break;

	case 0335:
			if (drep == P_REP)
				drep = P_REPE;
		break;

	case 0336:
	case 0337:
		break;

	case 0340:
		return false;

	case 0341:
		if (prefix->wait != 0x9B)
		return false;
		dwait = P_none;
		break;

	case4(0344):
		ins->oprs[0].basereg = reg_enum((*data++ >> 3) & 7);
		break;

	case 0360:
		if (prefix->osp || prefix->rep)
		return false;
		break;

	case 0361:
		if (!prefix->osp || prefix->rep)
		return false;
		o_used = true;
		break;

	case 0362:
		if (prefix->osp || prefix->rep != 0xf2)
		return false;
		drep = P_none;
		break;

	case 0363:
		if (prefix->osp || prefix->rep != 0xf3)
		return false;
		drep = P_none;
		break;

	case 0364:
		if (prefix->osp)
		return false;
		break;

	case 0365:
		if (prefix->asp)
		return false;
		break;

	case 0366:
		if (!prefix->osp)
		return false;
		o_used = true;
		break;

	case 0367:
		if (!prefix->asp)
		return false;
		a_used = true;
		break;

	default:
		return false;	/* Unknown code */
	}
	}

	if (!vex_ok && (ins->rex & REX_V))
	return false;

	/* REX cannot be combined with DREX or VEX */
	if ((ins->rex & (REX_D|REX_V)) && (prefix->rex & REX_P))
	return false;

	/*
	 * Check for unused rep or a/o prefixes.
	 */
	for (i = 0; i < t->operands; i++) {
	if (ins->oprs[i].segment != SEG_RMREG)
		a_used = true;
	}

	if (lock) {
	if (ins->prefixes[PPS_LREP])
		return false;
	ins->prefixes[PPS_LREP] = P_LOCK;
	}
	if (drep) {
	if (ins->prefixes[PPS_LREP])
		return false;
		ins->prefixes[PPS_LREP] = drep;
	}
	ins->prefixes[PPS_WAIT] = dwait;
	if (!o_used) {
	if (osize != ((segsize == 16) ? 16 : 32)) {
		enum prefixes pfx = P_none;

		switch (osize) {
		case 16:
		pfx = P_O16;
		break;
		case 32:
		pfx = P_O32;
		break;
		case 64:
		pfx = P_O64;
		break;
		}

		if (ins->prefixes[PPS_OSIZE])
		return false;
		ins->prefixes[PPS_OSIZE] = pfx;
	}
	}
	if (!a_used && asize != segsize) {
	if (ins->prefixes[PPS_ASIZE])
		return false;
		ins->prefixes[PPS_ASIZE] = asize == 16 ? P_A16 : P_A32;
	}

	/* Fix: check for redundant REX prefixes */

	return data - origdata;
}

/* Condition names for disassembly, sorted by x86 code */
static const char * const condition_name[16] = {
	"o", "no", "c", "nc", "z", "nz", "na", "a",
	"s", "ns", "pe", "po", "l", "nl", "ng", "g"
};

int32 DecodeX86Buffer(byte* data, char* output, int outbufsize,
	int segsize, uint64 address, uint32 preference,
	bool& has_target, uint64& target)
{
	const struct itemplate * const *p, * const *best_p;
	const struct disasm_index *ix;
	uint8_t *dp;
	int length, best_length = 0;
	rostr segover;
	int i, slen, colon, n;
	uint8_t *origdata;
	int works;
	insn tmp_ins, ins;
	uint32_t goodness, best;
	int best_pref;
	X86DecodePrefixes prefix;
	bool end_prefix;

	MemSet(&ins, 0, sizeof ins);

	/*
	 * Scan for prefixes.
	 */
	MemSet(&prefix, 0, sizeof prefix);
	prefix.asize = segsize;
	prefix.osize = (segsize == 64) ? 32 : segsize;
	segover = NULL;
	origdata = data;

	ix = itable;

	end_prefix = false;
	while (!end_prefix) {
	switch (*data) {
	case 0xF2:
	case 0xF3:
			prefix.rep = *data++;
		break;

	case 0x9B:
		prefix.wait = *data++;
		break;

	case 0xF0:
			prefix.lock = *data++;
		break;

	case 0x2E:
		segover = "cs", prefix.seg = *data++;
		break;
	case 0x36:
		segover = "ss", prefix.seg = *data++;
		break;
	case 0x3E:
		segover = "ds", prefix.seg = *data++;
		break;
	case 0x26:
		segover = "es", prefix.seg = *data++;
		break;
	case 0x64:
		segover = "fs", prefix.seg = *data++;
		break;
	case 0x65:
		segover = "gs", prefix.seg = *data++;
		break;

	case 0x66:
		prefix.osize = (segsize == 16) ? 32 : 16;
		prefix.osp = *data++;
		break;
	case 0x67:
		prefix.asize = (segsize == 32) ? 16 : 32;
		prefix.asp = *data++;
		break;

	case 0xC4:
	case 0xC5:
		if (segsize == 64 || (data[1] & 0xc0) == 0xc0) {
		prefix.vex[0] = *data++;
		prefix.vex[1] = *data++;

		prefix.rex = REX_V;
		prefix.vex_c = RV_VEX;

		if (prefix.vex[0] == 0xc4) {
			prefix.vex[2] = *data++;
			prefix.rex |= (~prefix.vex[1] >> 5) & 7; /* REX_RXB */
			prefix.rex |= (prefix.vex[2] >> (7-3)) & REX_W;
			prefix.vex_m = prefix.vex[1] & 0x1f;
			prefix.vex_v = (~prefix.vex[2] >> 3) & 15;
			prefix.vex_lp = prefix.vex[2] & 7;
		} else {
			prefix.rex |= (~prefix.vex[1] >> (7-2)) & REX_R;
			prefix.vex_m = 1;
			prefix.vex_v = (~prefix.vex[1] >> 3) & 15;
			prefix.vex_lp = prefix.vex[1] & 7;
		}

		ix = itable_vex[RV_VEX][prefix.vex_m][prefix.vex_lp];
		}
		end_prefix = true;
		break;

	case 0x8F:
		if ((data[1] & 030) != 0 &&
		(segsize == 64 || (data[1] & 0xc0) == 0xc0)) {
		prefix.vex[0] = *data++;
		prefix.vex[1] = *data++;
		prefix.vex[2] = *data++;

		prefix.rex = REX_V;
		prefix.vex_c = RV_XOP;

		prefix.rex |= (~prefix.vex[1] >> 5) & 7; /* REX_RXB */
		prefix.rex |= (prefix.vex[2] >> (7-3)) & REX_W;
		prefix.vex_m = prefix.vex[1] & 0x1f;
		prefix.vex_v = (~prefix.vex[2] >> 3) & 15;
		prefix.vex_lp = prefix.vex[2] & 7;

		ix = itable_vex[RV_XOP][prefix.vex_m][prefix.vex_lp];
		}
		end_prefix = true;
		break;

	case REX_P + 0x0:
	case REX_P + 0x1:
	case REX_P + 0x2:
	case REX_P + 0x3:
	case REX_P + 0x4:
	case REX_P + 0x5:
	case REX_P + 0x6:
	case REX_P + 0x7:
	case REX_P + 0x8:
	case REX_P + 0x9:
	case REX_P + 0xA:
	case REX_P + 0xB:
	case REX_P + 0xC:
	case REX_P + 0xD:
	case REX_P + 0xE:
	case REX_P + 0xF:
		if (segsize == 64) {
		prefix.rex = *data++;
		if (prefix.rex & REX_W)
			prefix.osize = 64;
		}
		end_prefix = true;
		break;

	default:
		end_prefix = true;
		break;
	}
	}

	best = -1;			/* Worst possible */
	best_p = NULL;
	best_pref = INT_MAX;

	if (!ix)
	return 0;		/* No instruction table at all... */

	dp = data;
	ix += *dp++;
	while (ix->n == -1) {
	ix = (const struct disasm_index *)ix->p + *dp++;
	}

	p = (const struct itemplate * const *)ix->p;
	for (n = ix->n; n; n--, p++) {
		if ((length = matches(*p, data, &prefix, segsize, &tmp_ins))) {
			works = true;
			/*
			 * Final check to make sure the types of r/m match up.
		 * XXX: Need to make sure this is actually correct.
			 */
			for (i = 0; i < (*p)->operands; i++) {
				if (!((*p)->opd[i] & SAME_AS) &&
			(
			/* If it's a mem-only EA but we have a
			   register, die. */
			((tmp_ins.oprs[i].segment & SEG_RMREG) &&
			 !(MEMORY & ~(*p)->opd[i])) ||
			/* If it's a reg-only EA but we have a memory
			   ref, die. */
			(!(tmp_ins.oprs[i].segment & SEG_RMREG) &&
			 !(REG_EA & ~(*p)->opd[i]) &&
			 !((*p)->opd[i] & REG_SMASK)) ||
			/* Register type mismatch (eg FS vs REG_DESS):
			   die. */
			((((*p)->opd[i] & (REGISTER | FPUREG)) ||
			  (tmp_ins.oprs[i].segment & SEG_RMREG)) &&
			 !X86DecodeRegister((*p)->opd[i],
				   tmp_ins.oprs[i].basereg, tmp_ins.rex))
			)) {
					works = false;
					break;
				}
			}

		/*
		 * Note: we always prefer instructions which incorporate
		 * prefixes in the instructions themselves.  This is to allow
		 * e.g. PAUSE to be preferred to REP NOP, and deal with
		 * MMX/SSE instructions where prefixes are used to select
		 * between MMX and SSE register sets or outright opcode
		 * selection.
		 */
			if (works) {
		int i, nprefix;
				goodness = ((*p)->flags & IF_PFMASK) ^ preference;
		nprefix = 0;
		for (i = 0; i < MAXPREFIX; i++)
			if (tmp_ins.prefixes[i])
			nprefix++;
				if (nprefix < best_pref ||
			(nprefix == best_pref && goodness < best)) {
					/* This is the best one found so far */
					best = goodness;
					best_p = p;
			best_pref = nprefix;
					best_length = length;
					ins = tmp_ins;
				}
			}
		}
	}

	if (!best_p)
		return 0;               /* no instruction was matched */

	/* Pick the best match */
	p = best_p;
	length = best_length;

	slen = 0;

	/* TODO: snprintf returns the value that the string would have if
	 *      the buffer were long enough, and not the actual length of
	 *      the returned string, so each instance of using the return
	 *      value of snprintf should actually be checked to assure that
	 *      the return value is "sane."  Maybe a macro wrapper could
	 *      be used for that purpose.
	 */
	for (i = 0; i < MAXPREFIX; i++) {
	const char *prefix = prefix_name(ins.prefixes[i]);
	if (prefix)
		slen += snprintf(output+slen, outbufsize-slen, "%s ", prefix);
	}

	i = (*p)->opcode;
	if (i >= _AASM_INST_FIRST_COND_OPCODE)
	slen += snprintf(output + slen, outbufsize - slen, "%s%s",
			 _asm_inst_literal_list[i], condition_name[ins.condition]);
	else
		slen += snprintf(output + slen, outbufsize - slen, "%s",
			 _asm_inst_literal_list[i]);

	colon = false;
	length += data - origdata;  /* fix up for prefixes */
	for (i = 0; i < (*p)->operands; i++) {
	opflags_t t = (*p)->opd[i];
	const operand *o = &ins.oprs[i];
	int64_t offs;

	if (t & SAME_AS) {
		o = &ins.oprs[t & ~SAME_AS];
		t = (*p)->opd[t & ~SAME_AS];
	}

		output[slen++] = (colon ? ':' : i == 0 ? ' ' : ',');

	offs = o->offset;
		if (o->segment & SEG_RELATIVE) {
			offs += address + length;
			/*
			 * sort out wraparound
			 */
			if (!(o->segment & (SEG_32BIT|SEG_64BIT)))
		offs &= 0xffff;
		else if (segsize != 64)
		offs &= 0xffffffff;

				if (!has_target) {
					has_target = true;
					target = static_cast<uint64>(offs);
				}
		}

		if (t & COLON)
			colon = true;
		else
			colon = false;

		if ((t & (REGISTER | FPUREG)) ||
			(o->segment & SEG_RMREG)) {
		enum reg_enum reg;
			reg = X86DecodeRegister(t, o->basereg, ins.rex);
			if (t & TO)
				slen += snprintf(output + slen, outbufsize - slen, "to ");
			slen += snprintf(output + slen, outbufsize - slen, "%s",
							 aasm_reg_names[reg-EXPR_REG_START]);
		} else if (!(UNITY & ~t)) {
			output[slen++] = '1';
		} else if (t & IMMEDIATE) {
			if (t & BITS8) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "byte ");
				if (o->segment & SEG_SIGNED) {
					if (offs < 0) {
						offs *= -1;
						output[slen++] = '-';
					} else
						output[slen++] = '+';
				}
			} else if (t & BITS16) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "word ");
			} else if (t & BITS32) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "dword ");
			} else if (t & BITS64) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "qword ");
			} else if (t & NEAR) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "near ");
			} else if (t & SHORT) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "short ");
			}
			slen += snprintf(output + slen, outbufsize - slen, "0x%" PRIx64 "", offs);
		} else if (!(MEM_OFFS & ~t)) {
			slen +=
				snprintf(output + slen, outbufsize - slen,
			 "[%s%s%s0x%" PRIx64 "]",
						 (segover ? segover : ""),
						 (segover ? ":" : ""),
			 (o->disp_size == 64 ? "qword " :
			  o->disp_size == 32 ? "dword " :
			  o->disp_size == 16 ? "word " : ""), offs);
			segover = NULL;
		} else if (!(REGMEM & ~t)) {
			int started = false;
			if (t & BITS8)
				slen += snprintf(output + slen, outbufsize - slen, "byte ");
			if (t & BITS16)
				slen +=
					snprintf(output + slen, outbufsize - slen, "word ");
			if (t & BITS32)
				slen +=
					snprintf(output + slen, outbufsize - slen, "dword ");
			if (t & BITS64)
				slen +=
					snprintf(output + slen, outbufsize - slen, "qword ");
			if (t & BITS80)
				slen +=
					snprintf(output + slen, outbufsize - slen, "tword ");
			if (t & BITS128)
				slen +=
					snprintf(output + slen, outbufsize - slen, "oword ");
			if (t & BITS256)
				slen +=
					snprintf(output + slen, outbufsize - slen, "yword ");
			if (t & FAR)
				slen += snprintf(output + slen, outbufsize - slen, "far ");
			if (t & NEAR)
				slen +=
					snprintf(output + slen, outbufsize - slen, "near ");
			output[slen++] = '[';
			if (o->disp_size)
				slen += snprintf(output + slen, outbufsize - slen, "%s",
								 (o->disp_size == 64 ? "qword " :
				  o->disp_size == 32 ? "dword " :
								  o->disp_size == 16 ? "word " :
				  ""));
		if (o->eaflags & EAF_REL)
		slen += snprintf(output + slen, outbufsize - slen, "rel ");
			if (segover) {
				slen +=
					snprintf(output + slen, outbufsize - slen, "%s:",
							 segover);
				segover = NULL;
			}
			if (o->basereg != -1) {
				slen += snprintf(output + slen, outbufsize - slen, "%s",
								 aasm_reg_names[(o->basereg-EXPR_REG_START)]);
				started = true;
			}
			if (o->indexreg != -1) {
				if (started)
					output[slen++] = '+';
				slen += snprintf(output + slen, outbufsize - slen, "%s",
								 aasm_reg_names[(o->indexreg-EXPR_REG_START)]);
				if (o->scale > 1)
					slen +=
						snprintf(output + slen, outbufsize - slen, "*%d",
								 o->scale);
				started = true;
			}


			if (o->segment & SEG_DISP8) {
		const char *prefix;
		uint8_t offset = offs;
		if ((int8_t)offset < 0) {
			prefix = "-";
			offset = -offset;
		} else {
			prefix = "+";
		}
				slen +=
					snprintf(output + slen, outbufsize - slen, "%s0x%" PRIx8 "",
				 prefix, offset);
			} else if (o->segment & SEG_DISP16) {
		const char *prefix;
		uint16_t offset = offs;
		if ((int16_t)offset < 0 && started) {
			offset = -offset;
			prefix = "-";
		} else {
			prefix = started ? "+" : "";
		}
				slen +=
					snprintf(output + slen, outbufsize - slen,
				 "%s0x%" PRIx16 "", prefix, offset);
			} else if (o->segment & SEG_DISP32) {
		if (prefix.asize == 64) {
			const char *prefix;
			uint64_t offset = (int64_t)(int32_t)offs;
			if ((int32_t)offs < 0 && started) {
			offset = -offset;
			prefix = "-";
			} else {
			prefix = started ? "+" : "";
			}
			slen +=
			snprintf(output + slen, outbufsize - slen,
				 "%s0x%" PRIx64 "", prefix, offset);
		} else {
			const char *prefix;
			uint32_t offset = offs;
			if ((int32_t) offset < 0 && started) {
			offset = -offset;
			prefix = "-";
			} else {
			prefix = started ? "+" : "";
			}
			slen +=
			snprintf(output + slen, outbufsize - slen,
				 "%s0x%" PRIx32 "", prefix, offset);
		}
			}
			output[slen++] = ']';
		} else {
			slen +=
				snprintf(output + slen, outbufsize - slen, "<operand%d>",
						 i);
		}
	}
	output[slen] = '\0';
	if (segover) {              /* unused segment override */
		char *p = output;
		int count = slen + 1;
		while (count--)
			p[count + 3] = p[count];
		StrCopyN(output, segover, 2);
		output[2] = ' ';
	}
	return length;
}

/*
 * This is called when we don't have a complete instruction.  If it
 * is a standalone *single-byte* prefix show it as such, otherwise
 * print it as a literal.
 */
int32 DecodeX86Byte(const byte* data,
	char* output, int outbufsize, int segsize)
{
	uint8_t byte = *data;
	const char *str = NULL;

	switch (byte) {
	case 0xF2:
	str = "repne";
	break;
	case 0xF3:
	str = "rep";
	break;
	case 0x9B:
	str = "wait";
	break;
	case 0xF0:
	str = "lock";
	break;
	case 0x2E:
	str = "cs";
	break;
	case 0x36:
	str = "ss";
	break;
	case 0x3E:
	str = "ss";
	break;
	case 0x26:
	str = "es";
	break;
	case 0x64:
	str = "fs";
	break;
	case 0x65:
	str = "gs";
	break;
	case 0x66:
	str = (segsize == 16) ? "o32" : "o16";
	break;
	case 0x67:
	str = (segsize == 32) ? "a16" : "a32";
	break;
	case REX_P + 0x0:
	case REX_P + 0x1:
	case REX_P + 0x2:
	case REX_P + 0x3:
	case REX_P + 0x4:
	case REX_P + 0x5:
	case REX_P + 0x6:
	case REX_P + 0x7:
	case REX_P + 0x8:
	case REX_P + 0x9:
	case REX_P + 0xA:
	case REX_P + 0xB:
	case REX_P + 0xC:
	case REX_P + 0xD:
	case REX_P + 0xE:
	case REX_P + 0xF:
	if (segsize == 64) {
		snprintf(output, outbufsize, "rex%s%s%s%s%s",
			 (byte == REX_P) ? "" : ".",
			 (byte & REX_W) ? "w" : "",
			 (byte & REX_R) ? "r" : "",
			 (byte & REX_X) ? "x" : "",
			 (byte & REX_B) ? "b" : "");
		break;
	}
	/* else fall through */
	default:
	snprintf(output, outbufsize, "db 0x%02x", byte);
	break;
	}

	if (str)
	StrCopy(output, str);

	return 1;
}

}

constexpr stduint X86MaximumInstructionBytes = 32;

int32 X86DecodeInstruction(const byte* data, stduint available,
	char* output, stduint output_capacity, int bits, uint64 address,
	uint32 preference, bool& need_more_data,
	bool& has_target, uint64& target)
{
	need_more_data = false;
	has_target = false;
	target = 0;
	if (!data || !available || !output || !output_capacity) return 0;

	byte staging[X86MaximumInstructionBytes] = {};
	byte* source = const_cast<byte*>(data);
	if (available < X86MaximumInstructionBytes) {
		MemCopyN(staging, data, available);
		source = staging;
	}

	int32 consumed = DecodeX86Buffer(source, output,
		static_cast<int>(output_capacity), bits, address, preference,
		has_target, target);
	if (consumed > static_cast<int32>(available)) {
		need_more_data = true;
		has_target = false;
		target = 0;
		return 0;
	}
	return consumed;
}

int32 X86DecodeByte(const byte* data,
	char* output, stduint output_capacity, int bits)
{
	if (!data || !output || !output_capacity) return 0;
	return DecodeX86Byte(data, output,
		static_cast<int>(output_capacity), bits);
}

namespace uni::lango {

namespace {

class X86DecoderInterface final : public ArchendDecodeInterface {
private:
	uint32 Preference = 0;

public:
	virtual const ArchendInfo& DumpArchend() const override
	{
		static const ArchendInfo info {
			"Intel and AMD x86 architecture",
			"x86",
			ArchitectureKind::X86,
			16,
			"16/32/64",
			"CPU levels: 8086, 186, 286, 386, 486, 586, 686, P3, P4, Prescott, x64, IA64"
		};
		return info;
	}

	virtual uint32 DefaultCpu() const override
	{
		return IF_PLEVEL;
	}

	virtual bool ParseCpu(const char* name, uint32& cpu) const override
	{
		cpu = X86CpuId(name);
		return cpu != IF_PLEVEL;
	}

	virtual bool ValidateBits(int bits, uint32 cpu) const override
	{
		switch (bits) {
		case 16:
			return true;
		case 32:
			return cpu >= IF_386;
		case 64:
			return cpu >= IF_X86_64;
		default:
			return false;
		}
	}

	virtual bool SupportsDisassembly() const override
	{
		return true;
	}

	virtual bool ConfigureDisassembly(
		const char* option, const char* value) override
	{
		if (StrCompareInsensitive(option, "vendor")) return false;
		if (!StrCompareInsensitive(value, "intel")) Preference = 0;
		else if (!StrCompareInsensitive(value, "amd"))
			Preference = IF_AMD | IF_3DNOW;
		else if (!StrCompareInsensitive(value, "cyrix"))
			Preference = IF_CYRIX | IF_3DNOW;
		else if (!StrCompareInsensitive(value, "idt") ||
			!StrCompareInsensitive(value, "centaur") ||
			!StrCompareInsensitive(value, "winchip"))
			Preference = IF_3DNOW;
		else return false;
		return true;
	}

	virtual bool DecodeInstruction(const ArchendDecodeRequest& request,
		char* output, stduint output_capacity,
		ArchendDecodeResult& result) const override
	{
		result = ArchendDecodeResult {};
		if (!request.data || !request.available ||
			!output || !output_capacity) return false;
		(void)request.cpu;

		bool need_more_data = false;
		bool has_target = false;
		uint64 target = 0;
		int32 consumed = X86DecodeInstruction(request.data, request.available,
			output, output_capacity, request.bits, request.address,
			Preference, need_more_data, has_target, target);
		if (consumed > 0) {
			result.consumed = consumed;
			result.instruction = true;
			result.has_target = has_target;
			result.target = target;
			return true;
		}

		consumed = X86DecodeByte(
			request.data, output, output_capacity, request.bits);
		if (consumed <= 0) return false;
		result.consumed = consumed;
		result.need_more_data = need_more_data;
		return true;
	}
};

}

ArchendDecodeInterface& X86Decoder()
{
	static X86DecoderInterface decoder;
	return decoder;
}

}
