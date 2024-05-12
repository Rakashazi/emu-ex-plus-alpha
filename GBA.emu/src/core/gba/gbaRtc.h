#ifndef VBAM_CORE_GBA_GBARTC_H_
#define VBAM_CORE_GBA_GBARTC_H_

#include <cstdint>

#if !defined(__LIBRETRO__)
#include <zlib.h>
#endif  // defined(__LIBRETRO__)

struct GBASys;

uint16_t rtcRead(GBASys &gba, uint32_t address);
void rtcUpdateTime(int ticks);
bool rtcWrite(uint32_t address, uint16_t value);
void rtcEnable(bool);
void rtcEnableRumble(bool e);
bool rtcIsEnabled();
void rtcReset();


void rtcReadGame(const uint8_t*& data);
void rtcSaveGame(uint8_t*& data);

void rtcReadGame(gzFile gzFile);
void rtcSaveGame(gzFile gzFile);


#endif // VBAM_CORE_GBA_GBARTC_H_
