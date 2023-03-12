#ifndef GBA_H
#define GBA_H

#include "../common/Types.h"
#include "../common/Port.h"
#include "../System.h"
#include "../Util.h"
#include "../NLS.h"
#include <imagine/util/mayAliasInt.h>
#include <imagine/util/memory/Buffer.hh>
#include <array>
#include <cstring>

namespace IG
{
class IO;
}

struct GBASys;
struct ARM7TDMI;

const uint64_t TICKS_PER_SECOND = 16777216;

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION_9 9
#define SAVE_GAME_VERSION_10 10
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_10

#define gbaWidth  240
#define gbaHeight 160

enum {
    GBA_SAVE_AUTO = 0,
    GBA_SAVE_EEPROM,
    GBA_SAVE_SRAM,
    GBA_SAVE_FLASH,
    GBA_SAVE_EEPROM_SENSOR,
    GBA_SAVE_NONE
};

enum {
    SIZE_SRAM      = 32768,
    SIZE_FLASH512   = 65536,
    SIZE_FLASH1M   = 131072,
    SIZE_EEPROM_512 = 512,
    SIZE_EEPROM_8K = 8192
};

enum {
    SIZE_ROM   = 0x2000000,
    SIZE_BIOS  = 0x0004000,
    SIZE_IRAM  = 0x0008000,
    SIZE_WRAM  = 0x0040000,
    SIZE_PRAM  = 0x0000400,
    SIZE_VRAM  = 0x0020000,
    SIZE_OAM   = 0x0000400,
    SIZE_IOMEM = 0x0000400,
#ifndef __LIBRETRO__
    SIZE_PIX   = (4 * 241 * 162)
#else
    SIZE_PIX   = (4 * 240 * 160)
#endif
};

//#define USE_MEM_HANDLERS
struct memoryMap {
	typedef uint32_t (*readFunc)(ARM7TDMI &cpu, uint32_t address);
	constexpr memoryMap() {}
	#ifdef USE_MEM_HANDLERS
	constexpr memoryMap(uint8_t *address, uint32_t mask, readFunc read8, readFunc read16, readFunc read32):
			address(address), mask(mask), read8(read8), read16(read16), read32(read32) {}
	#else
	constexpr memoryMap(uint8_t *address, uint32_t mask, readFunc read8, readFunc read16, readFunc read32):
		address(address), mask(mask) {}
	#endif
  uint8_t *address{};
  uint32_t mask{};
#ifdef BKPT_SUPPORT
  uint8_t* breakPoints;
  uint8_t* searchMatch;
  uint8_t* trace;
  uint32_t size;
#endif
#ifdef USE_MEM_HANDLERS
  uint32_t (*read8)(ARM7TDMI &cpu, uint32_t address) = nullptr;
  uint32_t (*read16)(ARM7TDMI &cpu, uint32_t address) = nullptr;
  uint32_t (*read32)(ARM7TDMI &cpu, uint32_t address) = nullptr;
  /*void (*write8)(uint32_t address, uint32_t data) = nullptr;
  void (*write16)(uint32_t address, uint32_t data) = nullptr;
  void (*write32)(uint32_t address, uint32_t data) = nullptr;*/
#endif
};

union reg_pair {
  struct {
#ifdef WORDS_BIGENDIAN
    uint8_t B3;
    uint8_t B2;
    uint8_t B1;
    uint8_t B0;
#else
    uint8_t B0;
    uint8_t B1;
    uint8_t B2;
    uint8_t B3;
#endif
  } B;
  struct {
#ifdef WORDS_BIGENDIAN
    uint16_t W1;
    uint16_t W0;
#else
    uint16_t W0;
    uint16_t W1;
#endif
  } W;
#ifdef WORDS_BIGENDIAN
  volatile uint32_t I{};
#else
	uint32_t I{};
#endif
};

extern void (*cpuSaveGameFunc)(uint32_t, uint8_t);

extern bool cpuSramEnabled;
extern bool cpuFlashEnabled;
extern bool cpuEEPROMEnabled;
extern bool cpuEEPROMSensorEnabled;
extern bool saveMemoryIsMappedFile;

#ifdef BKPT_SUPPORT
extern uint8_t freezeWorkRAM[0x40000];
extern uint8_t freezeInternalRAM[0x8000];
extern uint8_t freezeVRAM[0x18000];
extern uint8_t freezeOAM[0x400];
extern uint8_t freezePRAM[0x400];
extern bool debugger_last;
extern int  oldreg[18];
extern char oldbuffer[10];
extern bool debugger;
#endif

extern bool CPUReadGSASnapshot(const char*);
extern bool CPUReadGSASPSnapshot(const char*);
extern bool CPUWriteGSASnapshot(const char*, const char*, const char*, const char*);
extern bool CPUWriteBatteryFile(const char*);
extern bool CPUReadBatteryFile(const char*);
extern bool CPUExportEepromFile(const char*);
extern bool CPUImportEepromFile(const char*);
extern bool CPUWritePNGFile(const char*);
extern bool CPUWriteBMPFile(const char*);
extern void CPUCleanUp();
extern void CPUUpdateRender(GBASys &gba);
extern void CPUUpdateRenderBuffers(bool);
extern bool CPUReadMemState(GBASys &gba, char *, int);
extern bool CPUWriteMemState(GBASys &gba, char *, int);
#ifdef __LIBRETRO__
extern bool CPUReadState(const uint8_t*);
extern unsigned int CPUWriteState(uint8_t* data, unsigned int size);
#else
extern bool CPUReadState(GBASys &gba, const char*);
extern bool CPUWriteState(GBASys &gba, const char*);
#endif
extern int CPULoadRom(GBASys &gba, const char *);
extern int CPULoadRomWithIO(GBASys &gba, IG::IO &);
extern void doMirroring(GBASys &gba, bool);
extern void CPUUpdateRegister(ARM7TDMI &cpu, uint32_t, uint16_t);
extern void applyTimer(ARM7TDMI &cpu);
extern void CPUInit(GBASys &gba, const char *,bool);
void SetSaveType(int st);
extern void CPUReset(GBASys &gba);
extern void CPULoop(int);
extern void CPUCheckDMA(GBASys &gba, ARM7TDMI &cpu, int,int);
extern bool CPUIsGBAImage(const char*);
extern bool CPUIsZipFile(const char*);
#ifdef PROFILING
#include "prof/prof.h"
extern void cpuProfil(profile_segment* seg);
extern void cpuEnableProfiling(int hz);
#endif

const char* GetLoadDotCodeFile();
const char* GetSaveDotCodeFile();
void ResetLoadDotCodeFile();
void SetLoadDotCodeFile(const char* szFile);
void ResetSaveDotCodeFile();
void SetSaveDotCodeFile(const char* szFile);

// Updates romSize and realloc rom pointer if needed after soft-patching
void gbaUpdateRomSize(int size);

void setSaveMemory(IG::ByteBuffer buff);
size_t saveMemorySize();

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

#include <main/GBASys.hh>
#include "Cheats.h"
#include "EEprom.h"
#include "Flash.h"
#include "Globals.h"

#endif // GBA_H
