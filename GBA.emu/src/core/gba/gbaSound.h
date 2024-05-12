#ifndef VBAM_CORE_GBA_GBASOUND_H_
#define VBAM_CORE_GBA_GBASOUND_H_

#include <cstdint>

#if !defined(__LIBRETRO__)
#include <zlib.h>
#endif  // !defined(__LIBRETRO__)

// Sound emulation setup/options and GBA sound emulation

namespace EmuEx
{
class EmuAudio;
}

struct GBASys;
struct ARM7TDMI;

//// Setup/options (these affect GBA and GB sound)

// Initializes sound and returns true if successful. Sets sound quality to
// current value in soundQuality global.
bool soundInit();

// sets the Sound throttle
void soundSetThrottle(unsigned short throttle);

// Manages sound volume, where 1.0 is normal
void soundSetVolume(GBASys &, float vol, bool gbVol);
float soundGetVolume(GBASys &, bool gbVol);

// Manages muting bitmask. The bits control the following channels:
// 0x001 Pulse 1
// 0x002 Pulse 2
// 0x004 Wave
// 0x008 Noise
// 0x100 PCM 1
// 0x200 PCM 2
void soundSetEnable(GBASys &, int mask);
int  soundGetEnable(GBASys &);

// Pauses/resumes system sound output
void soundPause();
void soundResume();
extern bool soundPaused; // current paused state

// Cleans up sound. Afterwards, soundInit() can be called again.
void soundShutdown();

//// GBA sound options

long soundGetSampleRate();
void soundSetSampleRate(GBASys &gba, long sampleRate);

// Sound settings
void soundSetFiltering(GBASys &, float level);
float soundGetFiltering(GBASys &);
void soundSetInterpolation(GBASys &, bool on);
bool soundGetInterpolation(GBASys &);

//// GBA sound emulation

// GBA sound registers
#define SGCNT0_H 0x82
#define SOUNDBIAS 0x88
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

// Resets emulated sound hardware
void soundReset(GBASys &gba);

// Emulates write to sound hardware
void soundEvent8(GBASys &gba, uint32_t addr, uint8_t  data);
void soundEvent16(GBASys &gba, uint32_t addr, uint16_t data); // TODO: error-prone to overload like this

// Notifies emulator that a timer has overflowed
void soundTimerOverflow(GBASys &gba, ARM7TDMI &cpu, int which );

// Notifies emulator that PCM rate may have changed
void interp_rate();

// Notifies emulator that SOUND_CLOCK_TICKS clocks have passed
void psoundTickfn(EmuEx::EmuAudio *audio);
extern const int SOUND_CLOCK_TICKS;   // Number of 16.8 MHz clocks between calls to soundTick()
// 2018-12-10 - counts up from 0 since last psoundTickfn() was called
extern int &soundTicks;          // Number of 16.8 MHz clocks until soundTick() will be called

// Saves/loads emulator state

void soundSaveGame(uint8_t*&);
void soundReadGame(GBASys &gba, const uint8_t*& in);

void soundSaveGame(gzFile);
void soundReadGame(GBASys &gba, gzFile, int version);


class Multi_Buffer;

void flush_samples(Multi_Buffer * buffer, EmuEx::EmuAudio *audio);

#endif // VBAM_CORE_GBA_GBASOUND_H_
