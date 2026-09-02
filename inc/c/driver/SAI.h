// UTF-8 C/C++11 TAB4 CRLF
// Docutitle: (Device) Serial Audio Interface, SAI
// Codifiers: @dosconio: 2025xxxx~;
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
// Copyright: UNISYM, under Apache License 2.0
// Dependens: RCC
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

#if !defined(_INC_Standard_SAI)
#define _INC_Standard_SAI
#include "../stdinc.h"
#if defined(_MCU_Intel8051)


#elif defined(_INC_CPP) // Below are C++ Area
#include "../../cpp/reference"
#include "_predefine/predef.sai.hpp"

#define DA_PP 1

namespace uni {


#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
#include "../../cpp/Device/_inner/SAI-STM32H7.hpp"

	class DMAStream;// forward declaration for SAI DMA integration

	// AKA HAL_SAI_StateTypeDef
	enum class SAIXState : byte {
		Reset, Ready, Busy, BusyTX, BusyRX, Error
	};

	class SAI_Block {
	protected:
		byte id;// 1..2
		byte block;// 1=A, 2=B
		stduint sai_base;// SAI_BASE (peripheral-level: GCR/PDMCR/PDMDLY)
		// AKA SAI_InitTypeDef
		SAIAudioMode_E audio_mode = SAIAudioMode_E::MasterTx;
		SAIProtocol_E protocol = SAIProtocol_E::Free;
		SAIDataSize_E datasize = SAIDataSize_E::Data16;
		SAIFirstBit_E first_bit = SAIFirstBit_E::MSB;
		SAIClockStrobing_E strobing = SAIClockStrobing_E::FallingEdge;
		SAISynchro_E synchro = SAISynchro_E::Asynchronous;
		SAISynchroExt_E synchro_ext = SAISynchroExt_E::Disable;
		SAIOutputDrive_E outdrive = SAIOutputDrive_E::Disable;
		SAINoDivider_E nodiv = SAINoDivider_E::Divider;
		SAIFifoThreshold_E fth = SAIFifoThreshold_E::Empty;
		SAIMonoStereo_E mono = SAIMonoStereo_E::Stereo;
		SAICompanding_E comp = SAICompanding_E::None;
		SAITriState_E tristate = SAITriState_E::Disable;
		stduint audio_freq = 0;// sampling frequency (Hz); 0 = MCKDIV passthrough
		stduint mckdiv = 0;// AKA Init.Mckdiv (computed or set directly)
	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		SAIMckOverSampling_E mckosr = SAIMckOverSampling_E::Disable;
		bool pdm_act = false;// PDM interface activation
		stduint mic_pairs = 1;// number of microphone pairs (1..4)
		SAIClockEnable_E clken = SAIClockEnable_E::Disable;
	#endif
	#if defined(_MPU_STM32MP13)
		SAIMckOutput_E mckout = SAIMckOutput_E::Disable;
	#endif
		// AKA SAI_FrameInitTypeDef
		stduint frame_length = 0;
		stduint active_frame_length = 0;
		SAIFSDefinition_E fsdef = SAIFSDefinition_E::StartFrame;
		SAIFSPolarity_E fspol = SAIFSPolarity_E::ActiveLow;
		SAIFSOffset_E fsoff = SAIFSOffset_E::FirstBit;
		// AKA SAI_SlotInitTypeDef
		stduint firstbit_offset = 0;
		SAISlotSize_E slotsize = SAISlotSize_E::DataSize;
		stduint slot_number = 1;
		stduint slot_active = 0;
		// AKA SAI_HandleTypeDef transfer fields
		bool lock = false;
		SAIXState state = SAIXState::Reset;
		stduint error_code = 0;
		const byte* tx_buff = nullptr;
		stduint tx_size = 0, tx_count = 0;
		byte* rx_buff = nullptr;
		stduint rx_size = 0, rx_count = 0;
		const DMAStream* hdmatx = nullptr;
		const DMAStream* hdmarx = nullptr;
		Handler_t mute_handler = nullptr;
	public:
		SAI_Block(byte _id, byte _block) : id(_id), block(_block), sai_base(0) {}
		void setBase(stduint _sai_base) { sai_base = _sai_base; }
		stduint blockBase() const { return sai_base + (block == 2 ? 0x024 : 0x004); }
		byte getID() const { return id; }
		byte getBlock() const { return block; }

		Reference operator[](SAIReg reg);
		bool canMode();
		bool enAble(bool ena = true);

		// AKA SAI_InitTypeDef setters (chain, return self)
		SAI_Block& setAudioMode(SAIAudioMode_E val) { audio_mode = val; return self; }
		SAI_Block& setProtocol(SAIProtocol_E val) { protocol = val; return self; }
		SAI_Block& setDataSize(SAIDataSize_E val) { datasize = val; return self; }
		SAI_Block& setFirstBit(SAIFirstBit_E val) { first_bit = val; return self; }
		SAI_Block& setClockStrobing(SAIClockStrobing_E val) { strobing = val; return self; }
		SAI_Block& setSynchro(SAISynchro_E val) { synchro = val; return self; }
		SAI_Block& setSynchroExt(SAISynchroExt_E val) { synchro_ext = val; return self; }
		SAI_Block& setOutputDrive(SAIOutputDrive_E val) { outdrive = val; return self; }
		SAI_Block& setNoDivider(SAINoDivider_E val) { nodiv = val; return self; }
		SAI_Block& setFifoThreshold(SAIFifoThreshold_E val) { fth = val; return self; }
		SAI_Block& setMonoStereo(SAIMonoStereo_E val) { mono = val; return self; }
		SAI_Block& setCompanding(SAICompanding_E val) { comp = val; return self; }
		SAI_Block& setTriState(SAITriState_E val) { tristate = val; return self; }
		SAI_Block& setAudioFreq(stduint hz) { audio_freq = hz; return self; }
		SAI_Block& setMckDiv(stduint div) { mckdiv = div; return self; }
	#if defined(_MCU_STM32H7x) || defined(_MPU_STM32MP13)
		SAI_Block& setMckOverSampling(SAIMckOverSampling_E val) { mckosr = val; return self; }
		SAI_Block& setPdmActivation(bool ena = true) { pdm_act = ena; return self; }
		SAI_Block& setMicPairsNbr(stduint val) { mic_pairs = val; return self; }
		SAI_Block& setClockEnable(SAIClockEnable_E val) { clken = val; return self; }
		bool setPdmMicDelay(stduint mic_pair, stduint left_delay, stduint right_delay);// AKA HAL_SAIEx_ConfigPdmMicDelay
	#endif
	#if defined(_MPU_STM32MP13)
		SAI_Block& setMckOutput(SAIMckOutput_E val) { mckout = val; return self; }
	#endif
		// AKA SAI_FrameInitTypeDef / SAI_SlotInitTypeDef setters
		SAI_Block& setFrameLength(stduint val) { frame_length = val; return self; }
		SAI_Block& setActiveFrameLength(stduint val) { active_frame_length = val; return self; }
		SAI_Block& setFSDefinition(SAIFSDefinition_E val) { fsdef = val; return self; }
		SAI_Block& setFSPolarity(SAIFSPolarity_E val) { fspol = val; return self; }
		SAI_Block& setFSOffset(SAIFSOffset_E val) { fsoff = val; return self; }
		SAI_Block& setFirstBitOffset(stduint val) { firstbit_offset = val; return self; }
		SAI_Block& setSlotSize(SAISlotSize_E val) { slotsize = val; return self; }
		SAI_Block& setSlotNumber(stduint val) { slot_number = val; return self; }
		SAI_Block& setSlotActive(stduint val) { slot_active = val; return self; }

		// AKA HAL_SAI_Transmit / _IT / _DMA
		stduint Transmit(const byte* tx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_SAI_Receive / _IT / _DMA
		stduint Receive(byte* rx, stduint size, IOMethod method = IOMethod::Loop);
		// AKA HAL_SAI_DMAPause / DMAResume / DMAStop / Abort
		bool PauseDMA();
		bool ResumeDMA();
		bool StopDMA();
		bool Abort();
		// AKA HAL_SAI_EnableTxMuteMode / DisableTxMuteMode / EnableRxMuteMode / DisableRxMuteMode
		bool EnableTxMuteMode(stduint val);
		bool DisableTxMuteMode();
		bool EnableRxMuteMode(Handler_t cb, stduint counter);
		bool DisableRxMuteMode();
		// AKA HAL_SAI_GetState / GetError
		SAIXState getState() const { return state; }
		stduint getError() const { return error_code; }
		void setState(SAIXState s) { state = s; }
		void setError(stduint e) { error_code |= e; }
		// callbacks (Handler_t, AKA HAL weak callbacks)
		Handler_t TxCpltHandler, RxCpltHandler, ErrorHandler;
		// ISR helpers (called from interrupt_sai.hpp; AKA HAL_SAI_IRQHandler split)
		void irqHandler();
		void innHandlerByInterrupt();
		void outHandlerByInterrupt();
		// internal helpers
		stduint _prtcfg() const;// PRTCFG[1:0] value for the abstract protocol
		stduint _itErrorFlags() const;// AKA SAI_InterruptFlag (master WCKCFG / slave AFSDET+LFSDET)
		void closeTransfer();
		void fillFifo();
		void disableSai();
		byte dataUnitBytes() const;// 1 / 2 / 4 per DataSize + Companding
		bool dmaTxStart(pureptr_t mem, pureptr_t peri, stduint ndtr);
		bool dmaRxStart(pureptr_t peri, pureptr_t mem, stduint ndtr);
		void dmaCpltISR();
		// AKA HAL_SAI_InitProtocol (I2S/PCM standard -> auto frame/slot config)
		bool applyProtocolStandard();
	};

	class SAI_t {
	protected:
		byte id;
		stduint baseaddr;
		SAI_Block blkA, blkB;
	public:
		SAI_t(byte _id, stduint _base) : id(_id), baseaddr(_base), blkA(_id, 1), blkB(_id, 2) {
			blkA.setBase(_base);
			blkB.setBase(_base);
		}
		SAI_Block& operator[](byte block) { return block == 2 ? blkB : blkA; }
		Reference operator[](SAIGlobalReg reg) { return baseaddr + _IMM(reg); }
		bool enClock(bool ena = true);
	};

	static struct SAI_Global {
		SAI_t& operator[](byte id);
	} SAI;

	extern SAI_t SAI1, SAI2;
#endif

}

#endif

#endif
