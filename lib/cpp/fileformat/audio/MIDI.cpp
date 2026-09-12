// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: MIDI File Format & Soft Acoustic Synthesizer Implementation
// Codifiers: @dosconio, @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#include "../../../../inc/c/format/audio/MIDI.h"
#include "../../../../inc/c/arith.h"
#include "../../../../inc/c/ustring.h"

namespace {

	static uint16 ReadBe16(const uint8* p) {
		return (uint16)((p[0] << 8) | p[1]);
	}

	static uint32 ReadBe24(const uint8* p) {
		return (uint32)((p[0] << 16) | (p[1] << 8) | p[2]);
	}

	static uint32 ReadBe32(const uint8* p) {
		return (uint32)((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
	}

	static uint32 ReadVlq(const uint8* data, uint32 max_len, uint32& offset) {
		uint32 val = 0;
		for (uint32 i = 0; i < 4 && offset < max_len; ++i) {
			uint8 b = data[offset++];
			val = (val << 7) | (b & 0x7F);
			if (!(b & 0x80)) break;
		}
		return val;
	}

	static uint32 ReadStorageVlq(uni::StorageTrait& storage, uint64 max_len, uint64& offset) {
		uint32 val = 0;
		for (uint32 i = 0; i < 4 && offset < max_len; ++i) {
			int b = storage[offset++];
			if (b < 0) break;
			val = (val << 7) | ((uint8)b & 0x7F);
			if (!((uint8)b & 0x80)) break;
		}
		return val;
	}

	// Precomputed high-resolution sine table with 2048 entries
	constexpr uint32 kSineTableSize = 2048;
	static float s_sin_table[kSineTableSize];
	static bool s_sin_table_init = false;

	static void EnsureSinTable() {
		if (s_sin_table_init) return;
		for (uint32 i = 0; i < kSineTableSize; ++i) {
			double angle = (2.0 * _VAL_PI * (double)i) / (double)kSineTableSize;
			while (angle > _VAL_PI) angle -= 2.0 * _VAL_PI;
			while (angle < -_VAL_PI) angle += 2.0 * _VAL_PI;

			double a2 = angle * angle;
			double term = angle;
			double sum = term;
			term *= -a2 / (2.0 * 3.0);
			sum += term;
			term *= -a2 / (4.0 * 5.0);
			sum += term;
			term *= -a2 / (6.0 * 7.0);
			sum += term;
			term *= -a2 / (8.0 * 9.0);
			sum += term;
			term *= -a2 / (10.0 * 11.0);
			sum += term;
			term *= -a2 / (12.0 * 13.0);
			sum += term;
			s_sin_table[i] = (float)sum;
		}
		s_sin_table_init = true;
	}

	static inline float FastSin(float phase) {
		// phase in [0.0, 1.0)
		phase -= (int)phase;
		if (phase < 0.0f) phase += 1.0f;
		uint32 idx = (uint32)(phase * (float)kSineTableSize) & (kSineTableSize - 1);
		return s_sin_table[idx];
	}

	// MIDI Note to Frequency table (A4 = 440Hz, note 69)
	static float s_note_freq[128];
	static bool s_note_freq_init = false;

	static void EnsureNoteFreqTable() {
		if (s_note_freq_init) return;
		for (int n = 0; n < 128; ++n) {
			double exp_val = (double)(n - 69) / 12.0;
			double p = exp_val * _VAL_L2;
			double term = 1.0;
			double sum = 1.0;
			for (int k = 1; k <= 16; ++k) {
				term *= p / (double)k;
				sum += term;
			}
			s_note_freq[n] = (float)(440.0 * sum);
		}
		s_note_freq_init = true;
	}

	// Acoustic General MIDI Instrument Profile Types
	enum class InstrumentFamily : uint8 {
		Piano,
		ChromaticPercussion,
		Organ,
		Guitar,
		Bass,
		Strings,
		Ensemble,
		Brass,
		Reed,
		Pipe,
		SynthLead,
		SynthPad,
		SynthFX,
		Ethnic,
		Percussive,
		SoundFX,
		DrumKick,
		DrumSnare,
		DrumHiHat,
		DrumCymbal,
		DrumTom,
		DrumPercussion
	};

	struct InstrumentPatch {
		InstrumentFamily family;
		float attack_time;       // Attack ramp duration (seconds)
		float decay_rate;        // Decay multiplier per sample
		float sustain_level;     // 0.0 .. 1.0
		float release_rate;      // Release multiplier per sample
		float harmonic_rolloff;  // Harmonic rolloff rate (higher = softer)
		float lpf_cutoff;        // Lowpass filter coefficient (0.05..0.6)
		float detune_amount;     // Chorus / unison detune
		float vibrato_depth;     // Vibrato depth (0.0..0.02)
		float vibrato_rate;      // Vibrato frequency (Hz)
	};

	static InstrumentPatch GetGMPatch(uint8 program) {
		InstrumentPatch p{};
		p.detune_amount = 0.0005f;
		p.vibrato_depth = 0.0f;
		p.vibrato_rate = 5.0f;

		if (program < 8) {
			// Piano (0..7): Acoustic Grand, Bright, Electric Grand, Honky-tonk, Rhodes, FM EPiano, Harpsichord, Clavinet
			p.family = InstrumentFamily::Piano;
			p.attack_time = 0.004f;
			p.decay_rate = 0.99992f;
			p.sustain_level = 0.25f;
			p.release_rate = 0.9985f;
			p.harmonic_rolloff = (program == 1 || program == 7) ? 0.45f : 0.65f;
			p.lpf_cutoff = 0.35f;
			p.detune_amount = 0.0008f;
		} else if (program < 16) {
			// Chromatic Percussion (8..15): Celesta, Glockenspiel, Music Box, Vibraphone, Marimba, Xylophone, Tubular Bells
			p.family = InstrumentFamily::ChromaticPercussion;
			p.attack_time = 0.002f;
			p.decay_rate = 0.99985f;
			p.sustain_level = 0.05f;
			p.release_rate = 0.9980f;
			p.harmonic_rolloff = 0.80f;
			p.lpf_cutoff = 0.45f;
			p.vibrato_depth = (program == 11) ? 0.003f : 0.0f; // Vibraphone tremolo/vibrato
		} else if (program < 24) {
			// Organ (16..23): Hammond, Percussive, Rock, Church, Reed, Accordion, Harmonica, Tango
			p.family = InstrumentFamily::Organ;
			p.attack_time = 0.015f;
			p.decay_rate = 0.99998f;
			p.sustain_level = 0.90f;
			p.release_rate = 0.9960f;
			p.harmonic_rolloff = 0.50f;
			p.lpf_cutoff = 0.40f;
			p.detune_amount = 0.0012f;
		} else if (program < 32) {
			// Guitar (24..31): Nylon, Steel, Jazz, Clean, Muted, Overdriven, Distortion, Harmonics
			p.family = InstrumentFamily::Guitar;
			p.attack_time = 0.003f;
			p.decay_rate = 0.99988f;
			p.sustain_level = 0.35f;
			p.release_rate = 0.9975f;
			p.harmonic_rolloff = (program >= 29) ? 0.35f : 0.60f;
			p.lpf_cutoff = 0.30f;
			p.detune_amount = 0.0006f;
		} else if (program < 40) {
			// Bass (32..39): Acoustic, Finger, Pick, Fretless, Slap 1/2, Synth 1/2
			p.family = InstrumentFamily::Bass;
			p.attack_time = 0.005f;
			p.decay_rate = 0.99990f;
			p.sustain_level = 0.55f;
			p.release_rate = 0.9980f;
			p.harmonic_rolloff = 0.75f;
			p.lpf_cutoff = 0.22f; // Warm low-pass for deep, round bass tone
		} else if (program < 48) {
			// Strings (40..47): Violin, Viola, Cello, Contrabass, Tremolo, Pizzicato, Harp, Timpani
			p.family = InstrumentFamily::Strings;
			if (program == 45) { // Pizzicato
				p.attack_time = 0.002f;
				p.decay_rate = 0.9996f;
				p.sustain_level = 0.0f;
				p.release_rate = 0.9950f;
				p.harmonic_rolloff = 0.65f;
				p.lpf_cutoff = 0.35f;
			} else if (program == 46) { // Harp
				p.attack_time = 0.003f;
				p.decay_rate = 0.99985f;
				p.sustain_level = 0.15f;
				p.release_rate = 0.9970f;
				p.harmonic_rolloff = 0.70f;
				p.lpf_cutoff = 0.38f;
			} else {
				p.attack_time = 0.060f;
				p.decay_rate = 0.99998f;
				p.sustain_level = 0.85f;
				p.release_rate = 0.9970f;
				p.harmonic_rolloff = 0.55f;
				p.lpf_cutoff = 0.28f;
				p.vibrato_depth = 0.004f;
				p.vibrato_rate = 5.5f;
				p.detune_amount = 0.0010f;
			}
		} else if (program < 56) {
			// Ensemble (48..55): String Ensembles 1/2, Synth Strings 1/2, Choir Aahs, Voice Oohs, Synth Voice, Orchestra Hit
			p.family = InstrumentFamily::Ensemble;
			p.attack_time = 0.090f;
			p.decay_rate = 0.99998f;
			p.sustain_level = 0.90f;
			p.release_rate = 0.9965f;
			p.harmonic_rolloff = 0.60f;
			p.lpf_cutoff = 0.25f;
			p.detune_amount = 0.0015f; // Lush stereo chorus
		} else if (program < 64) {
			// Brass (56..63): Trumpet, Trombone, Tuba, Muted Trumpet, French Horn, Brass Section, Synth Brass
			p.family = InstrumentFamily::Brass;
			p.attack_time = 0.035f;
			p.decay_rate = 0.99996f;
			p.sustain_level = 0.80f;
			p.release_rate = 0.9970f;
			p.harmonic_rolloff = 0.48f;
			p.lpf_cutoff = 0.32f;
			p.vibrato_depth = 0.002f;
			p.vibrato_rate = 5.0f;
		} else if (program < 72) {
			// Reed (64..71): Soprano/Alto/Tenor/Baritone Sax, Oboe, English Horn, Bassoon, Clarinet
			p.family = InstrumentFamily::Reed;
			p.attack_time = 0.025f;
			p.decay_rate = 0.99997f;
			p.sustain_level = 0.85f;
			p.release_rate = 0.9975f;
			p.harmonic_rolloff = 0.52f;
			p.lpf_cutoff = 0.30f;
			p.vibrato_depth = 0.003f;
			p.vibrato_rate = 5.2f;
		} else if (program < 80) {
			// Pipe (72..79): Piccolo, Flute, Recorder, Pan Flute, Blown Bottle, Shakuhachi, Whistle, Ocarina
			p.family = InstrumentFamily::Pipe;
			p.attack_time = 0.030f;
			p.decay_rate = 0.99998f;
			p.sustain_level = 0.90f;
			p.release_rate = 0.9980f;
			p.harmonic_rolloff = 0.85f; // Soft pure fundamental
			p.lpf_cutoff = 0.25f;
			p.vibrato_depth = 0.0035f;
			p.vibrato_rate = 5.0f;
		} else if (program < 88) {
			// Synth Lead (80..87)
			p.family = InstrumentFamily::SynthLead;
			p.attack_time = 0.008f;
			p.decay_rate = 0.99996f;
			p.sustain_level = 0.80f;
			p.release_rate = 0.9970f;
			p.harmonic_rolloff = 0.45f;
			p.lpf_cutoff = 0.35f;
			p.detune_amount = 0.0010f;
		} else if (program < 96) {
			// Synth Pad (88..95): Warm Pad, Polysynth, Choir, Bowed, Metallic, Halo, Sweep
			p.family = InstrumentFamily::SynthPad;
			p.attack_time = 0.120f;
			p.decay_rate = 0.99998f;
			p.sustain_level = 0.92f;
			p.release_rate = 0.9960f;
			p.harmonic_rolloff = 0.70f;
			p.lpf_cutoff = 0.20f; // Very soft and warm
			p.detune_amount = 0.0018f;
		} else {
			// FX, Ethnic, Percussive, Sound Effects (96..127)
			p.family = InstrumentFamily::SoundFX;
			p.attack_time = 0.015f;
			p.decay_rate = 0.99990f;
			p.sustain_level = 0.60f;
			p.release_rate = 0.9970f;
			p.harmonic_rolloff = 0.60f;
			p.lpf_cutoff = 0.30f;
		}
		return p;
	}

	static InstrumentPatch GetDrumPatch(uint8 note) {
		InstrumentPatch p{};
		p.attack_time = 0.001f;
		p.sustain_level = 0.0f;
		p.detune_amount = 0.0f;
		p.vibrato_depth = 0.0f;

		if (note == 35 || note == 36) {
			// Bass Drum 1 / Acoustic Bass Drum
			p.family = InstrumentFamily::DrumKick;
			p.decay_rate = 0.99970f;
			p.release_rate = 0.9992f;
			p.lpf_cutoff = 0.20f;
		} else if (note == 38 || note == 40) {
			// Snare (Acoustic / Electric)
			p.family = InstrumentFamily::DrumSnare;
			p.decay_rate = 0.99960f;
			p.release_rate = 0.9985f;
			p.lpf_cutoff = 0.35f;
		} else if (note == 42 || note == 44) {
			// Closed / Pedal Hi-Hat
			p.family = InstrumentFamily::DrumHiHat;
			p.decay_rate = 0.99850f;
			p.release_rate = 0.9950f;
			p.lpf_cutoff = 0.50f;
		} else if (note == 46) {
			// Open Hi-Hat
			p.family = InstrumentFamily::DrumHiHat;
			p.decay_rate = 0.99975f;
			p.release_rate = 0.9985f;
			p.lpf_cutoff = 0.45f;
		} else if (note == 49 || note == 51 || note == 52 || note == 55 || note == 57) {
			// Crash / Ride Cymbals, Splash, China
			p.family = InstrumentFamily::DrumCymbal;
			p.decay_rate = 0.99990f;
			p.release_rate = 0.9990f;
			p.lpf_cutoff = 0.40f;
		} else if (note >= 41 && note <= 50) {
			// Toms
			p.family = InstrumentFamily::DrumTom;
			p.decay_rate = 0.99965f;
			p.release_rate = 0.9990f;
			p.lpf_cutoff = 0.25f;
		} else {
			// Other Percussion
			p.family = InstrumentFamily::DrumPercussion;
			p.decay_rate = 0.99950f;
			p.release_rate = 0.9980f;
			p.lpf_cutoff = 0.35f;
		}
		return p;
	}

	enum class VoiceStage : uint8 {
		Idle,
		Attack,
		Decay,
		Sustain,
		Release
	};

	struct SynthVoice {
		VoiceStage      stage;
		uint8           channel;
		uint8           note;
		uint8           velocity;
		bool            is_drum;
		float           base_freq;
		float           phase1;
		float           phase2;
		float           vibrato_phase;
		float           env_gain;
		float           filter_state_l;
		float           filter_state_r;
		float           drum_freq_sweep;
		float           noise_lpf;
		InstrumentPatch patch;
	};

	constexpr uint32 kMaxPolyphony = 64;

	struct MidiChannelState {
		uint8  program;
		uint8  volume;     // CC 7 (default 100)
		uint8  pan;        // CC 10 (default 64)
		uint8  expression; // CC 11 (default 127)
		bool   sustain;    // CC 64
		int16  pitch_bend; // 0..16383, center 8192 (+/- 2 semitones)
	};

	struct MidiTrackState {
		uint64 file_offset;
		uint32 length;
		uint32 current_offset;
		uint64 current_tick;
		uint64 next_event_tick;
		uint8  running_status;
		bool   ended;
	};

}

namespace uni {

	namespace Engine {

		class MIDIStream : public uni::IAudioStream {
		private:
			uni::StorageTrait&    m_storage;
			uni::trait::Malloc&   m_allocator;
			uni::AudioInfo        m_info;

			uint16                m_format;
			uint16                m_num_tracks;
			uint16                m_time_division;

			MidiTrackState*       m_tracks;
			uint8*                m_track_data[128];
			MidiChannelState      m_channels[16];
			SynthVoice            m_voices[kMaxPolyphony];

			uint32                m_sample_rate;
			uint32                m_tempo_us;
			uint64                m_current_sample;
			uint64                m_current_tick;
			double                m_tick_accumulator;
			uint32                m_rand_seed;
			bool                  m_eos;

			// Master bus smoothing filter state
			float                 m_master_lpf_l;
			float                 m_master_lpf_r;

		public:
			MIDIStream(uni::StorageTrait& storage, uni::trait::Malloc& alloc)
				: m_storage(storage), m_allocator(alloc),
				  m_format(0), m_num_tracks(0), m_time_division(480),
				  m_tracks(nullptr), m_sample_rate(44100), m_tempo_us(500000),
				  m_current_sample(0), m_current_tick(0), m_tick_accumulator(0.0),
				  m_rand_seed(0x12345678), m_eos(false),
				  m_master_lpf_l(0.0f), m_master_lpf_r(0.0f) {
				MemSet(&m_info, 0, sizeof(m_info));
				MemSet(m_track_data, 0, sizeof(m_track_data));
				resetSynthesizer();
			}

			virtual ~MIDIStream() {
				cleanup();
			}

			void cleanup() {
				if (m_tracks) {
					m_allocator.deallocate(m_tracks);
					m_tracks = nullptr;
				}
				for (uint32 i = 0; i < 128; ++i) {
					if (m_track_data[i]) {
						m_allocator.deallocate(m_track_data[i]);
						m_track_data[i] = nullptr;
					}
				}
			}

			float nextNoise() {
				m_rand_seed = m_rand_seed * 1103515245 + 12345;
				return ((float)((m_rand_seed >> 16) & 0x7FFF) / 16384.0f) - 1.0f;
			}

			void resetSynthesizer() {
				EnsureSinTable();
				EnsureNoteFreqTable();

				for (uint32 i = 0; i < 16; ++i) {
					m_channels[i].program = 0;
					m_channels[i].volume = 100;
					m_channels[i].pan = 64;
					m_channels[i].expression = 127;
					m_channels[i].sustain = false;
					m_channels[i].pitch_bend = 8192;
				}

				for (uint32 i = 0; i < kMaxPolyphony; ++i) {
					m_voices[i].stage = VoiceStage::Idle;
					m_voices[i].env_gain = 0.0f;
					m_voices[i].phase1 = 0.0f;
					m_voices[i].phase2 = 0.0f;
					m_voices[i].vibrato_phase = 0.0f;
					m_voices[i].filter_state_l = 0.0f;
					m_voices[i].filter_state_r = 0.0f;
					m_voices[i].drum_freq_sweep = 0.0f;
					m_voices[i].noise_lpf = 0.0f;
				}

				m_master_lpf_l = 0.0f;
				m_master_lpf_r = 0.0f;
			}

			bool init() {
				uint64 total_bytes = m_storage.getUnits() * m_storage.Block_Size;
				if (total_bytes < 14) return false;

				uint8 header_buf[14];
				for (uint32 i = 0; i < 14; ++i) {
					int b = m_storage[i];
					if (b < 0) return false;
					header_buf[i] = (uint8)b;
				}

				if (header_buf[0] != 'M' || header_buf[1] != 'T' || header_buf[2] != 'h' || header_buf[3] != 'd') {
					return false;
				}

				uint32 header_len = ReadBe32(header_buf + 4);
				if (header_len < 6) return false;

				m_format = ReadBe16(header_buf + 8);
				m_num_tracks = ReadBe16(header_buf + 10);
				m_time_division = ReadBe16(header_buf + 12);

				if (m_num_tracks == 0 || m_time_division == 0) return false;
				if (m_num_tracks > 128) m_num_tracks = 128;

				m_tracks = (MidiTrackState*)m_allocator.allocate(m_num_tracks * sizeof(MidiTrackState));
				if (!m_tracks) return false;

				uint64 cur_pos = 8 + header_len;
				for (uint32 t = 0; t < m_num_tracks; ++t) {
					if (cur_pos + 8 > total_bytes) {
						m_num_tracks = (uint16)t;
						break;
					}

					uint8 trk_hdr[8];
					for (uint32 i = 0; i < 8; ++i) {
						int b = m_storage[cur_pos + i];
						if (b < 0) break;
						trk_hdr[i] = (uint8)b;
					}

					if (trk_hdr[0] != 'M' || trk_hdr[1] != 'T' || trk_hdr[2] != 'r' || trk_hdr[3] != 'k') {
						bool found = false;
						while (cur_pos + 8 <= total_bytes) {
							if (m_storage[cur_pos] == 'M' && m_storage[cur_pos + 1] == 'T' &&
								m_storage[cur_pos + 2] == 'r' && m_storage[cur_pos + 3] == 'k') {
								for (uint32 i = 0; i < 8; ++i) trk_hdr[i] = (uint8)m_storage[cur_pos + i];
								found = true;
								break;
							}
							cur_pos++;
						}
						if (!found) {
							m_num_tracks = (uint16)t;
							break;
						}
					}

					uint32 trk_len = ReadBe32(trk_hdr + 4);
					m_tracks[t].file_offset = cur_pos + 8;
					m_tracks[t].length = trk_len;
					m_tracks[t].current_offset = 0;
					m_tracks[t].current_tick = 0;
					m_tracks[t].running_status = 0;
					m_tracks[t].ended = false;

					if (trk_len > 0 && trk_len <= 512 * 1024) {
						m_track_data[t] = (uint8*)m_allocator.allocate(trk_len);
						if (m_track_data[t]) {
							for (uint32 i = 0; i < trk_len; ++i) {
								int b = m_storage[m_tracks[t].file_offset + i];
								m_track_data[t][i] = (b >= 0) ? (uint8)b : 0;
							}
						}
					}

					uint64 off = 0;
					uint32 dt = 0;
					if (m_track_data[t]) {
						uint32 mem_off = 0;
						dt = ReadVlq(m_track_data[t], trk_len, mem_off);
						m_tracks[t].current_offset = mem_off;
					} else {
						dt = ReadStorageVlq(m_storage, m_tracks[t].file_offset + trk_len, off);
						m_tracks[t].current_offset = (uint32)off;
					}
					m_tracks[t].next_event_tick = dt;

					cur_pos += 8 + trk_len;
				}

				if (m_num_tracks == 0) return false;

				// Scan duration
				uint64 max_ticks = 0;
				for (uint32 t = 0; t < m_num_tracks; ++t) {
					uint64 trk_tick = 0;
					uint32 trk_off = 0;
					uint8 run_status = 0;
					const uint8* data = m_track_data[t];
					uint32 tlen = m_tracks[t].length;

					while (data && trk_off < tlen) {
						uint32 dt = ReadVlq(data, tlen, trk_off);
						trk_tick += dt;
						if (trk_off >= tlen) break;

						uint8 status = data[trk_off];
						if (status & 0x80) {
							run_status = status;
							trk_off++;
						} else {
							status = run_status;
						}

						if (status == 0xFF) {
							if (trk_off >= tlen) break;
							uint8 meta_type = data[trk_off++];
							uint32 meta_len = ReadVlq(data, tlen, trk_off);
							trk_off += meta_len;
							if (meta_type == 0x2F) break;
						} else if (status == 0xF0 || status == 0xF7) {
							uint32 sysex_len = ReadVlq(data, tlen, trk_off);
							trk_off += sysex_len;
						} else {
							uint8 cmd = status & 0xF0;
							if (cmd == 0xC0 || cmd == 0xD0) {
								trk_off += 1;
							} else {
								trk_off += 2;
							}
						}
					}
					if (trk_tick > max_ticks) max_ticks = trk_tick;
				}

				uint32 div_val = (m_time_division & 0x8000) ? 480 : m_time_division;
				if (div_val == 0) div_val = 480;
				uint64 duration_ms = (max_ticks * (uint64)m_tempo_us) / ((uint64)div_val * 1000);
				if (duration_ms < 1000) duration_ms = 1000;

				m_sample_rate = 44100;
				m_info.format.sample_format = uni::AudioSampleFormat::S16LE;
				m_info.format.channels = 2;
				m_info.format.sample_rate = m_sample_rate;
				m_info.containerFormat = uni::AudioContainerFormat::MIDI;
				m_info.bitsPerSample = 16;
				m_info.durationMs = (uint32)duration_ms;
				m_info.totalSamples = (uint32)(((uint64)duration_ms * m_sample_rate) / 1000);
				m_info.dataByteLength = m_info.totalSamples * 4;

				return true;
			}

			void noteOn(uint8 ch, uint8 note, uint8 velocity) {
				if (velocity == 0) {
					noteOff(ch, note);
					return;
				}
				if (note >= 128) return;

				int free_slot = -1;
				float lowest_env = 1e9f;
				int oldest_slot = 0;

				for (uint32 i = 0; i < kMaxPolyphony; ++i) {
					if (m_voices[i].stage == VoiceStage::Idle) {
						free_slot = (int)i;
						break;
					}
					if (m_voices[i].env_gain < lowest_env) {
						lowest_env = m_voices[i].env_gain;
						oldest_slot = (int)i;
					}
				}

				if (free_slot < 0) free_slot = oldest_slot;

				SynthVoice& v = m_voices[free_slot];
				v.stage = VoiceStage::Attack;
				v.channel = ch;
				v.note = note;
				v.velocity = velocity;
				v.is_drum = (ch == 9);
				v.base_freq = s_note_freq[note];
				v.phase1 = 0.0f;
				v.phase2 = 0.25f; // phase offset for stereo richness
				v.vibrato_phase = 0.0f;
				v.env_gain = 0.0f;
				v.filter_state_l = 0.0f;
				v.filter_state_r = 0.0f;
				v.drum_freq_sweep = 1.0f;
				v.noise_lpf = 0.0f;

				if (v.is_drum) {
					v.patch = GetDrumPatch(note);
				} else {
					v.patch = GetGMPatch(m_channels[ch].program);
				}
			}

			void noteOff(uint8 ch, uint8 note) {
				for (uint32 i = 0; i < kMaxPolyphony; ++i) {
					if (m_voices[i].stage != VoiceStage::Idle &&
						m_voices[i].channel == ch &&
						m_voices[i].note == note) {
						if (!m_channels[ch].sustain) {
							m_voices[i].stage = VoiceStage::Release;
						}
					}
				}
			}

			void processNextEvents() {
				bool all_ended = true;

				for (uint32 t = 0; t < m_num_tracks; ++t) {
					MidiTrackState& trk = m_tracks[t];
					if (trk.ended) continue;
					all_ended = false;

					while (!trk.ended && trk.next_event_tick <= m_current_tick) {
						const uint8* data = m_track_data[t];
						uint32 tlen = trk.length;
						uint32& off = trk.current_offset;

						if (!data || off >= tlen) {
							trk.ended = true;
							break;
						}

						uint8 status = data[off];
						if (status & 0x80) {
							trk.running_status = status;
							off++;
						} else {
							status = trk.running_status;
						}

						if (status == 0xFF) { // Meta Event
							if (off >= tlen) { trk.ended = true; break; }
							uint8 meta_type = data[off++];
							uint32 meta_len = ReadVlq(data, tlen, off);
							if (meta_type == 0x51 && meta_len == 3 && off + 3 <= tlen) { // Set Tempo
								m_tempo_us = ReadBe24(data + off);
								if (m_tempo_us == 0) m_tempo_us = 500000;
							} else if (meta_type == 0x2F) {
								trk.ended = true;
							}
							off += meta_len;
						} else if (status == 0xF0 || status == 0xF7) {
							uint32 sysex_len = ReadVlq(data, tlen, off);
							off += sysex_len;
						} else {
							uint8 cmd = status & 0xF0;
							uint8 ch = status & 0x0F;

							if (cmd == 0x80) { // Note Off
								if (off + 2 <= tlen) {
									uint8 note = data[off++];
									/* uint8 vel = */ data[off++];
									noteOff(ch, note);
								}
							} else if (cmd == 0x90) { // Note On
								if (off + 2 <= tlen) {
									uint8 note = data[off++];
									uint8 vel = data[off++];
									noteOn(ch, note, vel);
								}
							} else if (cmd == 0xA0) {
								off += 2;
							} else if (cmd == 0xB0) { // Control Change
								if (off + 2 <= tlen) {
									uint8 cc = data[off++];
									uint8 val = data[off++];
									if (cc == 7) m_channels[ch].volume = val;
									else if (cc == 10) m_channels[ch].pan = val;
									else if (cc == 11) m_channels[ch].expression = val;
									else if (cc == 64) {
										m_channels[ch].sustain = (val >= 64);
										if (!m_channels[ch].sustain) {
											for (uint32 i = 0; i < kMaxPolyphony; ++i) {
												if (m_voices[i].channel == ch && m_voices[i].stage == VoiceStage::Sustain) {
													m_voices[i].stage = VoiceStage::Release;
												}
											}
										}
									} else if (cc == 120 || cc == 123) {
										for (uint32 i = 0; i < kMaxPolyphony; ++i) {
											if (m_voices[i].channel == ch) m_voices[i].stage = VoiceStage::Release;
										}
									}
								}
							} else if (cmd == 0xC0) { // Program Change
								if (off + 1 <= tlen) {
									m_channels[ch].program = data[off++];
								}
							} else if (cmd == 0xD0) {
								off += 1;
							} else if (cmd == 0xE0) { // Pitch Bend
								if (off + 2 <= tlen) {
									uint8 lsb = data[off++];
									uint8 msb = data[off++];
									m_channels[ch].pitch_bend = (int16)((msb << 7) | lsb);
								}
							}
						}

						if (off < tlen && !trk.ended) {
							uint32 dt = ReadVlq(data, tlen, off);
							trk.next_event_tick += dt;
						} else {
							trk.ended = true;
						}
					}
				}

				if (all_ended) {
					m_eos = true;
				}
			}

			void renderAudioBlock(int16* buffer, uint32 frame_count) {
				const float dt_sec = 1.0f / (float)m_sample_rate;
				uint32 div_val = (m_time_division & 0x8000) ? 480 : m_time_division;
				if (div_val == 0) div_val = 480;

				for (uint32 f = 0; f < frame_count; ++f) {
					// Tick advance calculation
					double ticks_per_sample = ((double)div_val * 1000000.0) / ((double)m_tempo_us * (double)m_sample_rate);
					m_tick_accumulator += ticks_per_sample;
					while (m_tick_accumulator >= 1.0) {
						m_tick_accumulator -= 1.0;
						m_current_tick++;
						processNextEvents();
					}

					float sum_l = 0.0f;
					float sum_r = 0.0f;

					for (uint32 i = 0; i < kMaxPolyphony; ++i) {
						SynthVoice& v = m_voices[i];
						if (v.stage == VoiceStage::Idle) continue;

						// ADSR Envelope
						if (v.stage == VoiceStage::Attack) {
							float attack_step = dt_sec / (v.patch.attack_time > 0.001f ? v.patch.attack_time : 0.001f);
							v.env_gain += attack_step;
							if (v.env_gain >= 1.0f) {
								v.env_gain = 1.0f;
								v.stage = VoiceStage::Decay;
							}
						} else if (v.stage == VoiceStage::Decay) {
							v.env_gain *= v.patch.decay_rate;
							if (v.env_gain <= v.patch.sustain_level) {
								v.env_gain = v.patch.sustain_level;
								v.stage = (v.patch.sustain_level > 0.005f) ? VoiceStage::Sustain : VoiceStage::Idle;
							}
						} else if (v.stage == VoiceStage::Release) {
							v.env_gain *= v.patch.release_rate;
							if (v.env_gain <= 0.0005f) {
								v.env_gain = 0.0f;
								v.stage = VoiceStage::Idle;
							}
						}

						if (v.stage == VoiceStage::Idle || v.env_gain <= 0.0005f) continue;

						// Pitch Calculation with Pitch Bend & Vibrato
						float bend_semitones = ((float)m_channels[v.channel].pitch_bend - 8192.0f) / 8192.0f * 2.0f;
						float actual_freq = v.base_freq;
						if (bend_semitones != 0.0f && !v.is_drum) {
							actual_freq *= (1.0f + bend_semitones * 0.057762265f);
						}

						if (v.patch.vibrato_depth > 0.0f && !v.is_drum) {
							v.vibrato_phase += v.patch.vibrato_rate * dt_sec;
							if (v.vibrato_phase >= 1.0f) v.vibrato_phase -= (int)v.vibrato_phase;
							float vib = FastSin(v.vibrato_phase) * v.patch.vibrato_depth;
							actual_freq *= (1.0f + vib);
						}

						// Acoustic Multi-Harmonic Waveform Synthesis
						float wave_l = 0.0f;
						float wave_r = 0.0f;

						float f1 = actual_freq * (1.0f - v.patch.detune_amount);
						float f2 = actual_freq * (1.0f + v.patch.detune_amount);

						v.phase1 += f1 * dt_sec;
						if (v.phase1 >= 1.0f) v.phase1 -= (int)v.phase1;
						v.phase2 += f2 * dt_sec;
						if (v.phase2 >= 1.0f) v.phase2 -= (int)v.phase2;

						if (v.is_drum) {
							// Drum Synthesis
							if (v.patch.family == InstrumentFamily::DrumKick) {
								v.drum_freq_sweep *= 0.9985f;
								float kick_freq = 45.0f + 110.0f * v.drum_freq_sweep;
								v.phase1 += kick_freq * dt_sec;
								if (v.phase1 >= 1.0f) v.phase1 -= (int)v.phase1;
								wave_l = FastSin(v.phase1) * 1.2f;
								wave_r = wave_l;
							} else if (v.patch.family == InstrumentFamily::DrumSnare) {
								v.drum_freq_sweep *= 0.9970f;
								float snare_tone = FastSin(v.phase1 * 2.0f);
								float noise = nextNoise();
								v.noise_lpf += 0.35f * (noise - v.noise_lpf);
								wave_l = 0.5f * snare_tone + 0.5f * v.noise_lpf;
								wave_r = wave_l;
							} else if (v.patch.family == InstrumentFamily::DrumHiHat || v.patch.family == InstrumentFamily::DrumCymbal) {
								float noise = nextNoise();
								v.noise_lpf += 0.6f * (noise - v.noise_lpf);
								float hi_noise = noise - v.noise_lpf; // High-pass filtered noise
								wave_l = hi_noise * 0.7f;
								wave_r = hi_noise * 0.7f;
							} else if (v.patch.family == InstrumentFamily::DrumTom) {
								v.drum_freq_sweep *= 0.9980f;
								float tom_freq = 65.0f + 120.0f * v.drum_freq_sweep;
								v.phase1 += tom_freq * dt_sec;
								if (v.phase1 >= 1.0f) v.phase1 -= (int)v.phase1;
								wave_l = FastSin(v.phase1);
								wave_r = wave_l;
							} else {
								float noise = nextNoise();
								v.noise_lpf += 0.3f * (noise - v.noise_lpf);
								wave_l = v.noise_lpf;
								wave_r = wave_l;
							}
						} else {
							// Melodic Instruments: Multi-Harmonic Additive Synthesis with Natural Rolloff
							float r = v.patch.harmonic_rolloff; // Harmonic attenuation factor
							float r2 = r * r;
							float r3 = r2 * r;
							float r4 = r3 * r;

							// Oscillator 1 (Left / Center)
							float osc1 = FastSin(v.phase1) +
								r * FastSin(v.phase1 * 2.0f) +
								r2 * FastSin(v.phase1 * 3.0f) +
								r3 * FastSin(v.phase1 * 4.0f) +
								r4 * FastSin(v.phase1 * 5.0f);

							// Oscillator 2 (Right / Stereo Detune)
							float osc2 = FastSin(v.phase2) +
								r * FastSin(v.phase2 * 2.0f) +
								r2 * FastSin(v.phase2 * 3.0f) +
								r3 * FastSin(v.phase2 * 4.0f) +
								r4 * FastSin(v.phase2 * 5.0f);

							float norm = 1.0f / (1.0f + r + r2 + r3 + r4);
							wave_l = osc1 * norm;
							wave_r = osc2 * norm;
						}

						// Velocity & Dynamic Low-Pass Filter (removes digital harshness)
						float vel_scale = (float)v.velocity / 127.0f;
						float cutoff = v.patch.lpf_cutoff * (0.5f + 0.5f * vel_scale);
						if (cutoff > 0.8f) cutoff = 0.8f;

						v.filter_state_l += cutoff * (wave_l - v.filter_state_l);
						v.filter_state_r += cutoff * (wave_r - v.filter_state_r);

						// Amplitude and Panning
						float voice_gain = vel_scale * v.env_gain * 0.35f;
						float ch_gain = ((float)m_channels[v.channel].volume / 127.0f) *
							((float)m_channels[v.channel].expression / 127.0f);

						float pan = (float)m_channels[v.channel].pan / 127.0f;
						sum_l += v.filter_state_l * voice_gain * ch_gain * (1.0f - pan * 0.7f);
						sum_r += v.filter_state_r * voice_gain * ch_gain * (0.3f + pan * 0.7f);
					}

					// Master bus gentle lowpass filter to unify mix
					m_master_lpf_l += 0.85f * (sum_l - m_master_lpf_l);
					m_master_lpf_r += 0.85f * (sum_r - m_master_lpf_r);

					// Master 16-bit PCM output
					int32 pcm_l = (int32)(m_master_lpf_l * 32767.0f);
					int32 pcm_r = (int32)(m_master_lpf_r * 32767.0f);

					if (pcm_l > 32767) pcm_l = 32767;
					if (pcm_l < -32768) pcm_l = -32768;
					if (pcm_r > 32767) pcm_r = 32767;
					if (pcm_r < -32768) pcm_r = -32768;

					buffer[f * 2 + 0] = (int16)pcm_l;
					buffer[f * 2 + 1] = (int16)pcm_r;

					m_current_sample++;
				}
			}

			virtual uni::AudioResult ReadSamples(void* destBuffer, uint32 maxBytes, uint32& bytesRead) override {
				bytesRead = 0;
				if (!destBuffer || maxBytes < 4) return uni::AudioResult::InvalidArgument;

				uint32 requested_frames = maxBytes / 4;
				int16* out_ptr = (int16*)destBuffer;

				renderAudioBlock(out_ptr, requested_frames);
				bytesRead = requested_frames * 4;

				if (m_eos) {
					bool active_voices = false;
					for (uint32 i = 0; i < kMaxPolyphony; ++i) {
						if (m_voices[i].stage != VoiceStage::Idle) {
							active_voices = true;
							break;
						}
					}
					if (!active_voices) return uni::AudioResult::EndOfStream;
				}

				return uni::AudioResult::OK;
			}

			virtual uni::AudioResult Seek(uint32 sampleIndex) override {
				resetSynthesizer();
				m_current_sample = 0;
				m_current_tick = 0;
				m_tick_accumulator = 0.0;
				m_tempo_us = 500000;
				m_eos = false;

				for (uint32 t = 0; t < m_num_tracks; ++t) {
					m_tracks[t].current_offset = 0;
					m_tracks[t].current_tick = 0;
					m_tracks[t].running_status = 0;
					m_tracks[t].ended = false;

					if (m_track_data[t]) {
						uint32 off = 0;
						uint32 dt = ReadVlq(m_track_data[t], m_tracks[t].length, off);
						m_tracks[t].current_offset = off;
						m_tracks[t].next_event_tick = dt;
					}
				}

				uint32 div_val = (m_time_division & 0x8000) ? 480 : m_time_division;
				if (div_val == 0) div_val = 480;

				while (m_current_sample < sampleIndex && !m_eos) {
					double ticks_per_sample = ((double)div_val * 1000000.0) / ((double)m_tempo_us * (double)m_sample_rate);
					m_tick_accumulator += ticks_per_sample;
					while (m_tick_accumulator >= 1.0) {
						m_tick_accumulator -= 1.0;
						m_current_tick++;
						processNextEvents();
					}
					m_current_sample++;
				}

				return uni::AudioResult::OK;
			}

			virtual void Release() override {
				cleanup();
				m_allocator.deallocate(this);
			}

			virtual uni::AudioResult GetInfo(uni::AudioInfo& outInfo) const override {
				outInfo = m_info;
				return uni::AudioResult::OK;
			}
		};

	} // namespace Engine

	const char* MIDICodec::GetName() const {
		return "Standard MIDI File";
	}

	AudioContainerFormat MIDICodec::GetFormat() const {
		return AudioContainerFormat::MIDI;
	}

	const char* const* MIDICodec::GetExtensions() const {
		static const char* exts[] = { "mid", "midi", nullptr };
		return exts;
	}

	AudioResult MIDICodec::Probe(StorageTrait& storage, bool& matched) const {
		matched = false;
		if (storage.getUnits() * storage.Block_Size < 4) return AudioResult::OK;
		if (storage[0] == 'M' && storage[1] == 'T' && storage[2] == 'h' && storage[3] == 'd') {
			matched = true;
		}
		return AudioResult::OK;
	}

	AudioResult MIDICodec::ReadInfo(StorageTrait& storage, AudioInfo& outInfo) const {
		trait::Malloc* alloc = nullptr;
		IAudioStream* stream = nullptr;
		AudioResult res = OpenStream(storage, stream, *alloc);
		if (res == AudioResult::OK && stream) {
			res = stream->GetInfo(outInfo);
			stream->Release();
		}
		return res;
	}

	AudioResult MIDICodec::OpenStream(
		StorageTrait& storage,
		IAudioStream*& outStream,
		trait::Malloc& allocator
	) const {
		outStream = nullptr;
		void* mem = allocator.allocate(sizeof(Engine::MIDIStream));
		if (!mem) return AudioResult::OutOfMemory;

		Engine::MIDIStream* stream = new (mem) Engine::MIDIStream(storage, allocator);
		if (!stream || !stream->init()) {
			if (stream) stream->Release();
			else allocator.deallocate(mem);
			return AudioResult::InvalidFormat;
		}

		outStream = stream;
		return AudioResult::OK;
	}

} // namespace uni
