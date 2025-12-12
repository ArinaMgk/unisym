
#ifndef _AASM_LIMITS
#define _AASM_LIMITS

#define NO_SEG -1L              /* null segment value */
#define SEG_ABS 0x40000000L     /* mask for far-absolute segments */

#ifndef FILENAME_MAX
#define FILENAME_MAX 256
#endif

#ifndef PREFIX_MAX
#define PREFIX_MAX 10
#endif

#ifndef POSTFIX_MAX
#define POSTFIX_MAX 10
#endif

#define IDLEN_MAX 4096

#endif
