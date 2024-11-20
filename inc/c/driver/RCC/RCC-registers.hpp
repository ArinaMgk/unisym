namespace uni {
	namespace RCCReg {
	#if 0

	#elif defined(_MPU_STM32MP13)
		namespace _MP_MP_RSTSyR {
			enum MP_CIxR_POS { // off 0x11C and 0x120
				_MASK = 0b1001'1111'0111'1111
			};
		}
		namespace _MP_CIxR { // _MP_CIER and _MP_CIFR
			enum MP_CIxR_POS { // off 0x200 and 0x204
				_MASK = 0b1'0001'0000'1111'0001'1111
			};
		}
		enum class _BDCR { // off 0x400
			LSEON = 0, LSEBYP, LSERDY, DIGBYP,
			LSEDRV = 4, // len 2
			LSECSSON = 8, LSECSSD,
			RTCSRC = 16, // len 2
			RTCCKEN = 20, VSWRST = 31
		};
		enum class _RDLSICR { // off 0x404
			LSION = 0,
			LSIRDY = 1,
			MRD = 16, // len 5
			EADLY = 24, // len 3
			SPARE = 27, // len 5
		};
		namespace _OCENSETR {
			enum OCENSETR_POS { // off 0x420
				HSION = 0,
				HSIKERON = 1,
				CSION = 4, CSIKERON,
				DIGBYP = 7, HSEON, HSEKERON, HSEBYP, HSECSSON
			};
		}
		namespace _OCENCLRR {
			enum OCENCLRR_POS { // POS off 0x424
				HSION = 0,
				HSIKERON = 1,
				CSION = 4, CSIKERON,
				DIGBYP = 7, HSEON, HSEKERON, HSEBYP,
				_MASK = 0b111110110011
			};
		}
		namespace _OCRDYR {
			enum OCRDYR_POS {
				HSIRDY = 0,// HSI Ready
				HSIDIVRDY = 2,
				CSIRDY = 4,
				HSERDY = 8,
				MPUCKRDY = 23,
				AXICKRDY = 24
			};
		}
		namespace _HSICFGR {
			enum HSICFGR_POS { // off 0x440
				HSIDIV = 0, // len 2
				HSITRIM = 8, // len 7
				HSICAL = 16, // len 12
			};
		}
		enum class _CSICFGR { // off 0x444
			CSITRIM = 8, // len 5
			CSICAL = 16, // len 8
		};


	#endif
	}
}

