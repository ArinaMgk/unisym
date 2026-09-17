// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device.Audio) Audio Bridge (SAI1 Block A + DMA1 Stream5)
// Codifiers: @dosconio
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0

#ifndef _INCPP_Device_Audio_AudioBridge
#define _INCPP_Device_Audio_AudioBridge


#include "../../../c/stdinc.h"
#ifdef _MCU_STM32

#include "../../../c/consio.h"
#include "../../MCU/ST/STM32H7"
#include "../../Device/SAI"
#include "../../Device/DMA"
#include "../Audiosys.hpp"


namespace uni {

	// AudioBridge: the board-level AudioControlInterface implementation.
	// Data plane   : ring double buffer -> DMA1 Stream5 -> SAI1 Block A -> I2S lines
	// Control plane: AudioDeviceInterface (SubACI) -> codec registers over I2C
	// SAI/DMA and the codec are two parallel legs driven by this bridge; neither is above the other.
	// The caller provides the DMA buffers, their count, size and element width.
	class AudioBridge : public AudioControlInterface {
		AudioDeviceInterface* chip;// nullable: SAI/DMA still run without a codec
		byte** buffers;// caller-owned, must be reachable by DMA1
		uint32 buffer_count;// DMA double-buffer mode provides 2 regions (M0/M1)
		uint32 buffer_size;// bytes per buffer
		uint32 element_size;// DMA element width in bytes (2 = 16-bit samples)
		AudioPcmRefillHandler refill_cb;
		void* refill_context;
		AudioFormat format;

		volatile uint32 dma_done_count[2];// XferCplt (M0) / XferM1Cplt (M1) events
		volatile uint32 dma_underruns;// kept from the original engine; currently only reset
		uint32 buffer_bytes[2];// engine bytes currently held by each DMA buffer
		uint32 played_bytes;// engine bytes already consumed by the DMA
		bool need_refill[2];
		bool running;
		bool paused;
		bool stream_over;

		// One bridge per program: the DMA ISRs forward to the active instance.
		static AudioBridge*& active() {
			static AudioBridge* instance = nullptr;
			return instance;
		}

		static void on_dma_xfer_cplt0() {
			if (active()) active()->dma_done_count[0]++;
		}

		static void on_dma_xfer_cplt1() {
			if (active()) active()->dma_done_count[1]++;
		}

		// PLL2 presets for SAI1 (HSE = 25MHz, PLL2M = 25 -> 1MHz VCO input).
		// OSR = 0: Fs = freq/(256*MCKDIV) -> MCKDIV = freq/(256*Fs)
		struct SAIPreset {
			uint16 rate_div10;// sample rate / 10
			uint16 pll2n;
			uint8  pll2p;
		};

		static const SAIPreset* RateTable(stduint& count) {
			static const SAIPreset tbl[] = {
				{ 800 , 344, 7 },// 8kHz     (ker_ck = 49.143MHz, MCKDIV = 24 -> 7998Hz)
				{ 1102, 429, 2 },// 11.025kHz(ker_ck = 214.5MHz,  MCKDIV = 76 -> above the 63 limit, unsupported)
				{ 1600, 344, 7 },// 16kHz    (ker_ck = 49.143MHz, MCKDIV = 12 -> 15995Hz)
				{ 2205, 429, 2 },// 22.05kHz (ker_ck = 214.5MHz,  MCKDIV = 38 -> 22049Hz)
				{ 3200, 344, 7 },// 32kHz    (ker_ck = 49.143MHz, MCKDIV = 6  -> 31990Hz)
				{ 4410, 271, 2 },// 44.1kHz  (ker_ck = 135.5MHz,  MCKDIV = 12 -> 44108Hz)
				{ 4800, 344, 7 },// 48kHz    (ker_ck = 49.143MHz, MCKDIV = 4  -> 47986Hz)
				{ 8820, 271, 2 },// 88.2kHz  (ker_ck = 135.5MHz,  MCKDIV = 6  -> 88216Hz)
				{ 9600, 344, 7 },// 96kHz    (ker_ck = 49.143MHz, MCKDIV = 2  -> 95971Hz)
				{ 17640,271, 6 },// 176.4kHz (ker_ck = 45.166MHz, MCKDIV = 1  -> 176430Hz)
				{ 19200,295, 6 },// 192kHz   (ker_ck = 49.166MHz, MCKDIV = 1  -> 192055Hz)
			};
			count = sizeof(tbl) / sizeof(tbl[0]);
			return tbl;
		}

		// Route PLL2_P to SAI1 and program its master clock divider for the wanted frame rate.
		bool sai_set_samplerate(uint32 samplerate) {
			uint16 rate_div10 = (uint16)(samplerate / 10);
			stduint tbl_count = 0;
			const SAIPreset* tbl = RateTable(tbl_count);
			const SAIPreset* match = nullptr;
			for (stduint i = 0; i < tbl_count; i++) {
				if (tbl[i].rate_div10 == rate_div10) {
					match = &tbl[i];
					break;
				}
			}
			if (!match) {
				outsfmt("[SAI] Unsupported sample rate: %u Hz\r\n", (unsigned)samplerate);
				return false;
			}

			// Disable SAI1 Block A
			SAI1[1].enAble(false);

			// Turn PLL2 off
			RCC[RCCReg::CR].setof(26, false);
			while (RCC[RCCReg::CR].bitof(27));// wait PLL2RDY = 0

			// PLL2: HSE(25MHz) / 25 = 1MHz
			RCC[RCCReg::PLLCKSELR].maset(12, 6, 25);// DIVM2 = 25
			RCC[RCCReg::PLL2DIVR] = (match->pll2n - 1U) | (((uint32)match->pll2p - 1U) << 9U);
			RCC[RCCReg::PLLCFGR].maset(6, 2, 0);    // PLL2RGE = 0 (1~2MHz)
			RCC[RCCReg::PLLCFGR].rstof(5);         // PLL2VCOSEL = 0 (WIDE)
			RCC[RCCReg::PLLCFGR].rstof(4);         // PLL2FRACEN = 0
			RCC[RCCReg::PLLCFGR].setof(19);        // DIVP2EN

			// Enable PLL2 and wait for lock
			RCC[RCCReg::CR].setof(26, true);
			while (!RCC[RCCReg::CR].bitof(27));

			// Route PLL2_P as SAI1 clock source (RCC_D2CCIP1R.SAI1SEL = 1)
			RCC[RCCReg::D2CCIP1R].maset(0, 3, 1);

			// MCKDIV: with OSR = 0 the codec frames at Fs = SAI_CK/(256*MCKDIV)
			uint32 freq = (1000000ULL * match->pll2n) / match->pll2p;
			uint32 tmpval = (freq * 10) / (samplerate * 256);
			uint32 mckdiv = tmpval / 10;
			if ((tmpval % 10) > 8) mckdiv += 1;
			if (mckdiv == 0) mckdiv = 1;

			// SAI1 MCKDIV (CR1 bits [25:20]); NOMCK (bit 19) stays 0 so MCLK is driven out
			SAI1[1][SAIReg::CR1].maset(20, 6, mckdiv);
			SAI1[1][SAIReg::CR1].rstof(19);// NOMCK = 0 (master clock generator enabled)
			SAI1[1][SAIReg::CR1].setof(17);// DMAEN
			SAI1[1][SAIReg::CLRFR] = 0x77;// clear all flags

			outsfmt("[SAI] Set Rate=%u Hz, PLL2N=%u, PLL2P=%u, Freq=%u, MCKDIV=%u, CR1=0x%08X\r\n",
				(unsigned)samplerate, (unsigned)match->pll2n, (unsigned)match->pll2p,
				(unsigned)freq, (unsigned)mckdiv, (unsigned)(uint32)SAI1[1][SAIReg::CR1]);
			return true;
		}

		// SAI1 Block A pins and registers: 64-bit frame, 32-bit slots, free protocol
		void sai1a_init(byte bits_per_sample) {
			GPIOE.enClock(true);
			// PE2 (MCLK), PE4 (FS), PE5 (SCK), PE6 (SD) -> AF6, pull enabled
			GPIOE[2].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);
			GPIOE[2]._set_alternate(6);
			GPIOE[4].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);
			GPIOE[4]._set_alternate(6);
			GPIOE[5].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);
			GPIOE[5]._set_alternate(6);
			GPIOE[6].setMode(GPIOMode::OUT_AF_PushPull, GPIOSpeed::Veryhigh).setPull(true);
			GPIOE[6]._set_alternate(6);

			SAI1.enClock(true);
			SAI1[1].enAble(false);

			uint32 datasize_bits = (bits_per_sample == 24) ? (6 << 5) : (4 << 5);// 24-bit (DS=6) : 16-bit (DS=4)

			// SAI Block A CR1: master TX, free protocol, asynchronous, stereo, OUTDRIV, DMAEN
			SAI1[1][SAIReg::CR1] = (0 << 0)    // master TX
			                     | (0 << 2)    // free protocol
			                     | datasize_bits
			                     | (0 << 9)    // clock strobing on falling edge
			                     | (0 << 10)   // asynchronous
			                     | (0 << 12)   // stereo mode (MONO = 0)
			                     | (1 << 13)   // OUTDRIV enable
			                     | (1 << 17)   // DMAEN
			                     | (0 << 19);  // NOMCK = 0 (master clock divider enabled)

			SAI1[1][SAIReg::CR2] = (1 << 0);   // FIFO threshold = 1/4

			// FRL=63, FSALL=31, FSDEF=1 (SOF + channel ID), FSPOL=0 (active low), FSOFF=1 (before first bit)
			SAI1[1][SAIReg::FRCR] = (63 << 0) | (31 << 8) | (1 << 16) | (0 << 17) | (1 << 18);

			// FBOFF=0, SLOTSZ=2 (32-bit slot), NBSLOT=2 (NBSLOT=1 in reg), slot 0 and slot 1 active
			SAI1[1][SAIReg::SLOTR] = (0 << 0) | (2 << 6) | (1 << 8) | (3 << 16);

			SAI1[1][SAIReg::CLRFR] = 0x77;
		}

		// DMA1 Stream5, circular double buffer. Request 87 = SAI1_A
		void dma_audio_init(byte* buf0, byte* buf1, uint32 num_items, uint32 element_size) {
			DMA1.enClock();// also enables DMAMUX1 (RCC_AHB1ENR.DMAMUX1EN, bit 2)
			// NOTE: never poke RCC_AHB1ENR bit 28 here: on STM32H743 that bit is
			// USB2OTGHSULPIEN (USB2 ULPI PHY clock), not DMAMUX1EN — writing it
			// reaches into the USB clock tree for no reason.
			//(RCC_AHB1ENR_ADDR = 0x580244D8)
			*(volatile uint32*)(0x40020800 + 5 * 4) = 87U;// DMAMUX1 channel 5 = request 87 (SAI1_A)

			DMA1.XferCpltCallback = on_dma_xfer_cplt0;
			DMA1.XferM1CpltCallback = on_dma_xfer_cplt1;

			// Disable DMA1 Stream5
			*(volatile uint32*)0x40020088 = 0;
			while (*(volatile uint32*)0x40020088 & 1);

			// Clear Stream5 flags
			*(volatile uint32*)0x4002000C = (0x3F << 6);// HIFCR
			SAI1[1][SAIReg::CLRFR] = 0x77;

			*(volatile uint32*)0x40020090 = 0x40015820;// SAI1_Block_A->DR (0x40015804 + 0x1C)
			*(volatile uint32*)0x40020094 = (uint32)buf0;
			*(volatile uint32*)0x40020098 = (uint32)buf1;
			*(volatile uint32*)0x4002008C = num_items;

			// DIR=M2P (01), CIRC=1, MINC=1, DBM=1, TCIE=1, TEIE=1, DMEIE=1, PL=High(10)
			// PSIZE/MSIZE field: 0 = byte, 1 = half-word, 2 = word
			uint32 size_field = (element_size == 1) ? 0U : ((element_size == 4) ? 2U : 1U);
			uint32 size_bits = (size_field << 11) | (size_field << 13);
			*(volatile uint32*)0x40020088 = (1 << 6) | (1 << 8) | (1 << 10) | size_bits | (2 << 16) | (1 << 18) | (1 << 4) | (1 << 2) | (1 << 1);

			DMA1[5].setInterruptPriority(0, 0);
			DMA1[5].enInterruptNVIC(true);

			outsfmt("[DMA] Init Stream5: PAR=0x%08X, MUX5=0x%08X, Items=%u, ElementSize=%u, CR=0x%08X\r\n",
				(unsigned)*(volatile uint32*)0x40020090, (unsigned)*(volatile uint32*)(0x40020800 + 5 * 4),
				(unsigned)num_items, (unsigned)element_size, (unsigned)*(volatile uint32*)0x40020088);
		}

		void audio_play_start() {
			*(volatile uint32*)0x40020088 |= 1;// DMA1_Stream5->CR EN = 1
			SAI1[1][SAIReg::CLRFR] = 0x77;
			SAI1[1].enAble(true);
		}

		void audio_play_stop() {
			SAI1[1].enAble(false);
			*(volatile uint32*)0x40020088 &= ~1;// DMA1_Stream5->CR EN = 0
			while (*(volatile uint32*)0x40020088 & 1);
			SAI1[1][SAIReg::CLRFR] = 0x77;
		}

	public:
		AudioBridge(byte** bufs, uint32 bcount, uint32 bsize, AudioDeviceInterface* c = nullptr, uint32 esize = 2) : chip(c),
			buffers(bufs), buffer_count(bcount), buffer_size(bsize), element_size(esize), refill_cb(nullptr), refill_context(nullptr),
			dma_underruns(0), played_bytes(0), running(false), paused(false), stream_over(false) {
			dma_done_count[0] = 0;
			dma_done_count[1] = 0;
			buffer_bytes[0] = 0;
			buffer_bytes[1] = 0;
			need_refill[0] = false;
			need_refill[1] = false;
		}

		void attach(AudioDeviceInterface* c) {
			chip = c;
		}

		AudioDeviceInterface* getChip() const {
			return chip;
		}

		// format.sample_rate drives the SAI/PLL2 clock; the engine output is always 16-bit stereo,
		// so the codec is always configured for 16-bit I2S here.
		bool StartStream(const AudioFormat& format, AudioPcmRefillHandler refill_cb, void* context) override {
			if (!refill_cb || !buffers || !buffer_size || !element_size) return false;
			if (buffer_count != 2) {// the double-buffer mode provides only M0/M1
				outsfmt("[Bridge] unsupported buffer count: %u (double-buffer mode needs 2)\r\n", (unsigned)buffer_count);
				return false;
			}
			if (element_size != 1 && element_size != 2 && element_size != 4) {
				outsfmt("[Bridge] unsupported DMA element size: %u byte(s)\r\n", (unsigned)element_size);
				return false;
			}
			if (running) StopStream();
			this->refill_cb = refill_cb;
			this->refill_context = context;
			this->format = format;
			dma_done_count[0] = 0;
			dma_done_count[1] = 0;
			dma_underruns = 0;
			played_bytes = 0;
			stream_over = false;
			paused = false;
			active() = this;

			// Control plane
			if (chip) {
				chip->setFormat(format);
				chip->ConfigI2S(0, 16);
			}
			// Data plane
			sai1a_init(16);
			if (!sai_set_samplerate(format.sample_rate)) {
				active() = nullptr;
				return false;
			}

			// Prefill both DMA buffers
			buffer_bytes[0] = refill_cb(refill_context, buffers[0], buffer_size);
			if (!buffer_bytes[0]) {
				active() = nullptr;
				return false;// nothing to play
			}
			buffer_bytes[1] = refill_cb(refill_context, buffers[1], buffer_size);
			stream_over = (buffer_bytes[1] == 0);
			need_refill[0] = false;
			need_refill[1] = false;

			dma_audio_init(buffers[0], buffers[1], buffer_size / element_size, element_size);
			audio_play_start();
			running = true;
			return true;
		}

		bool StopStream() override {
			if (running) audio_play_stop();
			running = false;
			paused = false;
			refill_cb = nullptr;
			refill_context = nullptr;
			if (active() == this) active() = nullptr;
			return true;
		}

		// Pause keeps the DMA fed with silence instead of stopping the stream.
		bool PauseStream() override {
			if (!running) return false;
			paused = true;
			return true;
		}

		bool ResumeStream() override {
			if (!running) return false;
			paused = false;
			return true;
		}

		// Returns: 0 = idle, 1 = serviced (buffers may have been refilled), 2 = stream over.
		uint8 ServicePlayback() override {
			if (!running) return 0;
			// One completion event covers a full M0+M1 round: refill both buffers, keeping the 1x cadence.
			if (dma_done_count[0] || dma_done_count[1]) {
				DMA1[5].enInterruptNVIC(false);
				dma_done_count[0] = 0;
				dma_done_count[1] = 0;
				DMA1[5].enInterruptNVIC(true);
				played_bytes += buffer_bytes[0] + buffer_bytes[1];
				need_refill[0] = true;
				need_refill[1] = true;
			}
			// Refill a buffer only while the DMA reads the other one:
			// CT (CR bit 19): 0 = DMA reads M0, 1 = DMA reads M1. No write/read contention, no replay.
			bool dma_on1 = ((*(volatile uint32*)0x40020088) >> 19) & 1;
			for (byte sb = 0; sb < buffer_count; sb++) {
				bool dma_reading_this = (sb == 0) ? !dma_on1 : dma_on1;
				if (!need_refill[sb] || dma_reading_this) continue;
				byte* target = buffers[sb];
				if (!paused && !stream_over) {
					uint32 c = refill_cb(refill_context, target, buffer_size);
					buffer_bytes[sb] = c;
					if (!c) stream_over = true;
				} else {
					for (uint32 i = 0; i < buffer_size; i++) target[i] = 0;
					buffer_bytes[sb] = 0;
				}
				need_refill[sb] = false;
			}
			return stream_over ? 2 : 1;
		}

		void setVolume(uint32 percent) override {
			if (!chip) return;
			stduint channel = 0;
			if (!chip->getMainChannel(channel)) return;
			chip->setVolume(channel, percent, percent);
		}

		uint32 getVolume() const override {
			if (!chip) return 0;
			stduint channel = 0;
			if (!chip->getMainChannel(channel)) return 0;
			uint32 left = 0, right = 0;
			if (!chip->getVolume(channel, left, right)) return 0;
			return (left + right) / 2;
		}

		void setMute(bool mute = true) override {
			if (!chip) return;
			stduint channel = 0;
			if (!chip->getMainChannel(channel)) return;
			chip->setMute(channel, mute);
		}

		bool isRunning() const {
			return running;
		}

		bool isStreamOver() const {
			return stream_over;
		}

		// Engine bytes already consumed by the DMA (progress and end-of-song detection)
		uint32 getPlayedBytes() const {
			return played_bytes;
		}

		uint32 getUnderruns() const {
			return dma_underruns;
		}

		// True when the SAI/PLL2 presets can generate this rate with a valid MCKDIV (6-bit field, 1..63).
		static bool isRateSupported(uint32 samplerate) {
			if (!samplerate) return false;
			uint16 rate_div10 = (uint16)(samplerate / 10);
			stduint tbl_count = 0;
			const SAIPreset* tbl = RateTable(tbl_count);
			for (stduint i = 0; i < tbl_count; i++) {
				if (tbl[i].rate_div10 != rate_div10) continue;
				uint32 freq = (1000000ULL * tbl[i].pll2n) / tbl[i].pll2p;
				uint32 tmpval = (freq * 10) / (samplerate * 256);
				uint32 mckdiv = tmpval / 10;
				if ((tmpval % 10) > 8) mckdiv += 1;
				return (mckdiv >= 1 && mckdiv <= 63);
			}
			return false;
		}
	};

}

#endif
#endif
