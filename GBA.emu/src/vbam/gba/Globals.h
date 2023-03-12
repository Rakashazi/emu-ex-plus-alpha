#ifndef GLOBALS_H
#define GLOBALS_H

#include "../common/Types.h"
#include "GBA.h"
#include <imagine/util/preprocessor/repeat.h>

#define VERBOSE_SWI                  1
#define VERBOSE_UNALIGNED_MEMORY     2
#define VERBOSE_ILLEGAL_WRITE        4
#define VERBOSE_ILLEGAL_READ         8
#define VERBOSE_DMA0                16
#define VERBOSE_DMA1                32
#define VERBOSE_DMA2                64
#define VERBOSE_DMA3               128
#define VERBOSE_UNDEFINED          256
#define VERBOSE_AGBPRINT           512
#define VERBOSE_SOUNDOUTPUT       1024

#define PP_ZERO(z, n, text)  0,
#define PP_ONE(z, n, text)  1,
#define PP_ZERO_REPEAT(n) BOOST_PP_REPEAT(n, PP_ZERO, )
#define PP_ONE_REPEAT(n) BOOST_PP_REPEAT(n, PP_ONE, )

static const bool ioReadable[0x400] =
{
	  PP_ONE_REPEAT(16)
	  PP_ZERO_REPEAT(56)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(4)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(12)
	  PP_ONE_REPEAT(44)
	  PP_ZERO_REPEAT(4)
	  PP_ONE_REPEAT(16)
	  PP_ZERO_REPEAT(24)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(8)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(8)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(8)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(32)
	  PP_ONE_REPEAT(16)
	  PP_ZERO_REPEAT(16)
	  PP_ONE_REPEAT(12)
	  PP_ZERO_REPEAT(4)
	  PP_ONE_REPEAT(8)
	  PP_ZERO_REPEAT(8)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(12)
	  PP_ONE_REPEAT(12)
	  PP_ZERO_REPEAT(164)
	  PP_ONE_REPEAT(12)
	  PP_ZERO_REPEAT(244)
	  PP_ONE_REPEAT(4)
	  PP_ZERO_REPEAT(252)
};

static const uint32_t stop = 0x08000568;
extern bool gba_joybus_enabled;
extern bool gba_joybus_active;
constexpr int customBackdropColor = -1;

static uint16a &P1 = *((uint16a*)&gGba.mem.ioMem.b[0x130]);

#endif // GLOBALS_H
