#ifndef VBAM_CORE_GBA_GBAFLASH_H_
#define VBAM_CORE_GBA_GBAFLASH_H_

#include <cstdint>

#if !defined(__LIBRETRO__)
#include <zlib.h>
#endif  // defined(__LIBRETRO__)

#include <imagine/util/memory/Buffer.hh>

#define FLASH_128K_SZ 0x20000

void flashDetectSaveType(const uint8_t *rom, const int size);

extern void flashSaveGame(uint8_t*& data);
extern void flashReadGame(const uint8_t*& data);

extern void flashSaveGame(gzFile _gzFile);
extern void flashReadGame(gzFile _gzFile, int version);
extern void flashReadGameSkip(gzFile _gzFile, int version);

extern IG::ByteBuffer flashSaveMemory;
extern uint8_t flashRead(uint32_t address);
extern void flashWrite(uint32_t address, uint8_t byte);
extern void flashDelayedWrite(uint32_t address, uint8_t byte);
extern void flashSaveDecide(uint32_t address, uint8_t byte);
extern void flashReset();
extern void flashSetSize(int size);
extern void flashInit();

extern int g_flashSize;

#endif // VBAM_CORE_GBA_GBAFLASH_H_
