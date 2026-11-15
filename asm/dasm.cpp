// ASCII C++11 TAB4 LF
// Docutitle: AASM Disassembler Command
// Codifiers: @ArinaMgk
// Copyright: UNISYM, under Apache License 2.0

#include <errno.h>
#include "inc/aasm.h"
#include "../inc/cpp/Lango/Archend.hpp"

//unuse
_ESYM_C{
int pass0, log_pass1, log_0file;
enum warn_t log_warnt;
}

_ESYM_C
char* strerror(int errnum);
_ESYM_C
void exit(int);


// sync
void init_sync(void);
void add_sync(uint32_t position, uint32_t length);
uint32_t next_sync(uint32_t position, uint32_t *length);

#define DASM_MAX_INSTRUCTION_BYTES 32
#define BPL 8

static void output_ins(uint32_t, uint8_t *, int, char *);
static void skip(uint32_t dist, FILE * fp);

static void PrintArchitectures(
	const uni::lango::ArchendDecodeInterface& selected)
{
	printf("Supported disassembly architectures and CPU/INST choices:\n");
	uni::lango::ListArchendDecoders(selected, stdout);
}

static void PrintInputFormats()
{
	printf("Supported input formats:\n"
		"  * bin       flat binary byte stream\n"
		"      Architectures: ");
	uni::lango::ListArchendDecoderNames(stdout);
	printf("\n");
}

static void PrintCodeendInformation()
{
	printf("Codeend output formats:\n"
		"  not applicable; DASM reads input formats selected by -f\n");
}

static void PrintHelp(const uni::lango::ArchendDecodeInterface& selected)
{
	printf(
		"usage: dasm [-f format] [-march=name] [-b bits] [options...] [--] file\n"
		"    or dasm -v   for version info\n\n"
		"    -a or -i    enable automatic instruction synchronization\n"
		"    -b bits     select the processor bit width\n"
		"    -e bytes    skip bytes at the beginning of the input\n"
		"    -f format   select the input format\n"
		"    -ha         list disassembly architectures and bit widths\n"
		"    -hf         list input formats and supported architectures\n"
		"    -k start,bytes\n"
		"                treat an input range as data\n"
		"    -march=name select the instruction architecture\n"
		"    --arch name select the instruction architecture\n"
		"    -o origin   set the origin address for raw binary input\n"
		"    -p vendor   select the preferred x86 vendor\n"
		"    -s offset   add an instruction synchronization point\n"
		"    -u          same as -b 32; -uu selects 64 bits\n"
		"    -v          display version information\n\n");
	PrintArchitectures(selected);
	printf("\n");
	PrintCodeendInformation();
	printf("\n");
	PrintInputFormats();
}

static char* RequireValue(const char* program, const char* option,
	char* attached, int& index, int argc, char** argv)
{
	if (attached && *attached == '=') attached++;
	if (attached && *attached) return attached;
	if (index + 1 < argc) return argv[++index];
	fprintf(stderr, "%s: option `%s' requires an argument\n",
		program, option);
	return nullptr;
}

static bool ParseUint32(const char* text, uint32_t& result)
{
	char* end;
	uint64 value = StrTokenULong(text, &end, 10);
	if (*end || value > UINT32_MAX) return false;
	result = static_cast<uint32>(value);
	return true;
}

int main(int argc, char **argv)
{
	byte buffer[DASM_MAX_INSTRUCTION_BYTES * 2], *p, *q;
	char outbuf[256];
	char *pname = *argv;
	char *filename = NULL;
	const char* vendor = "intel";
	uint32_t nextsync, synclen, initskip = 0L;
	int lenread;
	int32_t lendis;
	bool autosync = false;
	bool bits_explicit = false;
	bool stop_options = false;
	bool vendor_explicit = false;
	bool eof = false;
	bool rn_error;
	uint32 offset = 0;
	uni::lango::ArchendDecodeInterface* archend =
		&uni::lango::DefaultArchendDecoder();
	uint32 cpu = archend->DefaultCpu();
	int bits = archend->DumpArchend().default_bits;
	FILE *fp;

	init_sync();

	for (int index = 1; index < argc; index++) {
		char* argument = argv[index];
		if (!stop_options && !StrCompare(argument, "--")) {
			stop_options = true;
			continue;
		}
		if (!stop_options && argument[0] == '-' && argument[1]) {
			if (!StrCompareInsensitive(argument, "-h")) {
				PrintHelp(*archend);
				return 0;
			}
			if (!StrCompareInsensitive(argument, "-ha")) {
				PrintArchitectures(*archend);
				return 0;
			}
			if (!StrCompareInsensitive(argument, "-hf")) {
				PrintInputFormats();
				return 0;
			}
			if (!StrCompareInsensitive(argument, "-v") ||
				!StrCompareInsensitive(argument, "-r")) {
				printf("DASM version %s compiled on %s\n",
					aasm_version, aasm_date);
				return 0;
			}

			char* value = nullptr;
			if (!StrCompareInsensitive(argument, "-march") ||
				!StrCompareInsensitive(argument, "--arch"))
				value = RequireValue(pname, argument,
					nullptr, index, argc, argv);
			else if (!StrCompareNInsensitive(argument, "-march=", 7) ||
				!StrCompareNInsensitive(argument, "--arch=", 7))
				value = argument + 7;
			if (value) {
				uni::lango::ArchendDecodeInterface* selected =
					uni::lango::FindArchendDecoder(value);
				if (!selected || !selected->SupportsDisassembly()) {
					fprintf(stderr,
						"%s: unsupported disassembly architecture `%s'"
						" - use -ha for a list\n", pname, value);
					return 1;
				}
				archend = selected;
				cpu = archend->DefaultCpu();
				if (!bits_explicit) bits = archend->DumpArchend().default_bits;
				continue;
			}
			if (!StrCompareInsensitive(argument, "-march") ||
				!StrCompareInsensitive(argument, "--arch")) return 1;

			if (!StrCompareInsensitive(argument, "-a") ||
				!StrCompareInsensitive(argument, "-i")) {
				autosync = true;
				continue;
			}
			if (!StrCompareInsensitive(argument, "-u") ||
				!StrCompareInsensitive(argument, "-uu")) {
				bits = argument[2] == 'u' ? 64 : 32;
				bits_explicit = true;
				continue;
			}

			char option = argument[1];
			char* attached = argument[2] ? argument + 2 : nullptr;
			if (option == 'b' || option == 'B' || option == 'e' ||
				option == 'E' || option == 'f' || option == 'F' ||
				option == 'k' || option == 'K' || option == 'o' ||
				option == 'O' || option == 'p' || option == 'P' ||
				option == 's' || option == 'S') {
				value = RequireValue(pname, argument,
					attached, index, argc, argv);
				if (!value) return 1;
			}
			else {
				fprintf(stderr, "%s: unrecognised option `%s'\n",
					pname, argument);
				return 1;
			}

			switch (ascii_tolower(option)) {
			case 'b': {
				uint32 parsed;
				if (!ParseUint32(value, parsed) || parsed > INT_MAX) {
					fprintf(stderr, "%s: invalid bit width `%s'\n",
						pname, value);
					return 1;
				}
				bits = static_cast<int>(parsed);
				bits_explicit = true;
				break;
			}
			case 'e':
				if (!ParseUint32(value, initskip)) {
					fprintf(stderr, "%s: `-e' requires a 32-bit number\n", pname);
					return 1;
				}
				break;
			case 'f':
				if (StrCompareInsensitive(value, "bin")) {
					fprintf(stderr,
						"%s: unsupported input format `%s' - use -hf for a list\n",
						pname, value);
					return 1;
				}
				break;
			case 'k': {
				char* comma = (char*)StrIndexChar(value, ',');
				if (!comma) {
					fprintf(stderr,
						"%s: `-k' requires start,bytes\n", pname);
					return 1;
				}
				*comma++ = '\0';
				if (!ParseUint32(value, nextsync) ||
					!ParseUint32(comma, synclen)) {
					fprintf(stderr,
						"%s: `-k' requires 32-bit numbers\n", pname);
					return 1;
				}
				add_sync(nextsync, synclen);
				break;
			}
			case 'o': {
				int64 parsed = readnum(value, &rn_error);
				if (rn_error || parsed < 0 ||
					static_cast<uint64>(parsed) > UINT32_MAX) {
					fprintf(stderr,
						"%s: `-o' requires a 32-bit address\n", pname);
					return 1;
				}
				offset = static_cast<uint32>(parsed);
				break;
			}
			case 'p':
				vendor = value;
				vendor_explicit = true;
				break;
			case 's':
				if (!ParseUint32(value, nextsync)) {
					fprintf(stderr, "%s: `-s' requires a 32-bit address\n", pname);
					return 1;
				}
				add_sync(nextsync, 0);
				break;
			}
			continue;
		}

		if (!filename) filename = argument;
		else {
			fprintf(stderr, "%s: more than one filename specified\n", pname);
			return 1;
		}
	}

	if (!filename) {
		PrintHelp(*archend);
		return 0;
	}
	if (!archend->ValidateBits(bits, cpu)) {
		fprintf(stderr, "%s: architecture `%s' does not support %d-bit code\n",
			pname, archend->DumpArchend().shortname, bits);
		return 1;
	}
	if (vendor_explicit &&
		!archend->ConfigureDisassembly("vendor", vendor)) {
		fprintf(stderr,
			"%s: vendor `%s' is not supported by architecture `%s'\n",
			pname, vendor, archend->DumpArchend().shortname);
		return 1;
	}

	if (StrCompare(filename, "-")) {
		fp = fopen(filename, "rb");
		if (!fp) {
			fprintf(stderr, "%s: unable to open `%s': %s\n",
					pname, filename, strerror(errno));
			return 1;
		}
	} else
		fp = stdin;

	if (initskip > 0)
		skip(initskip, fp);

	/*
	 * This main loop is really horrible, and wants rewriting with
	 * an axe. It'll stay the way it is for a while though, until I
	 * find the energy...
	 */

	p = q = buffer;
	nextsync = next_sync(offset, &synclen);
	do {
		uint32_t to_read = buffer + sizeof(buffer) - p;
	if ((nextsync || synclen) &&
		to_read > nextsync - offset - (p - q))
			to_read = nextsync - offset - (p - q);
		if (to_read) {
			lenread = fread(p, 1, to_read, fp);
			if (lenread == 0)
				eof = true;     /* help along systems with bad feof */
		} else
			lenread = 0;
		p += lenread;
		if ((nextsync || synclen) &&
		(uint32_t)offset == nextsync) {
			if (synclen) {
				fprintf(stdout, "%08" PRIX32 "  skipping 0x%" PRIX32 " bytes\n",
			offset, synclen);
				offset += synclen;
				skip(synclen, fp);
			}
			p = q = buffer;
			nextsync = next_sync(offset, &synclen);
		}
		while (p > q &&
			(p - q >= DASM_MAX_INSTRUCTION_BYTES || lenread == 0)) {
			stduint available = static_cast<stduint>(p - q);
			if ((nextsync || synclen) && available > nextsync - offset)
				available = nextsync - offset;
			uni::lango::ArchendDecodeRequest request;
			request.data = q;
			request.available = available;
			request.address = offset;
			request.bits = bits;
			request.cpu = cpu;
			uni::lango::ArchendDecodeResult decoded;
			if (!archend->DecodeInstruction(
				request, outbuf, sizeof(outbuf), decoded) ||
				!decoded.consumed || decoded.consumed > available) {
				fprintf(stderr,
					"%s: architecture `%s' failed to decode input at 0x%08" PRIX32 "\n",
					pname, archend->DumpArchend().shortname, offset);
				if (fp != stdin) fclose(fp);
				return 1;
			}
			lendis = static_cast<int32>(decoded.consumed);
			if (autosync && decoded.has_target &&
				decoded.target <= UINT32_MAX)
				add_sync(static_cast<uint32>(decoded.target), 0);
			output_ins(offset, (uint8_t *) q, lendis, outbuf);
			q += lendis;
			offset += lendis;
		}
		if (q >= buffer + DASM_MAX_INSTRUCTION_BYTES) {
			uint8_t *r = (uint8_t *) buffer, *s = (uint8_t *) q;
			int count = p - q;
			while (count--)
				*r++ = *s++;
			p -= (q - buffer);
			q = buffer;
		}
	} while (lenread > 0 || !(eof || feof(fp)));

	if (fp != stdin)
		fclose(fp);

	return 0;
}

static void output_ins(uint32_t offset, uint8_t *data,
					   int datalen, char *insn)
{
	int bytes;
	fprintf(stdout, "%08" PRIX32 "  ", offset);

	bytes = 0;
	while (datalen > 0 && bytes < BPL) {
		fprintf(stdout, "%02X", *data++);
		bytes++;
		datalen--;
	}

	fprintf(stdout, "%*s%s\n", (BPL + 1 - bytes) * 2, "", insn);

	while (datalen > 0) {
		fprintf(stdout, "         -");
		bytes = 0;
		while (datalen > 0 && bytes < BPL) {
			fprintf(stdout, "%02X", *data++);
			bytes++;
			datalen--;
		}
		fprintf(stdout, "\n");
	}
}

/*
 * Skip a certain amount of data in a file, either by seeking if
 * possible, or if that fails then by reading and discarding.
 */
static void skip(uint32_t dist, FILE * fp)
{
	char buffer[256];           /* should fit on most stacks :-) */

	/*
	 * Got to be careful with fseek: at least one fseek I've tried
	 * doesn't approve of SEEK_CUR. So I'll use SEEK_SET and
	 * ftell... horrible but apparently necessary.
	 */
	if (fseek(fp, dist + ftell(fp), SEEK_SET)) {
		while (dist > 0) {
			uint32_t len = (dist < sizeof(buffer) ?
								 dist : sizeof(buffer));
			if (fread(buffer, 1, len, fp) < len) {
				perror("fread");
				exit(1);
			}
			dist -= len;
		}
	}
}

