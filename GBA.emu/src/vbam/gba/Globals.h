#ifndef GLOBALS_H
#define GLOBALS_H

#include "../common/Types.h"
#include "GBA.h"
#include <util/preprocessor/repeat.h>

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

static const u32 stop = 0x08000568;
extern int saveType;
extern bool useBios;
extern bool skipBios;
static const bool cpuDisableSfx = 0;
extern bool cpuIsMultiBoot;
extern bool parseDebug;
static const bool speedHack = 1;
extern int cpuSaveType;
#ifdef USE_CHEATS
extern bool cheatsEnabled;
#else
static const bool cheatsEnabled = false;
#endif
extern bool skipSaveGameBattery; // skip battery data when reading save states
extern bool skipSaveGameCheats;  // skip cheat list data when reading save states
static const int customBackdropColor = -1;

static uint16a &P1 = *((uint16a*)&gGba.mem.ioMem.b[0x130]);

#endif // GLOBALS_H
