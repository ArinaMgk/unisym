// ASCII CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Hash, HASH
// Codifiers: @ArinaMgk
// Attribute: Arn-Covenant Any-Architect Env-Freestanding Non-Dependence
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

#include "../../../inc/cpp/Device/HASH"
#include "../../../inc/cpp/Device/SysTick"
#include "../../../inc/cpp/Device/NVIC"
#include "../../../inc/cpp/Device/Interrupt/interrupt_tab.h"
#include "../../../inc/c/ustring.h"
#include "Interrupt/interrupt_hash.hpp"

namespace uni {
#if defined(_MCU_STM32F4x) || defined(_MCU_STM32H7x)

	#define _HASH_TIMEOUT_VALUE 1000U

	HASH HASH1;

	// ---- file-local helpers (AKA static HASH_* functions) ----

	// CR.ALGO spans two non-contiguous bits: ALGO0 (bit 7) and ALGO1 (bit 18, H7 only).
	// HashAlgo is the logical 2-bit value (SHA1=0, MD5=1, SHA224=2, SHA256=3).
	static void _HASH_AlgoWrite(HASH& hash, HashAlgo algo) {
		hash[HASHReg::CR].setof(_HASH_CR_POS_ALGO0, (stduint)algo & 1);
#if defined(_MCU_STM32H7x)
		hash[HASHReg::CR].setof(_HASH_CR_POS_ALGO1, ((stduint)algo >> 1) & 1);
#endif
	}
	static stduint _HASH_AlgoRead(HASH& hash) {
		stduint algo = (stduint)hash[HASHReg::CR].bitof(_HASH_CR_POS_ALGO0);
#if defined(_MCU_STM32H7x)
		algo |= ((stduint)hash[HASHReg::CR].bitof(_HASH_CR_POS_ALGO1)) << 1;
#endif
		return algo;
	}

	// CR r/w bits saved by ContextSaving (AKA READ_BIT mask in HAL_HASH_ContextSaving)
	#define _HASH_CR_SAVE_MASK ((1U << _HASH_CR_POS_DMAE) | (0x3U << _HASH_CR_POS_DATATYPE) | (1U << _HASH_CR_POS_MODE) | (1U << _HASH_CR_POS_ALGO0) | (1U << _HASH_CR_POS_MDMAT) | (1U << _HASH_CR_POS_LKEY) | (1U << _HASH_CR_POS_ALGO1))

	// AKA HAL HASH_DMAXferCplt: DMA input transfer complete (friend of uni::HASH)
	void _HASH_DMA_Cplt() {
		HASH* hash = 0;
		if (DMA1.XferCpltCallback == _HASH_DMA_Cplt) hash = (HASH*)DMA1.bind;
		else if (DMA2.XferCpltCallback == _HASH_DMA_Cplt) hash = (HASH*)DMA2.bind;
		if (!hash) return;
		HASH& h = *hash;
		if (h.State == _HASH_STATE_SUSPENDED) return;
		/* Disable the DMA transfer. Clearing DMAE starts the digest calculation
		   in hardware (that is why MDMAT must stay reset for single-buffer use). */
		h[HASHReg::CR].rstof(_HASH_CR_POS_DMAE);
		if (!h[HASHReg::CR].bitof(_HASH_CR_POS_MODE)) {
			// plain HASH: input data transfer is now over
			h.State = _HASH_STATE_READY;
			if (h.InCpltHandler) h.InCpltHandler();
			return;
		}
		// HMAC processing: initiate the next step DMA transfer
		stduint inputaddr = 0;
		stduint buffersize = 0;
		if (h.Phase == HASH::PhaseType::HMACStep3) {
			// end of HMAC processing (last DMA transfer was the outer key)
			h.State = _HASH_STATE_READY;
			h.Phase = HASH::PhaseType::Ready;// allow a fresh restart
			if (h.InCpltHandler) h.InCpltHandler();
			return;
		}
		else if (h.Phase == HASH::PhaseType::HMACStep1) {
			inputaddr = (stduint)h.pHashMsgBuffPtr;
			buffersize = h.HashBuffSize;
			h.Phase = HASH::PhaseType::HMACStep2;
			h.HashInCount = h.HashBuffSize;
			h.pHashInBuffPtr = h.pHashMsgBuffPtr;
			if (h.DigestCalculationDisable != 0) {
				// multi-buffer: the digest must not start at the end of this feed
				h[HASHReg::CR].setof(_HASH_CR_POS_MDMAT);
			}
		}
		else {
			// HMACStep2
			if (h.DigestCalculationDisable != 0) {
				// multi-buffer: stay in Step 2 for the next message buffer
				h.State = _HASH_STATE_READY;
				if (h.InCpltHandler) h.InCpltHandler();
				return;
			}
			// enter the key for the outer hash function (single buffer or last buffer)
			inputaddr = (stduint)h.key.address;
			buffersize = h.key.length;
			h.Phase = HASH::PhaseType::HMACStep3;
			h.HashInCount = h.key.length;
			h.pHashInBuffPtr = (byte*)h.key.address;
		}
		h.SetNbValidBits(buffersize);
		// re-arm the same DMA channel for the next step
		const DMAChannel& ch = *(const DMAChannel*)h.dma_in;
		stduint words = (buffersize % 4) ? (buffersize + 3) / 4 : buffersize / 4;
		ch.Transfer((pureptr_t)(_HASH_ADDR + _IMM(HASHReg::DIN)), (pureptr_t)inputaddr, words, IOMethod::Rupt);
		h[HASHReg::CR].setof(_HASH_CR_POS_DMAE);
	}

	// AKA HAL HASH_DMAError (friend of uni::HASH)
	void _HASH_DMA_Error() {
		HASH* hash = 0;
		if (DMA1.XferErrorCallback == _HASH_DMA_Error) hash = (HASH*)DMA1.bind;
		else if (DMA2.XferErrorCallback == _HASH_DMA_Error) hash = (HASH*)DMA2.bind;
		if (!hash) return;
		if (hash->State == _HASH_STATE_SUSPENDED) return;
		hash->State = _HASH_STATE_READY;
		if (hash->ErrorHandler) hash->ErrorHandler();
	}

	// ---- internal helpers ----

	// AKA HASH_WaitOnFlagUntilTimeout
	bool HASH::WaitOnFlag(stduint flag, bool status, stduint timeout) {
		uint64 tickstart = SysTick::getTick();
		while (self[HASHReg::SR].bitof(flag) == status) {
			if ((SysTick::getTick() - tickstart) > timeout) return false;
		}
		return true;
	}

	// AKA HASH_WriteData: feed input buffer to DIN, 4 bytes at a time
	bool HASH::WriteData(const byte* in, stduint size) {
		stduint inputaddr = (stduint)in;
		for (stduint bc = 0; bc < size; bc += 4) {
			self[HASHReg::DIN] = *(const uint32_t*)inputaddr;
			inputaddr += 4;
			// suspension support
			if ((SuspendRequest == _HASH_SUSPEND) && ((bc + 4) < size)) {
				if (self[HASHReg::SR].bitof(_HASH_SR_POS_DINIS)) {
					SuspendRequest = _HASH_SUSPEND_NONE;
					if ((Phase == PhaseType::Process) || (Phase == PhaseType::HMACStep2)) {
						pHashInBuffPtr = (byte*)inputaddr;
						HashInCount = size - (bc + 4);
					}
					else if ((Phase == PhaseType::HMACStep1) || (Phase == PhaseType::HMACStep3)) {
						pHashKeyBuffPtr = (byte*)inputaddr;
						HashKeyCount = size - (bc + 4);
					}
					else {
						State = _HASH_STATE_READY;
						return false;
					}
					State = _HASH_STATE_SUSPENDED;
					return true;
				}
			}
		}
		return true;
	}

	// AKA HASH_GetDigest: read HR[n] with byte reversal per digest size
	void HASH::GetDigest(byte* out, stduint size) {
		stduint msgdigest = (stduint)out;
		for (stduint i = 0; i < (size / 4); i++) {
			*(uint32_t*)msgdigest = MemReverseL(stduint(HR(i)));
			msgdigest += 4;
		}
	}

	// AKA __HAL_HASH_SET_NBVALIDBITS
	void HASH::SetNbValidBits(stduint size) {
		// NBW = number of valid bits in last word: (size % 4) * 8, or 0x10 if multiple of 4
		stduint nbw = (size % 4) ? ((size % 4) * 8) : 0x10;
		self[HASHReg::STR].maset(_HASH_STR_POS_NBLW, 5, nbw & 0x1F);
	}

	// AKA __HAL_HASH_START_DIGEST (STR.DCAL = 1)
	void HASH::StartDigest() {
		self[HASHReg::STR].setof(_HASH_STR_POS_DCAL);
	}

	// AKA HASH_DIGEST_LENGTH
	stduint HASH::getDigestLength() {
		switch ((HashAlgo)_HASH_AlgoRead(self)) {
		case HashAlgo::SHA1: return 20;
		case HashAlgo::SHA224: return 28;
		case HashAlgo::SHA256: return 32;
		default: return 16;// MD5
		}
	}

	// ---- public API ----

	// AKA HASH_Write_Block_Data (interrupt mode): feed 64-byte blocks, or the
	// remainder + start digest. Returns 1 when digest calculation has started.
	stduint HASH::WriteBlockData() {
		stduint ret = 0;// HASH_DIGEST_CALCULATION_NOT_STARTED
		if (HashInCount > 64) {
			stduint inputaddr = (stduint)pHashInBuffPtr;
			// write 16 words (64 bytes)
			for (stduint bc = 0; bc < 64; bc += 4) {
				self[HASHReg::DIN] = *(const uint32_t*)inputaddr;
				inputaddr += 4;
			}
			if (HashITCounter == 2) {
				// start-up extra word
				self[HASHReg::DIN] = *(const uint32_t*)inputaddr;
				inputaddr += 4;
				if (HashInCount >= 68) {
					HashInCount -= 68;
					pHashInBuffPtr += 68;
				}
				else HashInCount = 0;
			}
			else {
				HashInCount -= 64;
				pHashInBuffPtr += 64;
			}
		}
		else {
			stduint inputaddr = (stduint)pHashInBuffPtr;
			stduint inputcounter = HashInCount;
			// disable DINI interrupt
			self[HASHReg::IMR].rstof(_HASH_IMR_POS_DINIE);
			// write the remainder
			for (stduint bc = 0; bc < (inputcounter + 3) / 4; bc++) {
				self[HASHReg::DIN] = *(const uint32_t*)inputaddr;
				inputaddr += 4;
			}
			StartDigest();
			ret = 1;// HASH_DIGEST_CALCULATION_STARTED
			HashInCount = 0;
		}
		return ret;
	}

	// AKA HASH_IT: interrupt-mode dispatch (DCIS digest ready / DINIS feed data)
	bool HASH::ProcessIT() {
		if (State != _HASH_STATE_BUSY) return false;
		if (HashITCounter == 0) {
			self[HASHReg::IMR] = 0;// disable IT (DINI|DCI)
			State = _HASH_STATE_READY;
			return false;
		}
		else if (HashITCounter == 1) HashITCounter = 2;
		else HashITCounter = 3;
		// digest ready
		if (self[HASHReg::SR].bitof(_HASH_SR_POS_DCIS)) {
			GetDigest(pHashOutBuffPtr, getDigestLength());
			self[HASHReg::IMR] = 0;// disable IT (DINI|DCI)
			State = _HASH_STATE_READY;
			// HMAC is a one-shot run: reset the phase so the next Start() re-initializes
			// (AKA F4 HMAC_Processing; H7 keeps the phase, breaking a second start)
			if ((Phase == PhaseType::HMACStep1) || (Phase == PhaseType::HMACStep2) || (Phase == PhaseType::HMACStep3))
				Phase = PhaseType::Ready;
			if (DgstCpltHandler) DgstCpltHandler();
			return true;
		}
		// input ready
		if (self[HASHReg::SR].bitof(_HASH_SR_POS_DINIS)) {
			if ((SuspendRequest == _HASH_SUSPEND) && (HashInCount != 0)) {
				self[HASHReg::IMR] = 0;
				SuspendRequest = _HASH_SUSPEND_NONE;
				State = _HASH_STATE_SUSPENDED;
				return true;
			}
			if (WriteBlockData() == 1) {
				if (InCpltHandler) InCpltHandler();
				// HMAC step transitions (Phase HMACStep1 -> 2 -> 3)
				if (Phase == PhaseType::HMACStep1) {
					if (!WaitOnFlag(_HASH_SR_POS_BUSY, true, _HASH_TIMEOUT_VALUE)) {
						self[HASHReg::IMR] = 0;
						return false;
					}
					Phase = PhaseType::HMACStep2;
					SetNbValidBits(HashBuffSize);
					HashInCount = HashBuffSize;
					pHashInBuffPtr = pHashMsgBuffPtr;
					HashITCounter = 1;
					self[HASHReg::IMR].setof(_HASH_IMR_POS_DINIE);// enable DINI
				}
				else if (Phase == PhaseType::HMACStep2) {
					if (!WaitOnFlag(_HASH_SR_POS_BUSY, true, _HASH_TIMEOUT_VALUE)) {
						self[HASHReg::IMR] = 0;
						return false;
					}
					Phase = PhaseType::HMACStep3;
					SetNbValidBits(key.length);
					HashInCount = key.length;
					pHashInBuffPtr = (byte*)key.address;
					HashITCounter = 1;
					self[HASHReg::IMR].setof(_HASH_IMR_POS_DINIE);// enable DINI
				}
			}
		}
		return true;
	}

	// RCC AHB2ENR.HASHEN (bit 5)
	void HASH::enClock(bool ena) {
		Reference(_RCC_AHB2ENR_ADDR).setof(_RCC_AHB2ENR_POSI_ENCLK_HASH, ena);
	}

	// AKA HAL_HASH_Init (data type + clear MDMAT; reset counters/phase) + mode switch
	bool HASH::setMode(HashMode mode) {
		if (!initialized) {
			enClock();
			initialized = true;
		}
		State = _HASH_STATE_BUSY;
		HashInCount = 0;
		HashBuffSize = 0;
		HashITCounter = 0;
		DigestCalculationDisable = 0;
		Phase = PhaseType::Ready;
		// MODIFY_REG(CR, DATATYPE|MDMAT, DataType) then select HASH or HMAC (CR.MODE)
		self[HASHReg::CR].maset(_HASH_CR_POS_DATATYPE, 2, (stduint)data_type);
		self[HASHReg::CR].rstof(_HASH_CR_POS_MDMAT);
		self[HASHReg::CR].setof(_HASH_CR_POS_MODE, mode == HashMode::HMAC);
		State = _HASH_STATE_READY;
		return true;
	}

	// AKA HAL_HASH_DeInit
	void HASH::canMode() {
		State = _HASH_STATE_BUSY;
		Phase = PhaseType::Ready;
		HashInCount = 0;
		HashBuffSize = 0;
		HashITCounter = 0;
		DigestCalculationDisable = 0;
		enClock(false);
		State = _HASH_STATE_RESET;
		initialized = false;
	}

	// AKA HAL_HMAC key configuration (store key; LKEY decided in Start)
	bool HASH::ConfigHMAC(const byte* key, stduint key_len) {
		if (!key || !key_len) return false;
		this->key = { (stduint)key, key_len };
		return true;
	}

	// AKA HASH_Start (polling) / HASH_Start_IT (interrupt) / HASH_Start_DMA (DMA);
	// HMAC variants (AKA HMAC_Start / HMAC_Start_IT / HMAC_Start_DMA) are selected
	// by CR.MODE (set via setMode(HashMode::HMAC)) together with ConfigHMAC().
	// Multi-buffer HMAC DMA (AKA HAL_HMACEx_*_Step1_2_DMA / Step2_DMA /
	// Step2_3_DMA): keep DigestCalculationDisable non-zero for the first and
	// intermediate buffers and clear it for the last one; out is only required
	// for the first buffer (the digest is read later with Finish()).
	bool HASH::Start(HashAlgo algo, const byte* in, stduint size, byte* out, IOMethod method) {
		if ((State != _HASH_STATE_READY) && (State != _HASH_STATE_SUSPENDED)) return false;
		if (!in || !size) { State = _HASH_STATE_READY; return false; }
		bool hmac = self[HASHReg::CR].bitof(_HASH_CR_POS_MODE);
		if (hmac && !key.address) { State = _HASH_STATE_READY; return false; }
		if (!out && !((method == IOMethod::DMA) && (Phase == PhaseType::HMACStep2))) {
			State = _HASH_STATE_READY;
			return false;
		}
		State = _HASH_STATE_BUSY;

		// AKA HASH_Start_DMA / HMAC_Start_DMA
		if (method == IOMethod::DMA) {
			if (!dma_in) { State = _HASH_STATE_READY; return false; }
			const DMAChannel& ch = *(const DMAChannel*)dma_in;
			stduint inputaddr = 0;
			stduint inputSize = 0;
			if (State == _HASH_STATE_SUSPENDED) {
				// resumption: continue from the feeding point saved by DMAFeedProcessSuspend()
				inputaddr = (stduint)pHashInBuffPtr;
				inputSize = HashInCount;
			}
			else if (Phase == PhaseType::Ready) {
				// MODIFY_REG(CR, MDMAT|LKEY|ALGO|MODE|INIT, Algorithm | HMAC? | LKEY? | INIT)
				_HASH_AlgoWrite(self, algo);
				self[HASHReg::CR].rstof(_HASH_CR_POS_MDMAT);
				self[HASHReg::CR].setof(_HASH_CR_POS_MODE, hmac);
				self[HASHReg::CR].setof(_HASH_CR_POS_LKEY, hmac && (key.length > 64));
				self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
				if (hmac) {
					// step 1 enters the key first
					HashInCount = key.length;
					pHashKeyBuffPtr = (byte*)key.address;
					pHashInBuffPtr = (byte*)key.address;
					pHashMsgBuffPtr = (byte*)in;
					HashBuffSize = size;
					pHashOutBuffPtr = out;
					SetNbValidBits(key.length);
					Phase = PhaseType::HMACStep1;
					inputaddr = (stduint)key.address;
					inputSize = key.length;
				}
				else {
					SetNbValidBits(size);
					pHashInBuffPtr = (byte*)in;
					HashInCount = size;
					Phase = PhaseType::Process;
					inputaddr = (stduint)in;
					inputSize = size;
				}
			}
			else if (Phase == PhaseType::HMACStep2) {
				// AKA HMAC_Start_DMA multi-buffer continuation: feed the next message buffer
				HashInCount = size;
				pHashInBuffPtr = (byte*)in;
				inputaddr = (stduint)in;
				inputSize = size;
				if (DigestCalculationDisable == 0) {
					// last buffer of the multi-buffer sequence: DCAL must be set (MDMAT reset)
					self[HASHReg::CR].rstof(_HASH_CR_POS_MDMAT);
					SetNbValidBits(inputSize);
				}
			}
			else {
				State = _HASH_STATE_READY;
				return false;
			}
			// AKA hdmain XferCpltCallback/XferErrorCallback + HAL_DMA_Start_IT
			DMA_t& dma = ch.getParent().getParent();
			dma.bind = (pureptr_t)this;
			dma.XferCpltCallback = _HASH_DMA_Cplt;
			dma.XferErrorCallback = _HASH_DMA_Error;
			stduint words = (inputSize % 4) ? (inputSize + 3) / 4 : inputSize / 4;
			if (!ch.Transfer((pureptr_t)(_HASH_ADDR + _IMM(HASHReg::DIN)), (pureptr_t)inputaddr, words, IOMethod::Rupt)) {
				State = _HASH_STATE_READY;
				return false;
			}
			// enable DMA requests; the digest calculation starts automatically when
			// DMAE is cleared by the DMA complete callback (MDMAT kept reset here)
			self[HASHReg::CR].setof(_HASH_CR_POS_DMAE);
			return true;
		}

		// AKA HASH_Start_IT / HMAC_Start_IT
		if (method == IOMethod::Rupt) {
			HashITCounter = 1;
			if (Phase == PhaseType::Ready) {
				// MODIFY_REG(CR, LKEY|ALGO|MODE|INIT, Algorithm | HMAC? | LKEY? | INIT)
				_HASH_AlgoWrite(self, algo);
				self[HASHReg::CR].setof(_HASH_CR_POS_MODE, hmac);
				self[HASHReg::CR].setof(_HASH_CR_POS_LKEY, hmac && (key.length > 64));
				self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
				if (hmac) {
					// step 1 enters the key first
					HashInCount = key.length;
					pHashInBuffPtr = (byte*)key.address;
					pHashKeyBuffPtr = (byte*)key.address;
					pHashMsgBuffPtr = (byte*)in;
					HashBuffSize = size;
					pHashOutBuffPtr = out;
					SetNbValidBits(key.length);
					Phase = PhaseType::HMACStep1;
				}
				else {
					SetNbValidBits(size);
					HashInCount = size;
					pHashInBuffPtr = (byte*)in;
					pHashOutBuffPtr = out;
					Phase = PhaseType::Process;
				}
			}
			// resumption after suspension: saved pointers/counts are used by ProcessIT()
			// enable DINI | DCI interrupts
			self[HASHReg::IMR].setof(_HASH_IMR_POS_DINIE);
			self[HASHReg::IMR].setof(_HASH_IMR_POS_DCIE);
			return true;
		}

		// AKA HASH_Start / HMAC_Start (polling); HMAC steps run HMAC_Processing
		if (hmac) {
			if (Phase == PhaseType::Ready) {
				// AKA HMAC_Start: LKEY per key size, HMAC mode, reset core
				_HASH_AlgoWrite(self, algo);
				self[HASHReg::CR].setof(_HASH_CR_POS_MODE, true);
				self[HASHReg::CR].setof(_HASH_CR_POS_LKEY, key.length > 64);
				self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
				Phase = PhaseType::HMACStep1;
				pHashOutBuffPtr = out;
				pHashInBuffPtr = (byte*)in;
				HashInCount = size;
				HashBuffSize = size;
				pHashKeyBuffPtr = (byte*)key.address;
				HashKeyCount = key.length;
			}
			// STEP 1: hash the key
			if (Phase == PhaseType::HMACStep1) {
				SetNbValidBits(key.length);
				if (!WriteData(pHashKeyBuffPtr, HashKeyCount)) return false;
				if (State == _HASH_STATE_SUSPENDED) return true;
				StartDigest();
				if (!WaitOnFlag(_HASH_SR_POS_BUSY, true, _HASH_TIMEOUT_VALUE)) { State = _HASH_STATE_READY; return false; }
				Phase = PhaseType::HMACStep2;
			}
			// STEP 2: hash the message
			if (Phase == PhaseType::HMACStep2) {
				SetNbValidBits(HashBuffSize);
				if (!WriteData(pHashInBuffPtr, HashInCount)) return false;
				if (State == _HASH_STATE_SUSPENDED) return true;
				StartDigest();
				if (!WaitOnFlag(_HASH_SR_POS_BUSY, true, _HASH_TIMEOUT_VALUE)) { State = _HASH_STATE_READY; return false; }
				Phase = PhaseType::HMACStep3;
				// re-set the key in case step 1 was suspended and resumed
				pHashKeyBuffPtr = (byte*)key.address;
				HashKeyCount = key.length;
			}
			// STEP 3: hash the key again, then read the digest
			if (Phase == PhaseType::HMACStep3) {
				SetNbValidBits(key.length);
				if (!WriteData(pHashKeyBuffPtr, HashKeyCount)) return false;
				if (State == _HASH_STATE_SUSPENDED) return true;
				StartDigest();
				if (!WaitOnFlag(_HASH_SR_POS_DCIS, false, _HASH_TIMEOUT_VALUE)) { State = _HASH_STATE_READY; return false; }
				GetDigest(out, getDigestLength());
				Phase = PhaseType::Ready;// AKA F4 HMAC_Processing: allow a fresh restart
			}
			State = _HASH_STATE_READY;
			return true;
		}

		// AKA HASH_Start (polling)
		const byte* in_tmp = in;
		stduint size_tmp = size;
		if (Phase == PhaseType::Ready) {
			// MODIFY_REG(CR, LKEY|ALGO|MODE|INIT, Algorithm | INIT)
			_HASH_AlgoWrite(self, algo);
			self[HASHReg::CR].rstof(_HASH_CR_POS_MODE);
			self[HASHReg::CR].rstof(_HASH_CR_POS_LKEY);
			self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
			SetNbValidBits(size);
			Phase = PhaseType::Process;
		}
		else if (Phase == PhaseType::Process) {
			if (State == _HASH_STATE_SUSPENDED) {
				// resumption: continue from the feeding point saved by WriteData()
				in_tmp = pHashInBuffPtr;
				size_tmp = HashInCount;
			}
			else {
				// multi-buffer processing: feed a new input block
				SetNbValidBits(size);
			}
		}
		else {
			State = _HASH_STATE_READY;
			return false;
		}
		if (!WriteData(in_tmp, size_tmp)) return false;
		if (State != _HASH_STATE_SUSPENDED) {
			StartDigest();
			if (!WaitOnFlag(_HASH_SR_POS_DCIS, false, _HASH_TIMEOUT_VALUE)) {
				State = _HASH_STATE_READY;
				return false;
			}
			GetDigest(out, getDigestLength());
			State = _HASH_STATE_READY;
		}
		return true;
	}
	// AKA HASH_Accumulate: feed a block (multiple of 4) of a large message
	bool HASH::Accumulate(HashAlgo algo, const byte* in, stduint size) {
		if ((State != _HASH_STATE_READY) && (State != _HASH_STATE_SUSPENDED)) return false;
		if (!in || !size) { State = _HASH_STATE_READY; return false; }
		const byte* in_tmp = in;
		stduint size_tmp = size;
		if (State == _HASH_STATE_SUSPENDED) {
			// resumption: continue from the feeding point saved by WriteData()
			in_tmp = pHashInBuffPtr;
			size_tmp = HashInCount;
		}
		else {
			if (Phase == PhaseType::Ready) {
				// select algorithm, clear HMAC mode, reset core
				_HASH_AlgoWrite(self, algo);
				self[HASHReg::CR].rstof(_HASH_CR_POS_MODE);
				self[HASHReg::CR].rstof(_HASH_CR_POS_LKEY);
				self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
			}
			Phase = PhaseType::Process;
		}
		State = _HASH_STATE_BUSY;
		if (!WriteData(in_tmp, size_tmp)) return false;
		if (State != _HASH_STATE_SUSPENDED) State = _HASH_STATE_READY;
		return true;
	}

	// AKA HASH_Finish: wait DCIS then read digest (after Accumulate)
	bool HASH::Finish(byte* out, IOMethod method) {
		if (State != _HASH_STATE_READY) return false;
		if (!out) return false;
		State = _HASH_STATE_BUSY;
		if (!WaitOnFlag(_HASH_SR_POS_DCIS, false, _HASH_TIMEOUT_VALUE)) {
			return false;
		}
		GetDigest(out, getDigestLength());
		State = _HASH_STATE_READY;
		return true;
	}

	// AKA HAL_HASH_GetState: current state machine value
	// (RESET/READY/BUSY/ERROR/SUSPENDED; HAL status is folded into State in unisym)
	stduint HASH::getStatus() {
		return State;
	}

	// AKA HAL_HASH_ContextSaving: IMR, STR, CR (r/w bits) then CSR[0..53].
	// mem_buffer must hold at least (54 + 3) * 4 bytes.
	void HASH::ContextSaving(byte* mem_buffer) {
		stduint mem_ptr = (stduint)mem_buffer;
		*(uint32_t*)mem_ptr = (uint32_t)self[HASHReg::IMR] & 0x3;// DINIE | DCIE
		mem_ptr += 4;
		*(uint32_t*)mem_ptr = (uint32_t)self[HASHReg::STR] & 0x1F;// NBLW
		mem_ptr += 4;
		*(uint32_t*)mem_ptr = (uint32_t)self[HASHReg::CR] & _HASH_CR_SAVE_MASK;
		mem_ptr += 4;
		for (stduint i = 54; i > 0; i--) {
			*(uint32_t*)mem_ptr = (uint32_t)CSR(54 - i);
			mem_ptr += 4;
		}
	}

	// AKA HAL_HASH_ContextRestoring
	void HASH::ContextRestoring(const byte* mem_buffer) {
		stduint mem_ptr = (stduint)mem_buffer;
		self[HASHReg::IMR] = *(const uint32_t*)mem_ptr;
		mem_ptr += 4;
		self[HASHReg::STR] = *(const uint32_t*)mem_ptr;
		mem_ptr += 4;
		self[HASHReg::CR] = *(const uint32_t*)mem_ptr;
		mem_ptr += 4;
		// reset the HASH processor before restoring the CSRs (AKA __HAL_HASH_INIT)
		self[HASHReg::CR].setof(_HASH_CR_POS_INIT);
		for (stduint i = 54; i > 0; i--) {
			CSR(54 - i) = *(const uint32_t*)mem_ptr;
			mem_ptr += 4;
		}
	}

	// AKA HAL_HASH_SwFeed_ProcessSuspend: request suspension of polling/IT processing.
	// The actual suspension is carried out by WriteData() (polling) or ProcessIT() (IT).
	void HASH::SwFeedProcessSuspend() {
		SuspendRequest = _HASH_SUSPEND;
	}

	// AKA HAL_HASH_DMAFeed_ProcessSuspend: suspend an on-going DMA feed
	bool HASH::DMAFeedProcessSuspend() {
		if (State == _HASH_STATE_READY) return false;
		State = _HASH_STATE_SUSPENDED;
		// clear DMAE then wait DMAS to reset
		self[HASHReg::CR].rstof(_HASH_CR_POS_DMAE);
		if (!WaitOnFlag(_HASH_SR_POS_DMAS, true, _HASH_TIMEOUT_VALUE)) return false;
		if (!dma_in) { State = _HASH_STATE_READY; return false; }
		const DMAChannel& ch = *(const DMAChannel*)dma_in;
		if (!ch.Abort()) return false;
		// how many words remain to be written (AKA NDTR read)
		DMA_t& dma = ch.getParent().getParent();
		stduint remaining = dma[DMAReg::NDTR[ch.getParent().getID()]];
		if (remaining == 0) {
			// all data already entered the IP: suspension failed, wrap up by reading the digest
			State = _HASH_STATE_READY;
			return false;
		}
		if (!WaitOnFlag(_HASH_SR_POS_BUSY, true, _HASH_TIMEOUT_VALUE)) return false;
		// update the feeding point with the words actually transferred
		stduint initial = (HashInCount % 4) ? (HashInCount + 3) / 4 : HashInCount / 4;
		pHashInBuffPtr += 4 * (initial - remaining);
		HashInCount = 4 * remaining;
		return true;
	}

	// ---- RuptTrait (NVIC + IRQ_RNG, shared with RNG on F4/H7) ----
	void HASH::setInterrupt(Handler_t f) const {
		FUNC_HASH[0] = f;
	}
	void HASH::setInterruptPriority(byte preempt, byte sub_priority) const {
		NVIC.setPriority(IRQ_RNG, preempt, sub_priority);
	}
	void HASH::enInterrupt(bool enable) const {
		NVIC.setAble(IRQ_RNG, enable);
	}

#endif // _MCU_STM32F4x || _MCU_STM32H7x
}
