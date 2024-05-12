#ifndef VBAM_CORE_GBA_GBAGLOBALS_H_
#define VBAM_CORE_GBA_GBAGLOBALS_H_

#include "core/gba/gba.h"
#include <main/GBASys.hh>
#include <array>

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

constexpr std::array<bool, 0x400> ioReadable = []
{
	std::array<bool, 0x400> ioReadable;
	int i;
	for (i = 0; i < 0x400; i++)
		ioReadable[i] = true;
	for (i = 0x10; i < 0x48; i++)
		ioReadable[i] = false;
	for (i = 0x4c; i < 0x50; i++)
		ioReadable[i] = false;
	for (i = 0x54; i < 0x60; i++)
		ioReadable[i] = false;
	for (i = 0x8c; i < 0x90; i++)
		ioReadable[i] = false;
	for (i = 0xa0; i < 0xb8; i++)
		ioReadable[i] = false;
	for (i = 0xbc; i < 0xc4; i++)
		ioReadable[i] = false;
	for (i = 0xc8; i < 0xd0; i++)
		ioReadable[i] = false;
	for (i = 0xd4; i < 0xdc; i++)
		ioReadable[i] = false;
	for (i = 0xe0; i < 0x100; i++)
		ioReadable[i] = false;
	for (i = 0x110; i < 0x120; i++)
		ioReadable[i] = false;
	for (i = 0x12c; i < 0x130; i++)
		ioReadable[i] = false;
	for (i = 0x138; i < 0x140; i++)
		ioReadable[i] = false;
	for (i = 0x144; i < 0x150; i++)
		ioReadable[i] = false;
	for (i = 0x15c; i < 0x200; i++)
		ioReadable[i] = false;
	for (i = 0x20c; i < 0x300; i++)
		ioReadable[i] = false;
	for (i = 0x304; i < 0x400; i++)
		ioReadable[i] = false;
	return ioReadable;
}();

constexpr uint32_t stop = 0x08000568;
extern bool gba_joybus_enabled;
extern bool gba_joybus_active;
constexpr int customBackdropColor = -1;

static uint16a &P1 = *((uint16a*)&gGba.mem.ioMem.b[0x130]);

#endif // VBAM_CORE_GBA_GBAGLOBALS_H_
