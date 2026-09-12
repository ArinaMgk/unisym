// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: Ogg Vorbis Audio Codec Implementation
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

// These are chores. //{TODO} clean

#include "../../../../inc/c/format/audio/OGG.h"
#include "../../../../inc/c/arith.h"
#include "../../../../inc/c/ustring.h"

namespace ogg {

	// =========================================================================
	// 0. Diagnostic taps
	//
	// Rate limited on purpose -- a handful of lines per stream, never per packet:
	//
	//   [OGG] ident ...        once, from the identification header
	//   [OGG] setup ...        once, from the setup header
	//   [OGG] residue<N> ...   once per residue config
	//   [OGG] mapping<N> ...   once per mapping config
	//   [OGG] cls ...          once, class indices vs the cascade codebooks
	//   [OGG] pkt ...          first 2 packets of EACH block size only
	//
	// Values are printed as integer thousandths ("..._milli") so that only %u/%d/%s
	// are needed -- printlog's float formatting is not relied on.
	//
	// The taps are compiled out by defining OGG_DIAG as 0. Note that ploginfo
	// itself expands to nothing unless _DEBUG is defined.
	// =========================================================================
// Off by default: the per-packet dumps are noise, and Diag::NoteSample runs
// once per output sample (with two double multiplies for the lag-2
// autocorrelation), which costs real time on a slow machine. Build with
// -DOGG_DIAG=1 to get the packet dumps and the periodic quality report back.
// The one-time ident/setup/residue/mapping lines are printed outside this
// switch and always stay on.
#ifndef OGG_DIAG
#define OGG_DIAG 0
#endif

#ifndef OGG_RUNTIME_PCM_DIAG
#define OGG_RUNTIME_PCM_DIAG 0
#endif

#if OGG_DIAG
	namespace Diag {

		static uint32 shown_long = 0;
		static uint32 shown_short = 0;
		static uint32 shown_class = 0;
		static uint32 pkts = 0;

		static float pk_residue = 0.0f;
		static float pk_spectrum = 0.0f;
		static float pk_imdct = 0.0f;
		static float fmin = 1.0e30f;
		static float fmax = 0.0f;

		// Whole-stream accumulators, reported every kReportEvery packets (~3 minutes
		// of audio at 1024-sample long blocks). This is the ONLY line that keeps
		// printing while a track plays; the per-packet dumps above stop after two
		// long and two short packets.
		static const uint32 kReportEvery = 8192;
		static uint32 next_report = kReportEvery;
		static float run_X = 0.0f;      // max |X| (IMDCT input) over the whole stream
		static float run_X_long = 0.0f; // ... split by block size, because the missing
		static float run_X_short = 0.0f;//     normalisation is per block size (2 / n)
		static uint32 bs_long = 2048;
		static uint32 bs_short = 256;
		static float run_out = 0.0f;    // max |sample| after overlap-add
		static double run_band[3] = { 0.0, 0.0, 0.0 };   // sum |X| per third
		static double run_x2[3] = { 0.0, 0.0, 0.0 };     // sum X^2 per third (energy!)
		static double run_f2[3] = { 0.0, 0.0, 0.0 };     // sum F^2 per third (energy)
		static uint64 run_samples = 0;
		static uint64 run_clipped = 0;
		// time-domain / stereo statistics (see NoteSample)
		static double ac_num = 0.0, ac_den = 0.0;
		// Lag-2 history: in an interleaved stereo stream x[n] and x[n-2] are always
		// the SAME channel, whatever the interleave phase, so this is the reliable
		// "is it music or noise" number.
		static double ac2_num = 0.0, ac2_den = 0.0;
		static float h1 = 0.0f, h2 = 0.0f;
		static bool have_h1 = false, have_h2 = false;
		static double hi_num = 0.0, hi_den = 0.0;
		static double st_num = 0.0, st_den_l = 0.0, st_den_r = 0.0;
		static float prev_left = 0.0f, last_left = 0.0f;
		static bool have_left = false;

		// 2/n for the block each value came from: if that normalisation is the right
		// one, the printed Xnorm values land near 1000 milli (= 1.0 = full scale).

		static int32 Milli(float x) {
			if (x < 0.0f) x = -x;
			if (x > 2147483.0f) return 2147483647;
			return (int32)(x * 1000.0f + 0.5f);
		}
		// Same but keeps the sign, for values that can legitimately be negative.
		static int32 MilliS(float x) {
			float a = x < 0.0f ? -x : x;
			if (a > 2147483.0f) return x < 0.0f ? -2147483647 : 2147483647;
			int32 v = (int32)(a * 1000.0f + 0.5f);
			return x < 0.0f ? -v : v;
		}

		// 2/n for the block each value came from: if that normalisation is the right
		// one, the printed Xnorm values land near 1000 milli (= 1.0 = full scale).
		static int32 NormLong(float x) { return Milli(x * 2.0f / (float)bs_long); }
		static int32 NormShort(float x) { return Milli(x * 2.0f / (float)bs_short); }

		// Cross-correlation between the two spectrum channels, sampled before and
		// after the coupling step. If the coupling is doing its job, the "after"
		// value must come out clearly positive; if the decoder's output channels end
		// up uncorrelated, this says whether the coupling even ran.
		static double cc_pre_num = 0.0, cc_pre_l = 0.0, cc_pre_r = 0.0;
		static double cc_post_num = 0.0, cc_post_l = 0.0, cc_post_r = 0.0;
		static void NoteChannelPair(const float* a, const float* b, uint32 n, uint32 after) {
			for (uint32 i = 0; i < n; ++i) {
				double x = (double)a[i], y = (double)b[i];
				if (after) { cc_post_num += x * y; cc_post_l += x * x; cc_post_r += y * y; }
				else { cc_pre_num += x * y; cc_pre_l += x * x; cc_pre_r += y * y; }
			}
		}
		static uint32 Corr1000(double num, double l, double r) {
			double den = l + r;
			if (den <= 0.0) return 0;
			double c = 2000.0 * num / den;
			return (c >= 0.0) ? (uint32)(c + 0.5) : 0;
		}

		static int32 Micro(float x) {
			double v = (double)x * 1.0e6;
			if (v > 2147483000.0) return 2147483000;
			if (v < -2147483000.0) return -2147483000;
			return (int32)v;
		}

		// Dump 16 evenly spaced samples of a curve. 1000000 == 1.0. Used for the
		// floor envelope and the floor-multiplied spectrum, so their SHAPES can be
		// read directly instead of being guessed from sum-of-squares statistics
		// (which are dominated by the largest few values).
		static void DumpCurve(const char* tag, const float* p, uint32 n) {
			if ((shown_long >= 2) || (shown_short >= 2)) return;
			uint32 step = n / 16u;
			if (step == 0) step = 1;
			ploginfo("[OGG] %s n=%u : %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
				tag, (unsigned)n,
				(int)Micro(p[0]), (int)Micro(p[step]), (int)Micro(p[2 * step]), (int)Micro(p[3 * step]),
				(int)Micro(p[4 * step]), (int)Micro(p[5 * step]), (int)Micro(p[6 * step]), (int)Micro(p[7 * step]),
				(int)Micro(p[8 * step]), (int)Micro(p[9 * step]), (int)Micro(p[10 * step]), (int)Micro(p[11 * step]),
				(int)Micro(p[12 * step]), (int)Micro(p[13 * step]), (int)Micro(p[14 * step]), (int)Micro(p[15 * step]));
		}

		// Residue usage counters. A correct Vorbis decode writes a residue value into
		// essentially every spectrum bin, so "skipped partitions" and "no value
		// vector" both have to be near zero.
		static long long res_part_skipped = 0;   // partition had no book in any pass
		static long long res_part_used = 0;      // partition had a book
		static long long res_book_novalue = 0;   // chosen book has no value_vector
		static long long res_vals_written = 0;
		static long long res_vals_nonzero = 0;
		static int32 res_book_min = 9999;
		static int32 res_book_max = -1;
		static void NoteResidueBook(int used, int novalue, int32 idx) {
			if (used) ++res_part_used; else ++res_part_skipped;
			if (novalue) ++res_book_novalue;
			if (idx < res_book_min) res_book_min = idx;
			if (idx > res_book_max) res_book_max = idx;
		}
		static void NoteResidueValue(float v) {
			++res_vals_written;
			if (v != 0.0f) ++res_vals_nonzero;
		}
		static uint32 Pct100(long long part, long long whole) {
			if (whole <= 0) return 0;
			return (uint32)(10000ll * part / whole);
		}

		// Raw cascade fields as read, per classification: low3 / flag / high5 and the
		// cascade byte they compose to. This is the ground truth for the residue
		// stage->codebook mapping, which is what decides whether every partition can
		// actually carry residue.
		static uint32 cas_low[64];
		static uint32 cas_flag[64];
		static uint32 cas_high[64];
		static uint32 cas_n = 0;
		static uint32 cas_label = 0;
		static void CascadeAdd(uint32 i, uint32 low, uint32 flag, uint32 high) {
			if (i < 64) { cas_low[i] = low; cas_flag[i] = flag; cas_high[i] = high; }
			if (i + 1 > cas_n) cas_n = i + 1;
		}
		static void CascadeDump(uint32 label) {
			if (label != 0) return;   // first residue only, one line
			cas_label = 1;
			ploginfo("[OGG] cascade raw (class: low3 flag high5 -> byte), %u classes", (unsigned)cas_n);
			char buf[320];
			uint32 p = 0;
			for (uint32 i = 0; i < cas_n && i < 10 && p + 24 < sizeof(buf); ++i) {
				uint32 b = (cas_high[i] << 3) | cas_low[i];
				int n = 0;
				char tmp[24];
				uint32 v = i; do { tmp[n++] = (char)('0' + (v % 10)); v /= 10; } while (v);
				while (n) buf[p++] = tmp[--n];
				buf[p++] = ':';
				v = cas_low[i]; do { tmp[n++] = (char)('0' + (v % 10)); v /= 10; } while (v);
				while (n) buf[p++] = tmp[--n];
				buf[p++] = '/';
				buf[p++] = (char)('0' + (cas_flag[i] & 1));
				buf[p++] = '/';
				v = cas_high[i]; do { tmp[n++] = (char)('0' + (v % 10)); v /= 10; } while (v);
				while (n) buf[p++] = tmp[--n];
				buf[p++] = '=';
				v = b; do { tmp[n++] = (char)('0' + (v % 10)); v /= 10; } while (v);
				while (n) buf[p++] = tmp[--n];
				buf[p++] = ' ';
			}
			buf[p] = 0;
			ploginfo("[OGG] %s", buf);
		}

		// Packet-level trace: does the residue decode actually stay inside the packet?
		// If the bit reader walks past the end, getBits() silently returns 0, the
		// classbook then yields its shortest codeword forever (entry 0 -> class 0),
		// and partitions whose class has no cascade stage silently get no residue.
		static uint32 pkt_shown_long = 0, pkt_shown_short = 0;
		static void TracePacket(const uint8* d, uint32 len, uint32 bit_start, uint32 bit_end, uint32 is_long) {
			// Only real audio packets: the file opens with a run of 1-byte silent
			// packets, and tracing those tells us nothing.
			if (len <= 64) return;
			if (is_long ? (pkt_shown_long >= 1) : (pkt_shown_short >= 1)) return;
			if (is_long) ++pkt_shown_long; else ++pkt_shown_short;
			ploginfo("[OGG] pkt %s bytes=%u pktbits=%u residue_bit_in=%u residue_bit_out=%u (out>pktbits means past end)",
				is_long ? "long" : "short", (unsigned)len, (unsigned)(len * 8),
				(unsigned)bit_start, (unsigned)bit_end);
			char hex[160];
			uint32 p = 0;
			uint32 n = len < 24 ? len : 24;
			for (uint32 i = 0; i < n && p + 4 < sizeof(hex); ++i) {
				const char* H = "0123456789abcdef";
				hex[p++] = H[(d[i] >> 4) & 15];
				hex[p++] = H[d[i] & 15];
				hex[p++] = ' ';
			}
			hex[p] = 0;
			ploginfo("[OGG] pkt bytes: %s", hex);
		}

		static uint32 pkt_bit_in = 0;
		static uint32 floor_bit_in = 0, floor_bit_out = 0;
		static void PacketBitIn(uint32 b) { pkt_bit_in = b; }
		static void FloorBitIn(uint32 b) { floor_bit_in = b; }
		static void FloorBitOut(uint32 b) { floor_bit_out = b; }
		static void PacketBitOut(const uint8* d, uint32 len, uint32 b, uint32 is_long) {
			if (len <= 64) return;
			if (is_long ? (pkt_shown_long >= 1) : (pkt_shown_short >= 1)) return;
			ploginfo("[OGG] pktbits=%u header_bits=%u floor_bits=%u..%u(used %u) residue_bits=%u..%u",
				(unsigned)(len * 8), (unsigned)pkt_bit_in,
				(unsigned)floor_bit_in, (unsigned)floor_bit_out,
				(unsigned)(floor_bit_out - floor_bit_in), (unsigned)floor_bit_out, (unsigned)b);
			TracePacket(d, len, pkt_bit_in, b, is_long);
		}

		// Sanity of the sin/cos used to build the window and the IMDCT cosine table.
		// Expected: sin30=500 cos0=1000 cos90=0 cos180=-1000.
		static void ReportTrig(float s30, float c0, float c90, float c180) {
			ploginfo("[OGG] diag trig sin30=%d cos0=%d cos90=%d cos180=%d (expect 500 1000 0 -1000)",
				(int)MilliS(s30), (int)MilliS(c0), (int)MilliS(c90), (int)MilliS(c180));
		}
		// Sanity of the two generated tables. Direct path: w0 0/1000/0, w1mid=1000,
		// cos table 1000/0/-1000/0. FFT path (no cos table exists): the twiddle table
		// is checked at j=0 (1,0) and j=n/4 (0,1).
		static void ReportTables(float w0f, float w0m, float w0l, float w1m,
			float c0, float cn, float c2n, float c3n, int fast_path) {
			ploginfo("[OGG] diag win0=%d/%d/%d win1mid=%d (expect 0/1000/0 1000)",
				(int)MilliS(w0f), (int)MilliS(w0m), (int)MilliS(w0l), (int)MilliS(w1m));
			if (fast_path) {
				ploginfo("[OGG] diag imdct=fft costab=%d/%d/%d/%d (expect 1000/1000/0/0)",
					(int)MilliS(c0), (int)MilliS(cn), (int)MilliS(c2n), (int)MilliS(c3n));
			} else {
				ploginfo("[OGG] diag imdct=direct costab=%d/%d/%d/%d (expect 1000/0/-1000/0)",
					(int)MilliS(c0), (int)MilliS(cn), (int)MilliS(c2n), (int)MilliS(c3n));
			}
		}

		// Only the first packets of each block size get swept; after that every
		// Note* call returns immediately.
		static bool WantDetail() { return (shown_long < 2) || (shown_short < 2); }

		static void BeginPacket() {
			pk_residue = pk_spectrum = pk_imdct = 0.0f;
			fmin = 1.0e30f;
			fmax = 0.0f;
		}
		static void NoteResidue(float v) {
			if (!WantDetail()) return;
			float a = v < 0.0f ? -v : v;
			if (a > pk_residue) pk_residue = a;
		}
		static void NoteFloor(const float* f, uint32 n) {
			uint32 third = (n + 2u) / 3u;
			if (third == 0) third = 1;
			for (uint32 i = 0; i < n; ++i) {
				float v = f[i];
				if (v < fmin) fmin = v;
				if (v > fmax) fmax = v;
				uint32 b = (i / third) > 2u ? 2u : (i / third);
				run_f2[b] += (double)v * (double)v;
			}
		}
		// Accumulated for the whole stream (cheap: one pass over n2 floats per packet).
		static void NoteSpectrum(const float* s, uint32 n, uint32 is_long) {
			uint32 third = (n + 2u) / 3u;
			if (third == 0) third = 1;
			if (is_long) bs_long = n << 1; else bs_short = n << 1;
			for (uint32 i = 0; i < n; ++i) {
				float a = s[i] < 0.0f ? -s[i] : s[i];
				if (a > pk_spectrum) pk_spectrum = a;
				if (a > run_X) run_X = a;
				if (is_long) { if (a > run_X_long) run_X_long = a; }
				else { if (a > run_X_short) run_X_short = a; }
				run_band[(i / third) > 2u ? 2u : (i / third)] += (double)a;
				run_x2[(i / third) > 2u ? 2u : (i / third)] += (double)a * (double)a;
			}
		}
		static void NoteSample(float v) {
			float a = v < 0.0f ? -v : v;
			if (a > run_out) run_out = a;
			if (a >= 1.0f) ++run_clipped;
			++run_samples;

			// Channel-phase independent: pair with the sample two calls back.
			if (have_h2) {
				ac2_num += (double)h2 * (double)v;
				ac2_den += (double)h2 * (double)h2;
			}
			h2 = h1; h1 = v;
			have_h2 = have_h1; have_h1 = true;

			// Time-domain statistics on the left channel only. These separate
			// "music played wrong" from "noise shaped like music":
			//   ac  = lag-1 sample autocorrelation (music 800+, noise <=300)
			//   hi  = energy of the first difference / total energy, x1000
			//         (low-frequency-dominated audio is small, flat noise is ~1000+)
			//   st  = correlation between the two channels (music is strongly
			//         correlated; two independent noise streams are ~0)
			bool is_left = ((run_samples - 1ull) % 2ull) == 0ull;
			if (is_left) {
				if (have_left) {
					ac_num += (double)prev_left * (double)v;
					ac_den += (double)prev_left * (double)prev_left;
					hi_num += (double)(v - prev_left) * (double)(v - prev_left);
				}
				hi_den += (double)v * (double)v;
				prev_left = v;
				have_left = true;
				last_left = v;
			} else {
				st_num += (double)last_left * (double)v;
				st_den_l += (double)last_left * (double)last_left;
				st_den_r += (double)v * (double)v;
			}
		}
		static void NoteImdct(const float* t, uint32 n) {
			if (!WantDetail()) return;
			for (uint32 i = 0; i < n; ++i) {
				float a = t[i] < 0.0f ? -t[i] : t[i];
				if (a > pk_imdct) pk_imdct = a;
			}
		}
		static void EndPacket(uint32 is_long, uint32 n2) {
			++pkts;
			if (!(is_long ? (shown_long >= 2) : (shown_short >= 2))) {
				if (is_long) ++shown_long; else ++shown_short;
				ploginfo("[OGG] pkt%u %s n2=%u residue_max_milli=%d X_max_milli=%d floor_min_milli=%d floor_max_milli=%d imdct_max_milli=%d",
					(unsigned)pkts, is_long ? "long" : "short", (unsigned)n2,
					(int)Milli(pk_residue), (int)Milli(pk_spectrum),
					(int)Milli(fmin), (int)Milli(fmax), (int)Milli(pk_imdct));
			}
			if (pkts < next_report) return;
			next_report += kReportEvery;
			double bsum = run_band[0] + run_band[1] + run_band[2];
			uint32 lo = 0, mi = 0, xh = 0, clip = 0;
			if (bsum > 0.0) {
				lo = (uint32)(100.0 * run_band[0] / bsum + 0.5);
				mi = (uint32)(100.0 * run_band[1] / bsum + 0.5);
				xh = (uint32)(100.0 * run_band[2] / bsum + 0.5);
			}
			if (run_samples) clip = (uint32)(10000ull * run_clipped / run_samples);
			uint32 ac = (ac_den > 0.0) ? (uint32)(1000.0 * ac_num / ac_den) : 0;
			int32 ac_signed = (ac_den > 0.0) ? (int32)(1000.0 * ac_num / ac_den) : 0;
			int32 ac2 = (ac2_den > 0.0) ? (int32)(1000.0 * ac2_num / ac2_den) : 0;
			uint32 hf = (hi_den > 0.0) ? (uint32)(1000.0 * hi_num / hi_den) : 0;
			(void)ac; (void)ac_signed; (void)hf;
			// Energy (not |X| sums) per third: this is what actually determines the
			// spectrum of the output.
			uint32 elo = 0, emi = 0, ehi = 0, flo = 0, fmi = 0, fhi = 0;
			double esum = run_x2[0] + run_x2[1] + run_x2[2];
			if (esum > 0.0) {
				elo = (uint32)(100.0 * run_x2[0] / esum + 0.5);
				emi = (uint32)(100.0 * run_x2[1] / esum + 0.5);
				ehi = (uint32)(100.0 * run_x2[2] / esum + 0.5);
			}
			double fsum = run_f2[0] + run_f2[1] + run_f2[2];
			if (fsum > 0.0) {
				flo = (uint32)(100.0 * run_f2[0] / fsum + 0.5);
				fmi = (uint32)(100.0 * run_f2[1] / fsum + 0.5);
				fhi = (uint32)(100.0 * run_f2[2] / fsum + 0.5);
			}
			// Normalised cross-correlation without a square root: the Cauchy-Schwarz
			// bound makes 2*sum(L*R)/(sum L^2 + sum R^2) land in [-1, 1].
			uint32 st = 0;
			{
				double den = st_den_l + st_den_r;
				if (den > 0.0) {
					double c = 2000.0 * st_num / den;
					st = (c >= 0.0) ? (uint32)(c + 0.5) : 0;
				}
			}
			ploginfo("[OGG] sum pkts=%u XbandE=%u/%u/%u out_max_milli=%d clipped_pct_x100=%u autocorr2_x1000=%d stereo_x1000=%u corrPost_x1000=%u",
				(unsigned)pkts, (unsigned)elo, (unsigned)emi, (unsigned)ehi,
				(int)Milli(run_out), (unsigned)clip,
				(int)ac2, (unsigned)st,
				(unsigned)Corr1000(cc_post_num, cc_post_l, cc_post_r));
			ploginfo("[OGG] res parts_used=%lld parts_skipped=%lld no_value_book=%lld vals=%lld nonzero_x10000=%u book_idx_min=%d book_idx_max=%d",
				res_part_used, res_part_skipped, res_book_novalue, res_vals_written,
				(unsigned)Pct100(res_vals_nonzero, res_vals_written),
				(int)res_book_min, (int)res_book_max);
		}
		// One-shot: the class index of each partition and the codebook that the
		// cascade table then selects for pass 0. If the classbook digit order is
		// wrong these indices are a permutation of the intended ones, and the books
		// they select come out incoherent (wide-range books on loud partitions).
		static void TraceClass(uint32 partitions, uint32 cdim, uint32 centries, uint32 nclass,
			const uint8* idx, const int16 (*booktab)[8]) {
			if (shown_class || partitions < 8) return;
			shown_class = 1;
			ploginfo("[OGG] cls partitions=%u classbook_dim=%u entries=%u classifications=%u",
				(unsigned)partitions, (unsigned)cdim, (unsigned)centries, (unsigned)nclass);
			ploginfo("[OGG] cls idx=%u %u %u %u %u %u %u %u",
				(unsigned)idx[0], (unsigned)idx[1], (unsigned)idx[2], (unsigned)idx[3],
				(unsigned)idx[4], (unsigned)idx[5], (unsigned)idx[6], (unsigned)idx[7]);
			ploginfo("[OGG] cls pass0book=%d %d %d %d %d %d %d %d",
				(int)booktab[idx[0]][0], (int)booktab[idx[1]][0], (int)booktab[idx[2]][0], (int)booktab[idx[3]][0],
				(int)booktab[idx[4]][0], (int)booktab[idx[5]][0], (int)booktab[idx[6]][0], (int)booktab[idx[7]][0]);
		}
	}

#define OGG_DIAG_BEGIN()                       Diag::BeginPacket()
#define OGG_DIAG_RESIDUE(v)                    Diag::NoteResidue(v)
#define OGG_DIAG_FLOOR(p, n)                   Diag::NoteFloor(p, n)
#define OGG_DIAG_SPECTRUM(p, n, il)            Diag::NoteSpectrum(p, n, il)
#define OGG_DIAG_IMDCT(p, n)                   Diag::NoteImdct(p, n)
#define OGG_DIAG_END(is_long, n2)              Diag::EndPacket(is_long, n2)
#define OGG_DIAG_CLASS(p, d, e, n, i, t)       Diag::TraceClass(p, d, e, n, i, t)
#define OGG_DIAG_SAMPLE(v)                     Diag::NoteSample(v)
#define OGG_DIAG_TRIG(a, b, c, d)              Diag::ReportTrig(a, b, c, d)
#define OGG_DIAG_TABLES(a, b, c, d, e, f, g, h, i) Diag::ReportTables(a, b, c, d, e, f, g, h, i)
#define OGG_DIAG_CORR(a, b, n, after)          Diag::NoteChannelPair(a, b, n, after)
#define OGG_DIAG_DUMP(tag, p, n)               Diag::DumpCurve(tag, p, n)
#define OGG_DIAG_RESBOOK(u, nv, i)             Diag::NoteResidueBook(u, nv, i)
#define OGG_DIAG_RESVAL(v)                     Diag::NoteResidueValue(v)
#define OGG_DIAG_CASCADE(i, l, f, h)           Diag::CascadeAdd(i, l, f, h)
#define OGG_DIAG_CASCADE_DUMP(l)               Diag::CascadeDump(l)
#define OGG_DIAG_PKT(d, l, bs, be, il)         Diag::TracePacket(d, l, bs, be, il)
#define OGG_DIAG_BITIN(b)                      Diag::PacketBitIn(b)
#define OGG_DIAG_BITOUT(d, l, b, il)           Diag::PacketBitOut(d, l, b, il)
#define OGG_DIAG_FBITIN(b)                     Diag::FloorBitIn(b)
#define OGG_DIAG_FBITOUT(b)                    Diag::FloorBitOut(b)
#else
#define OGG_DIAG_BEGIN()
#define OGG_DIAG_RESIDUE(v)
#define OGG_DIAG_FLOOR(p, n)
#define OGG_DIAG_SPECTRUM(p, n, il)
#define OGG_DIAG_IMDCT(p, n)
#define OGG_DIAG_END(is_long, n2)
#define OGG_DIAG_CLASS(p, d, e, n, i, t)
#define OGG_DIAG_SAMPLE(v)
#define OGG_DIAG_TRIG(a, b, c, d)
#define OGG_DIAG_TABLES(a, b, c, d, e, f, g, h, i)
#define OGG_DIAG_CORR(a, b, n, after)
#define OGG_DIAG_DUMP(tag, p, n)
#define OGG_DIAG_RESBOOK(u, nv, i)
#define OGG_DIAG_RESVAL(v)
#define OGG_DIAG_CASCADE(i, l, f, h)
#define OGG_DIAG_CASCADE_DUMP(l)
#define OGG_DIAG_PKT(d, l, bs, be, il)
#define OGG_DIAG_BITIN(b)
#define OGG_DIAG_BITOUT(d, l, b, il)
#define OGG_DIAG_FBITIN(b)
#define OGG_DIAG_FBITOUT(b)
#endif

	// =========================================================================
	// 1. Bitstream Module: Vorbis LSB-first bit reader
	// =========================================================================
	namespace Bitstream {

		class BitReader {
		private:
			const uint8* m_data;
			uint32       m_byte_len;
			uint32       m_byte_pos;
			uint32       m_bit_pos;

		public:
			BitReader() : m_data(nullptr), m_byte_len(0), m_byte_pos(0), m_bit_pos(0) {}
			BitReader(const uint8* data, uint32 len)
				: m_data(data), m_byte_len(len), m_byte_pos(0), m_bit_pos(0) {}

			void init(const uint8* data, uint32 len) {
				m_data = data;
				m_byte_len = len;
				m_byte_pos = 0;
				m_bit_pos = 0;
			}

			bool isEof() const {
				return m_byte_pos >= m_byte_len;
			}

			uint32 getBitOffset() const {
				return (m_byte_pos << 3) + m_bit_pos;
			}

			uint32 getBitsLeft() const {
				if (m_byte_pos >= m_byte_len) return 0;
				uint32 total_bits = m_byte_len << 3;
				uint32 cur_bits = (m_byte_pos << 3) + m_bit_pos;
				return (total_bits > cur_bits) ? (total_bits - cur_bits) : 0;
			}

			uint32 getBit() {
				if (m_byte_pos >= m_byte_len) return 0;
				uint32 bit = (m_data[m_byte_pos] >> m_bit_pos) & 1;
				m_bit_pos++;
				if (m_bit_pos == 8) {
					m_bit_pos = 0;
					m_byte_pos++;
				}
				return bit;
			}

			uint32 getBits(uint32 count) {
				if (count == 0) return 0;
				if (count > 32) count = 32;

				uint32 result = 0;
				uint32 bits_read = 0;

				while (bits_read < count) {
					if (m_byte_pos >= m_byte_len) break;
					uint32 avail = 8 - m_bit_pos;
					uint32 take = count - bits_read;
					if (take > avail) take = avail;

					uint32 mask = (1u << take) - 1u;
					uint32 chunk = (m_data[m_byte_pos] >> m_bit_pos) & mask;
					result |= (chunk << bits_read);

					bits_read += take;
					m_bit_pos += take;
					if (m_bit_pos == 8) {
						m_bit_pos = 0;
						m_byte_pos++;
					}
				}
				return result;
			}

			int32 getBitsSigned(uint32 count) {
				uint32 val = getBits(count);
				if (count > 0 && count < 32) {
					if (val & (1u << (count - 1))) {
						val |= ~((1u << count) - 1u);
					}
				}
				return (int32)val;
			}
		};

		static inline uint32 ilog(uint32 v) {
			uint32 ret = 0;
			while (v > 0) {
				ret++;
				v >>= 1;
			}
			return ret;
		}

		static inline float float32_unpack(uint32 val) {
			int32 mantissa = (int32)(val & 0x1FFFFF);
			uint32 sign = val & 0x80000000;
			int32 exp = (int32)((val & 0x7FE00000) >> 21);
			if (sign) mantissa = -mantissa;
			
			double res = (double)mantissa;
			int32 e = exp - 788;
			if (e > 0) {
				while (e >= 30) { res *= (double)(1u << 30); e -= 30; }
				if (e > 0) res *= (double)(1u << e);
			} else if (e < 0) {
				int32 ne = -e;
				while (ne >= 30) { res /= (double)(1u << 30); ne -= 30; }
				if (ne > 0) res /= (double)(1u << ne);
			}
			return (float)res;
		}

	} // namespace Bitstream

	// =========================================================================
	// 2. Codebook Module: Vector Quantization & Huffman Trees
	// =========================================================================
	namespace Codebook {

		// Upper bound accepted for a codebook vector dimension. Real Vorbis books use
		// dimensions 1, 2, 4 or 8; this guard only keeps a corrupt stream from
		// overflowing the residue decode scratch vector.
		constexpr uint32 kMaxVectorDim = 64;

		struct VorbisCodebook {
			uint32  dimensions;
			uint32  entries;
			uint8*  codeword_lengths;
			uint32* reversed_codewords;
			float*  value_vector;
			uint32  lookup_type;
			uint32  lookup_values;
			uint32  sequence_p;
			float   min_value;
			float   delta_value;
			uint8   min_length;
			uint8   max_length;

			// Symbol lookup index: entries grouped by codeword length and sorted by
			// reversed codeword, so DecodeSymbol can binary search a length group
			// instead of scanning every entry for every candidate length.
			// Length L occupies [length_begin[L], length_begin[L + 1]).
			uint32* sorted_entries;
			uint32* sorted_codes;
			uint32  length_begin[34];

			VorbisCodebook() : dimensions(0), entries(0), codeword_lengths(nullptr),
				reversed_codewords(nullptr), value_vector(nullptr), lookup_type(0),
				lookup_values(0), sequence_p(0), min_value(0.0f), delta_value(0.0f),
				min_length(0), max_length(0),
				sorted_entries(nullptr), sorted_codes(nullptr) {
				for (uint32 i = 0; i < 34; ++i) length_begin[i] = 0;
			}

			void release(uni::trait::Malloc& alloc) {
				if (codeword_lengths) { alloc.deallocate(codeword_lengths); codeword_lengths = nullptr; }
				if (reversed_codewords) { alloc.deallocate(reversed_codewords); reversed_codewords = nullptr; }
				if (value_vector) { alloc.deallocate(value_vector); value_vector = nullptr; }
				if (sorted_entries) { alloc.deallocate(sorted_entries); sorted_entries = nullptr; }
				if (sorted_codes) { alloc.deallocate(sorted_codes); sorted_codes = nullptr; }
			}

			// Counting sort by codeword length, then insertion sort by reversed
			// codeword inside each length group (the groups are small).
			bool buildLookup(uni::trait::Malloc& alloc) {
				sorted_entries = (uint32*)alloc.allocate(entries * sizeof(uint32));
				sorted_codes = (uint32*)alloc.allocate(entries * sizeof(uint32));
				if (!sorted_entries || !sorted_codes) return false;

				for (uint32 l = 0; l < 34; ++l) length_begin[l] = 0;
				for (uint32 i = 0; i < entries; ++i) {
					uint32 l = codeword_lengths[i];
					if (l > 0 && l <= 32) length_begin[l + 1]++;
				}
				for (uint32 l = 1; l < 34; ++l) length_begin[l] += length_begin[l - 1];

				uint32 fill[33];
				for (uint32 l = 0; l < 33; ++l) fill[l] = length_begin[l];
				for (uint32 i = 0; i < entries; ++i) {
					uint32 l = codeword_lengths[i];
					if (l == 0 || l > 32) continue; // unused entry
					uint32 pos = fill[l]++;
					sorted_entries[pos] = i;
					sorted_codes[pos] = reversed_codewords[i];
				}

				for (uint32 l = 1; l <= 32; ++l) {
					uint32 lo = length_begin[l];
					uint32 hi = length_begin[l + 1];
					for (uint32 k = lo + 1; k < hi; ++k) {
						uint32 ck = sorted_codes[k];
						uint32 ek = sorted_entries[k];
						uint32 j = k;
						while (j > lo && sorted_codes[j - 1] > ck) {
							sorted_codes[j] = sorted_codes[j - 1];
							sorted_entries[j] = sorted_entries[j - 1];
							--j;
						}
						sorted_codes[j] = ck;
						sorted_entries[j] = ek;
					}
				}
				return true;
			}
		};

		static inline uint32 bit_reverse(uint32 val, uint32 bits) {
			uint32 res = 0;
			for (uint32 i = 0; i < bits; ++i) {
				res = (res << 1) | (val & 1);
				val >>= 1;
			}
			return res;
		}

		static inline uint32 lookup1_quantvals(uint32 entries, uint32 dim) {
			if (dim == 0 || entries == 0) return 0;
			if (dim == 1) return entries;
			uint32 r = 1;
			while (1) {
				uint32 test = 1;
				bool overflow = false;
				for (uint32 d = 0; d < dim; ++d) {
					uint64 next = (uint64)test * (uint64)(r + 1);
					if (next > (uint64)entries) {
						overflow = true;
						break;
					}
					test = (uint32)next;
				}
				if (overflow || test > entries) break;
				r++;
			}
			return r;
		}

		static bool ParseCodebook(Bitstream::BitReader& br, VorbisCodebook& book, uni::trait::Malloc& alloc) {
			uint32 sync = br.getBits(24);
			if (sync != 0x564342) return false; // 'BCV'

			book.dimensions = br.getBits(16);
			book.entries = br.getBits(24);
			if (book.dimensions == 0 || book.entries == 0) return false;

			book.codeword_lengths = (uint8*)alloc.allocate(book.entries);
			book.reversed_codewords = (uint32*)alloc.allocate(book.entries * sizeof(uint32));
			if (!book.codeword_lengths || !book.reversed_codewords) return false;

			uint32 ordered = br.getBit();
			if (!ordered) {
				uint32 sparse = br.getBit();
				for (uint32 i = 0; i < book.entries; ++i) {
					if (sparse) {
						if (br.getBit()) {
							book.codeword_lengths[i] = (uint8)(br.getBits(5) + 1);
						} else {
							book.codeword_lengths[i] = 0;
						}
					} else {
						book.codeword_lengths[i] = (uint8)(br.getBits(5) + 1);
					}
				}
			} else {
				uint32 current_entry = 0;
				uint32 current_length = br.getBits(5) + 1;
				while (current_entry < book.entries) {
					uint32 count = br.getBits(Bitstream::ilog(book.entries - current_entry));
					for (uint32 i = 0; i < count && current_entry < book.entries; ++i) {
						book.codeword_lengths[current_entry++] = (uint8)current_length;
					}
					current_length++;
				}
			}

			// Assign codewords exactly as libvorbis _make_words() does: entries are
			// taken in ENTRY ORDER, each one claiming the next free node at its own
			// length, and the marker[] tree is re-pointed (the "prune" loop) so that
			// a later, shorter codeword stays prefix-free.
			//
			// This used to assign codewords grouped by length (every length-1 entry,
			// then every length-2 entry, ...). That yields a DIFFERENT code set which
			// is still prefix-free, so each symbol still consumes the right number of
			// bits -- packets keep ending exactly on their last bit -- while every
			// decoded entry NAME is wrong. That single error produces the whole
			// symptom set seen on a real file: floor1 Y values come out flat and tiny
			// (the envelope never rises), most residue partitions resolve to class 0
			// (which has no cascade books, so those bands stay empty), and the output
			// is a near-silent, low-passed version of the track.
			uint32 marker[33];
			for (uint32 i = 0; i < 33; ++i) marker[i] = 0;
			book.min_length = 32;
			book.max_length = 0;
			{
				uint32 used_entries = 0;
				for (uint32 i = 0; i < book.entries; ++i) {
					uint32 length = book.codeword_lengths[i];
					if (length == 0 || length > 32) {
						book.reversed_codewords[i] = 0;
						continue;
					}

					uint32 entry = marker[length];
					if (length < 32 && (entry >> length) != 0) return false; // overpopulated tree
					book.reversed_codewords[i] = bit_reverse(entry, length);
					++used_entries;
					if (length < book.min_length) book.min_length = (uint8)length;
					if (length > book.max_length) book.max_length = (uint8)length;

					for (int32 j = (int32)length; j > 0; --j) {
						if (marker[j] & 1u) {
							if (j == 1) ++marker[1];
							else marker[j] = marker[j - 1] << 1;
							break;
						}
						++marker[j];
					}

					for (uint32 j = length + 1; j < 33; ++j) {
						if ((marker[j] >> 1) == entry) {
							entry = marker[j];
							marker[j] = marker[j - 1] << 1;
						} else {
							break;
						}
					}
				}
				if (used_entries == 0) book.min_length = 0;
			}

			if (!book.buildLookup(alloc)) return false;

			// Read vector quantization lookup table
			book.lookup_type = br.getBits(4);
			if (book.lookup_type == 1 || book.lookup_type == 2) {
				book.min_value = Bitstream::float32_unpack(br.getBits(32));
				book.delta_value = Bitstream::float32_unpack(br.getBits(32));
				uint32 value_bits = br.getBits(4) + 1;
				book.sequence_p = br.getBit();

				if (book.lookup_type == 1) {
					book.lookup_values = lookup1_quantvals(book.entries, book.dimensions);
				} else {
					book.lookup_values = book.entries * book.dimensions;
				}

				uint32* quant_vals = (uint32*)alloc.allocate(book.lookup_values * sizeof(uint32));
				if (!quant_vals) return false;
				for (uint32 i = 0; i < book.lookup_values; ++i) {
					quant_vals[i] = br.getBits(value_bits);
				}

				uint32 total_vectors = book.entries * book.dimensions;
				book.value_vector = (float*)alloc.allocate(total_vectors * sizeof(float));
				if (!book.value_vector) {
					alloc.deallocate(quant_vals);
					return false;
				}

				if (book.lookup_type == 1) {
					for (uint32 i = 0; i < book.entries; ++i) {
						float last = 0.0f;
						uint32 index_div = 1;
						for (uint32 j = 0; j < book.dimensions; ++j) {
							uint32 quant_idx = (i / index_div) % book.lookup_values;
							float val = (float)quant_vals[quant_idx] * book.delta_value + book.min_value + last;
							if (book.sequence_p) last = val;
							book.value_vector[i * book.dimensions + j] = val;
							index_div *= book.lookup_values;
						}
					}
				} else {
					for (uint32 i = 0; i < book.entries; ++i) {
						float last = 0.0f;
						for (uint32 j = 0; j < book.dimensions; ++j) {
							uint32 quant_idx = i * book.dimensions + j;
							float val = (float)quant_vals[quant_idx] * book.delta_value + book.min_value + last;
							if (book.sequence_p) last = val;
							book.value_vector[i * book.dimensions + j] = val;
						}
					}
				}

				alloc.deallocate(quant_vals);
			}

			return true;
		}

		static int32 DecodeSymbol(Bitstream::BitReader& br, const VorbisCodebook& book) {
			if (book.max_length == 0 || !book.sorted_codes) return -1;
			uint32 cur_code = 0;

			for (uint32 cur_len = 1; cur_len <= book.max_length; ++cur_len) {
				cur_code |= (br.getBit() << (cur_len - 1));

				if (cur_len >= book.min_length) {
					// Codewords of one length are unique, so at most one entry can
					// match: binary search the length group instead of scanning every
					// entry of the book for every candidate length.
					uint32 lo = book.length_begin[cur_len];
					uint32 hi = book.length_begin[cur_len + 1];
					while (lo < hi) {
						uint32 mid = (lo + hi) >> 1;
						if (book.sorted_codes[mid] < cur_code) lo = mid + 1;
						else hi = mid;
					}
					if (lo < book.length_begin[cur_len + 1] && book.sorted_codes[lo] == cur_code) {
						return (int32)book.sorted_entries[lo];
					}
				}
			}
			return -1;
		}

		static bool DecodeVector(Bitstream::BitReader& br, const VorbisCodebook& book, float* out_vector) {
			int32 symbol = DecodeSymbol(br, book);
			if (symbol < 0 || symbol >= (int32)book.entries || !book.value_vector) {
				for (uint32 d = 0; d < book.dimensions; ++d) out_vector[d] = 0.0f;
				return symbol >= 0;
			}
			const float* src = book.value_vector + (symbol * book.dimensions);
			for (uint32 d = 0; d < book.dimensions; ++d) {
				out_vector[d] = src[d];
				OGG_DIAG_RESIDUE(src[d]);
			}
			return true;
		}

	} // namespace Codebook

	// =========================================================================
	// 3. Floor Module: Floor 1 (Line-interpolated envelope)
	// =========================================================================
	namespace Floor {

		constexpr uint32 kMaxFloor1Partitions = 32;
		constexpr uint32 kMaxFloor1Classes = 16;
		constexpr uint32 kMaxFloor1XList = 128;

		struct Floor1Config {
			uint32 partitions;
			uint8  partition_class_list[kMaxFloor1Partitions];
			uint8  class_dim[kMaxFloor1Classes];
			uint8  class_subs[kMaxFloor1Classes];
			uint8  class_sub_books[kMaxFloor1Classes];
			int16  subclass_books[kMaxFloor1Classes][8];
			uint8  multiplier;
			uint8  rangebits;
			uint32 x_count;
			uint16 x_list[kMaxFloor1XList];
			uint16 sorted_order[kMaxFloor1XList];
			uint16 low_neighbor[kMaxFloor1XList];
			uint16 high_neighbor[kMaxFloor1XList];
		};

		static const float kFloor1InverseDbTable[256] = {
			1.00000000e-07f, 1.06524862e-07f, 1.13475462e-07f, 1.20879580e-07f,
			1.28766805e-07f, 1.37168662e-07f, 1.46118728e-07f, 1.55652773e-07f,
			1.65808902e-07f, 1.76627704e-07f, 1.88152418e-07f, 2.00429104e-07f,
			2.13506826e-07f, 2.27437852e-07f, 2.42277858e-07f, 2.58086154e-07f,
			2.74925920e-07f, 2.92864456e-07f, 3.11973458e-07f, 3.32329296e-07f,
			3.54013324e-07f, 3.77112205e-07f, 4.01718256e-07f, 4.27929818e-07f,
			4.55851648e-07f, 4.85595339e-07f, 5.17279765e-07f, 5.51031556e-07f,
			5.86985605e-07f, 6.25285606e-07f, 6.66084629e-07f, 7.09545732e-07f,
			7.55842612e-07f, 8.05160300e-07f, 8.57695899e-07f, 9.13659373e-07f,
			9.73274386e-07f, 1.03677920e-06f, 1.10442761e-06f, 1.17648999e-06f,
			1.25325434e-06f, 1.33502745e-06f, 1.42213615e-06f, 1.51492857e-06f,
			1.61377557e-06f, 1.71907220e-06f, 1.83123929e-06f, 1.95072513e-06f,
			2.07800725e-06f, 2.21359436e-06f, 2.35802834e-06f, 2.51188643e-06f,
			2.67578356e-06f, 2.85037474e-06f, 3.03635776e-06f, 3.23447591e-06f,
			3.44552101e-06f, 3.67033650e-06f, 3.90982089e-06f, 4.16493131e-06f,
			4.43668733e-06f, 4.72617506e-06f, 5.03455146e-06f, 5.36304900e-06f,
			5.71298054e-06f, 6.08574464e-06f, 6.48283108e-06f, 6.90582687e-06f,
			7.35642254e-06f, 7.83641897e-06f, 8.34773449e-06f, 8.89241265e-06f,
			9.47263031e-06f, 1.00907064e-05f, 1.07491110e-05f, 1.14504757e-05f,
			1.21976034e-05f, 1.29934802e-05f, 1.38412869e-05f, 1.47444118e-05f,
			1.57064643e-05f, 1.67312894e-05f, 1.78229830e-05f, 1.89859080e-05f,
			2.02247123e-05f, 2.15443469e-05f, 2.29500858e-05f, 2.44475472e-05f,
			2.60427160e-05f, 2.77419673e-05f, 2.95520924e-05f, 3.14803256e-05f,
			3.35343734e-05f, 3.57224450e-05f, 3.80532853e-05f, 4.05362096e-05f,
			4.31811414e-05f, 4.59986513e-05f, 4.89999998e-05f, 5.21971822e-05f,
			5.56029763e-05f, 5.92309938e-05f, 6.30957344e-05f, 6.72126441e-05f,
			7.15981764e-05f, 7.62698586e-05f, 8.12463616e-05f, 8.65475746e-05f,
			9.21946845e-05f, 9.82102604e-05f, 1.04618344e-04f, 1.11444547e-04f,
			1.18716150e-04f, 1.26462215e-04f, 1.34713700e-04f, 1.43503583e-04f,
			1.52866994e-04f, 1.62841354e-04f, 1.73466528e-04f, 1.84784980e-04f,
			1.96841945e-04f, 2.09685610e-04f, 2.23367307e-04f, 2.37941715e-04f,
			2.53467084e-04f, 2.70005462e-04f, 2.87622945e-04f, 3.06389946e-04f,
			3.26381467e-04f, 3.47677407e-04f, 3.70362879e-04f, 3.94528545e-04f,
			4.20270989e-04f, 4.47693091e-04f, 4.76904447e-04f, 5.08021805e-04f,
			5.41169527e-04f, 5.76480092e-04f, 6.14094622e-04f, 6.54163449e-04f,
			6.96846712e-04f, 7.42314998e-04f, 7.90750027e-04f, 8.42345376e-04f,
			8.97307249e-04f, 9.55855309e-04f, 1.01822355e-03f, 1.08466123e-03f,
			1.15543388e-03f, 1.23082435e-03f, 1.31113394e-03f, 1.39668362e-03f,
			1.48781530e-03f, 1.58489319e-03f, 1.68830529e-03f, 1.79846488e-03f,
			1.91581223e-03f, 2.04081633e-03f, 2.17397678e-03f, 2.31582577e-03f,
			2.46693021e-03f, 2.62789400e-03f, 2.79936046e-03f, 2.98201486e-03f,
			3.17658722e-03f, 3.38385515e-03f, 3.60464703e-03f, 3.83984528e-03f,
			4.09038989e-03f, 4.35728218e-03f, 4.64158883e-03f, 4.94444610e-03f,
			5.26706439e-03f, 5.61073307e-03f, 5.97682566e-03f, 6.36680529e-03f,
			6.78223055e-03f, 7.22476174e-03f, 7.69616748e-03f, 8.19833179e-03f,
			8.73326162e-03f, 9.30309490e-03f, 9.91010900e-03f, 1.05567299e-02f,
			1.12455420e-02f, 1.19792981e-02f, 1.27609308e-02f, 1.35935639e-02f,
			1.44805252e-02f, 1.54253595e-02f, 1.64318429e-02f, 1.75039980e-02f,
			1.86461097e-02f, 1.98627426e-02f, 2.11587592e-02f, 2.25393390e-02f,
			2.40099998e-02f, 2.55766192e-02f, 2.72454583e-02f, 2.90231869e-02f,
			3.09169098e-02f, 3.29341955e-02f, 3.50831063e-02f, 3.73722306e-02f,
			3.98107171e-02f, 4.24083114e-02f, 4.51753952e-02f, 4.81230274e-02f,
			5.12629886e-02f, 5.46078279e-02f, 5.81709133e-02f, 6.19664851e-02f,
			6.60097128e-02f, 7.03167555e-02f, 7.49048268e-02f, 7.97922634e-02f,
			8.49985985e-02f, 9.05446397e-02f, 9.64525526e-02f, 1.02745949e-01f,
			1.09449980e-01f, 1.16591440e-01f, 1.24198871e-01f, 1.32302676e-01f,
			1.40935243e-01f, 1.50131073e-01f, 1.59926918e-01f, 1.70361929e-01f,
			1.81477810e-01f, 1.93318987e-01f, 2.05932784e-01f, 2.19369614e-01f,
			2.33683178e-01f, 2.48930683e-01f, 2.65173067e-01f, 2.82475244e-01f,
			3.00906364e-01f, 3.20540089e-01f, 3.41454887e-01f, 3.63734348e-01f,
			3.87467512e-01f, 4.12749233e-01f, 4.39680551e-01f, 4.68369100e-01f,
			4.98929537e-01f, 5.31484001e-01f, 5.66162599e-01f, 6.03103928e-01f,
			6.42455627e-01f, 6.84374970e-01f, 7.29029493e-01f, 7.76597661e-01f,
			8.27269587e-01f, 8.81247787e-01f, 9.38747989e-01f, 1.00000000e+00f
		};

		static bool ParseFloor1(Bitstream::BitReader& br, Floor1Config& f1) {
			f1.partitions = br.getBits(5);
			if (f1.partitions > kMaxFloor1Partitions) return false;

			uint32 max_class = 0;
			for (uint32 i = 0; i < f1.partitions; ++i) {
				f1.partition_class_list[i] = (uint8)br.getBits(4);
				if (f1.partition_class_list[i] > max_class) max_class = f1.partition_class_list[i];
			}
			if (max_class >= kMaxFloor1Classes) return false;

			for (uint32 i = 0; i <= max_class; ++i) {
				f1.class_dim[i] = (uint8)(br.getBits(3) + 1);
				f1.class_subs[i] = (uint8)br.getBits(2);
				if (f1.class_subs[i] > 0) {
					f1.class_sub_books[i] = (uint8)br.getBits(8);
				} else {
					f1.class_sub_books[i] = 0;
				}
				for (uint32 j = 0; j < (1u << f1.class_subs[i]); ++j) {
					f1.subclass_books[i][j] = (int16)(br.getBits(8) - 1);
				}
			}

			f1.multiplier = (uint8)(br.getBits(2) + 1);
			f1.rangebits = (uint8)br.getBits(4);

			f1.x_count = 2;
			f1.x_list[0] = 0;
			f1.x_list[1] = (uint16)(1u << f1.rangebits);

			for (uint32 i = 0; i < f1.partitions; ++i) {
				uint32 cls = f1.partition_class_list[i];
				for (uint32 j = 0; j < f1.class_dim[cls]; ++j) {
					if (f1.x_count >= kMaxFloor1XList) return false;
					f1.x_list[f1.x_count++] = (uint16)br.getBits(f1.rangebits);
				}
			}

			for (uint32 i = 0; i < f1.x_count; ++i) {
				f1.sorted_order[i] = (uint16)i;
			}
			for (uint32 i = 0; i < f1.x_count; ++i) {
				for (uint32 j = i + 1; j < f1.x_count; ++j) {
					if (f1.x_list[f1.sorted_order[i]] > f1.x_list[f1.sorted_order[j]]) {
						uint16 tmp = f1.sorted_order[i];
						f1.sorted_order[i] = f1.sorted_order[j];
						f1.sorted_order[j] = tmp;
					}
				}
			}

			for (uint32 i = 2; i < f1.x_count; ++i) {
				uint32 low = 0;
				uint32 high = 1;
				uint32 low_x = f1.x_list[0];
				uint32 high_x = f1.x_list[1];
				uint32 cur_x = f1.x_list[i];

				for (uint32 j = 0; j < i; ++j) {
					uint32 j_x = f1.x_list[j];
					if (j_x < cur_x && j_x > low_x) {
						low_x = j_x;
						low = j;
					}
					if (j_x > cur_x && j_x < high_x) {
						high_x = j_x;
						high = j;
					}
				}
				f1.low_neighbor[i] = (uint16)low;
				f1.high_neighbor[i] = (uint16)high;
			}

			return true;
		}

		static bool ValidateFloor1Books(const Floor1Config& f1, uint32 codebook_count) {
			for (uint32 i = 0; i < f1.partitions; ++i) {
				uint32 cls = f1.partition_class_list[i];
				if (cls >= kMaxFloor1Classes) return false;
				if (f1.class_subs[cls] > 0 && f1.class_sub_books[cls] >= codebook_count) return false;
				for (uint32 j = 0; j < (1u << f1.class_subs[cls]); ++j) {
					if (f1.subclass_books[cls][j] >= 0 &&
						(uint32)f1.subclass_books[cls][j] >= codebook_count) {
						return false;
					}
				}
			}
			return true;
		}

		static inline int32 i_abs(int32 x) {
			return (x < 0) ? -x : x;
		}

		static inline int32 render_point(int32 x0, int32 y0, int32 x1, int32 y1, int32 x) {
			int32 dy = y1 - y0;
			int32 adx = x1 - x0;
			int32 err = i_abs(dy) * (x - x0);
			int32 off = err / adx;
			if (dy < 0) return y0 - off;
			return y0 + off;
		}

		static void render_line(int32 x0, int32 y0, int32 x1, int32 y1, float* out_curve, uint32 n) {
			int32 dy = y1 - y0;
			int32 adx = x1 - x0;
			int32 ady = i_abs(dy);
			int32 base = dy / adx;
			int32 err = 0;
			int32 sy = (dy < 0) ? (base - 1) : (base + 1);
			int32 sy_rem = ady - (i_abs(base) * adx);
			int32 cur_y = y0;

			for (int32 x = x0; x < x1 && x < (int32)n; ++x) {
				if (cur_y < 0) out_curve[x] = kFloor1InverseDbTable[0];
				else if (cur_y >= 256) out_curve[x] = kFloor1InverseDbTable[255];
				else out_curve[x] = kFloor1InverseDbTable[cur_y];

				err += sy_rem;
				if (err >= adx) {
					err -= adx;
					cur_y += sy;
				} else {
					cur_y += base;
				}
			}
		}

		static bool DecodeFloor1(
			Bitstream::BitReader& br,
			const Floor1Config& f1,
			const Codebook::VorbisCodebook* books,
			float* out_curve,
			uint32 n,
			int32* y_list,
			int32* final_y,
			uint8* step2_flag,
			uint32* out_used_posts,
			uint32* out_max_y
		) {
			if (out_used_posts) *out_used_posts = 0;
			if (out_max_y) *out_max_y = 0;
			if (!br.getBit()) {
				for (uint32 i = 0; i < n; ++i) out_curve[i] = 0.0f;
				for (uint32 i = 0; i < f1.x_count; ++i) step2_flag[i] = 0;
				return false;
			}

			static const uint32 kFloor1Ranges[4] = { 256, 128, 86, 64 };
			uint32 mult_idx = (f1.multiplier > 0 && f1.multiplier <= 4) ? (f1.multiplier - 1) : 0;
			uint32 range_val = kFloor1Ranges[mult_idx];
			uint32 bits_to_read = Bitstream::ilog(range_val - 1);

			y_list[0] = br.getBits(bits_to_read);
			y_list[1] = br.getBits(bits_to_read);

			uint32 point_idx = 2;
			for (uint32 i = 0; i < f1.partitions; ++i) {
				uint32 cls = f1.partition_class_list[i];
				uint32 cdim = f1.class_dim[cls];
				uint32 csub = f1.class_subs[cls];
				uint32 csub_mask = (1u << csub) - 1u;

				int32 cval = 0;
				if (csub > 0) {
					cval = Codebook::DecodeSymbol(br, books[f1.class_sub_books[cls]]);
					if (cval < 0) {
						for (uint32 k = 0; k < n; ++k) out_curve[k] = 0.0f;
						return false;
					}
				}

				for (uint32 j = 0; j < cdim; ++j) {
					int32 book_idx = f1.subclass_books[cls][cval & csub_mask];
					cval >>= csub;
					if (book_idx >= 0) {
						int32 sym = Codebook::DecodeSymbol(br, books[book_idx]);
						if (sym < 0) {
							for (uint32 k = 0; k < n; ++k) out_curve[k] = 0.0f;
							return false;
						}
						y_list[point_idx++] = sym;
					} else {
						y_list[point_idx++] = 0;
					}
				}
			}

			// The prediction runs in the UN-multiplied domain. libvorbis floor1_inverse1
			// keeps fit_value[] raw (hiroom=quant_q-predicted, loroom=predicted, with
			// quant_q=[256,128,86,64][mult-1]) and applies info->mult only when it
			// renders the curve in floor1_inverse2 (ly=fit_value[0]*info->mult, then
			// clamped to 0..255 before the dB lookup). Multiplying the anchors here as
			// well makes the room arithmetic and the interpolated values inconsistent
			// whenever multiplier != 1, which is why the decoded envelope could come
			// out flat or rising instead of falling.
			final_y[0] = y_list[0];
			final_y[1] = y_list[1];
			step2_flag[0] = 1;
			step2_flag[1] = 1;
			for (uint32 i = 2; i < f1.x_count; ++i) step2_flag[i] = 0;
			int32 range_max = (int32)range_val;
			uint32 max_y = 0;
			if (final_y[0] > 0) max_y = (uint32)final_y[0];
			if (final_y[1] > (int32)max_y) max_y = (uint32)final_y[1];

			for (uint32 i = 2; i < f1.x_count; ++i) {
				uint32 low = f1.low_neighbor[i];
				uint32 high = f1.high_neighbor[i];
				int32 predicted = render_point(f1.x_list[low], final_y[low], f1.x_list[high], final_y[high], f1.x_list[i]);
				int32 val = y_list[i];
				int32 high_room = range_max - predicted;
				int32 low_room = predicted;
				int32 room = (high_room < low_room) ? (high_room * 2) : (low_room * 2);

				if (val > 0) {
					step2_flag[low] = 1;
					step2_flag[high] = 1;
					step2_flag[i] = 1;
					if (val >= room) {
						if (high_room > low_room) final_y[i] = val - low_room + predicted;
						else final_y[i] = predicted - val + high_room - 1;
					} else {
						if (val & 1) final_y[i] = predicted - ((val + 1) >> 1);
						else final_y[i] = predicted + (val >> 1);
					}
				} else {
					final_y[i] = predicted;
				}
				if (step2_flag[i] && final_y[i] > (int32)max_y) max_y = (uint32)final_y[i];
			}

			uint32 last_x = 0;
			int32 last_y = final_y[0] * (int32)f1.multiplier;
			if (last_y > 255) last_y = 255;
			uint32 used_posts = 1;
			for (uint32 i = 1; i < f1.x_count; ++i) {
				uint32 ord = f1.sorted_order[i];
				if (!step2_flag[ord]) continue;
				used_posts++;
				uint32 cur_x = f1.x_list[ord];
				int32 cur_y = final_y[ord] * (int32)f1.multiplier;
				if (cur_y > 255) cur_y = 255;
				render_line(last_x, last_y, cur_x, cur_y, out_curve, n);
				last_x = cur_x;
				last_y = cur_y;
			}
			if (last_x < n) {
				for (uint32 x = last_x; x < n; ++x) {
					if (last_y < 0) out_curve[x] = kFloor1InverseDbTable[0];
					else if (last_y >= 256) out_curve[x] = kFloor1InverseDbTable[255];
					else out_curve[x] = kFloor1InverseDbTable[last_y];
				}
			}
			if (out_used_posts) *out_used_posts = used_posts;
			if (out_max_y) *out_max_y = max_y;

			return true;
		}

	} // namespace Floor

	// =========================================================================
	// 4. Residue Module: Residue 0, 1, 2 (Spectral residual unpacking)
	// =========================================================================
	namespace Residue {

		constexpr uint32 kMaxResidueClassifications = 64;

		struct ResidueConfig {
			uint32 type;
			uint32 begin;
			uint32 end;
			uint32 partition_size;
			uint32 classifications;
			uint32 classbook;
			int16  cascade_books[kMaxResidueClassifications][8];
		};

		static bool ParseResidue(Bitstream::BitReader& br, uint32 type, ResidueConfig& res) {
			if (type > 2) return false;
			res.type = type;
			res.begin = br.getBits(24);
			res.end = br.getBits(24);
			res.partition_size = br.getBits(24) + 1;
			res.classifications = br.getBits(6) + 1;
			res.classbook = br.getBits(8);
			if (res.classifications > kMaxResidueClassifications) return false;

			uint8 cascades[kMaxResidueClassifications];
			for (uint32 i = 0; i < res.classifications; ++i) {
				uint32 low_bits = br.getBits(3);
				uint32 bitflag = br.getBit();
				uint32 high_bits = bitflag ? br.getBits(5) : 0;
				cascades[i] = (uint8)((high_bits << 3) | low_bits);
				OGG_DIAG_CASCADE(i, low_bits, bitflag, high_bits);
			}
			OGG_DIAG_CASCADE_DUMP(type);

			// Cascade sub-books are read classification-major: all eight passes of
			// classification 0, then all eight of classification 1, and so on.
			// libvorbis res0_unpack reads a flat list of popcount(cascade) bytes and
			// res0_look consumes that list with
			//   for(i=0;i<partitions;i++) for(j=0;j<8;j++)
			//     if(secondstages[i]&(1<<j)) partbooks[i][j]=booklist[acc++];
			// so the classification index is the OUTER loop. Reading the bytes
			// pass-major gives a classification the book that belongs to another
			// (classification,pass) pair, so partitions decode values of the wrong
			// magnitude in the wrong bands. (The "measured WORSE" note this replaces
			// came from a lag-1 autocorrelation metric that was itself invalid -- it
			// assumed a fixed left/right call phase and was in fact measuring
			// cross-channel correlation. It is retracted.)
			for (uint32 i = 0; i < res.classifications; ++i) {
				for (uint32 j = 0; j < 8; ++j) {
					if (cascades[i] & (1u << j)) {
						res.cascade_books[i][j] = (int16)br.getBits(8);
					} else {
						res.cascade_books[i][j] = -1;
					}
				}
			}
			return true;
		}

		static bool ValidateResidueBooks(const ResidueConfig& res, uint32 codebook_count) {
			if (res.classbook >= codebook_count) return false;
			for (uint32 i = 0; i < res.classifications; ++i) {
				for (uint32 j = 0; j < 8; ++j) {
					if (res.cascade_books[i][j] >= 0 &&
						(uint32)res.cascade_books[i][j] >= codebook_count) {
						return false;
					}
				}
			}
			return true;
		}

		static bool DecodeResidue(
			Bitstream::BitReader& br,
			const ResidueConfig& res,
			const Codebook::VorbisCodebook* books,
			float** channels,
			uint32 channel_count,
			uint32 n,
			const bool* do_not_decode_flags,
			uint8* class_idx_buf,
			uint32 class_idx_buf_size
		) {
			bool all_zero = true;
			for (uint32 ch = 0; ch < channel_count; ++ch) {
				if (!do_not_decode_flags[ch]) {
					all_zero = false;
					break;
				}
			}
			if (all_zero) return true;

			// Residue type 2 interleaves every channel into ONE vector, so residue
			// begin/end index that interleaved vector, not the per-channel spectrum.
			// libvorbis res2_inverse: max=(pcmend*ch)>>1; end=min(info->end,max);
			// n=end-begin; partvals=n/grouping. Clamping to the per-channel bin
			// count first and multiplying by the channel count afterwards yields a
			// DIFFERENT partition count than the encoder wrote (e.g. 64 instead of
			// 50 for a long block), which desynchronises the bitstream for the rest
			// of the packet: every later codebook symbol lands on the wrong bits.
			uint32 residue_limit = (res.type == 2) ? (n * channel_count) : n;
			uint32 limit_end = (res.end < residue_limit) ? res.end : residue_limit;
			if (res.begin >= limit_end) return true;

			uint32 total_samples = limit_end - res.begin;
			uint32 partitions_to_read = total_samples / res.partition_size;

			const Codebook::VorbisCodebook& classbook = books[res.classbook];
			if (classbook.dimensions == 0) return false;

			if (partitions_to_read > class_idx_buf_size) partitions_to_read = class_idx_buf_size;

			for (uint32 pass = 0; pass < 8; ++pass) {
				uint32 partition_count = 0;
				while (partition_count < partitions_to_read) {
					if (pass == 0) {
						int32 entry = Codebook::DecodeSymbol(br, classbook);
						if (entry < 0) return false;
						// One classbook symbol carries `dimensions` class indices in base
						// `classifications`, MOST SIGNIFICANT DIGIT FIRST: digit d belongs
						// to partition (partition_count + d).
						//
						// libvorbis builds look->decodemap[] as mult=parts^(dim-1);
						// deco=val/mult; val-=deco*mult; mult/=parts -- i.e. the most
						// significant digit comes first. Consuming the digits the other way
						// round (as an earlier revision of this file did) hands every
						// partition another partition's class, hence the wrong cascade
						// codebook, hence residue values of the wrong magnitude in the
						// wrong frequency bands.
						//
						// The "measured max|X|=1348 / 54% clipped" note that used to sit
						// here is retracted: that run still had the residue partition-count
						// bug in this function, so the digit order was never actually
						// tested in isolation.
						//
						// Floor1's base-N masterbook word uses the same most-significant
						// digit first convention.
						uint32 dmult = 1;
						for (uint32 e = 1; e < classbook.dimensions; ++e) dmult *= res.classifications;
						for (int32 d = 0; d < (int32)classbook.dimensions; ++d) {
							uint32 digit = (dmult > 0) ? ((uint32)entry / dmult) : 0;
							if (partition_count + (uint32)d < partitions_to_read) {
								class_idx_buf[partition_count + (uint32)d] = (uint8)digit;
							}
							entry -= (int32)(digit * dmult);
							if (dmult >= res.classifications) dmult /= res.classifications;
						}
						OGG_DIAG_CLASS(partitions_to_read, classbook.dimensions, classbook.entries,
							res.classifications, class_idx_buf, res.cascade_books);
					}

					for (uint32 d = 0; d < classbook.dimensions && partition_count < partitions_to_read; ++d, ++partition_count) {
						uint32 cls = class_idx_buf[partition_count];
						int16 book_idx = res.cascade_books[cls][pass];
						OGG_DIAG_RESBOOK(book_idx >= 0 ? 1 : 0,
							(book_idx >= 0 && !books[book_idx].value_vector) ? 1 : 0,
							(int32)book_idx);
						if (book_idx >= 0) {
							const Codebook::VorbisCodebook& book = books[book_idx];
							if (book.dimensions == 0) continue;
							if (book.dimensions > Codebook::kMaxVectorDim) return false; // corrupt setup

							uint32 offset = res.begin + (partition_count * res.partition_size);
							uint32 step = (book.dimensions != 0) ? (res.partition_size / book.dimensions) : 0;
							if (res.type == 0 && step == 0) return false;
							for (uint32 s = 0; s < res.partition_size; s += book.dimensions) {
								float v[Codebook::kMaxVectorDim];
								Codebook::DecodeVector(br, book, v);
								for (uint32 k = 0; k < book.dimensions && s + k < res.partition_size; ++k) {
									OGG_DIAG_RESVAL(v[k]);
									if (res.type == 2) {
										uint32 target_idx = (partition_count * res.partition_size) + s + k;
										uint32 ch = target_idx % channel_count;
										uint32 idx = res.begin + (target_idx / channel_count);
										if (idx < n) {
											channels[ch][idx] += v[k];
										}
									} else if (res.type == 0) {
										uint32 idx = offset + (k * step) + (s / book.dimensions);
										for (uint32 ch = 0; ch < channel_count; ++ch) {
											if (!do_not_decode_flags[ch] && idx < n) {
												channels[ch][idx] += v[k];
											}
										}
									} else {
										for (uint32 ch = 0; ch < channel_count; ++ch) {
											if (!do_not_decode_flags[ch] && offset + s + k < n) {
												channels[ch][offset + s + k] += v[k];
											}
										}
									}
								}
							}
						}
					}
				}
			}
			return true;
		}

	} // namespace Residue

	// =========================================================================
	// 5. Mapping Module: Coupling & Submap Dispatch
	// =========================================================================
	namespace Mapping {

		constexpr uint32 kMaxCouplingSteps = 256;
		constexpr uint32 kMaxSubmaps = 16;

		struct CouplingStep {
			uint16 magnitude;
			uint16 angle;
		};

		struct MappingConfig {
			uint32 submaps;
			uint16 coupling_steps;
			CouplingStep coupling[kMaxCouplingSteps];
			uint8  mux[256];
			uint8  submap_floor[kMaxSubmaps];
			uint8  submap_residue[kMaxSubmaps];
		};

		static bool ParseMapping(Bitstream::BitReader& br, MappingConfig& map, uint32 channels) {
			if (br.getBit()) {
				map.submaps = br.getBits(4) + 1;
			} else {
				map.submaps = 1;
			}
			if (map.submaps > kMaxSubmaps) return false;

			if (br.getBit()) {
				map.coupling_steps = (uint16)(br.getBits(8) + 1);
				if (map.coupling_steps > kMaxCouplingSteps) return false;
				for (uint32 i = 0; i < map.coupling_steps; ++i) {
					map.coupling[i].magnitude = (uint16)br.getBits(Bitstream::ilog(channels - 1));
					map.coupling[i].angle = (uint16)br.getBits(Bitstream::ilog(channels - 1));
					if (map.coupling[i].magnitude >= channels ||
						map.coupling[i].angle >= channels ||
						map.coupling[i].magnitude == map.coupling[i].angle) {
						return false;
					}
				}
			} else {
				map.coupling_steps = 0;
			}

			if (br.getBits(2) != 0) return false;

			if (map.submaps > 1) {
				for (uint32 i = 0; i < channels; ++i) {
					map.mux[i] = (uint8)br.getBits(4);
					if (map.mux[i] >= map.submaps) return false;
				}
			} else {
				for (uint32 i = 0; i < channels; ++i) map.mux[i] = 0;
			}

			for (uint32 i = 0; i < map.submaps; ++i) {
				br.getBits(8);
				map.submap_floor[i] = (uint8)br.getBits(8);
				map.submap_residue[i] = (uint8)br.getBits(8);
			}

			return true;
		}

		static bool ValidateMapping(const MappingConfig& map, uint32 floor_count, uint32 residue_count) {
			for (uint32 i = 0; i < map.submaps; ++i) {
				if (map.submap_floor[i] >= floor_count || map.submap_residue[i] >= residue_count) {
					return false;
				}
			}
			return true;
		}

		static void ApplyChannelCoupling(const MappingConfig& map, float** channels, uint32 n) {
			for (int32 i = (int32)map.coupling_steps - 1; i >= 0; --i) {
				uint32 mag_ch = map.coupling[i].magnitude;
				uint32 ang_ch = map.coupling[i].angle;
				float* mag = channels[mag_ch];
				float* ang = channels[ang_ch];

				for (uint32 j = 0; j < n; ++j) {
					float m = mag[j];
					float a = ang[j];
					if (m > 0.0f) {
						if (a > 0.0f) {
							mag[j] = m;
							ang[j] = m - a;
						} else {
							ang[j] = m;
							mag[j] = m + a;
						}
					} else {
						if (a > 0.0f) {
							mag[j] = m;
							ang[j] = m + a;
						} else {
							ang[j] = m;
							mag[j] = m - a;
						}
					}
				}
			}
		}

	} // namespace Mapping

	// =========================================================================
	// 6. MDCT & Windowing Module: IMDCT & Vorbis Window Overlap-Add
	// =========================================================================
	namespace Trig {

		// Self-contained sin/cos for the Vorbis window and the IMDCT cosine table.
		//
		// Deliberately NOT unisym's dblsin/dblcos from lib/c/arith.c: those are the
		// freestanding "TEMP" implementations whose Taylor loop terminates on a test
		// involving the global _EFDIGS, so the number of terms actually summed (and
		// therefore the accuracy) depends on runtime state. The window and the cosine
		// table must be good to float precision; if they are not, the decoded
		// spectrum comes out as noise whose magnitude no single scale factor fixes.
		constexpr double kPi = 3.14159265358979323846264338327950288;
		constexpr double kHalfPi = 1.57079632679489661923132169163975144;

		// Reduce x to r in [-pi/4, pi/4] plus a quadrant, then sum a FIXED number of
		// Taylor terms (8 for sin up to r^15/15!, 9 for cos up to r^16/16!). On
		// |r| <= pi/4 the truncation error is below 1e-16, far under float precision.
		inline void SinCos(double x, double& s, double& c) {
			long long q = (long long)(x / kHalfPi + ((x < 0.0) ? -0.5 : 0.5));
			double r = x - (double)q * kHalfPi;
			double r2 = r * r;

			double ps = r, ss = r;
			ps *= -r2 / 6.0;    ss += ps;   // r^3 / 3!
			ps *= -r2 / 20.0;   ss += ps;   // r^5 / 5!
			ps *= -r2 / 42.0;   ss += ps;   // r^7 / 7!
			ps *= -r2 / 72.0;   ss += ps;   // r^9 / 9!
			ps *= -r2 / 110.0;  ss += ps;   // r^11 / 11!
			ps *= -r2 / 156.0;  ss += ps;   // r^13 / 13!
			ps *= -r2 / 210.0;  ss += ps;   // r^15 / 15!

			double pc = 1.0, cc = 1.0;
			pc *= -r2 / 2.0;    cc += pc;   // r^2 / 2!
			pc *= -r2 / 12.0;   cc += pc;   // r^4 / 4!
			pc *= -r2 / 30.0;   cc += pc;   // r^6 / 6!
			pc *= -r2 / 56.0;   cc += pc;   // r^8 / 8!
			pc *= -r2 / 90.0;   cc += pc;   // r^10 / 10!
			pc *= -r2 / 132.0;  cc += pc;   // r^12 / 12!
			pc *= -r2 / 182.0;  cc += pc;   // r^14 / 14!
			pc *= -r2 / 240.0;  cc += pc;   // r^16 / 16!

			switch (q & 3) {
			case 0:  s =  ss; c =  cc; break;
			case 1:  s =  cc; c = -ss; break;
			case 2:  s = -ss; c = -cc; break;
			default: s = -cc; c =  ss; break;
			}
		}

		inline double Sin(double x) { double s, c; SinCos(x, s, c); return s; }
		inline double Cos(double x) { double s, c; SinCos(x, s, c); return c; }

	} // namespace Trig

	namespace MDCT {

		static void GenerateVorbisWindow(float* window, uint32 n) {
			for (uint32 i = 0; i < n; ++i) {
				double x = Trig::Sin(Trig::kPi * ((double)i + 0.5) / (double)n);
				window[i] = (float)Trig::Sin(0.5 * Trig::kPi * x * x);
			}
		}

		struct FastIMDCTPlan {
			static constexpr uint32 kBfBase[4] = { 6, 4, 2, 0 };
			uint32 n;
			uint32 n2;
			uint32 n4;
			float* cos_table;   // direct O(n^2) path only (fallback)
			// libvorbis mdct_lookup tables (mdct_init in libvorbis mdct.c). This
			// replaces a length-n complex FFT: the butterfly count drops from
			// (n/2)*log2(n) = 11264 at n=2048 to about n/8*log2(n/2) ~= 2300.
			float* mdct_trig;   // n + n/4 entries
			int32* mdct_bitrev; // n/4 entries; SIGNED because libvorbis stores -1
			uint32 mdct_log2n;
			bool   use_fast;

			FastIMDCTPlan() : n(0), n2(0), n4(0), cos_table(nullptr),
				mdct_trig(nullptr), mdct_bitrev(nullptr), mdct_log2n(0),
				use_fast(false) {}

			bool init(uint32 size, uni::trait::Malloc& alloc) {
				n = size;
				n2 = n >> 1;
				n4 = n >> 2;
				use_fast = false;
				if (n < 8 || n2 == 0 || n4 == 0) return false;

				// ------------------------------------------------------------------
				// Fast path: the IMDCT is
				//     x[m] = sum_k X[k] cos( pi/(2n) * (2m+1+n2) * (2k+1) )
				// Expanding the phase and using cos(a+b) = cos a cos b - sin a sin b
				// turns it into an odd-frequency, length-n inverse DFT:
				//     Y[k] = X[k] * e^{i pi (2k+1)/4} * e^{i pi (2k+1)/(4*n2)}
				//            for k < n2, zero for k >= n2
				//     x[m] = Re{ IDFT_n(Y)[m] * e^{i pi m/n} } * (2/n)
				// The direct sum costs n*n2/2 multiply-adds per channel per block
				// (105万 for n=2048); this costs n*log2(n) butterflies with a
				// sequential table, which is what the stuttering needed.
				//
				// Cross-checked against the direct sum in Python:
				// max |fast - direct| = 4e-13 for n = 16..256, i.e. exact to
				// floating point. The runtime self-test below re-checks it here.
				// ------------------------------------------------------------------
				if ((n & (n - 1)) == 0) { // radix-2 FFT needs a power of two
					uint32 lg = 0;
					while ((1u << lg) < n) ++lg;
					mdct_log2n = lg;
					mdct_trig = (float*)alloc.allocate((n + (n >> 2)) * sizeof(float));
					mdct_bitrev = (int32*)alloc.allocate((n >> 2) * sizeof(int32));
					if (mdct_trig && mdct_bitrev) {
						for (uint32 q = 0; q < (n >> 2); ++q) {
							mdct_trig[q * 2] = (float)Trig::Cos((Trig::kPi / (double)n) * (4.0 * (double)q));
							mdct_trig[q * 2 + 1] = (float)(-Trig::Sin((Trig::kPi / (double)n) * (4.0 * (double)q)));
							mdct_trig[n2 + q * 2] = (float)Trig::Cos((Trig::kPi / (2.0 * (double)n)) * (2.0 * (double)q + 1.0));
							mdct_trig[n2 + q * 2 + 1] = (float)Trig::Sin((Trig::kPi / (2.0 * (double)n)) * (2.0 * (double)q + 1.0));
						}
						for (uint32 q = 0; q < (n >> 3); ++q) {
							mdct_trig[n + q * 2] = (float)(Trig::Cos((Trig::kPi / (double)n) * (4.0 * (double)q + 2.0)) * 0.5);
							mdct_trig[n + q * 2 + 1] = (float)(-Trig::Sin((Trig::kPi / (double)n) * (4.0 * (double)q + 2.0)) * 0.5);
						}
						const uint32 bmask = (1u << (lg - 1)) - 1u;
						const uint32 bmsb = 1u << (lg - 2);
						for (uint32 q = 0; q < (n >> 3); ++q) {
							uint32 acc = 0;
							for (uint32 jj = 0; (bmsb >> jj) != 0; ++jj) {
								if (((bmsb >> jj) & q) != 0) acc |= (1u << jj);
							}
							mdct_bitrev[q * 2] = (int32)(((~acc) & bmask) - 1u);
							mdct_bitrev[q * 2 + 1] = (int32)acc;
						}

						// Self-check before trusting the fast path: compare it with the
						// direct sum on a deterministic vector at a few output positions.
						// If they disagree the O(n^2) path is used instead, so a mistake
						// in here can never turn into wrong audio.
						use_fast = true;
						float* t_in = (float*)alloc.allocate(n * sizeof(float));
						float* t_out = (float*)alloc.allocate(n * sizeof(float));
						float* t_work = (float*)alloc.allocate(2 * n * sizeof(float));
						if (t_in && t_out && t_work) {
							for (uint32 k = 0; k < n2; ++k) {
								t_in[k] = (float)Trig::Sin(0.37 * (double)k + 0.11) +
									0.3f * (float)Trig::Cos(0.19 * (double)k + 0.7);
							}
							transform_fast(t_in, t_out, t_work);
							float worst = 0.0f;
							float scale = 1.0e-9f;
							for (uint32 m = 0; m < n; ++m) {
								double acc = 0.0;
								for (uint32 k = 0; k < n2; ++k) {
									acc += (double)t_in[k] * Trig::Cos(Trig::kPi / (2.0 * (double)n) *
										(double)(2 * m + 1 + n2) * (double)(2 * k + 1));
								}
								const float ref = (float)acc;
								const float d = t_out[m] - ref;
								const float ad = (d < 0.0f) ? -d : d;
								if (ad > worst) worst = ad;
								const float ar = (ref < 0.0f) ? -ref : ref;
								if (ar > scale) scale = ar;
							}
							if (!(worst <= 1.0e-3f * scale)) use_fast = false;
						} else {
							use_fast = false;
						}
						if (t_in) alloc.deallocate(t_in);
						if (t_out) alloc.deallocate(t_out);
						if (t_work) alloc.deallocate(t_work);
					}
				}

				if (!use_fast) {
					// Direct O(n^2) fallback: same result, much slower.
					if (mdct_trig) { alloc.deallocate(mdct_trig); mdct_trig = nullptr; }
					if (mdct_bitrev) { alloc.deallocate(mdct_bitrev); mdct_bitrev = nullptr; }

					uint32 table_size = n << 2; // 4n points = full 2*PI period with step PI/(2n)
					cos_table = (float*)alloc.allocate(table_size * sizeof(float));
					if (!cos_table) return false;

					double step = Trig::kPi / (double)(n << 1);
					for (uint32 m = 0; m <= n; ++m) {
						cos_table[m] = (float)Trig::Cos((double)m * step);
					}
					for (uint32 m = n + 1; m < (n << 1); ++m) {
						cos_table[m] = -cos_table[(n << 1) - m];
					}
					for (uint32 m = (n << 1); m < 3 * n; ++m) {
						cos_table[m] = -cos_table[m - (n << 1)];
					}
					for (uint32 m = 3 * n; m < table_size; ++m) {
						cos_table[m] = cos_table[table_size - m];
					}
				}
				return true;
			}

			void release(uni::trait::Malloc& alloc) {
				if (cos_table) { alloc.deallocate(cos_table); cos_table = nullptr; }
				if (mdct_trig) { alloc.deallocate(mdct_trig); mdct_trig = nullptr; }
				if (mdct_bitrev) { alloc.deallocate(mdct_bitrev); mdct_bitrev = nullptr; }
			}

			void transform(const float* in, float* out, float* work) const {
				if (use_fast) transform_fast(in, out, work);
				else transform_direct(in, out);
			}

			// ---- libvorbis mdct.c mdct_backward, transcribed verbatim ----
			static constexpr float kCpi1_8 = 0.92387950420379638671875f;
			static constexpr float kCpi3_8 = 0.3826834261417388916015625f;
			static constexpr float kCpi2_8 = 0.707106769084930419921875f;

			static void MdctBf8(float* x, uint32 o) {
				float r0 = x[o + 6] + x[o + 2], r1 = x[o + 6] - x[o + 2];
				float r2 = x[o + 4] + x[o + 0], r3 = x[o + 4] - x[o + 0];
				x[o + 6] = r0 + r2; x[o + 4] = r0 - r2;
				r0 = x[o + 5] - x[o + 1]; r2 = x[o + 7] - x[o + 3];
				x[o + 0] = r1 + r0; x[o + 2] = r1 - r0;
				r0 = x[o + 5] + x[o + 1]; r1 = x[o + 7] + x[o + 3];
				x[o + 3] = r2 + r3; x[o + 1] = r2 - r3;
				x[o + 7] = r1 + r0; x[o + 5] = r1 - r0;
			}
			static void MdctBf16(float* x, uint32 o) {
				float r0 = x[o + 1] - x[o + 9], r1 = x[o + 0] - x[o + 8];
				x[o + 8] += x[o + 0]; x[o + 9] += x[o + 1];
				x[o + 0] = (r0 + r1) * kCpi2_8; x[o + 1] = (r0 - r1) * kCpi2_8;
				r0 = x[o + 3] - x[o + 11]; r1 = x[o + 10] - x[o + 2];
				x[o + 10] += x[o + 2]; x[o + 11] += x[o + 3];
				x[o + 2] = r0; x[o + 3] = r1;
				r0 = x[o + 12] - x[o + 4]; r1 = x[o + 13] - x[o + 5];
				x[o + 12] += x[o + 4]; x[o + 13] += x[o + 5];
				x[o + 4] = (r0 - r1) * kCpi2_8; x[o + 5] = (r0 + r1) * kCpi2_8;
				r0 = x[o + 14] - x[o + 6]; r1 = x[o + 15] - x[o + 7];
				x[o + 14] += x[o + 6]; x[o + 15] += x[o + 7];
				x[o + 6] = r0; x[o + 7] = r1;
				MdctBf8(x, o); MdctBf8(x, o + 8);
			}
			static void MdctBf32(float* x, uint32 o) {
				float r0 = x[o + 30] - x[o + 14], r1 = x[o + 31] - x[o + 15];
				x[o + 30] += x[o + 14]; x[o + 31] += x[o + 15];
				x[o + 14] = r0; x[o + 15] = r1;
				r0 = x[o + 28] - x[o + 12]; r1 = x[o + 29] - x[o + 13];
				x[o + 28] += x[o + 12]; x[o + 29] += x[o + 13];
				x[o + 12] = r0 * kCpi1_8 - r1 * kCpi3_8; x[o + 13] = r0 * kCpi3_8 + r1 * kCpi1_8;
				r0 = x[o + 26] - x[o + 10]; r1 = x[o + 27] - x[o + 11];
				x[o + 26] += x[o + 10]; x[o + 27] += x[o + 11];
				x[o + 10] = (r0 - r1) * kCpi2_8; x[o + 11] = (r0 + r1) * kCpi2_8;
				r0 = x[o + 24] - x[o + 8]; r1 = x[o + 25] - x[o + 9];
				x[o + 24] += x[o + 8]; x[o + 25] += x[o + 9];
				x[o + 8] = r0 * kCpi3_8 - r1 * kCpi1_8; x[o + 9] = r1 * kCpi3_8 + r0 * kCpi1_8;
				r0 = x[o + 22] - x[o + 6]; r1 = x[o + 7] - x[o + 23];
				x[o + 22] += x[o + 6]; x[o + 23] += x[o + 7];
				x[o + 6] = r1; x[o + 7] = r0;
				r0 = x[o + 4] - x[o + 20]; r1 = x[o + 5] - x[o + 21];
				x[o + 20] += x[o + 4]; x[o + 21] += x[o + 5];
				x[o + 4] = r1 * kCpi1_8 + r0 * kCpi3_8; x[o + 5] = r1 * kCpi3_8 - r0 * kCpi1_8;
				r0 = x[o + 2] - x[o + 18]; r1 = x[o + 3] - x[o + 19];
				x[o + 18] += x[o + 2]; x[o + 19] += x[o + 3];
				x[o + 2] = (r1 + r0) * kCpi2_8; x[o + 3] = (r1 - r0) * kCpi2_8;
				r0 = x[o + 0] - x[o + 16]; r1 = x[o + 1] - x[o + 17];
				x[o + 16] += x[o + 0]; x[o + 17] += x[o + 1];
				x[o + 0] = r1 * kCpi3_8 + r0 * kCpi1_8; x[o + 1] = r1 * kCpi1_8 - r0 * kCpi3_8;
				MdctBf16(x, o); MdctBf16(x, o + 16);
			}
			void MdctButterflyFirst(float* x, uint32 o, uint32 points) const {
				int32 x1 = (int32)(o + points - 8);
				int32 x2 = (int32)(o + (points >> 1) - 8);
				uint32 t = 0;
				while (x2 >= (int32)o) {
					for (uint32 g = 0; g < 4; ++g) {
						const uint32 b = kBfBase[g];
						const uint32 ti = t + g * 4u;
						const float r0 = x[x1 + b] - x[x2 + b];
						const float r1 = x[x1 + b + 1] - x[x2 + b + 1];
						x[x1 + b] += x[x2 + b]; x[x1 + b + 1] += x[x2 + b + 1];
						x[x2 + b] = r1 * mdct_trig[ti + 1] + r0 * mdct_trig[ti];
						x[x2 + b + 1] = r1 * mdct_trig[ti] - r0 * mdct_trig[ti + 1];
					}
					t += 16; x1 -= 8; x2 -= 8;
				}
			}
			void MdctButterflyGeneric(float* x, uint32 o, uint32 points, uint32 trigint) const {
				int32 x1 = (int32)(o + points - 8);
				int32 x2 = (int32)(o + (points >> 1) - 8);
				uint32 t = 0;
				while (x2 >= (int32)o) {
					for (uint32 g = 0; g < 4; ++g) {
						const uint32 b = kBfBase[g];
						const float r0 = x[x1 + b] - x[x2 + b];
						const float r1 = x[x1 + b + 1] - x[x2 + b + 1];
						x[x1 + b] += x[x2 + b]; x[x1 + b + 1] += x[x2 + b + 1];
						x[x2 + b] = r1 * mdct_trig[t + 1] + r0 * mdct_trig[t];
						x[x2 + b + 1] = r1 * mdct_trig[t] - r0 * mdct_trig[t + 1];
						t += trigint;
					}
					x1 -= 8; x2 -= 8;
				}
			}
			void MdctButterflies(float* x, uint32 o, uint32 points) const {
				int32 stages = (int32)mdct_log2n - 5;
				--stages;
				if (stages > 0) MdctButterflyFirst(x, o, points);
				--stages;
				for (uint32 i = 1; stages > 0; ++i, --stages) {
					const uint32 pts = points >> i;
					for (uint32 j = 0; j < (1u << i); ++j) {
						MdctButterflyGeneric(x, o + pts * j, pts, 4u << i);
					}
				}
				for (uint32 j = 0; j < points; j += 32) MdctBf32(x, o + j);
			}
			void MdctBitReverse(float* x) const {
				uint32 w0 = 0;
				uint32 w1 = n >> 1;
				uint32 t = n;
				uint32 idx = 0;
				while (w0 < w1) {
					const uint32 b0 = (uint32)((int32)(n >> 1) + mdct_bitrev[idx]);
					const uint32 b1 = (uint32)((int32)(n >> 1) + mdct_bitrev[idx + 1]);
					float r0 = x[b0 + 1] - x[b1 + 1];
					float r1 = x[b0 + 0] + x[b1 + 0];
					float r2 = r1 * mdct_trig[t] + r0 * mdct_trig[t + 1];
					float r3 = r1 * mdct_trig[t + 1] - r0 * mdct_trig[t];
					w1 -= 4;
					r0 = (x[b0 + 1] + x[b1 + 1]) * 0.5f;
					r1 = (x[b0 + 0] - x[b1 + 0]) * 0.5f;
					x[w0 + 0] = r0 + r2; x[w1 + 2] = r0 - r2;
					x[w0 + 1] = r1 + r3; x[w1 + 3] = r3 - r1;
					const uint32 b2 = (uint32)((int32)(n >> 1) + mdct_bitrev[idx + 2]);
					const uint32 b3 = (uint32)((int32)(n >> 1) + mdct_bitrev[idx + 3]);
					r0 = x[b2 + 1] - x[b3 + 1];
					r1 = x[b2 + 0] + x[b3 + 0];
					r2 = r1 * mdct_trig[t + 2] + r0 * mdct_trig[t + 3];
					r3 = r1 * mdct_trig[t + 3] - r0 * mdct_trig[t + 2];
					r0 = (x[b2 + 1] + x[b3 + 1]) * 0.5f;
					r1 = (x[b2 + 0] - x[b3 + 0]) * 0.5f;
					x[w0 + 2] = r0 + r2; x[w1 + 0] = r0 - r2;
					x[w0 + 3] = r1 + r3; x[w1 + 1] = r3 - r1;
					t += 4; idx += 4; w0 += 4;
				}
			}

			void transform_fast(const float* in, float* out, float* work) const {
				(void)work;
				const uint32 nn2 = n >> 1;
				const uint32 nn4 = n >> 2;
				const float* T = mdct_trig;
				int32 iX = (int32)nn2 - 7;
				uint32 oX = nn2 + nn4;
				uint32 t = nn4;
				while (iX >= 0) {
					oX -= 4;
					out[oX + 0] = -in[iX + 2] * T[t + 3] - in[iX + 0] * T[t + 2];
					out[oX + 1] = in[iX + 0] * T[t + 3] - in[iX + 2] * T[t + 2];
					out[oX + 2] = -in[iX + 6] * T[t + 1] - in[iX + 4] * T[t + 0];
					out[oX + 3] = in[iX + 4] * T[t + 1] - in[iX + 6] * T[t + 0];
					iX -= 8; t += 4;
				}
				iX = (int32)nn2 - 8;
				oX = nn2 + nn4;
				t = nn4;
				while (iX >= 0) {
					t -= 4;
					out[oX + 0] = in[iX + 4] * T[t + 3] + in[iX + 6] * T[t + 2];
					out[oX + 1] = in[iX + 4] * T[t + 2] - in[iX + 6] * T[t + 3];
					out[oX + 2] = in[iX + 0] * T[t + 1] + in[iX + 2] * T[t + 0];
					out[oX + 3] = in[iX + 0] * T[t + 0] - in[iX + 2] * T[t + 1];
					iX -= 8; oX += 4;
				}
				MdctButterflies(out, nn2, nn2);
				MdctBitReverse(out);
				{
					uint32 oX1 = nn2 + nn4, oX2 = nn2 + nn4;
					uint32 iXf = 0;
					t = nn2;
					while (iXf < oX1) {
						oX1 -= 4;
						const float i0 = out[iXf + 0], i1 = out[iXf + 1];
						const float i2 = out[iXf + 2], i3 = out[iXf + 3];
						const float i4 = out[iXf + 4], i5 = out[iXf + 5];
						const float i6 = out[iXf + 6], i7 = out[iXf + 7];
						out[oX1 + 3] = i0 * T[t + 1] - i1 * T[t + 0];
						out[oX2 + 0] = -(i0 * T[t + 0] + i1 * T[t + 1]);
						out[oX1 + 2] = i2 * T[t + 3] - i3 * T[t + 2];
						out[oX2 + 1] = -(i2 * T[t + 2] + i3 * T[t + 3]);
						out[oX1 + 1] = i4 * T[t + 5] - i5 * T[t + 4];
						out[oX2 + 2] = -(i4 * T[t + 4] + i5 * T[t + 5]);
						out[oX1 + 0] = i6 * T[t + 7] - i7 * T[t + 6];
						out[oX2 + 3] = -(i6 * T[t + 6] + i7 * T[t + 7]);
						oX2 += 4; iXf += 8; t += 8;
					}
				}
				{
					uint32 iX2 = nn2 + nn4, oX1 = nn4, oX2 = nn4;
					while (oX2 < iX2) {
						oX1 -= 4; iX2 -= 4;
						out[oX1 + 3] = out[iX2 + 3]; out[oX2 + 0] = -out[oX1 + 3];
						out[oX1 + 2] = out[iX2 + 2]; out[oX2 + 1] = -out[oX1 + 2];
						out[oX1 + 1] = out[iX2 + 1]; out[oX2 + 2] = -out[oX1 + 1];
						out[oX1 + 0] = out[iX2 + 0]; out[oX2 + 3] = -out[oX1 + 0];
						oX2 += 4;
					}
				}
				{
					uint32 iX3 = nn2 + nn4, oX1 = nn2 + nn4;
					const uint32 oX2 = nn2;
					while (oX1 > oX2) {
						oX1 -= 4;
						out[oX1 + 0] = out[iX3 + 3]; out[oX1 + 1] = out[iX3 + 2];
						out[oX1 + 2] = out[iX3 + 1]; out[oX1 + 3] = out[iX3 + 0];
						iX3 += 4;
					}
				}
				// libvorbis mdct_backward does not apply lookup->scale; that scale is
				// used by mdct_forward only. Scaling the inverse here pushes decoded
				// PCM toward the 16-bit quantisation floor, especially on long blocks.
			}

			void transform_direct(const float* in, float* out) const {
				uint32 mask = (n << 2) - 1;

				// Note: (base_idx * (2k + 1)) & mask is an arithmetic progression in k,
				// so the table index can be advanced by a constant step instead of
				// recomputing the product every tap. Mathematically identical.
				// First half symmetry: i in [0 .. n4 - 1]
				for (uint32 i = 0; i < n4; ++i) {
					float sum = 0.0f;
					uint32 base_idx = (i << 1) + 1 + n2;
					uint32 m = base_idx & mask;
					uint32 step = (base_idx << 1) & mask;
					for (uint32 k_ = 0; k_ < n2; ++k_) {
						sum += in[k_] * cos_table[m];
						m = (m + step) & mask;
					}
					out[i] = sum;
					out[n2 - 1 - i] = -sum;
				}

				// Second half symmetry: i in [0 .. n4 - 1] -> target n2 + i
				for (uint32 i = 0; i < n4; ++i) {
					float sum = 0.0f;
					uint32 base_idx = ((n2 + i) << 1) + 1 + n2;
					uint32 m = base_idx & mask;
					uint32 step = (base_idx << 1) & mask;
					for (uint32 k_ = 0; k_ < n2; ++k_) {
						sum += in[k_] * cos_table[m];
						m = (m + step) & mask;
					}
					out[n2 + i] = sum;
					out[n - 1 - i] = sum;
				}
			}
		};

	} // namespace MDCT

	// =========================================================================
	// 7. Engine & Stream Module: Ogg Page Parsing, Headers & Decoding Pipeline
	// =========================================================================
	namespace Engine {

		struct VorbisMode {
			uint8 blockflag;
			uint8 windowtype;
			uint8 transformtype;
			uint8 mapping;
		};

		class OGGStream : public uni::IAudioStream {
		private:
			uni::StorageTrait&    m_storage;
			uni::trait::Malloc&   m_allocator;
			uni::AudioInfo        m_info;
			VORBIS_INFO           m_vorbis_info;

			// Decoded Vorbis Setup
			uint32                m_codebook_count;
			Codebook::VorbisCodebook* m_codebooks;
			uint32                m_floor_count;
			Floor::Floor1Config*  m_floors;
			uint32                m_residue_count;
			Residue::ResidueConfig* m_residues;
			uint32                m_mapping_count;
			Mapping::MappingConfig* m_mappings;
			uint32                m_mode_count;
			VorbisMode*           m_modes;

			// Window and blocksize tables
			uint32                m_blocksize_0;
			uint32                m_blocksize_1;
			float*                m_window_0;
			float*                m_window_1;
			MDCT::FastIMDCTPlan   m_imdct_plan_0;
			MDCT::FastIMDCTPlan   m_imdct_plan_1;
			float*                m_imdct_work;

			// Overlap-Add state & decode buffers (dynamically allocated in heap to minimize stack usage)
			float*                m_overlap_buf[8];
			float*                m_spectrum_buf[8];
			float*                m_floor_curve_buf[8];
			float*                m_time_out_buf[8];
			uint8*                m_packet_buf;
			uint32                m_packet_buf_capacity;
			uint8*                m_class_idx_buf;
			int32*                m_floor_y_list;
			int32*                m_floor_final_y;
			uint8*                m_floor_step2_flag;
			bool                  m_no_residue[8];

			uint32                m_prev_blocksize;
			bool                  m_has_prev_window;

			// Stream output sample queue (interleaved 16-bit PCM)
			int16*                m_pcm_queue;
			uint32                m_pcm_capacity;
			uint32                m_pcm_head;
			uint32                m_pcm_tail;

			// Ogg Page Bitstream state
			uint64                m_page_offset;
			uint8                 m_seg_table[256];
			uint32                m_seg_count;
			uint32                m_seg_index;
			uint64                m_page_data_offset;
			bool                  m_in_page;

			// Storage access: a byte range ending at or below m_exact_limit is known
			// to lie inside the file, so it can be fetched with block reads instead
			// of one virtual call per byte.
			uint32                m_block_size;
			uint64                m_exact_limit;
			uint8*                m_io_scratch;
			uint8*                m_io_cache;
			uint32                m_io_cache_capacity;
			uint32                m_io_cache_valid;
			uint64                m_io_cache_offset;

			uint64                m_total_bytes;
			uint64                m_current_sample;
			uint64                m_total_samples;
			bool                  m_eos;
			uint32                m_packets_ok;
			uint32                m_packets_failed;
			uint32                m_read_calls;
			uint32                m_short_reads;
			uint64                m_pcm_samples_out;
			uint64                m_pcm_zero_samples;
			uint64                m_pcm_clipped_samples;
			uint64                m_pcm_hot_samples;
			uint32                m_pcm_peak_ppm;
			float                 m_pcm_last_sample[8];
			bool                  m_pcm_have_last_sample[8];
			uint32                m_pcm_peak_jump_ppm;
			uint64                m_pcm_jump_samples;
			uint32                m_prefill_runs;
			uint32                m_prefill_packets;
			uint32                m_prefill_max_packets_per_run;
			uint32                m_read_max_packets_per_call;
			uint32                m_read_max_prefill_packets;
			bool                  m_stats_reported;
			uint32                m_trace_short_packets;
			uint32                m_trace_long_packets;
			uint32                m_trace_spike_packets;
			uint32                m_trace_jump_packets;
			uint32                m_run_floor_max_milli;
			uint32                m_run_residue_max_milli;
			uint32                m_run_coupled_max_milli;
			uint32                m_run_spec_max_milli;
			uint32                m_run_imdct_max_milli;
			uint32                m_run_window_max_milli;
			uint32                m_run_output_max_milli;

		public:
			OGGStream(uni::StorageTrait& storage, uni::trait::Malloc& alloc)
				: m_storage(storage), m_allocator(alloc),
				  m_codebook_count(0), m_codebooks(nullptr),
				  m_floor_count(0), m_floors(nullptr),
				  m_residue_count(0), m_residues(nullptr),
				  m_mapping_count(0), m_mappings(nullptr),
				  m_mode_count(0), m_modes(nullptr),
				  m_blocksize_0(0), m_blocksize_1(0),
				  m_window_0(nullptr), m_window_1(nullptr),
				  m_imdct_work(nullptr),
				  m_packet_buf(nullptr), m_packet_buf_capacity(0),
				  m_class_idx_buf(nullptr), m_floor_y_list(nullptr),
				  m_floor_final_y(nullptr), m_floor_step2_flag(nullptr),
				  m_prev_blocksize(0), m_has_prev_window(false),
				  m_pcm_queue(nullptr), m_pcm_capacity(0),
				  m_pcm_head(0), m_pcm_tail(0),
				  m_page_offset(0), m_seg_count(0), m_seg_index(0),
				  m_page_data_offset(0), m_in_page(false),
				  m_block_size(0), m_exact_limit(0), m_io_scratch(nullptr),
				  m_io_cache(nullptr), m_io_cache_capacity(0),
				  m_io_cache_valid(0), m_io_cache_offset(0),
				  m_total_bytes(0), m_current_sample(0), m_total_samples(0),
				  m_eos(false), m_packets_ok(0), m_packets_failed(0),
				  m_read_calls(0), m_short_reads(0), m_pcm_samples_out(0),
				  m_pcm_zero_samples(0), m_pcm_clipped_samples(0),
				  m_pcm_hot_samples(0), m_pcm_peak_ppm(0),
				  m_pcm_peak_jump_ppm(0), m_pcm_jump_samples(0),
				  m_prefill_runs(0), m_prefill_packets(0),
				  m_prefill_max_packets_per_run(0),
				  m_read_max_packets_per_call(0), m_read_max_prefill_packets(0),
				  m_stats_reported(false),
				  m_trace_short_packets(0), m_trace_long_packets(0),
				  m_trace_spike_packets(0), m_trace_jump_packets(0),
				  m_run_floor_max_milli(0), m_run_residue_max_milli(0),
				  m_run_coupled_max_milli(0), m_run_spec_max_milli(0),
				  m_run_imdct_max_milli(0), m_run_window_max_milli(0),
				  m_run_output_max_milli(0) {
				for (int i = 0; i < 8; ++i) {
					m_overlap_buf[i] = nullptr;
					m_spectrum_buf[i] = nullptr;
					m_floor_curve_buf[i] = nullptr;
					m_time_out_buf[i] = nullptr;
					m_no_residue[i] = false;
					m_pcm_last_sample[i] = 0.0f;
					m_pcm_have_last_sample[i] = false;
				}
			}

			virtual ~OGGStream() {
				Cleanup();
			}

			void Cleanup() {
				ReportStats();
				if (m_codebooks) {
					for (uint32 i = 0; i < m_codebook_count; ++i) {
						m_codebooks[i].release(m_allocator);
					}
					m_allocator.deallocate(m_codebooks);
					m_codebooks = nullptr;
				}
				if (m_floors) { m_allocator.deallocate(m_floors); m_floors = nullptr; }
				if (m_residues) { m_allocator.deallocate(m_residues); m_residues = nullptr; }
				if (m_mappings) { m_allocator.deallocate(m_mappings); m_mappings = nullptr; }
				if (m_modes) { m_allocator.deallocate(m_modes); m_modes = nullptr; }
				if (m_window_0) { m_allocator.deallocate(m_window_0); m_window_0 = nullptr; }
				if (m_window_1) { m_allocator.deallocate(m_window_1); m_window_1 = nullptr; }
				m_imdct_plan_0.release(m_allocator);
				m_imdct_plan_1.release(m_allocator);
				if (m_imdct_work) { m_allocator.deallocate(m_imdct_work); m_imdct_work = nullptr; }
				for (int i = 0; i < 8; ++i) {
					if (m_overlap_buf[i]) { m_allocator.deallocate(m_overlap_buf[i]); m_overlap_buf[i] = nullptr; }
					if (m_spectrum_buf[i]) { m_allocator.deallocate(m_spectrum_buf[i]); m_spectrum_buf[i] = nullptr; }
					if (m_floor_curve_buf[i]) { m_allocator.deallocate(m_floor_curve_buf[i]); m_floor_curve_buf[i] = nullptr; }
					if (m_time_out_buf[i]) { m_allocator.deallocate(m_time_out_buf[i]); m_time_out_buf[i] = nullptr; }
				}
				if (m_packet_buf) { m_allocator.deallocate(m_packet_buf); m_packet_buf = nullptr; }
				if (m_io_scratch) { m_allocator.deallocate(m_io_scratch); m_io_scratch = nullptr; }
				if (m_io_cache) { m_allocator.deallocate(m_io_cache); m_io_cache = nullptr; }
				if (m_class_idx_buf) { m_allocator.deallocate(m_class_idx_buf); m_class_idx_buf = nullptr; }
				if (m_floor_y_list) { m_allocator.deallocate(m_floor_y_list); m_floor_y_list = nullptr; }
				if (m_floor_final_y) { m_allocator.deallocate(m_floor_final_y); m_floor_final_y = nullptr; }
				if (m_floor_step2_flag) { m_allocator.deallocate(m_floor_step2_flag); m_floor_step2_flag = nullptr; }
				if (m_pcm_queue) {
					m_allocator.deallocate(m_pcm_queue);
					m_pcm_queue = nullptr;
				}
			}

			void ReportStats() {
				if (m_stats_reported) return;
				m_stats_reported = true;
			}

			static uint32 AbsMilli(float x) {
				if (x < 0.0f) x = -x;
				if (x > 4294967.0f) return 0xFFFFFFFFu;
				return (uint32)(x * 1000.0f + 0.5f);
			}

			static uint32 AbsPpm(float x) {
				if (x < 0.0f) x = -x;
				if (x > 4294.0f) return 0xFFFFFFFFu;
				return (uint32)(x * 1000000.0f + 0.5f);
			}

			static void RaiseMax(uint32& dst, uint32 val) {
				if (val > dst) dst = val;
			}

			bool ReadCachedExact(uint64 offset, void* dest, uint32 size) {
				if (!m_io_cache || !m_io_cache_capacity || !m_block_size) return false;
				uint8* out = (uint8*)dest;
				while (size > 0) {
					if (offset < m_io_cache_offset ||
						offset >= m_io_cache_offset + (uint64)m_io_cache_valid) {
						uint64 aligned = offset - (offset % m_block_size);
						uint64 available = (aligned < m_exact_limit) ? (m_exact_limit - aligned) : 0;
						if (available == 0) return false;
						uint32 read_bytes = m_io_cache_capacity;
						if ((uint64)read_bytes > available) read_bytes = (uint32)available;
						read_bytes -= read_bytes % m_block_size;
						if (read_bytes == 0) return false;
						if (!m_storage.Read((stduint)(aligned / m_block_size), m_io_cache, read_bytes / m_block_size)) {
							m_io_cache_valid = 0;
							return false;
						}
						m_io_cache_offset = aligned;
						m_io_cache_valid = read_bytes;
					}

					uint32 in_cache = (uint32)(offset - m_io_cache_offset);
					uint32 can_copy = m_io_cache_valid - in_cache;
					if (can_copy > size) can_copy = size;
					MemCopyN(out, m_io_cache + in_cache, can_copy);
					out += can_copy;
					offset += can_copy;
					size -= can_copy;
				}
				return true;
			}

			static uint32 MaxAbsMilli(float* const* channels, uint32 channel_count, uint32 n) {
				uint32 max_val = 0;
				for (uint32 c = 0; c < channel_count; ++c) {
					const float* p = channels[c];
					for (uint32 i = 0; i < n; ++i) RaiseMax(max_val, AbsMilli(p[i]));
				}
				return max_val;
			}

			static uint32 MaxAbsPpm(float* const* channels, uint32 channel_count, uint32 n) {
				uint32 max_val = 0;
				for (uint32 c = 0; c < channel_count; ++c) {
					const float* p = channels[c];
					for (uint32 i = 0; i < n; ++i) RaiseMax(max_val, AbsPpm(p[i]));
				}
				return max_val;
			}

			static uint32 QueueFreeSamples(uint32 head, uint32 tail, uint32 capacity) {
				uint32 used = (tail - head) & (capacity - 1);
				return capacity - 1 - used;
			}

			static constexpr float kPcmOutputGain = 0.80f;
			static constexpr uint32 kPcmQueueCapacity = 1u << 21;
			static constexpr uint32 kIoCacheBytes = 32768;
			static constexpr uint32 kPrefillLowSamples = 1u << 19;
			static constexpr uint32 kPrefillHighSamples = 1u << 20;
			static constexpr uint32 kPrefillRuntimePacketBudget = 2;
			static constexpr bool kRuntimePcmDiag =
#if OGG_RUNTIME_PCM_DIAG
				true;
#else
				false;
#endif

			uint32 PcmQueuedSamples() const {
				return (m_pcm_tail - m_pcm_head) & (m_pcm_capacity - 1);
			}

			void NotePcmShape(uint32 channel, float sample, uint32& pkt_jump_peak_ppm, uint32& pkt_jump_count) {
				if (channel >= 8) return;
				if (m_pcm_have_last_sample[channel]) {
					uint32 jump_ppm = AbsPpm(sample - m_pcm_last_sample[channel]);
					RaiseMax(pkt_jump_peak_ppm, jump_ppm);
					RaiseMax(m_pcm_peak_jump_ppm, jump_ppm);
					if (jump_ppm >= 350000u) {
						pkt_jump_count++;
						m_pcm_jump_samples++;
					}
				}
				m_pcm_last_sample[channel] = sample;
				m_pcm_have_last_sample[channel] = true;
			}

			bool DecodeNextAudioPacket(uint32& failed_packets) {
				uint8* pkt_data = nullptr;
				uint32 pkt_len = 0;
				if (!ReadNextPacket(pkt_data, pkt_len)) {
					m_eos = true;
					return false;
				}
				if (!DecodeAudioPacket(pkt_data, pkt_len)) {
					m_packets_failed++;
					if (++failed_packets > 64) m_eos = true;
					return !m_eos;
				}
				failed_packets = 0;
				return true;
			}

			void DecodeUntilQueued(uint32 target_samples, uint32 packet_budget) {
				if (m_eos || !m_pcm_queue) return;
				uint32 failed_packets = 0;
				bool decoded_any = false;
				uint32 decoded_packets = 0;
				while (!m_eos && PcmQueuedSamples() < target_samples) {
					if (packet_budget && decoded_packets >= packet_budget) break;
					if (QueueFreeSamples(m_pcm_head, m_pcm_tail, m_pcm_capacity) <=
						(m_blocksize_1 * m_vorbis_info.audio_channels + 1)) {
						break;
					}
					uint32 before = m_packets_ok;
					if (!DecodeNextAudioPacket(failed_packets)) break;
					if (m_packets_ok != before) {
						decoded_any = true;
						m_prefill_packets++;
						decoded_packets++;
					}
				}
				if (decoded_any) m_prefill_runs++;
				RaiseMax(m_prefill_max_packets_per_run, decoded_packets);
			}

			bool ReadStorageBytes(uint64 offset, void* dest, uint32 size) {
				if (size == 0) return true;

				// Fast path: the whole range provably lies inside the file, so it can
				// be fetched with block reads. Doing this per byte instead costs a
				// virtual call (and a 64-bit divide in FileStorageDevice) for every
				// byte of the compressed stream.
				if (offset + (uint64)size <= m_exact_limit) {
					if (ReadCachedExact(offset, dest, size)) return true;
					if (m_io_scratch &&
						m_storage.Read((stduint)offset, dest, (stduint)size, m_io_scratch) == (stduint)size) {
							return true;
						}
				}

				// Exact path: StorageTrait::operator[] reports the real end of file by
				// returning -1, which is what the page scan depends on.
				uint8* out = (uint8*)dest;
				for (uint32 i = 0; i < size; ++i) {
					int b = m_storage[(uint64)(offset + i)];
					if (b < 0) return false;
					out[i] = (uint8)b;
				}
				return true;
			}

			bool ReadNextPacket(uint8*& out_packet_data, uint32& out_packet_len) {
				if (!m_packet_buf) return false;
				uint32 packet_len = 0;

				while (true) {
					if (!m_in_page || m_seg_index >= m_seg_count) {
						if (m_in_page) {
							uint32 payload_len = 0;
							for (uint32 i = 0; i < m_seg_count; ++i) payload_len += m_seg_table[i];
							m_page_offset += sizeof(OGG_PAGE_HEADER) + m_seg_count + payload_len;
							m_in_page = false;
						}

						while (m_page_offset + sizeof(OGG_PAGE_HEADER) < m_total_bytes) {
							OGG_PAGE_HEADER page{};
							if (!ReadStorageBytes(m_page_offset, &page, sizeof(page))) return false;
							if (page.capture_pattern == OGG_MAGIC) {
								m_seg_count = page.page_segments;
								if (!ReadStorageBytes(m_page_offset + sizeof(page), m_seg_table, m_seg_count)) {
									return false;
								}
								m_seg_index = 0;
								m_page_data_offset = m_page_offset + sizeof(page) + m_seg_count;
								m_in_page = true;
								break;
							}
							m_page_offset++;
						}

						if (!m_in_page) {
							if (packet_len > 0) {
								out_packet_data = m_packet_buf;
								out_packet_len = packet_len;
								return true;
							}
							return false;
						}
					}

					while (m_seg_index < m_seg_count) {
						uint8 seg_len = m_seg_table[m_seg_index];
						if (seg_len > 0) {
							if (packet_len + seg_len > m_packet_buf_capacity) return false; // oversized/corrupt packet
							if (!ReadStorageBytes(m_page_data_offset, m_packet_buf + packet_len, seg_len)) return false;
							packet_len += seg_len;
							m_page_data_offset += seg_len;
						}
						m_seg_index++;

						if (seg_len < 255) {
							out_packet_data = m_packet_buf;
							out_packet_len = packet_len;
							return true;
						}
					}
				}
			}

			bool ParseIdentificationHeader(const uint8* data, uint32 len) {
				if (len < 30 || data[0] != 0x01 || MemCompare((const char*)(data + 1), "vorbis", 6) != 0) return false;
				Bitstream::BitReader br(data + 7, len - 7);

				m_vorbis_info.vorbis_version = br.getBits(32);
				if (m_vorbis_info.vorbis_version != 0) return false;

				m_vorbis_info.audio_channels = (uint8)br.getBits(8);
				m_vorbis_info.audio_sample_rate = br.getBits(32);
				m_vorbis_info.bitrate_maximum = br.getBitsSigned(32);
				m_vorbis_info.bitrate_nominal = br.getBitsSigned(32);
				m_vorbis_info.bitrate_minimum = br.getBitsSigned(32);

				uint32 b0 = br.getBits(4);
				uint32 b1 = br.getBits(4);
				m_blocksize_0 = 1u << b0;
				m_blocksize_1 = 1u << b1;
				m_vorbis_info.blocksize_0 = (uint8)b0;
				m_vorbis_info.blocksize_1 = (uint8)b1;
				m_vorbis_info.framing_flag = (uint8)br.getBit();

				if (m_vorbis_info.audio_channels == 0 || m_vorbis_info.audio_channels > 8 || m_vorbis_info.audio_sample_rate == 0) return false;
				if (m_blocksize_0 > m_blocksize_1 || m_blocksize_1 > 8192) return false;

				return true;
			}

			bool ParseCommentHeader(const uint8* data, uint32 len) {
				if (len < 7 || data[0] != 0x03 || MemCompare((const char*)(data + 1), "vorbis", 6) != 0) return false;
				return true;
			}

			bool ParseSetupHeader(const uint8* data, uint32 len) {
				if (len < 7 || data[0] != 0x05 || MemCompare((const char*)(data + 1), "vorbis", 6) != 0) return false;
				Bitstream::BitReader br(data + 7, len - 7);

				// 1. Codebooks
				m_codebook_count = br.getBits(8) + 1;
				m_codebooks = (Codebook::VorbisCodebook*)m_allocator.allocate(m_codebook_count * sizeof(Codebook::VorbisCodebook));
				if (!m_codebooks) return false;
				for (uint32 i = 0; i < m_codebook_count; ++i) {
					new (&m_codebooks[i]) Codebook::VorbisCodebook();
					if (!Codebook::ParseCodebook(br, m_codebooks[i], m_allocator)) return false;
				}

				// 2. Time-domain transforms (placeholder in Vorbis I)
				uint32 time_count = br.getBits(6) + 1;
				for (uint32 i = 0; i < time_count; ++i) {
					if (br.getBits(16) != 0) return false;
				}

				// 3. Floors
				m_floor_count = br.getBits(6) + 1;
				m_floors = (Floor::Floor1Config*)m_allocator.allocate(m_floor_count * sizeof(Floor::Floor1Config));
				if (!m_floors) return false;
				for (uint32 i = 0; i < m_floor_count; ++i) {
					uint32 floor_type = br.getBits(16);
					if (floor_type != 1) return false;
					if (!Floor::ParseFloor1(br, m_floors[i])) return false;
					if (!Floor::ValidateFloor1Books(m_floors[i], m_codebook_count)) return false;
				}

				// 4. Residues
				m_residue_count = br.getBits(6) + 1;
				m_residues = (Residue::ResidueConfig*)m_allocator.allocate(m_residue_count * sizeof(Residue::ResidueConfig));
				if (!m_residues) return false;
				for (uint32 i = 0; i < m_residue_count; ++i) {
					uint32 residue_type = br.getBits(16);
					if (!Residue::ParseResidue(br, residue_type, m_residues[i])) return false;
					if (!Residue::ValidateResidueBooks(m_residues[i], m_codebook_count)) return false;
				}

				// 5. Mappings
				m_mapping_count = br.getBits(6) + 1;
				m_mappings = (Mapping::MappingConfig*)m_allocator.allocate(m_mapping_count * sizeof(Mapping::MappingConfig));
				if (!m_mappings) return false;
				for (uint32 i = 0; i < m_mapping_count; ++i) {
					uint32 mapping_type = br.getBits(16);
					if (mapping_type != 0) return false;
					if (!Mapping::ParseMapping(br, m_mappings[i], m_vorbis_info.audio_channels)) return false;
					if (!Mapping::ValidateMapping(m_mappings[i], m_floor_count, m_residue_count)) return false;
				}

				// 6. Modes
				m_mode_count = br.getBits(6) + 1;
				m_modes = (VorbisMode*)m_allocator.allocate(m_mode_count * sizeof(VorbisMode));
				if (!m_modes) return false;
				for (uint32 i = 0; i < m_mode_count; ++i) {
					m_modes[i].blockflag = (uint8)br.getBit();
					m_modes[i].windowtype = (uint8)br.getBits(16);
					m_modes[i].transformtype = (uint8)br.getBits(16);
					m_modes[i].mapping = (uint8)br.getBits(8);
					if (m_modes[i].windowtype != 0 || m_modes[i].transformtype != 0 || m_modes[i].mapping >= m_mapping_count) {
						return false;
					}
				}

				if (!br.getBit()) return false;

				return true;
			}

			bool Initialize() {
				// BlockTrait::Read falls back to 512 when Block_Size is unset; use the
				// same convention here so the byte math and the block reads agree.
				m_block_size = (uint32)(m_storage.Block_Size ? m_storage.Block_Size : 512);
				m_total_bytes = (uint64)m_storage.getUnits() * (uint64)m_block_size;
				// Bytes below this offset are guaranteed to be inside the file (only
				// the final block may be partial), so a block read there cannot return
				// stale padding bytes.
				uint64 units = (uint64)m_storage.getUnits();
				m_exact_limit = (units > 0) ? ((units - 1) * (uint64)m_block_size) : 0;
				m_page_offset = 0;
				m_seg_count = 0;
				m_seg_index = 0;
				m_page_data_offset = 0;
				m_in_page = false;

				m_packet_buf_capacity = 65536;
				m_packet_buf = (uint8*)m_allocator.allocate(m_packet_buf_capacity);
				if (!m_packet_buf) return false;

				m_io_scratch = (uint8*)m_allocator.allocate(m_block_size);
				if (!m_io_scratch) return false;
				m_io_cache_capacity = kIoCacheBytes;
				if (m_io_cache_capacity < m_block_size) m_io_cache_capacity = m_block_size;
				m_io_cache_capacity -= m_io_cache_capacity % m_block_size;
				if (m_io_cache_capacity == 0) m_io_cache_capacity = m_block_size;
				m_io_cache = (uint8*)m_allocator.allocate(m_io_cache_capacity);
				if (!m_io_cache) return false;
				m_io_cache_valid = 0;
				m_io_cache_offset = 0;

				uint8* pkt_data = nullptr;
				uint32 pkt_len = 0;

				if (!ReadNextPacket(pkt_data, pkt_len) || !ParseIdentificationHeader(pkt_data, pkt_len)) return false;
				if (!ReadNextPacket(pkt_data, pkt_len) || !ParseCommentHeader(pkt_data, pkt_len)) return false;
				if (!ReadNextPacket(pkt_data, pkt_len) || !ParseSetupHeader(pkt_data, pkt_len)) return false;

				m_window_0 = (float*)m_allocator.allocate(m_blocksize_0 * sizeof(float));
				m_window_1 = (float*)m_allocator.allocate(m_blocksize_1 * sizeof(float));
				// 2 * blocksize: the FFT path keeps the block as a split
				// real/imaginary complex array of n points.
				m_imdct_work = (float*)m_allocator.allocate((m_blocksize_1 * 2) * sizeof(float));
				if (!m_window_0 || !m_window_1 || !m_imdct_work) return false;

				MDCT::GenerateVorbisWindow(m_window_0, m_blocksize_0);
				MDCT::GenerateVorbisWindow(m_window_1, m_blocksize_1);

				if (!m_imdct_plan_0.init(m_blocksize_0, m_allocator) ||
					!m_imdct_plan_1.init(m_blocksize_1, m_allocator)) {
					return false;
				}

				OGG_DIAG_TRIG((float)Trig::Sin(Trig::kPi / 6.0),
					(float)Trig::Cos(0.0),
					(float)Trig::Cos(Trig::kHalfPi),
					(float)Trig::Cos(Trig::kPi));
				// NOTE: cos_table only exists on the O(n^2) fallback. Reading it
				// unconditionally crashed the process (SIGSEGV) as soon as the FFT
				// path became the default, so pick the table that actually exists.
				if (m_imdct_plan_0.cos_table) {
					OGG_DIAG_TABLES(m_window_0[0], m_window_0[m_blocksize_0 >> 1], m_window_0[m_blocksize_0 - 1],
						m_window_1[m_blocksize_1 >> 1],
						m_imdct_plan_0.cos_table[0], m_imdct_plan_0.cos_table[m_blocksize_0],
						m_imdct_plan_0.cos_table[m_blocksize_0 << 1], m_imdct_plan_0.cos_table[3 * m_blocksize_0],
						0);
				} else {
					const uint32 q4 = m_blocksize_0 >> 2;
					const uint32 half0 = m_blocksize_0 >> 1;
					OGG_DIAG_TABLES(m_window_0[0], m_window_0[m_blocksize_0 >> 1], m_window_0[m_blocksize_0 - 1],
						m_window_1[m_blocksize_1 >> 1],
						m_imdct_plan_0.mdct_trig[0], m_imdct_plan_0.mdct_trig[1],
						m_imdct_plan_0.mdct_trig[2], m_imdct_plan_0.mdct_trig[3],
						1);
				}

				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					m_overlap_buf[c] = (float*)m_allocator.allocate(m_blocksize_1 * sizeof(float));
					m_spectrum_buf[c] = (float*)m_allocator.allocate((m_blocksize_1 >> 1) * sizeof(float));
					m_floor_curve_buf[c] = (float*)m_allocator.allocate((m_blocksize_1 >> 1) * sizeof(float));
					m_time_out_buf[c] = (float*)m_allocator.allocate(m_blocksize_1 * sizeof(float));

					if (!m_overlap_buf[c] || !m_spectrum_buf[c] || !m_floor_curve_buf[c] || !m_time_out_buf[c]) {
						return false;
					}
					for (uint32 j = 0; j < m_blocksize_1; ++j) m_overlap_buf[c][j] = 0.0f;
				}

				m_class_idx_buf = (uint8*)m_allocator.allocate(1024);
				m_floor_y_list = (int32*)m_allocator.allocate(Floor::kMaxFloor1XList * sizeof(int32));
				m_floor_final_y = (int32*)m_allocator.allocate(Floor::kMaxFloor1XList * sizeof(int32));
				m_floor_step2_flag = (uint8*)m_allocator.allocate(Floor::kMaxFloor1XList);
				if (!m_class_idx_buf || !m_floor_y_list || !m_floor_final_y || !m_floor_step2_flag) return false;

				// Must stay a power of two: the per-sample hot loops index the ring
				// buffer with & (m_pcm_capacity - 1) instead of a modulo.
				m_pcm_capacity = kPcmQueueCapacity;
				m_pcm_queue = (int16*)m_allocator.allocate(m_pcm_capacity * sizeof(int16));
				if (!m_pcm_queue) return false;
				m_pcm_head = 0;
				m_pcm_tail = 0;

				m_total_samples = 0;
				if (m_total_bytes > sizeof(OGG_PAGE_HEADER)) {
					uint32 tail_len = (m_total_bytes > 4096) ? 4096 : (uint32)m_total_bytes;
					uint64 tail_start = m_total_bytes - tail_len;
					uint8 buf[4096];
					uint32 actual_read = 0;
					for (uint32 i = 0; i < tail_len; ++i) {
						int b = m_storage[(uint64)(tail_start + i)];
						if (b < 0) break;
						buf[i] = (uint8)b;
						actual_read++;
					}
					for (int32 i = (int32)actual_read - (int32)sizeof(OGG_PAGE_HEADER); i >= 0; --i) {
						if (buf[i] == 'O' && buf[i + 1] == 'g' && buf[i + 2] == 'g' && buf[i + 3] == 'S') {
							OGG_PAGE_HEADER* p = (OGG_PAGE_HEADER*)(buf + i);
							if (p->granule_position != (uint64)-1 && p->granule_position > 0) {
								m_total_samples = p->granule_position;
								break;
							}
						}
					}
				}

				m_info.format.sample_format = uni::AudioSampleFormat::S16LE;
				m_info.format.channels = m_vorbis_info.audio_channels;
				m_info.format.sample_rate = m_vorbis_info.audio_sample_rate;
				m_info.containerFormat = uni::AudioContainerFormat::OGG;
				m_info.bitsPerSample = 16;
				m_info.totalSamples = (uint32)m_total_samples;
				if (m_vorbis_info.audio_sample_rate > 0) {
					m_info.durationMs = (uint32)((m_total_samples * 1000) / m_vorbis_info.audio_sample_rate);
				}
				m_info.dataByteLength = (uint32)(m_total_samples * m_vorbis_info.audio_channels * sizeof(int16));

				DecodeUntilQueued(kPrefillHighSamples, 0);

				return true;
			}

			bool DecodeAudioPacket(const uint8* data, uint32 len) {
				if (len == 0 || (data[0] & 1)) return false;

				OGG_DIAG_BEGIN();

				Bitstream::BitReader br(data, len);
				br.getBit();

				uint32 mode_idx = br.getBits(Bitstream::ilog(m_mode_count - 1));
				if (mode_idx >= m_mode_count) return false;

				const VorbisMode& mode = m_modes[mode_idx];
				uint32 cur_blocksize = mode.blockflag ? m_blocksize_1 : m_blocksize_0;
				uint32 cur_n2 = cur_blocksize >> 1;
				uint64 pkt_start_sample = m_current_sample;
				bool pkt_collect_metrics = (m_packets_ok < 3) ||
					(mode.blockflag ? (m_trace_long_packets < 2) : (m_trace_short_packets < 2));
				uint32 pkt_floor_max_milli = 0;
				uint32 pkt_residue_max_milli = 0;
				uint32 pkt_coupled_max_milli = 0;
				uint32 pkt_spec_max_milli = 0;
				uint32 pkt_imdct_max_milli = 0;
				uint32 pkt_imdct_max_ppm = 0;
				uint32 pkt_window_max_milli = 0;
				uint32 pkt_window_max_ppm = 0;
				uint32 pkt_output_max_milli = 0;
				uint32 pkt_floor_used_posts = 0;
				uint32 pkt_floor_max_y = 0;
				uint32 pkt_floor_max_idx = 0;
				uint32 pkt_floor_nonzero_mask = 0;
				uint32 pkt_residue_decode_mask = 0;
				uint32 pkt_output_max_ppm = 0;
				uint32 pkt_pcm_samples = 0;
				uint32 pkt_pcm_nonzero = 0;
				uint32 pkt_pcm_clipped = 0;
				uint32 pkt_pcm_hot = 0;
				uint32 pkt_pcm_peak_ppm = 0;
				uint32 pkt_jump_peak_ppm = 0;
				uint32 pkt_jump_count = 0;
				uint32 pkt_floor_bits_start = 0;
				uint32 pkt_floor_bits_end = 0;
				uint32 pkt_residue_bits_end = 0;

				uint8 prev_win_flag = 0;
				uint8 next_win_flag = 0;
				if (mode.blockflag) {
					prev_win_flag = (uint8)br.getBit();
					next_win_flag = (uint8)br.getBit();
				}

				const Mapping::MappingConfig& map = m_mappings[mode.mapping];

				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					for (uint32 i = 0; i < cur_n2; ++i) m_spectrum_buf[c][i] = 0.0f;
					m_no_residue[c] = false;
				}

				// 1. Decode Floors
				OGG_DIAG_FBITIN(br.getBitOffset());
				pkt_floor_bits_start = br.getBitOffset();
				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					uint32 submap = map.mux[c];
					uint32 floor_idx = map.submap_floor[submap];
					uint32 floor_used_posts = 0;
					uint32 floor_max_y = 0;
					bool nonzero = Floor::DecodeFloor1(
						br, m_floors[floor_idx], m_codebooks, m_floor_curve_buf[c], cur_n2,
						m_floor_y_list, m_floor_final_y, m_floor_step2_flag,
						&floor_used_posts, &floor_max_y);
					if (!nonzero) m_no_residue[c] = true;
					else if (c < 32) pkt_floor_nonzero_mask |= (1u << c);
					pkt_floor_used_posts += floor_used_posts;
					RaiseMax(pkt_floor_max_y, floor_max_y);
					{
						uint32 floor_idx_milli = floor_max_y * (uint32)m_floors[floor_idx].multiplier;
						if (floor_idx_milli > 255) floor_idx_milli = 255;
						RaiseMax(pkt_floor_max_idx, floor_idx_milli);
					}
					if (pkt_collect_metrics) {
						for (uint32 i = 0; i < cur_n2; ++i) {
							RaiseMax(pkt_floor_max_milli, AbsMilli(m_floor_curve_buf[c][i]));
						}
					}
					OGG_DIAG_FLOOR(m_floor_curve_buf[c], cur_n2);
					OGG_DIAG_DUMP("floor", m_floor_curve_buf[c], cur_n2);
				}
				if (pkt_collect_metrics) RaiseMax(m_run_floor_max_milli, pkt_floor_max_milli);

				OGG_DIAG_FBITOUT(br.getBitOffset());
				pkt_floor_bits_end = br.getBitOffset();

				// Channel coupling can make a pair nonzero even if only one side has
				// a decoded floor. Vorbis requires propagating that nonzero state
				// before residue decode, otherwise the coupled residue bundle can be
				// decoded with the wrong do-not-decode mask.
				for (uint32 i = 0; i < map.coupling_steps; ++i) {
					uint32 mag = map.coupling[i].magnitude;
					uint32 ang = map.coupling[i].angle;
					if (!m_no_residue[mag] || !m_no_residue[ang]) {
						m_no_residue[mag] = false;
						m_no_residue[ang] = false;
					}
				}
				for (uint32 c = 0; c < m_vorbis_info.audio_channels && c < 32; ++c) {
					if (!m_no_residue[c]) pkt_residue_decode_mask |= (1u << c);
				}

				// 2. Decode Residues
				OGG_DIAG_BITIN(br.getBitOffset());
				// A submap only carries the residue of the channels mapped onto it
				// (mapping_mux); channels belonging to other submaps must stay
				// untouched instead of receiving this submap's residue as well.
				for (uint32 sub = 0; sub < map.submaps; ++sub) {
					uint32 res_idx = map.submap_residue[sub];
					float* bundle[8];
					bool   bundle_skip[8];
					uint32 bundle_count = 0;
					for (uint32 c = 0; c < m_vorbis_info.audio_channels && bundle_count < 8; ++c) {
						if (map.mux[c] != sub) continue;
						bundle[bundle_count] = m_spectrum_buf[c];
						bundle_skip[bundle_count] = m_no_residue[c];
						bundle_count++;
					}
					if (bundle_count == 0) continue;
					if (!Residue::DecodeResidue(
						br, m_residues[res_idx], m_codebooks, bundle,
						bundle_count, cur_n2, bundle_skip,
						m_class_idx_buf, 1024)) {
						// The bit reader desynchronised mid-residue: drop this packet
						// instead of emitting the garbage it would decode to. Packets
						// are self-contained, so the next one decodes cleanly.
						return false;
					}
				}
				if (pkt_collect_metrics) {
					pkt_residue_max_milli = MaxAbsMilli(m_spectrum_buf, m_vorbis_info.audio_channels, cur_n2);
					RaiseMax(m_run_residue_max_milli, pkt_residue_max_milli);
				}

				OGG_DIAG_BITOUT(data, len, br.getBitOffset(), mode.blockflag ? 1u : 0u);
				pkt_residue_bits_end = br.getBitOffset();

				// 3. Inverse Channel Coupling
				if (map.coupling_steps > 0) {
					if (m_vorbis_info.audio_channels >= 2) {
						OGG_DIAG_CORR(m_spectrum_buf[0], m_spectrum_buf[1], cur_n2, 0u);
					}
					Mapping::ApplyChannelCoupling(map, m_spectrum_buf, cur_n2);
					if (m_vorbis_info.audio_channels >= 2) {
						OGG_DIAG_CORR(m_spectrum_buf[0], m_spectrum_buf[1], cur_n2, 1u);
					}
				}
				if (pkt_collect_metrics) {
					pkt_coupled_max_milli = MaxAbsMilli(m_spectrum_buf, m_vorbis_info.audio_channels, cur_n2);
					RaiseMax(m_run_coupled_max_milli, pkt_coupled_max_milli);
				}

				// 4. Dot Product with Floor & IMDCT
				const MDCT::FastIMDCTPlan& cur_plan = mode.blockflag ? m_imdct_plan_1 : m_imdct_plan_0;
				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					for (uint32 i = 0; i < cur_n2; ++i) {
						m_spectrum_buf[c][i] *= m_floor_curve_buf[c][i];
					}
					if (pkt_collect_metrics) {
						for (uint32 i = 0; i < cur_n2; ++i) {
							RaiseMax(pkt_spec_max_milli, AbsMilli(m_spectrum_buf[c][i]));
						}
					}
					OGG_DIAG_SPECTRUM(m_spectrum_buf[c], cur_n2, mode.blockflag ? 1u : 0u);
					OGG_DIAG_DUMP("spec", m_spectrum_buf[c], cur_n2);
					cur_plan.transform(m_spectrum_buf[c], m_time_out_buf[c], m_imdct_work);
					if (pkt_collect_metrics) {
						for (uint32 i = 0; i < cur_blocksize; ++i) {
							RaiseMax(pkt_imdct_max_milli, AbsMilli(m_time_out_buf[c][i]));
							RaiseMax(pkt_imdct_max_ppm, AbsPpm(m_time_out_buf[c][i]));
						}
					}
					OGG_DIAG_IMDCT(m_time_out_buf[c], cur_blocksize);
				}
				if (pkt_collect_metrics) {
					RaiseMax(m_run_spec_max_milli, pkt_spec_max_milli);
					RaiseMax(m_run_imdct_max_milli, pkt_imdct_max_milli);
				}

				// 5. Windowing
				if (mode.blockflag == 0) {
					for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
						for (uint32 i = 0; i < cur_blocksize; ++i) {
							m_time_out_buf[c][i] *= m_window_0[i];
						}
					}
				} else {
					uint32 n1 = cur_blocksize;
					uint32 n0 = m_blocksize_0;
					uint32 left_start = (n1 >> 2) - (n0 >> 2);
					uint32 right_start = ((3 * n1) >> 2) - (n0 >> 2);

					for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
						// Left half
						if (prev_win_flag) {
							for (uint32 i = 0; i < (n1 >> 1); ++i) {
								m_time_out_buf[c][i] *= m_window_1[i];
							}
						} else {
							for (uint32 i = 0; i < left_start; ++i) {
								m_time_out_buf[c][i] = 0.0f;
							}
							for (uint32 i = 0; i < (n0 >> 1); ++i) {
								m_time_out_buf[c][left_start + i] *= m_window_0[i];
							}
						}

						// Right half
						if (next_win_flag) {
							for (uint32 i = (n1 >> 1); i < n1; ++i) {
								m_time_out_buf[c][i] *= m_window_1[i];
							}
						} else {
							for (uint32 i = 0; i < (n0 >> 1); ++i) {
								m_time_out_buf[c][right_start + i] *= m_window_0[(n0 >> 1) + i];
							}
							for (uint32 i = right_start + (n0 >> 1); i < n1; ++i) {
								m_time_out_buf[c][i] = 0.0f;
							}
						}
					}
				}
				if (pkt_collect_metrics) {
					pkt_window_max_milli = MaxAbsMilli(m_time_out_buf, m_vorbis_info.audio_channels, cur_blocksize);
					pkt_window_max_ppm = MaxAbsPpm(m_time_out_buf, m_vorbis_info.audio_channels, cur_blocksize);
					RaiseMax(m_run_window_max_milli, pkt_window_max_milli);
				}

				// 6. Overlap-Add & PCM queue output
				if (m_has_prev_window) {
					if (m_prev_blocksize == cur_blocksize) {
						for (uint32 s = 0; s < cur_n2; ++s) {
							for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
								float sample = m_overlap_buf[c][s] + m_time_out_buf[c][s];
								if (pkt_collect_metrics) {
									RaiseMax(pkt_output_max_milli, AbsMilli(sample));
									RaiseMax(pkt_output_max_ppm, AbsPpm(sample));
								}
								OGG_DIAG_SAMPLE(sample);
								float pcm_sample = sample * kPcmOutputGain;
								int32 pcm = (int32)(pcm_sample * 32767.0f);
								if (kRuntimePcmDiag) {
									uint32 sample_ppm = AbsPpm(pcm_sample);
									RaiseMax(pkt_pcm_peak_ppm, sample_ppm);
									RaiseMax(m_pcm_peak_ppm, sample_ppm);
									if (sample_ppm >= 900000u) { pkt_pcm_hot++; m_pcm_hot_samples++; }
									if (pcm_sample >= 1.0f || pcm_sample <= -1.0f) { pkt_pcm_clipped++; m_pcm_clipped_samples++; }
									NotePcmShape(c, pcm_sample, pkt_jump_peak_ppm, pkt_jump_count);
								}
								if (pcm > 32767) pcm = 32767;
								if (pcm < -32768) pcm = -32768;
								pkt_pcm_samples++;
								if (pcm != 0) pkt_pcm_nonzero++;

								if (((m_pcm_tail + 1) & (m_pcm_capacity - 1)) != m_pcm_head) {
									m_pcm_queue[m_pcm_tail] = (int16)pcm;
									m_pcm_tail = (m_pcm_tail + 1) & (m_pcm_capacity - 1);
								}
							}
							m_current_sample++;
						}
					} else if (m_prev_blocksize < cur_blocksize) {
						uint32 prev_n2 = m_prev_blocksize >> 1;
						uint32 out_cnt = (m_prev_blocksize >> 2) + (cur_blocksize >> 2);
						uint32 start = (cur_blocksize >> 2) - (m_prev_blocksize >> 2);

						for (uint32 s = 0; s < prev_n2; ++s) {
							for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
								float sample = m_overlap_buf[c][s] + m_time_out_buf[c][start + s];
								if (pkt_collect_metrics) {
									RaiseMax(pkt_output_max_milli, AbsMilli(sample));
									RaiseMax(pkt_output_max_ppm, AbsPpm(sample));
								}
								OGG_DIAG_SAMPLE(sample);
								float pcm_sample = sample * kPcmOutputGain;
								int32 pcm = (int32)(pcm_sample * 32767.0f);
								if (kRuntimePcmDiag) {
									uint32 sample_ppm = AbsPpm(pcm_sample);
									RaiseMax(pkt_pcm_peak_ppm, sample_ppm);
									RaiseMax(m_pcm_peak_ppm, sample_ppm);
									if (sample_ppm >= 900000u) { pkt_pcm_hot++; m_pcm_hot_samples++; }
									if (pcm_sample >= 1.0f || pcm_sample <= -1.0f) { pkt_pcm_clipped++; m_pcm_clipped_samples++; }
									NotePcmShape(c, pcm_sample, pkt_jump_peak_ppm, pkt_jump_count);
								}
								if (pcm > 32767) pcm = 32767;
								if (pcm < -32768) pcm = -32768;
								pkt_pcm_samples++;
								if (pcm != 0) pkt_pcm_nonzero++;

								if (((m_pcm_tail + 1) & (m_pcm_capacity - 1)) != m_pcm_head) {
									m_pcm_queue[m_pcm_tail] = (int16)pcm;
									m_pcm_tail = (m_pcm_tail + 1) & (m_pcm_capacity - 1);
								}
							}
							m_current_sample++;
						}
						for (uint32 s = prev_n2; s < out_cnt; ++s) {
							for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
								float sample = m_time_out_buf[c][start + s];
								if (pkt_collect_metrics) {
									RaiseMax(pkt_output_max_milli, AbsMilli(sample));
									RaiseMax(pkt_output_max_ppm, AbsPpm(sample));
								}
								OGG_DIAG_SAMPLE(sample);
								float pcm_sample = sample * kPcmOutputGain;
								int32 pcm = (int32)(pcm_sample * 32767.0f);
								if (kRuntimePcmDiag) {
									uint32 sample_ppm = AbsPpm(pcm_sample);
									RaiseMax(pkt_pcm_peak_ppm, sample_ppm);
									RaiseMax(m_pcm_peak_ppm, sample_ppm);
									if (sample_ppm >= 900000u) { pkt_pcm_hot++; m_pcm_hot_samples++; }
									if (pcm_sample >= 1.0f || pcm_sample <= -1.0f) { pkt_pcm_clipped++; m_pcm_clipped_samples++; }
									NotePcmShape(c, pcm_sample, pkt_jump_peak_ppm, pkt_jump_count);
								}
								if (pcm > 32767) pcm = 32767;
								if (pcm < -32768) pcm = -32768;
								pkt_pcm_samples++;
								if (pcm != 0) pkt_pcm_nonzero++;

								if (((m_pcm_tail + 1) & (m_pcm_capacity - 1)) != m_pcm_head) {
									m_pcm_queue[m_pcm_tail] = (int16)pcm;
									m_pcm_tail = (m_pcm_tail + 1) & (m_pcm_capacity - 1);
								}
							}
							m_current_sample++;
						}
					} else {
						uint32 cur_n2_val = cur_blocksize >> 1;
						uint32 start = (m_prev_blocksize >> 2) - (cur_blocksize >> 2);

						for (uint32 s = 0; s < start; ++s) {
							for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
								float sample = m_overlap_buf[c][s];
								if (pkt_collect_metrics) {
									RaiseMax(pkt_output_max_milli, AbsMilli(sample));
									RaiseMax(pkt_output_max_ppm, AbsPpm(sample));
								}
								OGG_DIAG_SAMPLE(sample);
								float pcm_sample = sample * kPcmOutputGain;
								int32 pcm = (int32)(pcm_sample * 32767.0f);
								if (kRuntimePcmDiag) {
									uint32 sample_ppm = AbsPpm(pcm_sample);
									RaiseMax(pkt_pcm_peak_ppm, sample_ppm);
									RaiseMax(m_pcm_peak_ppm, sample_ppm);
									if (sample_ppm >= 900000u) { pkt_pcm_hot++; m_pcm_hot_samples++; }
									if (pcm_sample >= 1.0f || pcm_sample <= -1.0f) { pkt_pcm_clipped++; m_pcm_clipped_samples++; }
									NotePcmShape(c, pcm_sample, pkt_jump_peak_ppm, pkt_jump_count);
								}
								if (pcm > 32767) pcm = 32767;
								if (pcm < -32768) pcm = -32768;
								pkt_pcm_samples++;
								if (pcm != 0) pkt_pcm_nonzero++;

								if (((m_pcm_tail + 1) & (m_pcm_capacity - 1)) != m_pcm_head) {
									m_pcm_queue[m_pcm_tail] = (int16)pcm;
									m_pcm_tail = (m_pcm_tail + 1) & (m_pcm_capacity - 1);
								}
							}
							m_current_sample++;
						}
						for (uint32 s = 0; s < cur_n2_val; ++s) {
							for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
								float sample = m_overlap_buf[c][start + s] + m_time_out_buf[c][s];
								if (pkt_collect_metrics) {
									RaiseMax(pkt_output_max_milli, AbsMilli(sample));
									RaiseMax(pkt_output_max_ppm, AbsPpm(sample));
								}
								OGG_DIAG_SAMPLE(sample);
								float pcm_sample = sample * kPcmOutputGain;
								int32 pcm = (int32)(pcm_sample * 32767.0f);
								if (kRuntimePcmDiag) {
									uint32 sample_ppm = AbsPpm(pcm_sample);
									RaiseMax(pkt_pcm_peak_ppm, sample_ppm);
									RaiseMax(m_pcm_peak_ppm, sample_ppm);
									if (sample_ppm >= 900000u) { pkt_pcm_hot++; m_pcm_hot_samples++; }
									if (pcm_sample >= 1.0f || pcm_sample <= -1.0f) { pkt_pcm_clipped++; m_pcm_clipped_samples++; }
									NotePcmShape(c, pcm_sample, pkt_jump_peak_ppm, pkt_jump_count);
								}
								if (pcm > 32767) pcm = 32767;
								if (pcm < -32768) pcm = -32768;
								pkt_pcm_samples++;
								if (pcm != 0) pkt_pcm_nonzero++;

								if (((m_pcm_tail + 1) & (m_pcm_capacity - 1)) != m_pcm_head) {
									m_pcm_queue[m_pcm_tail] = (int16)pcm;
									m_pcm_tail = (m_pcm_tail + 1) & (m_pcm_capacity - 1);
								}
							}
							m_current_sample++;
						}
					}
				}
				if (pkt_collect_metrics) RaiseMax(m_run_output_max_milli, pkt_output_max_milli);

				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					for (uint32 i = 0; i < cur_n2; ++i) {
						m_overlap_buf[c][i] = m_time_out_buf[c][cur_n2 + i];
					}
				}

				OGG_DIAG_END(mode.blockflag ? 1u : 0u, cur_n2);
				m_prev_blocksize = cur_blocksize;
				m_has_prev_window = true;
				m_packets_ok++;
				return true;
			}

			virtual uni::AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
				bytesRead = 0;
				if (!destBuffer || maxBytes == 0) return uni::AudioResult::InvalidArgument;
				m_read_calls++;

				int16* out_ptr = (int16*)destBuffer;
				uint32 samples_requested = maxBytes / sizeof(int16);
				uint32 samples_copied = 0;
				uint32 failed_packets = 0;

				while (samples_copied < samples_requested) {
					while (m_pcm_head != m_pcm_tail && samples_copied < samples_requested) {
						int16 sample = m_pcm_queue[m_pcm_head];
						out_ptr[samples_copied++] = sample;
						m_pcm_samples_out++;
						if (sample == 0) m_pcm_zero_samples++;
						m_pcm_head = (m_pcm_head + 1) & (m_pcm_capacity - 1);
					}

					if (samples_copied >= samples_requested) break;

					if (!DecodeNextAudioPacket(failed_packets)) break;
				}

				bytesRead = samples_copied * sizeof(int16);
				if (bytesRead > 0 && PcmQueuedSamples() < kPrefillLowSamples) {
					DecodeUntilQueued(kPrefillHighSamples, kPrefillRuntimePacketBudget);
				}
				if (bytesRead > 0 && bytesRead < maxBytes && !m_eos) m_short_reads++;
				if (bytesRead == 0 && m_eos) return uni::AudioResult::EndOfStream;
				return uni::AudioResult::OK;
			}

			virtual uni::AudioResult Seek(uint32 sampleIndex) override {
				m_pcm_head = 0;
				m_pcm_tail = 0;
				m_has_prev_window = false;
				m_eos = false;
				m_current_sample = 0;

				for (uint32 c = 0; c < m_vorbis_info.audio_channels; ++c) {
					for (uint32 j = 0; j < m_blocksize_1; ++j) m_overlap_buf[c][j] = 0.0f;
				}

				m_page_offset = 0;
				m_seg_count = 0;
				m_seg_index = 0;
				m_page_data_offset = 0;
				m_in_page = false;
				m_io_cache_valid = 0;

				uint8* pkt = nullptr;
				uint32 pkt_l = 0;
				ReadNextPacket(pkt, pkt_l);
				ReadNextPacket(pkt, pkt_l);
				ReadNextPacket(pkt, pkt_l);

				while (m_current_sample + (m_blocksize_0 >> 1) < sampleIndex) {
					if (!ReadNextPacket(pkt, pkt_l)) break;
					DecodeAudioPacket(pkt, pkt_l);
					m_pcm_head = 0;
					m_pcm_tail = 0;
				}

				return uni::AudioResult::OK;
			}

			bool isValid() const {
				return m_vorbis_info.audio_channels > 0 && m_vorbis_info.audio_sample_rate > 0;
			}

			virtual void Release() override {
				Cleanup();
				m_allocator.deallocate(this);
			}

			virtual uni::AudioResult GetInfo(uni::AudioInfo& outInfo) const override {
				outInfo = m_info;
				return uni::AudioResult::OK;
			}
		};

	} // namespace Engine

} // namespace ogg

namespace uni {

	const char* OGGCodec::GetName() const {
		return "OGG Vorbis";
	}

	AudioContainerFormat OGGCodec::GetFormat() const {
		return AudioContainerFormat::OGG;
	}

	const char* const* OGGCodec::GetExtensions() const {
		static const char* const s_extensions[] = { "ogg", "oga", nullptr };
		return s_extensions;
	}

	AudioResult OGGCodec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		if (storage.getUnits() == 0) return AudioResult::IoError;

		OGG_PAGE_HEADER page{};
		uint8* ptr = (uint8*)&page;
		for (uint32 i = 0; i < sizeof(page); ++i) {
			int b = storage[(uint64)i];
			if (b < 0) return AudioResult::IoError;
			ptr[i] = (uint8)b;
		}

		if (page.capture_pattern == OGG_MAGIC) {
			matched = true;
			return AudioResult::OK;
		}
		return AudioResult::OK;
	}

	AudioResult OGGCodec::ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const {
		bool matched = false;
		if (Probe(storage, matched) != AudioResult::OK || !matched) {
			return AudioResult::InvalidFormat;
		}

		OGG_PAGE_HEADER page{};
		for (uint32 i = 0; i < sizeof(page); ++i) {
			int b = storage[(uint64)i];
			if (b < 0) return AudioResult::IoError;
			((uint8*)&page)[i] = (uint8)b;
		}

		uint8 seg_table[256];
		for (uint32 i = 0; i < page.page_segments; ++i) {
			int b = storage[(uint64)(sizeof(page) + i)];
			if (b < 0) return AudioResult::IoError;
			seg_table[i] = (uint8)b;
		}

		uint64 data_off = sizeof(page) + page.page_segments;
		uint8 id_hdr[64];
		uint32 take = (seg_table[0] < sizeof(id_hdr)) ? seg_table[0] : sizeof(id_hdr);
		for (uint32 i = 0; i < take; ++i) {
			int b = storage[(uint64)(data_off + i)];
			if (b < 0) return AudioResult::IoError;
			id_hdr[i] = (uint8)b;
		}

		if (take < 30 || id_hdr[0] != 0x01 || MemCompare((const char*)(id_hdr + 1), "vorbis", 6) != 0) {
			return AudioResult::InvalidFormat;
		}

		ogg::Bitstream::BitReader br(id_hdr + 7, take - 7);
		uint32 version = br.getBits(32);
		if (version != 0) return AudioResult::InvalidFormat;
		uint8 channels = (uint8)br.getBits(8);
		uint32 sample_rate = br.getBits(32);

		outInfo.format.sample_format = AudioSampleFormat::S16LE;
		outInfo.format.channels = channels;
		outInfo.format.sample_rate = sample_rate;
		outInfo.containerFormat = AudioContainerFormat::OGG;
		outInfo.bitsPerSample = 16;
		outInfo.dataByteOffset = 0;

		stduint total_bytes = storage.getUnits() * (storage.Block_Size ? storage.Block_Size : 4096);
		outInfo.dataByteLength = (uint32)total_bytes;
		outInfo.totalSamples = 0;
		outInfo.durationMs = 0;

		if (total_bytes > sizeof(OGG_PAGE_HEADER)) {
			uint32 tail_len = (total_bytes > 4096) ? 4096 : (uint32)total_bytes;
			uint64 tail_start = total_bytes - tail_len;
			uint8 buf[4096];
			uint32 actual_read = 0;
			for (uint32 i = 0; i < tail_len; ++i) {
				int b = storage[(uint64)(tail_start + i)];
				if (b < 0) break;
				buf[i] = (uint8)b;
				actual_read++;
			}
			for (int32 i = (int32)actual_read - (int32)sizeof(OGG_PAGE_HEADER); i >= 0; --i) {
				if (buf[i] == 'O' && buf[i + 1] == 'g' && buf[i + 2] == 'g' && buf[i + 3] == 'S') {
					OGG_PAGE_HEADER* p = (OGG_PAGE_HEADER*)(buf + i);
					if (p->granule_position != (uint64)-1 && p->granule_position > 0) {
						outInfo.totalSamples = (uint32)p->granule_position;
						break;
					}
				}
			}
		}
		if (sample_rate > 0 && outInfo.totalSamples > 0) {
			outInfo.durationMs = (uint32)(((uint64)outInfo.totalSamples * 1000) / sample_rate);
		}
		return AudioResult::OK;
	}

	AudioResult OGGCodec::OpenStream(
		StorageTrait& storage,
		IAudioStream*& outStream,
		trait::Malloc& allocator
	) const {
		outStream = nullptr;
		void* mem = allocator.allocate(sizeof(ogg::Engine::OGGStream));
		if (!mem) return AudioResult::OutOfMemory;

		ogg::Engine::OGGStream* stream = new (mem) ogg::Engine::OGGStream(storage, allocator);
		if (!stream->Initialize()) {
			stream->Release();
			return AudioResult::InvalidFormat;
		}

		outStream = stream;
		return AudioResult::OK;
	}

}
