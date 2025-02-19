// UTF-8 CPP-ISO11 TAB4 CRLF
// Docutitle: (Device) Cache
// Codifiers: @dosconio: 20241208 ~ <Last-check> 
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

#include "../../../inc/cpp/Device/Cache"

#ifdef _MPU_STM32MP13
#include "../../../inc/c/proctrl/ARM/cortex_a7.h"
#include "../../../inc/c/arith.h"

// AKA __L1C_MaintainDCacheSetWay
// Apply cache maintenance to given cache level.
// level: cache level to be maintained
// maint: 0 - invalidate, 1 - clean, otherwise - invalidate and clean
static void __L1C_MaintainDCacheSetWay(uint32 level, uint32 maint) {
	uint32 Dummy;
	uint32 ccsidr;
	uint32 num_sets;
	uint32 num_ways;
	uint32 shift_way;
	uint32 log2_linesize;
	sint32 log2_num_ways;
	Dummy = level << 1U;
	// set csselr, select ccsidr register
	__set_CSSELR(Dummy);
	// get current ccsidr register
	ccsidr = __get_CCSIDR();
	num_sets = ((ccsidr & 0x0FFFE000U) >> 13U) + 1U;
	num_ways = ((ccsidr & 0x00001FF8U) >> 3U) + 1U;
	log2_linesize = (ccsidr & 0x00000007U) + 2U + 2U;
	log2_num_ways = intlog2__iexpo(num_ways);
	if ((log2_num_ways < 0) || (log2_num_ways > 32)) {
		return; // FATAL ERROR
	}
	shift_way = 32U - (uint32)log2_num_ways;
	for0r(way, num_ways) for0r(set, num_sets) {
		Dummy = (level << 1U) | (set << log2_linesize) | (way << shift_way);
		switch (maint) {
		case 0U: __set_DCISW(Dummy);  break;
		case 1U: __set_DCCSW(Dummy);  break;
		default: __set_DCCISW(Dummy); break;
		}
	}
	__DMB();
}

namespace uni {

	Cache_t L1C;


	void Cache_t::enAble(bool ena) const {
		SCTLR_Type sctlr;
		sctlr.w = __get_SCTLR();
		if (ena) {
			// sctlr |= SCTLR_I_Msk | SCTLR_C_Msk;
			sctlr.b.I = 1;
			sctlr.b.C = 1;
		}
		else {
			// sctlr &= ~_IMM(SCTLR_I_Msk | SCTLR_C_Msk);
			sctlr.b.I = 0;
			sctlr.b.C = 0;
		}
		__set_SCTLR(sctlr.w);
		__ISB();
		
	}

	void Cache_t::enBTAC(bool ena) const {
		SCTLR_Type sctlr;
		sctlr.w = __get_SCTLR();
		sctlr.b.Z = ena;
		__set_SCTLR(sctlr.w);
		__ISB();
	}

	void Cache_t::InvalidateBTAC() const {
		__set_BPIALL(0);
		__DSB();// ensure completion of the invalidation
		__ISB();// ensure instruction fetch path sees new state
	}

	void Cache_t::InvalidateICacheAll(void) const {
		__set_ICIALLU(0);
		__DSB();// ensure completion of the invalidation
		__ISB();// ensure instruction fetch path sees new I cache state
	}
	void Cache_t::InvalidateDCacheAll(void) const {
		CleanInvalidateCache(0);
	}

	void Cache_t::CleanDCacheMVA(pureptr_t va) const {
		__set_DCCMVAC((uint32_t)va);
		__DMB();// ensure the ordering of data cache maintenance operations and their effects
	}

	void Cache_t::InvalidateDCacheMVA(pureptr_t va) const {
		__set_DCIMVAC((uint32_t)va);
		__DMB();// ensure the ordering of data cache maintenance operations and their effects
	}

	void Cache_t::CleanInvalidateDCacheMVA(pureptr_t va) const {
		__set_DCCIMVAC((uint32_t)va);
		__DMB();// ensure the ordering of data cache maintenance operations and their effects
	}

	void Cache_t::CleanInvalidateCache(uint32 op) const {
		uint32_t clidr;
		uint32_t cache_type;
		clidr = __get_CLIDR();
		for0(i, 7) {
			cache_type = (clidr >> i * 3U) & 0x7UL;
			if ((cache_type >= 2U) && (cache_type <= 4U))
			{
				__L1C_MaintainDCacheSetWay(i, op);
			}
		}
	}

}


#elif defined(_MCU_STM32H7x)

#define _CortexM7_SCB_TEMP
#include "../../../inc/c/prochip/CortexM7.h"

// below are for CM7

// Data Synchronization Barrier
// Acts as a special kind of Data Memory Barrier.
// It completes when all explicit memory accesses before this instruction complete.
#define __DSB() do { \
	_ASM __volatile__ ("" : : : "memory", "cc"); _Comment("AKA __schedule_barrier");\
	__builtin_arm_dsb(0xF);\
	_ASM __volatile__ ("" : : : "memory", "cc"); _Comment("AKA __schedule_barrier"); } while (false)
// Instruction Synchronization Barrier
// Instruction Synchronization Barrier flushes the pipeline in the processor,
// All instructions following the ISB are fetched from cache or memory, after the instruction has been completed.
#define __ISB() do { \
	_ASM __volatile__ ("" : : : "memory", "cc"); _Comment("AKA __schedule_barrier");\
	__builtin_arm_isb(0xF);\
	_ASM __volatile__ ("" : : : "memory", "cc"); _Comment("AKA __schedule_barrier"); } while (false)

// Cache Size ID Register Macros
#define CCSIDR_WAYS(x)         (((x) & SCB_CCSIDR_ASSOCIATIVITY_Msk) >> SCB_CCSIDR_ASSOCIATIVITY_Pos)
#define CCSIDR_SETS(x)         (((x) & SCB_CCSIDR_NUMSETS_Msk      ) >> SCB_CCSIDR_NUMSETS_Pos      )


namespace uni {
    
	Cache_t L1C;

	void Cache_t::enAble(bool ena) const {
		enAbleICacheAll();
		enAbleDCacheAll();
		SCB->CACR |= 1 << 2;
	}

	void Cache_t::enAbleICacheAll(void) const {
		__DSB();
		__ISB();
		SCB->ICIALLU = 0UL;// invalidate I-Cache
		__DSB();
		__ISB();
		SCB->CCR |= _IMM1S(17);// SCB_CCR_IC_Msk;// enable
		__DSB();
		__ISB();
	}
	void Cache_t::enAbleDCacheAll(void) const {
		uint32_t ccsidr;
		uint32_t sets;
		uint32_t ways;
		SCB->CSSELR = 0U; /*(0U << 1U) | 0U;*/  /* Level 1 data cache */
		__DSB();
		ccsidr = SCB->CCSIDR;// invalidate D-Cache
		sets = CCSIDR_SETS(ccsidr);
		do {
			ways = CCSIDR_WAYS(ccsidr);
			do {
				SCB->DCISW = (((sets << SCB_DCISW_SET_Pos) & SCB_DCISW_SET_Msk) |
					((ways << SCB_DCISW_WAY_Pos) & SCB_DCISW_WAY_Msk));
			#if defined ( __CC_ARM )
				__schedule_barrier();
			#endif
			} while (ways-- != 0U);
		} while (sets-- != 0U);
		__DSB();
		SCB->CCR |= SCB_CCR_DC_Msk; // enable D-Cache */
		__DSB();
		__ISB();
	}


}


#endif
