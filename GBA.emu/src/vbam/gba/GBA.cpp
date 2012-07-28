#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>
#include "GBA.h"
#include "GBAcpu.h"
#include "GBAinline.h"
#include "Globals.h"
#include "GBAGfx.h"
#include "EEprom.h"
#include "Flash.h"
#include "Sound.h"
#include "Sram.h"
#include "bios.h"
#include "Cheats.h"
#include "../NLS.h"
#include "elf.h"
#include "../Util.h"
#include "../common/Port.h"
#include "../System.h"
#include "agbprint.h"
#include "GBALink.h"
#include <logger/interface.h>
#include <io/sys.hh>

#ifdef PROFILING
#include "prof/prof.h"
#endif

#ifdef __GNUC__
#define _stricmp strcasecmp
#endif

GBASys gGba;
GBALCD gLcd;
GBAMem gMem;


u32 mastercode = 0;
static auto &layerEnableDelay = gLcd.layerEnableDelay;

int gbaSaveType = 0; // used to remember the save type on reset
#ifdef VBAM_USE_HOLDTYPE
int holdType = 0;
#else
static int holdTypeDummy;
#endif
bool cpuSramEnabled = true;
bool cpuFlashEnabled = true;
bool cpuEEPROMEnabled = true;
bool cpuEEPROMSensorEnabled = false;

#ifdef PROFILING
int profilingTicks = 0;
int profilingTicksReload = 0;
static profile_segment *profilSegment = NULL;
#endif

#ifdef BKPT_SUPPORT
u8 freezeWorkRAM[0x40000];
u8 freezeInternalRAM[0x8000];
u8 freezeVRAM[0x18000];
u8 freezePRAM[0x400];
u8 freezeOAM[0x400];
bool debugger_last;
#endif

static auto &lcdTicks = gLcd.lcdTicks;
void (*cpuSaveGameFunc)(u32,u8) = flashSaveDecide;
static auto &renderLine = gLcd.renderLine;
static auto &fxOn = gLcd.fxOn;
static auto &windowOn = gLcd.windowOn;
static const bool trackOAM = 1, oamUpdated = 1;

static auto &lineMix = gLcd.lineMix;
static const int TIMER_TICKS[4] = {
  0,
  6,
  8,
  10
};

static const u8 gamepakRamWaitState[4] = { 4, 3, 2, 8 };
static const u8 gamepakWaitState[4] =  { 4, 3, 2, 8 };
static const u8 gamepakWaitState0[2] = { 2, 1 };
static const u8 gamepakWaitState1[2] = { 4, 1 };
static const u8 gamepakWaitState2[2] = { 8, 1 };
static const bool isInRom [16]=
  { false, false, false, false, false, false, false, false,
    true, true, true, true, true, true, false, false };

uint memoryWait[16] =
  { 0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
uint memoryWait32[16] =
  { 0, 0, 5, 0, 0, 1, 1, 0, 7, 7, 9, 9, 13, 13, 4, 0 };
uint memoryWaitSeq[16] =
  { 0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 0 };
uint memoryWaitSeq32[16] =
  { 0, 0, 5, 0, 0, 1, 1, 0, 5, 5, 9, 9, 17, 17, 4, 0 };

// The videoMemoryWait constants are used to add some waitstates
// if the opcode access video memory data outside of vblank/hblank
// It seems to happen on only one ticks for each pixel.
// Not used for now (too problematic with current code).
//const u8 videoMemoryWait[16] =
//  {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0};


u8 biosProtected[4];

#ifdef WORDS_BIGENDIAN
bool cpuBiosSwapped = false;
#endif

static const u32 myROM[] = {
0xEA000006,
0xEA000093,
0xEA000006,
0x00000000,
0x00000000,
0x00000000,
0xEA000088,
0x00000000,
0xE3A00302,
0xE1A0F000,
0xE92D5800,
0xE55EC002,
0xE28FB03C,
0xE79BC10C,
0xE14FB000,
0xE92D0800,
0xE20BB080,
0xE38BB01F,
0xE129F00B,
0xE92D4004,
0xE1A0E00F,
0xE12FFF1C,
0xE8BD4004,
0xE3A0C0D3,
0xE129F00C,
0xE8BD0800,
0xE169F00B,
0xE8BD5800,
0xE1B0F00E,
0x0000009C,
0x0000009C,
0x0000009C,
0x0000009C,
0x000001F8,
0x000001F0,
0x000000AC,
0x000000A0,
0x000000FC,
0x00000168,
0xE12FFF1E,
0xE1A03000,
0xE1A00001,
0xE1A01003,
0xE2113102,
0x42611000,
0xE033C040,
0x22600000,
0xE1B02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE1A01000,
0xE1A00003,
0xE1B0C08C,
0x22600000,
0x42611000,
0xE12FFF1E,
0xE92D0010,
0xE1A0C000,
0xE3A01001,
0xE1500001,
0x81A000A0,
0x81A01081,
0x8AFFFFFB,
0xE1A0000C,
0xE1A04001,
0xE3A03000,
0xE1A02001,
0xE15200A0,
0x91A02082,
0x3AFFFFFC,
0xE1500002,
0xE0A33003,
0x20400002,
0xE1320001,
0x11A020A2,
0x1AFFFFF9,
0xE0811003,
0xE1B010A1,
0xE1510004,
0x3AFFFFEE,
0xE1A00004,
0xE8BD0010,
0xE12FFF1E,
0xE0010090,
0xE1A01741,
0xE2611000,
0xE3A030A9,
0xE0030391,
0xE1A03743,
0xE2833E39,
0xE0030391,
0xE1A03743,
0xE2833C09,
0xE283301C,
0xE0030391,
0xE1A03743,
0xE2833C0F,
0xE28330B6,
0xE0030391,
0xE1A03743,
0xE2833C16,
0xE28330AA,
0xE0030391,
0xE1A03743,
0xE2833A02,
0xE2833081,
0xE0030391,
0xE1A03743,
0xE2833C36,
0xE2833051,
0xE0030391,
0xE1A03743,
0xE2833CA2,
0xE28330F9,
0xE0000093,
0xE1A00840,
0xE12FFF1E,
0xE3A00001,
0xE3A01001,
0xE92D4010,
0xE3A03000,
0xE3A04001,
0xE3500000,
0x1B000004,
0xE5CC3301,
0xEB000002,
0x0AFFFFFC,
0xE8BD4010,
0xE12FFF1E,
0xE3A0C301,
0xE5CC3208,
0xE15C20B8,
0xE0110002,
0x10222000,
0x114C20B8,
0xE5CC4208,
0xE12FFF1E,
0xE92D500F,
0xE3A00301,
0xE1A0E00F,
0xE510F004,
0xE8BD500F,
0xE25EF004,
0xE59FD044,
0xE92D5000,
0xE14FC000,
0xE10FE000,
0xE92D5000,
0xE3A0C302,
0xE5DCE09C,
0xE35E00A5,
0x1A000004,
0x05DCE0B4,
0x021EE080,
0xE28FE004,
0x159FF018,
0x059FF018,
0xE59FD018,
0xE8BD5000,
0xE169F00C,
0xE8BD5000,
0xE25EF004,
0x03007FF0,
0x09FE2000,
0x09FFC000,
0x03007FE0
};

static bool saveNFlag, saveZFlag;

static const variable_desc saveGameStruct[] = {
  { &ioMem.DISPCNT  , sizeof(u16) },
  { &ioMem.DISPSTAT , sizeof(u16) },
  { &ioMem.VCOUNT   , sizeof(u16) },
  { &ioMem.BG0CNT   , sizeof(u16) },
  { &ioMem.BG1CNT   , sizeof(u16) },
  { &ioMem.BG2CNT   , sizeof(u16) },
  { &ioMem.BG3CNT   , sizeof(u16) },
  { &ioMem.BG0HOFS  , sizeof(u16) },
  { &ioMem.BG0VOFS  , sizeof(u16) },
  { &ioMem.BG1HOFS  , sizeof(u16) },
  { &ioMem.BG1VOFS  , sizeof(u16) },
  { &ioMem.BG2HOFS  , sizeof(u16) },
  { &ioMem.BG2VOFS  , sizeof(u16) },
  { &ioMem.BG3HOFS  , sizeof(u16) },
  { &ioMem.BG3VOFS  , sizeof(u16) },
  { &ioMem.BG2PA    , sizeof(u16) },
  { &ioMem.BG2PB    , sizeof(u16) },
  { &ioMem.BG2PC    , sizeof(u16) },
  { &ioMem.BG2PD    , sizeof(u16) },
  { &ioMem.BG2X_L   , sizeof(u16) },
  { &ioMem.BG2X_H   , sizeof(u16) },
  { &ioMem.BG2Y_L   , sizeof(u16) },
  { &ioMem.BG2Y_H   , sizeof(u16) },
  { &ioMem.BG3PA    , sizeof(u16) },
  { &ioMem.BG3PB    , sizeof(u16) },
  { &ioMem.BG3PC    , sizeof(u16) },
  { &ioMem.BG3PD    , sizeof(u16) },
  { &ioMem.BG3X_L   , sizeof(u16) },
  { &ioMem.BG3X_H   , sizeof(u16) },
  { &ioMem.BG3Y_L   , sizeof(u16) },
  { &ioMem.BG3Y_H   , sizeof(u16) },
  { &ioMem.WIN0H    , sizeof(u16) },
  { &ioMem.WIN1H    , sizeof(u16) },
  { &ioMem.WIN0V    , sizeof(u16) },
  { &ioMem.WIN1V    , sizeof(u16) },
  { &ioMem.WININ    , sizeof(u16) },
  { &ioMem.WINOUT   , sizeof(u16) },
  { &ioMem.MOSAIC   , sizeof(u16) },
  { &ioMem.BLDMOD   , sizeof(u16) },
  { &ioMem.COLEV    , sizeof(u16) },
  { &ioMem.COLY     , sizeof(u16) },
  { &ioMem.DM0SAD_L , sizeof(u16) },
  { &ioMem.DM0SAD_H , sizeof(u16) },
  { &ioMem.DM0DAD_L , sizeof(u16) },
  { &ioMem.DM0DAD_H , sizeof(u16) },
  { &ioMem.DM0CNT_L , sizeof(u16) },
  { &ioMem.DM0CNT_H , sizeof(u16) },
  { &ioMem.DM1SAD_L , sizeof(u16) },
  { &ioMem.DM1SAD_H , sizeof(u16) },
  { &ioMem.DM1DAD_L , sizeof(u16) },
  { &ioMem.DM1DAD_H , sizeof(u16) },
  { &ioMem.DM1CNT_L , sizeof(u16) },
  { &ioMem.DM1CNT_H , sizeof(u16) },
  { &ioMem.DM2SAD_L , sizeof(u16) },
  { &ioMem.DM2SAD_H , sizeof(u16) },
  { &ioMem.DM2DAD_L , sizeof(u16) },
  { &ioMem.DM2DAD_H , sizeof(u16) },
  { &ioMem.DM2CNT_L , sizeof(u16) },
  { &ioMem.DM2CNT_H , sizeof(u16) },
  { &ioMem.DM3SAD_L , sizeof(u16) },
  { &ioMem.DM3SAD_H , sizeof(u16) },
  { &ioMem.DM3DAD_L , sizeof(u16) },
  { &ioMem.DM3DAD_H , sizeof(u16) },
  { &ioMem.DM3CNT_L , sizeof(u16) },
  { &ioMem.DM3CNT_H , sizeof(u16) },
  { &ioMem.TM0D     , sizeof(u16) },
  { &ioMem.TM0CNT   , sizeof(u16) },
  { &ioMem.TM1D     , sizeof(u16) },
  { &ioMem.TM1CNT   , sizeof(u16) },
  { &ioMem.TM2D     , sizeof(u16) },
  { &ioMem.TM2CNT   , sizeof(u16) },
  { &ioMem.TM3D     , sizeof(u16) },
  { &ioMem.TM3CNT   , sizeof(u16) },
  { &P1       , sizeof(u16) },
  { &ioMem.IE       , sizeof(u16) },
  { &ioMem.IF       , sizeof(u16) },
  { &ioMem.IME      , sizeof(u16) },
  { &gGba.cpu.holdState, sizeof(bool) },
#ifdef VBAM_USE_HOLDTYPE
  { &holdType, sizeof(int) },
#else
  { &holdTypeDummy, sizeof(int) },
#endif
  { &lcdTicks, sizeof(int) },
  { &gGba.timers.timer0On , sizeof(bool) },
  { &gGba.timers.timer0Ticks , sizeof(int) },
  { &gGba.timers.timer0Reload , sizeof(int) },
  { &gGba.timers.timer0ClockReload  , sizeof(int) },
  { &gGba.timers.timer1On , sizeof(bool) },
  { &gGba.timers.timer1Ticks , sizeof(int) },
  { &gGba.timers.timer1Reload , sizeof(int) },
  { &gGba.timers.timer1ClockReload  , sizeof(int) },
  { &gGba.timers.timer2On , sizeof(bool) },
  { &gGba.timers.timer2Ticks , sizeof(int) },
  { &gGba.timers.timer2Reload , sizeof(int) },
  { &gGba.timers.timer2ClockReload  , sizeof(int) },
  { &gGba.timers.timer3On , sizeof(bool) },
  { &gGba.timers.timer3Ticks , sizeof(int) },
  { &gGba.timers.timer3Reload , sizeof(int) },
  { &gGba.timers.timer3ClockReload  , sizeof(int) },
  { &gGba.dma.dma0Source , sizeof(u32) },
  { &gGba.dma.dma0Dest , sizeof(u32) },
  { &gGba.dma.dma1Source , sizeof(u32) },
  { &gGba.dma.dma1Dest , sizeof(u32) },
  { &gGba.dma.dma2Source , sizeof(u32) },
  { &gGba.dma.dma2Dest , sizeof(u32) },
  { &gGba.dma.dma3Source , sizeof(u32) },
  { &gGba.dma.dma3Dest , sizeof(u32) },
  { &fxOn, sizeof(bool) },
  { &windowOn, sizeof(bool) },
  { &saveNFlag , sizeof(bool) },
  { &gGba.cpu.C_FLAG , sizeof(bool) },
  { &saveZFlag , sizeof(bool) },
  { &gGba.cpu.V_FLAG , sizeof(bool) },
  { &gGba.cpu.armState , sizeof(bool) },
  { &gGba.cpu.armIrqEnable , sizeof(bool) },
  { &gGba.cpu.armNextPC , sizeof(u32) },
  { &gGba.cpu.armMode , sizeof(int) },
  { &saveType , sizeof(int) },
  { NULL, 0 }
};

static int romSize = 0x2000000;

#ifdef PROFILING
void cpuProfil(profile_segment *seg)
{
    profilSegment = seg;
}

void cpuEnableProfiling(int hz)
{
  if(hz == 0)
    hz = 100;
  profilingTicks = profilingTicksReload = 16777216 / hz;
  profSetHertz(hz);
}
#endif


inline int CPUUpdateTicks(ARM7TDMI &cpu)
{
#ifdef VBAM_USE_SWITICKS
	int &SWITicks = cpu.SWITicks;
#endif
#ifdef VBAM_USE_IRQTICKS
	int &IRQTicks = cpu.IRQTicks;
#endif
  int cpuLoopTicks = lcdTicks;

  if(soundTicks < cpuLoopTicks)
    cpuLoopTicks = soundTicks;

  auto &timer0On = gGba.timers.timer0On;
  auto &timer0Ticks = gGba.timers.timer0Ticks;
  auto &timer1On = gGba.timers.timer1On;
  auto &timer1Ticks = gGba.timers.timer1Ticks;
  auto &timer2On = gGba.timers.timer2On;
  auto &timer2Ticks = gGba.timers.timer2Ticks;
  auto &timer3On = gGba.timers.timer3On;
  auto &timer3Ticks = gGba.timers.timer3Ticks;

  if(timer0On && (timer0Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer0Ticks;
  }
  if(timer1On && !(ioMem.TM1CNT & 4) && (timer1Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer1Ticks;
  }
  if(timer2On && !(ioMem.TM2CNT & 4) && (timer2Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer2Ticks;
  }
  if(timer3On && !(ioMem.TM3CNT & 4) && (timer3Ticks < cpuLoopTicks)) {
    cpuLoopTicks = timer3Ticks;
  }
#ifdef PROFILING
  if(profilingTicksReload != 0) {
    if(profilingTicks < cpuLoopTicks) {
      cpuLoopTicks = profilingTicks;
    }
  }
#endif

#ifdef VBAM_USE_SWITICKS
  if (SWITicks) {
    if (SWITicks < cpuLoopTicks)
        cpuLoopTicks = SWITicks;
  }
#endif

#ifdef VBAM_USE_IRQTICKS
  if (IRQTicks) {
    if (IRQTicks < cpuLoopTicks)
        cpuLoopTicks = IRQTicks;
  }
#endif

  return cpuLoopTicks;
}

static void CPUUpdateWindow0()
{
  int x00 = ioMem.WIN0H>>8;
  int x01 = ioMem.WIN0H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 || i < x01);
    }
  }
}

static void CPUUpdateWindow1()
{
  int x00 = ioMem.WIN1H>>8;
  int x01 = ioMem.WIN1H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 || i < x01);
    }
  }
}

static void CPUUpdateRenderBuffers(bool force)
{
  if(!(layerEnable & 0x0100) || force) {
    gfxClearArray(line0);
  }
  if(!(layerEnable & 0x0200) || force) {
  	gfxClearArray(line1);
  }
  if(!(layerEnable & 0x0400) || force) {
  	gfxClearArray(line2);
  }
  if(!(layerEnable & 0x0800) || force) {
  	gfxClearArray(line3);
  }
}

static bool CPUWriteState(gzFile gzFile)
{
  utilWriteInt(gzFile, SAVE_GAME_VERSION);

  utilGzWrite(gzFile, &rom[0xa0], 16);

  utilWriteInt(gzFile, useBios);

  utilGzWrite(gzFile, &gGba.cpu.reg[0], sizeof(gGba.cpu.reg));

  saveNFlag = gGba.cpu.nFlag();
  saveZFlag = gGba.cpu.zFlag();
  utilWriteData(gzFile, saveGameStruct);

  // new to version 0.7.1
  utilWriteInt(gzFile, gGba.stopState);
  // new to version 0.8
#ifdef VBAM_USE_IRQTICKS
  utilWriteInt(gzFile, gCpu.IRQTicks);
#else
  int dummyIRQTicks = 0;
  utilWriteInt(gzFile, dummyIRQTicks);
#endif

  utilGzWrite(gzFile, internalRAM, 0x8000);
  utilGzWrite(gzFile, paletteRAM, 0x400);
  utilGzWrite(gzFile, workRAM, 0x40000);
  utilGzWrite(gzFile, vram, 0x20000);
  utilGzWrite(gzFile, oam, 0x400);
  u32 dummyPix[241*162] = {0};
  utilGzWrite(gzFile, dummyPix, 4*241*162);
  utilGzWrite(gzFile, ioMem.b, 0x400);

  eepromSaveGame(gzFile);
  flashSaveGame(gzFile);
  soundSaveGame(gzFile);

  cheatsSaveGame(gzFile);

  // version 1.5
  rtcSaveGame(gzFile);

  return true;
}

bool CPUWriteState(const char *file)
{
  gzFile gzFile = utilGzOpen(file, "wb");

  if(gzFile == NULL) {
    systemMessage(MSG_ERROR_CREATING_FILE, N_("Error creating file %s"), file);
    return false;
  }

  bool res = CPUWriteState(gzFile);

  utilGzClose(gzFile);

  return res;
}

bool CPUWriteMemState(char *memory, int available)
{
  gzFile gzFile = utilMemGzOpen(memory, available, "w");

  if(gzFile == NULL) {
    return false;
  }

  bool res = CPUWriteState(gzFile);

  long pos = utilGzMemTell(gzFile)+8;

  if(pos >= (available))
    res = false;

  utilGzClose(gzFile);

  return res;
}

static bool CPUReadState(gzFile gzFile)
{
  int version = utilReadInt(gzFile);

  if(version > SAVE_GAME_VERSION || version < SAVE_GAME_VERSION_1) {
    systemMessage(MSG_UNSUPPORTED_VBA_SGM,
                  N_("Unsupported VisualBoyAdvance save game version %d"),
                  version);
    return false;
  }

  u8 romname[17];

  utilGzRead(gzFile, romname, 16);

  if(memcmp(&rom[0xa0], romname, 16) != 0) {
    romname[16]=0;
    for(int i = 0; i < 16; i++)
      if(romname[i] < 32)
        romname[i] = 32;
    systemMessage(MSG_CANNOT_LOAD_SGM, N_("Cannot load save game for %s"), romname);
    return false;
  }

  bool ub = utilReadInt(gzFile) ? true : false;

  if(ub != useBios) {
    if(useBios)
      systemMessage(MSG_SAVE_GAME_NOT_USING_BIOS,
                    N_("Save game is not using the BIOS files"));
    else
      systemMessage(MSG_SAVE_GAME_USING_BIOS,
                    N_("Save game is using the BIOS file"));
    return false;
  }

  utilGzRead(gzFile, &gGba.cpu.reg[0], sizeof(gGba.cpu.reg));

  utilReadData(gzFile, saveGameStruct);
  gGba.cpu.updateNZFlags(saveNFlag, saveZFlag);

  if(version < SAVE_GAME_VERSION_3)
  	gGba.stopState = false;
  else
  	gGba.stopState = utilReadInt(gzFile) ? true : false;

  if(version < SAVE_GAME_VERSION_4)
  {
#ifdef VBAM_USE_IRQTICKS
  	gCpu.IRQTicks = 0;
#endif
  	gGba.intState = false;
  }
  else
  {
#ifdef VBAM_USE_IRQTICKS
  	gCpu.IRQTicks = utilReadInt(gzFile);
    if (gCpu.IRQTicks>0)
      intState = true;
    else
    {
      intState = false;
      gCpu.IRQTicks = 0;
    }
#else
    utilReadInt(gzFile);
    gGba.intState = false;
#endif
  }

  utilGzRead(gzFile, internalRAM, 0x8000);
  utilGzRead(gzFile, paletteRAM, 0x400);
  utilGzRead(gzFile, workRAM, 0x40000);
  utilGzRead(gzFile, vram, 0x20000);
  utilGzRead(gzFile, oam, 0x400);
  u32 dummyPix[241*162];
  if(version < SAVE_GAME_VERSION_6)
    utilGzRead(gzFile, dummyPix, 4*240*160);
  else
    utilGzRead(gzFile, dummyPix, 4*241*162);
  utilGzRead(gzFile, ioMem.b, 0x400);

  if(skipSaveGameBattery) {
    // skip eeprom data
    eepromReadGameSkip(gzFile, version);
    // skip flash data
    flashReadGameSkip(gzFile, version);
  } else {
    eepromReadGame(gzFile, version);
    flashReadGame(gzFile, version);
  }
  soundReadGame(gzFile, version);

  if(version > SAVE_GAME_VERSION_1) {
    if(skipSaveGameCheats) {
      // skip cheats list data
      cheatsReadGameSkip(gzFile, version);
    } else {
      cheatsReadGame(gzFile, version);
    }
  }
  if(version > SAVE_GAME_VERSION_6) {
    rtcReadGame(gzFile);
  }

  if(version <= SAVE_GAME_VERSION_7) {
    u32 temp;
#define SWAP(a,b,c) \
    temp = (a);\
    (a) = (b)<<16|(c);\
    (b) = (temp) >> 16;\
    (c) = (temp) & 0xFFFF;

    SWAP(gGba.dma.dma0Source, ioMem.DM0SAD_H, ioMem.DM0SAD_L);
    SWAP(gGba.dma.dma0Dest,   ioMem.DM0DAD_H, ioMem.DM0DAD_L);
    SWAP(gGba.dma.dma1Source, ioMem.DM1SAD_H, ioMem.DM1SAD_L);
    SWAP(gGba.dma.dma1Dest,   ioMem.DM1DAD_H, ioMem.DM1DAD_L);
    SWAP(gGba.dma.dma2Source, ioMem.DM2SAD_H, ioMem.DM2SAD_L);
    SWAP(gGba.dma.dma2Dest,   ioMem.DM2DAD_H, ioMem.DM2DAD_L);
    SWAP(gGba.dma.dma3Source, ioMem.DM3SAD_H, ioMem.DM3SAD_L);
    SWAP(gGba.dma.dma3Dest,   ioMem.DM3DAD_H, ioMem.DM3DAD_L);
  }

  if(version <= SAVE_GAME_VERSION_8) {
  	gGba.timers.timer0ClockReload = TIMER_TICKS[ioMem.TM0CNT & 3];
  	gGba.timers.timer1ClockReload = TIMER_TICKS[ioMem.TM1CNT & 3];
  	gGba.timers.timer2ClockReload = TIMER_TICKS[ioMem.TM2CNT & 3];
  	gGba.timers.timer3ClockReload = TIMER_TICKS[ioMem.TM3CNT & 3];

  	gGba.timers.timer0Ticks = ((0x10000 - ioMem.TM0D) << gGba.timers.timer0ClockReload) - gGba.timers.timer0Ticks;
  	gGba.timers.timer1Ticks = ((0x10000 - ioMem.TM1D) << gGba.timers.timer1ClockReload) - gGba.timers.timer1Ticks;
  	gGba.timers.timer2Ticks = ((0x10000 - ioMem.TM2D) << gGba.timers.timer2ClockReload) - gGba.timers.timer2Ticks;
  	gGba.timers.timer3Ticks = ((0x10000 - ioMem.TM3D) << gGba.timers.timer3ClockReload) - gGba.timers.timer3Ticks;
    interp_rate();
  }

  // set pointers!
  layerEnable = layerSettings & ioMem.DISPCNT;

  CPUUpdateRender();
  CPUUpdateRenderBuffers(true);
  CPUUpdateWindow0();
  CPUUpdateWindow1();
  gbaSaveType = 0;
  switch(saveType) {
  case 0:
    cpuSaveGameFunc = flashSaveDecide;
    break;
  case 1:
    cpuSaveGameFunc = sramWrite;
    gbaSaveType = 1;
    break;
  case 2:
    cpuSaveGameFunc = flashWrite;
    gbaSaveType = 2;
    break;
  case 3:
     break;
  case 5:
    gbaSaveType = 5;
    break;
  default:
    systemMessage(MSG_UNSUPPORTED_SAVE_TYPE,
                  N_("Unsupported save type %d"), saveType);
    break;
  }
  if(eepromInUse)
    gbaSaveType = 3;

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
  if(gGba.cpu.armState) {
  	gGba.cpu.ARM_PREFETCH();
  } else {
  	gGba.cpu.THUMB_PREFETCH();
  }

  CPUUpdateRegister(gGba.cpu, 0x204, CPUReadHalfWordQuick(0x4000204));

  return true;
}

bool CPUReadMemState(char *memory, int available)
{
  gzFile gzFile = utilMemGzOpen(memory, available, "r");

  bool res = CPUReadState(gzFile);

  utilGzClose(gzFile);

  return res;
}

bool CPUReadState(const char * file)
{
  gzFile gzFile = utilGzOpen(file, "rb");

  if(gzFile == NULL)
    return false;

  bool res = CPUReadState(gzFile);

  utilGzClose(gzFile);

  return res;
}

bool CPUExportEepromFile(const char *fileName)
{
  if(eepromInUse) {
    FILE *file = fopen(fileName, "wb");

    if(!file) {
      systemMessage(MSG_ERROR_CREATING_FILE, N_("Error creating file %s"),
                    fileName);
      return false;
    }

    for(int i = 0; i < eepromSize;) {
      for(int j = 0; j < 8; j++) {
        if(fwrite(&eepromData[i+7-j], 1, 1, file) != 1) {
          fclose(file);
          return false;
        }
      }
      i += 8;
    }
    fclose(file);
  }
  return true;
}

bool CPUWriteBatteryFile(const char *fileName)
{
  if(gbaSaveType == 0) {
    if(eepromInUse)
      gbaSaveType = 3;
    else switch(saveType) {
    case 1:
      gbaSaveType = 1;
      break;
    case 2:
      gbaSaveType = 2;
      break;
    }
  }

  if((gbaSaveType) && (gbaSaveType!=5)) {
    FILE *file = fopen(fileName, "wb");

    if(!file) {
      systemMessage(MSG_ERROR_CREATING_FILE, N_("Error creating file %s"),
                    fileName);
      return false;
    }

    // only save if Flash/Sram in use or EEprom in use
    if(gbaSaveType != 3) {
      if(gbaSaveType == 2) {
        if(fwrite(flashSaveMemory, 1, flashSize, file) != (size_t)flashSize) {
          fclose(file);
          return false;
        }
      } else {
        if(fwrite(flashSaveMemory, 1, 0x10000, file) != 0x10000) {
          fclose(file);
          return false;
        }
      }
    } else {
      if(fwrite(eepromData, 1, eepromSize, file) != (size_t)eepromSize) {
        fclose(file);
        return false;
      }
    }
    fclose(file);
  }
  return true;
}

bool CPUReadGSASnapshot(const char *fileName)
{
  int i;
  FILE *file = fopen(fileName, "rb");

  if(!file) {
    systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), fileName);
    return false;
  }

  // check file size to know what we should read
  fseek(file, 0, SEEK_END);

  // long size = ftell(file);
  fseek(file, 0x0, SEEK_SET);
  fread(&i, 1, 4, file);
  fseek(file, i, SEEK_CUR); // Skip SharkPortSave
  fseek(file, 4, SEEK_CUR); // skip some sort of flag
  fread(&i, 1, 4, file); // name length
  fseek(file, i, SEEK_CUR); // skip name
  fread(&i, 1, 4, file); // desc length
  fseek(file, i, SEEK_CUR); // skip desc
  fread(&i, 1, 4, file); // notes length
  fseek(file, i, SEEK_CUR); // skip notes
  int saveSize;
  fread(&saveSize, 1, 4, file); // read length
  saveSize -= 0x1c; // remove header size
  char buffer[17];
  char buffer2[17];
  fread(buffer, 1, 16, file);
  buffer[16] = 0;
  for(i = 0; i < 16; i++)
    if(buffer[i] < 32)
      buffer[i] = 32;
  memcpy(buffer2, &rom[0xa0], 16);
  buffer2[16] = 0;
  for(i = 0; i < 16; i++)
    if(buffer2[i] < 32)
      buffer2[i] = 32;
  if(memcmp(buffer, buffer2, 16)) {
    systemMessage(MSG_CANNOT_IMPORT_SNAPSHOT_FOR,
                  N_("Cannot import snapshot for %s. Current game is %s"),
                  buffer,
                  buffer2);
    fclose(file);
    return false;
  }
  fseek(file, 12, SEEK_CUR); // skip some flags
  if(saveSize >= 65536) {
    if(fread(flashSaveMemory, 1, saveSize, file) != (size_t)saveSize) {
      fclose(file);
      return false;
    }
  } else {
    systemMessage(MSG_UNSUPPORTED_SNAPSHOT_FILE,
                  N_("Unsupported snapshot file %s"),
                  fileName);
    fclose(file);
    return false;
  }
  fclose(file);
  CPUReset();
  return true;
}

bool CPUReadGSASPSnapshot(const char *fileName)
{
  const char gsvfooter[] = "xV4\x12";
  const size_t namepos=0x0c, namesz=12;
  const size_t footerpos=0x42c, footersz=4;

  char footer[footersz+1], romname[namesz+1], savename[namesz+1];;
  FILE *file = fopen(fileName, "rb");

  if(!file) {
    systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), fileName);
    return false;
  }

  // read save name
  fseek(file, namepos, SEEK_SET);
  fread(savename, 1, namesz, file);
  savename[namesz] = 0;

  memcpy(romname, &rom[0xa0], namesz);
  romname[namesz] = 0;

  if(memcmp(romname, savename, namesz)) {
    systemMessage(MSG_CANNOT_IMPORT_SNAPSHOT_FOR,
                  N_("Cannot import snapshot for %s. Current game is %s"),
                  savename,
                  romname);
    fclose(file);
    return false;
  }

  // read footer tag
  fseek(file, footerpos, SEEK_SET);
  fread(footer, 1, footersz, file);
  footer[footersz] = 0;

  if(memcmp(footer, gsvfooter, footersz)) {
    systemMessage(0,
                  N_("Unsupported snapshot file %s. Footer '%s' at %u should be '%s'"),
                  fileName,
				  footer,
                  footerpos,
				  gsvfooter);
    fclose(file);
    return false;
  }

  // Read up to 128k save
  fread(flashSaveMemory, 1, FLASH_128K_SZ, file);

  fclose(file);
  CPUReset();
  return true;
}


bool CPUWriteGSASnapshot(const char *fileName,
                         const char *title,
                         const char *desc,
                         const char *notes)
{
  FILE *file = fopen(fileName, "wb");

  if(!file) {
    systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s"), fileName);
    return false;
  }

  u8 buffer[17];

  utilPutDword(buffer, 0x0d); // SharkPortSave length
  fwrite(buffer, 1, 4, file);
  fwrite("SharkPortSave", 1, 0x0d, file);
  utilPutDword(buffer, 0x000f0000);
  fwrite(buffer, 1, 4, file); // save type 0x000f0000 = GBA save
  utilPutDword(buffer, (u32)strlen(title));
  fwrite(buffer, 1, 4, file); // title length
  fwrite(title, 1, strlen(title), file);
  utilPutDword(buffer, (u32)strlen(desc));
  fwrite(buffer, 1, 4, file); // desc length
  fwrite(desc, 1, strlen(desc), file);
  utilPutDword(buffer, (u32)strlen(notes));
  fwrite(buffer, 1, 4, file); // notes length
  fwrite(notes, 1, strlen(notes), file);
  int saveSize = 0x10000;
  if(gbaSaveType == 2)
    saveSize = flashSize;
  int totalSize = saveSize + 0x1c;

  utilPutDword(buffer, totalSize); // length of remainder of save - CRC
  fwrite(buffer, 1, 4, file);

  char *temp = new char[0x2001c];
  memset(temp, 0, 28);
  memcpy(temp, &rom[0xa0], 16); // copy internal name
  temp[0x10] = rom[0xbe]; // reserved area (old checksum)
  temp[0x11] = rom[0xbf]; // reserved area (old checksum)
  temp[0x12] = rom[0xbd]; // complement check
  temp[0x13] = rom[0xb0]; // maker
  temp[0x14] = 1; // 1 save ?
  memcpy(&temp[0x1c], flashSaveMemory, saveSize); // copy save
  fwrite(temp, 1, totalSize, file); // write save + header
  u32 crc = 0;

  for(int i = 0; i < totalSize; i++) {
    crc += ((u32)temp[i] << (crc % 0x18));
  }

  utilPutDword(buffer, crc);
  fwrite(buffer, 1, 4, file); // CRC?

  fclose(file);
  delete [] temp;
  return true;
}

bool CPUImportEepromFile(const char *fileName)
{
  FILE *file = fopen(fileName, "rb");

  if(!file)
    return false;

  // check file size to know what we should read
  fseek(file, 0, SEEK_END);

  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  if(size == 512 || size == 0x2000) {
    if(fread(eepromData, 1, size, file) != (size_t)size) {
      fclose(file);
      return false;
    }
    for(int i = 0; i < size;) {
      u8 tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      tmp = eepromData[i];
      eepromData[i] = eepromData[7-i];
      eepromData[7-i] = tmp;
      i++;
      i += 4;
    }
  } else {
    fclose(file);
    return false;
  }
  fclose(file);
  return true;
}

bool CPUReadBatteryFile(const char *fileName)
{
	// Converted to Imagine IO funcs due to WebOS fread glitch
  auto *file = IoSys::open(fileName);

  if(!file)
    return false;

  // check file size to know what we should read
  auto size = file->size();
  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

  if(size == 512 || size == 0x2000) {
    if(file->readUpTo(eepromData, size) != (size_t)size) {
      delete file;
      return false;
    }
  } else {
    if(size == 0x20000) {
      if(file->readUpTo(flashSaveMemory, 0x20000) != 0x20000) {
      	delete file;
        return false;
      }
      flashSetSize(0x20000);
    } else {
      if(file->readUpTo(flashSaveMemory, 0x10000) != 0x10000) {
      	delete file;
        return false;
      }
      flashSetSize(0x10000);
    }
  }
  delete file;
  return true;
}

/*bool CPUWritePNGFile(const char *fileName)
{
  return utilWritePNGFile(fileName, 240, 160, pix);
}

bool CPUWriteBMPFile(const char *fileName)
{
  return utilWriteBMPFile(fileName, 240, 160, pix);
}*/

bool CPUIsZipFile(const char * file)
{
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".zip") == 0)
        return true;
    }
  }

  return false;
}

bool CPUIsGBAImage(const char * file)
{
  cpuIsMultiBoot = false;
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".gba") == 0)
        return true;
      if(_stricmp(p, ".agb") == 0)
        return true;
      if(_stricmp(p, ".bin") == 0)
        return true;
      if(_stricmp(p, ".elf") == 0)
        return true;
      if(_stricmp(p, ".mb") == 0) {
        cpuIsMultiBoot = true;
        return true;
      }
    }
  }

  return false;
}

bool CPUIsGBABios(const char * file)
{
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".gba") == 0)
        return true;
      if(_stricmp(p, ".agb") == 0)
        return true;
      if(_stricmp(p, ".bin") == 0)
        return true;
      if(_stricmp(p, ".bios") == 0)
        return true;
      if(_stricmp(p, ".rom") == 0)
        return true;
    }
  }

  return false;
}

bool CPUIsELF(const char *file)
{
  if(file == NULL)
	  return false;

  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".elf") == 0)
        return true;
    }
  }
  return false;
}

void CPUCleanUp()
{
#ifdef PROFILING
  if(profilingTicksReload) {
    profCleanup();
  }
#endif

#ifndef NO_DEBUGGER
  elfCleanUp();
#endif //NO_DEBUGGER

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;
}

int CPULoadRom(const char *szFile)
{
  romSize = 0x2000000;
  /*if(rom != NULL) {
    CPUCleanUp();
  }*/

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

  memset(workRAM, 0, sizeof(workRAM));

  u8 *whereToLoad = cpuIsMultiBoot ? workRAM : rom;

#ifndef NO_DEBUGGER
  if(CPUIsELF(szFile)) {
    FILE *f = fopen(szFile, "rb");
    if(!f) {
      systemMessage(MSG_ERROR_OPENING_IMAGE, N_("Error opening image %s"),
                    szFile);
      return 0;
    }
    bool res = elfRead(szFile, romSize, f);
    if(!res || romSize == 0) {
      elfCleanUp();
      return 0;
    }
  } else
#endif //NO_DEBUGGER
  if(szFile!=NULL)
  {
	  if(!utilLoad(szFile,
						  utilIsGBAImage,
						  whereToLoad,
						  romSize)) {
		return 0;
	  }
  }

  u16 *temp = (u16 *)(rom+((romSize+1)&~1));
  int i;
  for(i = (romSize+1)&~1; i < 0x2000000; i+=2) {
    WRITE16LE(temp, (i >> 1) & 0xFFFF);
    temp++;
  }

  memset(bios, 0, sizeof(bios));

  memset(internalRAM, 0, sizeof(internalRAM));

  memset(ioMem.b, 0, sizeof(ioMem));

  gLcd.reset();

  flashInit();
  eepromInit();

  CPUUpdateRenderBuffers(true);

  return romSize;
}

void doMirroring (bool b)
{
  u32 mirroredRomSize = (((romSize)>>20) & 0x3F)<<20;
  if ((mirroredRomSize <=0x800000) && (b))
  {
    u32 mirroredRomAddress = mirroredRomSize;
    if (mirroredRomSize==0)
        mirroredRomSize=0x100000;
    while (mirroredRomAddress<0x01000000)
    {
    	logMsg("copying rom with size %X to %X", mirroredRomSize, mirroredRomAddress);
      memcpy ((u16 *)(rom+mirroredRomAddress), (u16 *)(rom), mirroredRomSize);
      mirroredRomAddress+=mirroredRomSize;
    }
  }
}

static const char *dispModeName(void (*renderLine)(MixColorType *lineMix, const GBAMem::IoMem &ioMem))
{
	if(renderLine == mode0RenderLine) return "0";
	else if(renderLine == mode0RenderLineNoWindow) return "0NW";
	else if(renderLine == mode0RenderLineAll) return "0A";
	else if(renderLine == mode1RenderLine) return "1";
	else if(renderLine == mode1RenderLineNoWindow) return "1NW";
	else if(renderLine == mode1RenderLineAll) return "1A";
	else if(renderLine == mode2RenderLine) return "2";
	else if(renderLine == mode2RenderLineNoWindow) return "2NW";
	else if(renderLine == mode2RenderLineAll) return "2A";
	else if(renderLine == mode3RenderLine) return "3";
	else if(renderLine == mode3RenderLineNoWindow) return "3NW";
	else if(renderLine == mode3RenderLineAll) return "3A";
	else if(renderLine == mode4RenderLine) return "4";
	else if(renderLine == mode4RenderLineNoWindow) return "4NW";
	else if(renderLine == mode4RenderLineAll) return "4A";
	else if(renderLine == mode5RenderLine) return "5";
	else if(renderLine == mode5RenderLineNoWindow) return "5NW";
	else if(renderLine == mode5RenderLineAll) return "5A";
	else return "Invalid";
}

static void blankLine(MixColorType *lineMix, const GBAMem::IoMem &ioMem)
{
	for(int x = 0; x < 240; x++)
		lineMix[x] = convColor(0x7fff);
}

static void blankLineUpdateLastVCount(MixColorType *lineMix, const GBAMem::IoMem &ioMem)
{
	blankLine(lineMix, ioMem);
	gfxLastVCOUNT = ioMem.VCOUNT;
}

void CPUUpdateRender()
{
  switch(ioMem.DISPCNT & 7) {
  case 0:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode0RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode0RenderLineNoWindow;
    else
      renderLine = mode0RenderLineAll;
    break;
  case 1:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode1RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode1RenderLineNoWindow;
    else
      renderLine = mode1RenderLineAll;
    break;
  case 2:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode2RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode2RenderLineNoWindow;
    else
      renderLine = mode2RenderLineAll;
    break;
  case 3:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode3RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode3RenderLineNoWindow;
    else
      renderLine = mode3RenderLineAll;
    break;
  case 4:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode4RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode4RenderLineNoWindow;
    else
      renderLine = mode4RenderLineAll;
    break;
  case 5:
    if((!fxOn && !windowOn && !(layerEnable & 0x8000)) ||
       cpuDisableSfx)
      renderLine = mode5RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode5RenderLineNoWindow;
    else
      renderLine = mode5RenderLineAll;
  default:
    break;
  }

  if(ioMem.DISPCNT & 0x80)
  {
	  systemMessage(0, "Set forced blank");
	  renderLine = ((ioMem.DISPCNT & 7) == 0) ? blankLine : blankLineUpdateLastVCount;
  }
  else
  {
	  //systemMessage(0, "Set mode %s\n", dispModeName(renderLine));
  }
}

static void CPUUpdateCPSR()
{
	gGba.cpu.updateCPSR();
}

void CPUSoftwareInterrupt(ARM7TDMI &cpu, int comment)
{
	auto &reg = cpu.reg;
#ifdef VBAM_USE_SWITICKS
	auto &SWITicks = cpu.SWITicks;
#endif
	auto &holdState = cpu.holdState;
  static bool disableMessage = false;
  //if(cpu.armState) comment >>= 16;
#ifdef BKPT_SUPPORT
  if(comment == 0xff) {
    dbgOutput(NULL, reg[0].I);
    return;
  }
#endif
#ifdef PROFILING
  if(comment == 0xfe) {
    profStartup(reg[0].I, reg[1].I);
    return;
  }
  if(comment == 0xfd) {
    profControl(reg[0].I);
    return;
  }
  if(comment == 0xfc) {
    profCleanup();
    return;
  }
  if(comment == 0xfb) {
    profCount();
    return;
  }
#endif
#ifdef VBAM_USE_AGB_PRINT
  if(comment == 0xfa) {
    agbPrintFlush();
    return;
  }
#endif
#ifdef SDL
  if(comment == 0xf9) {
    //emulating = 0;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    cpuBreakLoop = true;
    return;
  }
#endif
  if(useBios) {
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
          cpu.armState ? armNextPC - 4: armNextPC -2,
          reg[0].I,
          reg[1].I,
          reg[2].I,
          VCOUNT);
    }
#endif
    cpu.softwareInterrupt();
    return;
  }
  // This would be correct, but it causes problems if uncommented
  //  else {
  //    biosProtected = 0xe3a02004;
  //  }

  switch(comment) {
  case 0x00:
    BIOS_SoftReset(cpu);
    cpu.ARM_PREFETCH();
    break;
  case 0x01:
    BIOS_RegisterRamReset(cpu);
    break;
  case 0x02:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("Halt: (VCOUNT = %2d)\n",
          VCOUNT);
    }
#endif
    holdState = true;
#ifdef VBAM_USE_HOLDTYPE
    holdType = -1;
#endif
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x03:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("Stop: (VCOUNT = %2d)\n",
          VCOUNT);
    }
#endif
    holdState = true;
#ifdef VBAM_USE_HOLDTYPE
    holdType = -1;
#endif
    gGba.stopState = true;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x04:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("IntrWait: 0x%08x,0x%08x (VCOUNT = %2d)\n",
          reg[0].I,
          reg[1].I,
          VCOUNT);
    }
#endif
    cpu.softwareInterrupt();
    break;
  case 0x05:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("VBlankIntrWait: (VCOUNT = %2d)\n",
          VCOUNT);
    }
#endif
    cpu.softwareInterrupt();
    break;
  case 0x06:
  	cpu.softwareInterrupt();
    break;
  case 0x07:
  	cpu.softwareInterrupt();
    break;
  case 0x08:
    BIOS_Sqrt(cpu);
    break;
  case 0x09:
    BIOS_ArcTan(cpu);
    break;
  case 0x0A:
    BIOS_ArcTan2(cpu);
    break;
  case 0x0B:
#ifdef VBAM_USE_SWITICKS
    {
      int len = (reg[2].I & 0x1FFFFF) >>1;
      if (!(((reg[0].I & 0xe000000) == 0) ||
         ((reg[0].I + len) & 0xe000000) == 0))
      {
        if ((reg[2].I >> 24) & 1)
        {
          if ((reg[2].I >> 26) & 1)
          SWITicks = (7 + memoryWait32[(reg[1].I>>24) & 0xF]) * (len>>1);
          else
          SWITicks = (8 + memoryWait[(reg[1].I>>24) & 0xF]) * (len);
        }
        else
        {
          if ((reg[2].I >> 26) & 1)
          SWITicks = (10 + memoryWait32[(reg[0].I>>24) & 0xF] +
              memoryWait32[(reg[1].I>>24) & 0xF]) * (len>>1);
          else
          SWITicks = (11 + memoryWait[(reg[0].I>>24) & 0xF] +
              memoryWait[(reg[1].I>>24) & 0xF]) * len;
        }
      }
    }
#endif
    BIOS_CpuSet(cpu);
    break;
  case 0x0C:
#ifdef VBAM_USE_SWITICKS
    {
      int len = (reg[2].I & 0x1FFFFF) >>5;
      if (!(((reg[0].I & 0xe000000) == 0) ||
         ((reg[0].I + len) & 0xe000000) == 0))
      {
        if ((reg[2].I >> 24) & 1)
          SWITicks = (6 + memoryWait32[(reg[1].I>>24) & 0xF] +
              7 * (memoryWaitSeq32[(reg[1].I>>24) & 0xF] + 1)) * len;
        else
          SWITicks = (9 + memoryWait32[(reg[0].I>>24) & 0xF] +
              memoryWait32[(reg[1].I>>24) & 0xF] +
              7 * (memoryWaitSeq32[(reg[0].I>>24) & 0xF] +
              memoryWaitSeq32[(reg[1].I>>24) & 0xF] + 2)) * len;
      }
    }
#endif
    BIOS_CpuFastSet(cpu);
    break;
  case 0x0D:
    BIOS_GetBiosChecksum(cpu);
    break;
  case 0x0E:
    BIOS_BgAffineSet(cpu);
    break;
  case 0x0F:
    BIOS_ObjAffineSet(cpu);
    break;
  case 0x10:
#ifdef VBAM_USE_SWITICKS
    {
      int len = CPUReadHalfWord(cpu, reg[2].I);
      if (!(((reg[0].I & 0xe000000) == 0) ||
         ((reg[0].I + len) & 0xe000000) == 0))
        SWITicks = (32 + memoryWait[(reg[0].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_BitUnPack(cpu);
    break;
  case 0x11:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 8;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (9 + memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_LZ77UnCompWram(cpu);
    break;
  case 0x12:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 8;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (19 + memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_LZ77UnCompVram(cpu);
    break;
  case 0x13:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 8;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (29 + (memoryWait[(reg[0].I>>24) & 0xF]<<1)) * len;
    }
#endif
    BIOS_HuffUnComp(cpu);
    break;
  case 0x14:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 8;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (11 + memoryWait[(reg[0].I>>24) & 0xF] +
          memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_RLUnCompWram(cpu);
    break;
  case 0x15:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 9;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (34 + (memoryWait[(reg[0].I>>24) & 0xF] << 1) +
          memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_RLUnCompVram(cpu);
    break;
  case 0x16:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 8;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (13 + memoryWait[(reg[0].I>>24) & 0xF] +
          memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_Diff8bitUnFilterWram(cpu);
    break;
  case 0x17:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 9;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (39 + (memoryWait[(reg[0].I>>24) & 0xF]<<1) +
          memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_Diff8bitUnFilterVram(cpu);
    break;
  case 0x18:
#ifdef VBAM_USE_SWITICKS
    {
      u32 len = CPUReadMemory(cpu, reg[0].I) >> 9;
      if(!(((reg[0].I & 0xe000000) == 0) ||
          ((reg[0].I + (len & 0x1fffff)) & 0xe000000) == 0))
        SWITicks = (13 + memoryWait[(reg[0].I>>24) & 0xF] +
          memoryWait[(reg[1].I>>24) & 0xF]) * len;
    }
#endif
    BIOS_Diff16bitUnFilter(cpu);
    break;
  case 0x19:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("SoundBiasSet: 0x%08x (VCOUNT = %2d)\n",
          reg[0].I,
          VCOUNT);
    }
#endif
    if(reg[0].I)
      soundPause();
    else
      soundResume();
    break;
  case 0x1F:
    BIOS_MidiKey2Freq(cpu);
    break;
  case 0x2A:
    BIOS_SndDriverJmpTableCopy(cpu);
    // let it go, because we don't really emulate this function
  default:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_SWI) {
      log("SWI: %08x at %08x (0x%08x,0x%08x,0x%08x,VCOUNT = %2d)\n", comment,
          cpu.armState ? armNextPC - 4: armNextPC -2,
          reg[0].I,
          reg[1].I,
          reg[2].I,
          VCOUNT);
    }
#endif

    if(!disableMessage) {
      systemMessage(MSG_UNSUPPORTED_BIOS_FUNCTION,
                    N_("Unsupported BIOS function %02x called from %08x. A BIOS file is needed in order to get correct behaviour."),
                    comment,
                    cpu.oldPC());
      disableMessage = true;
    }
    break;
  }
}

void CPUCompareVCOUNT(ARM7TDMI &cpu)
{
  if(ioMem.VCOUNT == (ioMem.DISPSTAT >> 8)) {
  	ioMem.DISPSTAT |= 4;
    //UPDATE_REG(0x04, DISPSTAT);

    if(ioMem.DISPSTAT & 0x20) {
    	ioMem.IF |= 4;
      //UPDATE_REG(0x202, IF);
    }
  } else {
  	ioMem.DISPSTAT &= 0xFFFB;
    //UPDATE_REG(0x4, DISPSTAT);
  }
  if (layerEnableDelay>0)
  {
      layerEnableDelay--;
      if (layerEnableDelay==1)
          layerEnable = layerSettings & ioMem.DISPCNT;
  }

}

void doDMA(ARM7TDMI &cpu, u32 &s, u32 &d, u32 si, u32 di, u32 c, int transfer32)
{
	auto &reg = cpu.reg;
  int sm = s >> 24;
  int dm = d >> 24;
  int sw = 0;
  int dw = 0;
  int sc = c;

  gGba.dma.cpuDmaCount = c;
  // This is done to get the correct waitstates.
  if (sm>15)
      sm=15;
  if (dm>15)
      dm=15;

  //if ((sm>=0x05) && (sm<=0x07) || (dm>=0x05) && (dm <=0x07))
  //    blank = (((DISPSTAT | ((DISPSTAT>>1)&1))==1) ?  true : false);

  if(transfer32) {
    s &= 0xFFFFFFFC;
    if(s < 0x02000000 && (reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteMemory(cpu, d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
      	gGba.dma.cpuDmaLast = CPUReadMemory(cpu, s);
        CPUWriteMemory(cpu, d, gGba.dma.cpuDmaLast);
        d += di;
        s += si;
        c--;
      }
    }
  } else {
    s &= 0xFFFFFFFE;
    si = (int)si >> 1;
    di = (int)di >> 1;
    if(s < 0x02000000 && (reg[15].I >> 24)) {
      while(c != 0) {
        CPUWriteHalfWord(cpu, d, 0);
        d += di;
        c--;
      }
    } else {
      while(c != 0) {
      	gGba.dma.cpuDmaLast = CPUReadHalfWord(cpu, s);
        CPUWriteHalfWord(cpu, d, gGba.dma.cpuDmaLast);
        gGba.dma.cpuDmaLast |= (gGba.dma.cpuDmaLast<<16);
        d += di;
        s += si;
        c--;
      }
    }
  }

  gGba.dma.cpuDmaCount = 0;

  int totalTicks = 0;

  if(transfer32) {
      sw =1+memoryWaitSeq32[sm & 15];
      dw =1+memoryWaitSeq32[dm & 15];
      totalTicks = (sw+dw)*(sc-1) + 6 + memoryWait32[sm & 15] +
          memoryWaitSeq32[dm & 15];
  }
  else
  {
     sw = 1+memoryWaitSeq[sm & 15];
     dw = 1+memoryWaitSeq[dm & 15];
      totalTicks = (sw+dw)*(sc-1) + 6 + memoryWait[sm & 15] +
          memoryWaitSeq[dm & 15];
  }

  gGba.dma.cpuDmaTicksToUpdate += totalTicks;

}

void CPUCheckDMA(ARM7TDMI &cpu, int reason, int dmamask)
{
	auto &IF = ioMem.IF;
	auto &dma0Source = gGba.dma.dma0Source;
	auto &dma0Dest = gGba.dma.dma0Dest;
	auto &dma1Source = gGba.dma.dma1Source;
	auto &dma1Dest = gGba.dma.dma1Dest;
	auto &dma2Source = gGba.dma.dma2Source;
	auto &dma2Dest = gGba.dma.dma2Dest;
	auto &dma3Source = gGba.dma.dma3Source;
	auto &dma3Dest = gGba.dma.dma3Dest;

  // DMA 0
  if((ioMem.DM0CNT_H & 0x8000) && (dmamask & 1)) {
    if(((ioMem.DM0CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((ioMem.DM0CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((ioMem.DM0CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
#ifdef GBA_LOGGING
      if(systemVerbose & VERBOSE_DMA0) {
        int count = (DM0CNT_L ? DM0CNT_L : 0x4000) << 1;
        if(DM0CNT_H & 0x0400)
          count <<= 1;
        log("DMA0: s=%08x d=%08x c=%04x count=%08x\n", dma0Source, dma0Dest,
            DM0CNT_H,
            count);
      }
#endif
      doDMA(cpu, dma0Source, dma0Dest, sourceIncrement, destIncrement,
      		ioMem.DM0CNT_L ? ioMem.DM0CNT_L : 0x4000,
            		ioMem.DM0CNT_H & 0x0400);
      gGba.dma.cpuDmaHack = true;

      if(ioMem.DM0CNT_H & 0x4000) {
        IF |= 0x0100;
        //UPDATE_REG(0x202, IF);
        cpu.cpuNextEvent = cpu.cpuTotalTicks;
      }

      if(((ioMem.DM0CNT_H >> 5) & 3) == 3) {
        dma0Dest = ioMem.DM0DAD_L | (ioMem.DM0DAD_H << 16);
      }

      if(!(ioMem.DM0CNT_H & 0x0200) || (reason == 0)) {
      	ioMem.DM0CNT_H &= 0x7FFF;
        //UPDATE_REG(0xBA, DM0CNT_H);
      }
    }
  }

  // DMA 1
  if((ioMem.DM1CNT_H & 0x8000) && (dmamask & 2)) {
    if(((ioMem.DM1CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((ioMem.DM1CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((ioMem.DM1CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      if(reason == 3) {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_DMA1) {
          log("DMA1: s=%08x d=%08x c=%04x count=%08x\n", dma1Source, dma1Dest,
              DM1CNT_H,
              16);
        }
#endif
        doDMA(cpu, dma1Source, dma1Dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_DMA1) {
          int count = (DM1CNT_L ? DM1CNT_L : 0x4000) << 1;
          if(DM1CNT_H & 0x0400)
            count <<= 1;
          log("DMA1: s=%08x d=%08x c=%04x count=%08x\n", dma1Source, dma1Dest,
              DM1CNT_H,
              count);
        }
#endif
        doDMA(cpu, dma1Source, dma1Dest, sourceIncrement, destIncrement,
        		ioMem.DM1CNT_L ? ioMem.DM1CNT_L : 0x4000,
        				ioMem.DM1CNT_H & 0x0400);
      }
      gGba.dma.cpuDmaHack = true;

      if(ioMem.DM1CNT_H & 0x4000) {
        IF |= 0x0200;
        //UPDATE_REG(0x202, IF);
        cpu.cpuNextEvent = cpu.cpuTotalTicks;
      }

      if(((ioMem.DM1CNT_H >> 5) & 3) == 3) {
        dma1Dest = ioMem.DM1DAD_L | (ioMem.DM1DAD_H << 16);
      }

      if(!(ioMem.DM1CNT_H & 0x0200) || (reason == 0)) {
      	ioMem.DM1CNT_H &= 0x7FFF;
        //UPDATE_REG(0xC6, DM1CNT_H);
      }
    }
  }

  // DMA 2
  if((ioMem.DM2CNT_H & 0x8000) && (dmamask & 4)) {
    if(((ioMem.DM2CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((ioMem.DM2CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((ioMem.DM2CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
      if(reason == 3) {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_DMA2) {
          int count = (4) << 2;
          log("DMA2: s=%08x d=%08x c=%04x count=%08x\n", dma2Source, dma2Dest,
              DM2CNT_H,
              count);
        }
#endif
        doDMA(cpu, dma2Source, dma2Dest, sourceIncrement, 0, 4,
              0x0400);
      } else {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_DMA2) {
          int count = (DM2CNT_L ? DM2CNT_L : 0x4000) << 1;
          if(DM2CNT_H & 0x0400)
            count <<= 1;
          log("DMA2: s=%08x d=%08x c=%04x count=%08x\n", dma2Source, dma2Dest,
              DM2CNT_H,
              count);
        }
#endif
        doDMA(cpu, dma2Source, dma2Dest, sourceIncrement, destIncrement,
        		ioMem.DM2CNT_L ? ioMem.DM2CNT_L : 0x4000,
        				ioMem.DM2CNT_H & 0x0400);
      }
      gGba.dma.cpuDmaHack = true;

      if(ioMem.DM2CNT_H & 0x4000) {
        IF |= 0x0400;
        //UPDATE_REG(0x202, IF);
        cpu.cpuNextEvent = cpu.cpuTotalTicks;
      }

      if(((ioMem.DM2CNT_H >> 5) & 3) == 3) {
        dma2Dest = ioMem.DM2DAD_L | (ioMem.DM2DAD_H << 16);
      }

      if(!(ioMem.DM2CNT_H & 0x0200) || (reason == 0)) {
      	ioMem.DM2CNT_H &= 0x7FFF;
        //UPDATE_REG(0xD2, DM2CNT_H);
      }
    }
  }

  // DMA 3
  if((ioMem.DM3CNT_H & 0x8000) && (dmamask & 8)) {
    if(((ioMem.DM3CNT_H >> 12) & 3) == reason) {
      u32 sourceIncrement = 4;
      u32 destIncrement = 4;
      switch((ioMem.DM3CNT_H >> 7) & 3) {
      case 0:
        break;
      case 1:
        sourceIncrement = (u32)-4;
        break;
      case 2:
        sourceIncrement = 0;
        break;
      }
      switch((ioMem.DM3CNT_H >> 5) & 3) {
      case 0:
        break;
      case 1:
        destIncrement = (u32)-4;
        break;
      case 2:
        destIncrement = 0;
        break;
      }
#ifdef GBA_LOGGING
      if(systemVerbose & VERBOSE_DMA3) {
        int count = (DM3CNT_L ? DM3CNT_L : 0x10000) << 1;
        if(DM3CNT_H & 0x0400)
          count <<= 1;
        log("DMA3: s=%08x d=%08x c=%04x count=%08x\n", dma3Source, dma3Dest,
            DM3CNT_H,
            count);
      }
#endif
      doDMA(cpu, dma3Source, dma3Dest, sourceIncrement, destIncrement,
      		ioMem.DM3CNT_L ? ioMem.DM3CNT_L : 0x10000,
      				ioMem.DM3CNT_H & 0x0400);
      if(ioMem.DM3CNT_H & 0x4000) {
        IF |= 0x0800;
        //UPDATE_REG(0x202, IF);
        cpu.cpuNextEvent = cpu.cpuTotalTicks;
      }

      if(((ioMem.DM3CNT_H >> 5) & 3) == 3) {
        dma3Dest = ioMem.DM3DAD_L | (ioMem.DM3DAD_H << 16);
      }

      if(!(ioMem.DM3CNT_H & 0x0200) || (reason == 0)) {
      	ioMem.DM3CNT_H &= 0x7FFF;
        //UPDATE_REG(0xDE, DM3CNT_H);
      }
    }
  }
}

void CPUUpdateRegister(ARM7TDMI &cpu, u32 address, u16 value)
{
	auto &armIrqEnable = cpu.armIrqEnable;
	auto &IE = ioMem.IE;
	auto &IF = ioMem.IF;
	auto &IME = ioMem.IME;
	auto &busPrefetchCount = cpu.busPrefetchCount;
	auto &busPrefetch = cpu.busPrefetch;
	auto &busPrefetchEnable = cpu.busPrefetchEnable;
	auto &dma0Source = gGba.dma.dma0Source;
	auto &dma0Dest = gGba.dma.dma0Dest;
	auto &dma1Source = gGba.dma.dma1Source;
	auto &dma1Dest = gGba.dma.dma1Dest;
	auto &dma2Source = gGba.dma.dma2Source;
	auto &dma2Dest = gGba.dma.dma2Dest;
	auto &dma3Source = gGba.dma.dma3Source;
	auto &dma3Dest = gGba.dma.dma3Dest;
	auto &timerOnOffDelay = gGba.timers.timerOnOffDelay;
	auto &timer0Value = gGba.timers.timer0Value;
	auto &timer0On = gGba.timers.timer0On;
	auto &timer0Ticks = gGba.timers.timer0Ticks;
	auto &timer0ClockReload  = gGba.timers.timer0ClockReload;
	auto &timer0Reload  = gGba.timers.timer0Reload;
	auto &timer1Value = gGba.timers.timer1Value;
	auto &timer1On = gGba.timers.timer1On;
	auto &timer1Ticks = gGba.timers.timer1Ticks;
	auto &timer1ClockReload  = gGba.timers.timer1ClockReload;
	auto &timer1Reload  = gGba.timers.timer1Reload;
	auto &timer2Value = gGba.timers.timer2Value;
	auto &timer2On = gGba.timers.timer2On;
	auto &timer2Ticks = gGba.timers.timer2Ticks;
	auto &timer2ClockReload  = gGba.timers.timer2ClockReload;
	auto &timer2Reload  = gGba.timers.timer2Reload;
	auto &timer3Value = gGba.timers.timer3Value;
	auto &timer3On = gGba.timers.timer3On;
	auto &timer3Ticks = gGba.timers.timer3Ticks;
	auto &timer3ClockReload  = gGba.timers.timer3ClockReload;
	auto &timer3Reload  = gGba.timers.timer3Reload;

  switch(address)
  {
  case 0x00:
    { // we need to place the following code in { } because we declare & initialize variables in a case statement
      if((value & 7) > 5) {
        // display modes above 0-5 are prohibited
      	ioMem.DISPCNT = (value & 7);
      }
      bool change = (0 != ((ioMem.DISPCNT ^ value) & 0x80));
      bool changeBG = (0 != ((ioMem.DISPCNT ^ value) & 0x0F00));
      u16 changeBGon = ((~ioMem.DISPCNT) & value) & 0x0F00; // these layers are being activated

      ioMem.DISPCNT = (value & 0xFFF7); // bit 3 can only be accessed by the BIOS to enable GBC mode
      //UPDATE_REG(0x00, DISPCNT);

      if(changeBGon) {
        layerEnableDelay = 4;
        layerEnable = layerSettings & value & (~changeBGon);
      } else {
        layerEnable = layerSettings & value;
        // CPUUpdateTicks();
      }

      windowOn = (layerEnable & 0x6000) ? true : false;
      if(change && !((value & 0x80))) {
        if(!(ioMem.DISPSTAT & 1)) {
          lcdTicks = 1008;
          //      VCOUNT = 0;
          //      UPDATE_REG(0x06, VCOUNT);
          ioMem.DISPSTAT &= 0xFFFC;
          //UPDATE_REG(0x04, DISPSTAT);
          CPUCompareVCOUNT(cpu);
        }
        //        (*renderLine)();
      }
      CPUUpdateRender();
      // we only care about changes in BG0-BG3
      if(changeBG) {
      	logMsg("changed bg mode: %d", ioMem.DISPCNT & 7);
        CPUUpdateRenderBuffers(false);
      }
      break;
    }
  case 0x04:
  	ioMem.DISPSTAT = (value & 0xFF38) | (ioMem.DISPSTAT & 7);
    //UPDATE_REG(0x04, DISPSTAT);
    break;
  case 0x06:
    // not writable
    break;
  case 0x08:
  	ioMem.BG0CNT = (value & 0xDFCF);
    //UPDATE_REG(0x08, BG0CNT);
    break;
  case 0x0A:
  	ioMem.BG1CNT = (value & 0xDFCF);
    //UPDATE_REG(0x0A, BG1CNT);
    break;
  case 0x0C:
  	ioMem.BG2CNT = (value & 0xFFCF);
    //UPDATE_REG(0x0C, BG2CNT);
    break;
  case 0x0E:
  	ioMem.BG3CNT = (value & 0xFFCF);
    //UPDATE_REG(0x0E, BG3CNT);
    break;
  case 0x10:
  	ioMem.BG0HOFS = value & 511;
    //UPDATE_REG(0x10, BG0HOFS);
    break;
  case 0x12:
  	ioMem.BG0VOFS = value & 511;
    //UPDATE_REG(0x12, BG0VOFS);
    break;
  case 0x14:
  	ioMem.BG1HOFS = value & 511;
    //UPDATE_REG(0x14, BG1HOFS);
    break;
  case 0x16:
  	ioMem.BG1VOFS = value & 511;
    //UPDATE_REG(0x16, BG1VOFS);
    break;
  case 0x18:
  	ioMem.BG2HOFS = value & 511;
    //UPDATE_REG(0x18, BG2HOFS);
    break;
  case 0x1A:
  	ioMem.BG2VOFS = value & 511;
    //UPDATE_REG(0x1A, BG2VOFS);
    break;
  case 0x1C:
  	ioMem.BG3HOFS = value & 511;
    //UPDATE_REG(0x1C, BG3HOFS);
    break;
  case 0x1E:
  	ioMem.BG3VOFS = value & 511;
    //UPDATE_REG(0x1E, BG3VOFS);
    break;
  case 0x20:
  	ioMem.BG2PA = value;
    //UPDATE_REG(0x20, BG2PA);
    break;
  case 0x22:
  	ioMem.BG2PB = value;
    //UPDATE_REG(0x22, BG2PB);
    break;
  case 0x24:
  	ioMem.BG2PC = value;
    //UPDATE_REG(0x24, BG2PC);
    break;
  case 0x26:
  	ioMem.BG2PD = value;
    //UPDATE_REG(0x26, BG2PD);
    break;
  case 0x28:
  	ioMem.BG2X_L = value;
    //UPDATE_REG(0x28, BG2X_L);
    gfxBG2Changed |= 1;
    break;
  case 0x2A:
  	ioMem.BG2X_H = (value & 0xFFF);
    //UPDATE_REG(0x2A, BG2X_H);
    gfxBG2Changed |= 1;
    break;
  case 0x2C:
  	ioMem.BG2Y_L = value;
    //UPDATE_REG(0x2C, BG2Y_L);
    gfxBG2Changed |= 2;
    break;
  case 0x2E:
  	ioMem.BG2Y_H = value & 0xFFF;
    //UPDATE_REG(0x2E, BG2Y_H);
    gfxBG2Changed |= 2;
    break;
  case 0x30:
  	ioMem.BG3PA = value;
    //UPDATE_REG(0x30, BG3PA);
    break;
  case 0x32:
  	ioMem.BG3PB = value;
    //UPDATE_REG(0x32, BG3PB);
    break;
  case 0x34:
  	ioMem.BG3PC = value;
    //UPDATE_REG(0x34, BG3PC);
    break;
  case 0x36:
  	ioMem.BG3PD = value;
    //UPDATE_REG(0x36, BG3PD);
    break;
  case 0x38:
  	ioMem.BG3X_L = value;
    //UPDATE_REG(0x38, BG3X_L);
    gfxBG3Changed |= 1;
    break;
  case 0x3A:
  	ioMem.BG3X_H = value & 0xFFF;
    //UPDATE_REG(0x3A, BG3X_H);
    gfxBG3Changed |= 1;
    break;
  case 0x3C:
  	ioMem.BG3Y_L = value;
    //UPDATE_REG(0x3C, BG3Y_L);
    gfxBG3Changed |= 2;
    break;
  case 0x3E:
  	ioMem.BG3Y_H = value & 0xFFF;
    //UPDATE_REG(0x3E, BG3Y_H);
    gfxBG3Changed |= 2;
    break;
  case 0x40:
  	ioMem.WIN0H = value;
    //UPDATE_REG(0x40, WIN0H);
    CPUUpdateWindow0();
    break;
  case 0x42:
  	ioMem.WIN1H = value;
    //UPDATE_REG(0x42, WIN1H);
    CPUUpdateWindow1();
    break;
  case 0x44:
  	ioMem.WIN0V = value;
    //UPDATE_REG(0x44, WIN0V);
    break;
  case 0x46:
  	ioMem.WIN1V = value;
    //UPDATE_REG(0x46, WIN1V);
    break;
  case 0x48:
  	ioMem.WININ = value & 0x3F3F;
    //UPDATE_REG(0x48, WININ);
    break;
  case 0x4A:
  	ioMem.WINOUT = value & 0x3F3F;
    //UPDATE_REG(0x4A, WINOUT);
    break;
  case 0x4C:
  	ioMem.MOSAIC = value;
    //UPDATE_REG(0x4C, MOSAIC);
    break;
  case 0x50:
  	ioMem.BLDMOD = value & 0x3FFF;
    //UPDATE_REG(0x50, BLDMOD);
    fxOn = ((ioMem.BLDMOD>>6)&3) != 0;
    CPUUpdateRender();
    break;
  case 0x52:
  	ioMem.COLEV = value & 0x1F1F;
    //UPDATE_REG(0x52, COLEV);
    break;
  case 0x54:
  	ioMem.COLY = value & 0x1F;
    //UPDATE_REG(0x54, COLY);
    break;
  case 0x60:
  case 0x62:
  case 0x64:
  case 0x68:
  case 0x6c:
  case 0x70:
  case 0x72:
  case 0x74:
  case 0x78:
  case 0x7c:
  case 0x80:
  case 0x84:
    soundEvent(address&0xFF, (u8)(value & 0xFF));
    soundEvent((address&0xFF)+1, (u8)(value>>8));
    break;
  case 0x82:
  case 0x88:
  case 0xa0:
  case 0xa2:
  case 0xa4:
  case 0xa6:
  case 0x90:
  case 0x92:
  case 0x94:
  case 0x96:
  case 0x98:
  case 0x9a:
  case 0x9c:
  case 0x9e:
    soundEvent(address&0xFF, value);
    break;
  case 0xB0:
  	ioMem.DM0SAD_L = value;
    //UPDATE_REG(0xB0, DM0SAD_L);
    break;
  case 0xB2:
  	ioMem.DM0SAD_H = value & 0x07FF;
    //UPDATE_REG(0xB2, DM0SAD_H);
    break;
  case 0xB4:
  	ioMem.DM0DAD_L = value;
    //UPDATE_REG(0xB4, DM0DAD_L);
    break;
  case 0xB6:
  	ioMem.DM0DAD_H = value & 0x07FF;
    //UPDATE_REG(0xB6, DM0DAD_H);
    break;
  case 0xB8:
  	ioMem.DM0CNT_L = value & 0x3FFF;
    //UPDATE_REG(0xB8, 0);
    break;
  case 0xBA:
    {
      bool start = ((ioMem.DM0CNT_H ^ value) & 0x8000) ? true : false;
      value &= 0xF7E0;

      ioMem.DM0CNT_H = value;
      //UPDATE_REG(0xBA, DM0CNT_H);

      if(start && (value & 0x8000)) {
        dma0Source = ioMem.DM0SAD_L | (ioMem.DM0SAD_H << 16);
        dma0Dest = ioMem.DM0DAD_L | (ioMem.DM0DAD_H << 16);
        CPUCheckDMA(cpu,0, 1);
      }
    }
    break;
  case 0xBC:
  	ioMem.DM1SAD_L = value;
    //UPDATE_REG(0xBC, DM1SAD_L);
    break;
  case 0xBE:
  	ioMem.DM1SAD_H = value & 0x0FFF;
    //UPDATE_REG(0xBE, DM1SAD_H);
    break;
  case 0xC0:
  	ioMem.DM1DAD_L = value;
    //UPDATE_REG(0xC0, DM1DAD_L);
    break;
  case 0xC2:
  	ioMem.DM1DAD_H = value & 0x07FF;
    //UPDATE_REG(0xC2, DM1DAD_H);
    break;
  case 0xC4:
  	ioMem.DM1CNT_L = value & 0x3FFF;
    //UPDATE_REG(0xC4, 0);
    break;
  case 0xC6:
    {
      bool start = ((ioMem.DM1CNT_H ^ value) & 0x8000) ? true : false;
      value &= 0xF7E0;

      ioMem.DM1CNT_H = value;
      //UPDATE_REG(0xC6, DM1CNT_H);

      if(start && (value & 0x8000)) {
        dma1Source = ioMem.DM1SAD_L | (ioMem.DM1SAD_H << 16);
        dma1Dest = ioMem.DM1DAD_L | (ioMem.DM1DAD_H << 16);
        CPUCheckDMA(cpu,0, 2);
      }
    }
    break;
  case 0xC8:
  	ioMem.DM2SAD_L = value;
    //UPDATE_REG(0xC8, DM2SAD_L);
    break;
  case 0xCA:
  	ioMem.DM2SAD_H = value & 0x0FFF;
    //UPDATE_REG(0xCA, DM2SAD_H);
    break;
  case 0xCC:
  	ioMem.DM2DAD_L = value;
    //UPDATE_REG(0xCC, DM2DAD_L);
    break;
  case 0xCE:
  	ioMem.DM2DAD_H = value & 0x07FF;
    //UPDATE_REG(0xCE, DM2DAD_H);
    break;
  case 0xD0:
  	ioMem.DM2CNT_L = value & 0x3FFF;
    //UPDATE_REG(0xD0, 0);
    break;
  case 0xD2:
    {
      bool start = ((ioMem.DM2CNT_H ^ value) & 0x8000) ? true : false;

      value &= 0xF7E0;

      ioMem.DM2CNT_H = value;
      //UPDATE_REG(0xD2, DM2CNT_H);

      if(start && (value & 0x8000)) {
        dma2Source = ioMem.DM2SAD_L | (ioMem.DM2SAD_H << 16);
        dma2Dest = ioMem.DM2DAD_L | (ioMem.DM2DAD_H << 16);

        CPUCheckDMA(cpu,0, 4);
      }
    }
    break;
  case 0xD4:
  	ioMem.DM3SAD_L = value;
    //UPDATE_REG(0xD4, DM3SAD_L);
    break;
  case 0xD6:
  	ioMem.DM3SAD_H = value & 0x0FFF;
    //UPDATE_REG(0xD6, DM3SAD_H);
    break;
  case 0xD8:
  	ioMem.DM3DAD_L = value;
    //UPDATE_REG(0xD8, DM3DAD_L);
    break;
  case 0xDA:
  	ioMem.DM3DAD_H = value & 0x0FFF;
    //UPDATE_REG(0xDA, DM3DAD_H);
    break;
  case 0xDC:
  	ioMem.DM3CNT_L = value;
    //UPDATE_REG(0xDC, 0);
    break;
  case 0xDE:
    {
      bool start = ((ioMem.DM3CNT_H ^ value) & 0x8000) ? true : false;

      value &= 0xFFE0;

      ioMem.DM3CNT_H = value;
      //UPDATE_REG(0xDE, DM3CNT_H);

      if(start && (value & 0x8000)) {
        dma3Source = ioMem.DM3SAD_L | (ioMem.DM3SAD_H << 16);
        dma3Dest = ioMem.DM3DAD_L | (ioMem.DM3DAD_H << 16);
        CPUCheckDMA(cpu,0,8);
      }
    }
    break;
  case 0x100:
    timer0Reload = value;
    interp_rate();
    break;
  case 0x102:
    timer0Value = value;
    timerOnOffDelay|=1;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x104:
    timer1Reload = value;
    interp_rate();
    break;
  case 0x106:
    timer1Value = value;
    timerOnOffDelay|=2;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x108:
    timer2Reload = value;
    break;
  case 0x10A:
    timer2Value = value;
    timerOnOffDelay|=4;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x10C:
    timer3Reload = value;
    break;
  case 0x10E:
    timer3Value = value;
    timerOnOffDelay|=8;
    cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;


  case COMM_SIOCNT:
	  StartLink(value);
	  break;

  case COMM_SIODATA8:
	  if (gba_link_enabled)
	  {
		  LinkSSend(value);
	  }
	  UPDATE_REG(COMM_SIODATA8, value);
	  break;

  case 0x130:
	  P1 |= (value & 0x3FF);
	  //UPDATE_REG(0x130, P1);
	  break;

  case 0x132:
	  UPDATE_REG(0x132, value & 0xC3FF);
	  break;

  case COMM_RCNT:
	  StartGPLink(value);
	  break;

  case COMM_JOYCNT:
	  {
		  u16 cur = READ16LE(&ioMem.b[COMM_JOYCNT]);

		  if (value & JOYCNT_RESET)			cur &= ~JOYCNT_RESET;
		  if (value & JOYCNT_RECV_COMPLETE)	cur &= ~JOYCNT_RECV_COMPLETE;
		  if (value & JOYCNT_SEND_COMPLETE)	cur &= ~JOYCNT_SEND_COMPLETE;
		  if (value & JOYCNT_INT_ENABLE)	cur |= JOYCNT_INT_ENABLE;

		  UPDATE_REG(COMM_JOYCNT, cur);
	  }
	  break;

  case COMM_JOY_RECV_L:
	  UPDATE_REG(COMM_JOY_RECV_L, value);
	  break;
  case COMM_JOY_RECV_H:
	  UPDATE_REG(COMM_JOY_RECV_H, value);	  
	  break;

  case COMM_JOY_TRANS_L:
	  UPDATE_REG(COMM_JOY_TRANS_L, value);
	  UPDATE_REG(COMM_JOYSTAT, READ16LE(&ioMem.b[COMM_JOYSTAT]) | JOYSTAT_SEND);
	  break;
  case COMM_JOY_TRANS_H:
	  UPDATE_REG(COMM_JOY_TRANS_H, value);
	  break;

  case COMM_JOYSTAT:
	  UPDATE_REG(COMM_JOYSTAT, (READ16LE(&ioMem.b[COMM_JOYSTAT]) & 0xf) | (value & 0xf0));
	  break;

  case 0x200:
    IE = value & 0x3FFF;
    //UPDATE_REG(0x200, IE);
    if ((IME & 1) && (IF & IE) && armIrqEnable)
      cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x202:
    IF ^= (value & IF);
    //UPDATE_REG(0x202, IF);
    break;
  case 0x204:
    {
      memoryWait[0x0e] = memoryWaitSeq[0x0e] = gamepakRamWaitState[value & 3];

      if(!speedHack) {
        memoryWait[0x08] = memoryWait[0x09] = gamepakWaitState[(value >> 2) & 3];
        memoryWaitSeq[0x08] = memoryWaitSeq[0x09] =
          gamepakWaitState0[(value >> 4) & 1];

        memoryWait[0x0a] = memoryWait[0x0b] = gamepakWaitState[(value >> 5) & 3];
        memoryWaitSeq[0x0a] = memoryWaitSeq[0x0b] =
          gamepakWaitState1[(value >> 7) & 1];

        memoryWait[0x0c] = memoryWait[0x0d] = gamepakWaitState[(value >> 8) & 3];
        memoryWaitSeq[0x0c] = memoryWaitSeq[0x0d] =
          gamepakWaitState2[(value >> 10) & 1];
      } else {
        memoryWait[0x08] = memoryWait[0x09] = 3;
        memoryWaitSeq[0x08] = memoryWaitSeq[0x09] = 1;

        memoryWait[0x0a] = memoryWait[0x0b] = 3;
        memoryWaitSeq[0x0a] = memoryWaitSeq[0x0b] = 1;

        memoryWait[0x0c] = memoryWait[0x0d] = 3;
        memoryWaitSeq[0x0c] = memoryWaitSeq[0x0d] = 1;
      }

      for(int i = 8; i < 15; i++) {
        memoryWait32[i] = memoryWait[i] + memoryWaitSeq[i] + 1;
        memoryWaitSeq32[i] = memoryWaitSeq[i]*2 + 1;
      }

      if((value & 0x4000) == 0x4000) {
        busPrefetchEnable = true;
        busPrefetch = false;
        busPrefetchCount = 0;
      } else {
        busPrefetchEnable = false;
        busPrefetch = false;
        busPrefetchCount = 0;
      }
      UPDATE_REG(0x204, value & 0x7FFF);

    }
    break;
  case 0x208:
    IME = value & 1;
    //UPDATE_REG(0x208, IME);
    if ((IME & 1) && (IF & IE) && armIrqEnable)
      cpu.cpuNextEvent = cpu.cpuTotalTicks;
    break;
  case 0x300:
    if(value != 0)
      value &= 0xFFFE;
    UPDATE_REG(0x300, value);
    break;
  default:
    UPDATE_REG(address&0x3FE, value);
    break;
  }
}

void applyTimer(ARM7TDMI &cpu)
{
	auto &timerOnOffDelay = gGba.timers.timerOnOffDelay;
	auto &timer0Value = gGba.timers.timer0Value;
	auto &timer0On = gGba.timers.timer0On;
	auto &timer0Ticks = gGba.timers.timer0Ticks;
	auto &timer0ClockReload  = gGba.timers.timer0ClockReload;
	auto &timer0Reload  = gGba.timers.timer0Reload;
	auto &timer1Value = gGba.timers.timer1Value;
	auto &timer1On = gGba.timers.timer1On;
	auto &timer1Ticks = gGba.timers.timer1Ticks;
	auto &timer1ClockReload  = gGba.timers.timer1ClockReload;
	auto &timer1Reload  = gGba.timers.timer1Reload;
	auto &timer2Value = gGba.timers.timer2Value;
	auto &timer2On = gGba.timers.timer2On;
	auto &timer2Ticks = gGba.timers.timer2Ticks;
	auto &timer2ClockReload  = gGba.timers.timer2ClockReload;
	auto &timer2Reload  = gGba.timers.timer2Reload;
	auto &timer3Value = gGba.timers.timer3Value;
	auto &timer3On = gGba.timers.timer3On;
	auto &timer3Ticks = gGba.timers.timer3Ticks;
	auto &timer3ClockReload  = gGba.timers.timer3ClockReload;
	auto &timer3Reload  = gGba.timers.timer3Reload;

  if (timerOnOffDelay & 1)
  {
    timer0ClockReload = TIMER_TICKS[timer0Value & 3];
    if(!timer0On && (timer0Value & 0x80)) {
      // reload the counter
    	ioMem.TM0D = timer0Reload;
      timer0Ticks = (0x10000 - ioMem.TM0D) << timer0ClockReload;
      //UPDATE_REG(0x100, TM0D);
    }
    timer0On = timer0Value & 0x80 ? true : false;
    ioMem.TM0CNT = timer0Value & 0xC7;
    interp_rate();
    //UPDATE_REG(0x102, TM0CNT);
    //    CPUUpdateTicks();
  }
  if (timerOnOffDelay & 2)
  {
    timer1ClockReload = TIMER_TICKS[timer1Value & 3];
    if(!timer1On && (timer1Value & 0x80)) {
      // reload the counter
    	ioMem.TM1D = timer1Reload;
      timer1Ticks = (0x10000 - ioMem.TM1D) << timer1ClockReload;
      //UPDATE_REG(0x104, TM1D);
    }
    timer1On = timer1Value & 0x80 ? true : false;
    ioMem.TM1CNT = timer1Value & 0xC7;
    interp_rate();
    //UPDATE_REG(0x106, TM1CNT);
  }
  if (timerOnOffDelay & 4)
  {
    timer2ClockReload = TIMER_TICKS[timer2Value & 3];
    if(!timer2On && (timer2Value & 0x80)) {
      // reload the counter
    	ioMem.TM2D = timer2Reload;
      timer2Ticks = (0x10000 - ioMem.TM2D) << timer2ClockReload;
      //UPDATE_REG(0x108, TM2D);
    }
    timer2On = timer2Value & 0x80 ? true : false;
    ioMem.TM2CNT = timer2Value & 0xC7;
    //UPDATE_REG(0x10A, TM2CNT);
  }
  if (timerOnOffDelay & 8)
  {
    timer3ClockReload = TIMER_TICKS[timer3Value & 3];
    if(!timer3On && (timer3Value & 0x80)) {
      // reload the counter
    	ioMem.TM3D = timer3Reload;
      timer3Ticks = (0x10000 - ioMem.TM3D) << timer3ClockReload;
      //UPDATE_REG(0x10C, TM3D);
    }
    timer3On = timer3Value & 0x80 ? true : false;
    ioMem.TM3CNT = timer3Value & 0xC7;
    //UPDATE_REG(0x10E, TM3CNT);
  }
  cpu.cpuNextEvent = CPUUpdateTicks(cpu);
  timerOnOffDelay = 0;
}

void CPUInit(const char *biosFileName, bool useBiosFile)
{
#ifdef WORDS_BIGENDIAN
  if(!cpuBiosSwapped) {
    for(unsigned int i = 0; i < sizeof(myROM)/4; i++) {
      WRITE32LE(&myROM[i], myROM[i]);
    }
    cpuBiosSwapped = true;
  }
#endif
  gbaSaveType = 0;
  eepromInUse = 0;
  saveType = 0;
  useBios = false;

  if(useBiosFile) {
    int size = 0x4000;
    if(utilLoad(biosFileName,
                CPUIsGBABios,
                bios,
                size)) {
      if(size == 0x4000)
        useBios = true;
      else
        systemMessage(MSG_INVALID_BIOS_FILE_SIZE, N_("Invalid BIOS file size"));
    }
  }

  if(!useBios) {
    memcpy(bios, myROM, sizeof(myROM));
  }

  int i = 0;

  biosProtected[0] = 0x00;
  biosProtected[1] = 0xf0;
  biosProtected[2] = 0x29;
  biosProtected[3] = 0xe1;

  /*FILE *outFile = fopen("cpuBitsSet.txt", "wb");
  for(i = 0; i < 256; i++) {
    int count = 0;
    int j;
    for(j = 0; j < 8; j++)
      if(i & (1 << j))
        count++;
    gCpu.cpuBitsSet[i] = count;
    fprintf(outFile, "%d, ", count);

    /*for(j = 0; j < 8; j++)
      if(i & (1 << j))
        break;
    cpuLowestBitSet[i] = j;*/
  /*}
  fclose(outFile);*/

  /*for(i = 0; i < 0x400; i++)
    ioReadable[i] = true;
  for(i = 0x10; i < 0x48; i++)
    ioReadable[i] = false;
  for(i = 0x4c; i < 0x50; i++)
    ioReadable[i] = false;
  for(i = 0x54; i < 0x60; i++)
    ioReadable[i] = false;
  for(i = 0x8c; i < 0x90; i++)
    ioReadable[i] = false;
  for(i = 0xa0; i < 0xb8; i++)
    ioReadable[i] = false;
  for(i = 0xbc; i < 0xc4; i++)
    ioReadable[i] = false;
  for(i = 0xc8; i < 0xd0; i++)
    ioReadable[i] = false;
  for(i = 0xd4; i < 0xdc; i++)
    ioReadable[i] = false;
  for(i = 0xe0; i < 0x100; i++)
    ioReadable[i] = false;
  for(i = 0x110; i < 0x120; i++)
    ioReadable[i] = false;
  for(i = 0x12c; i < 0x130; i++)
    ioReadable[i] = false;
  for(i = 0x138; i < 0x140; i++)
    ioReadable[i] = false;
  for(i = 0x144; i < 0x150; i++)
    ioReadable[i] = false;
  for(i = 0x15c; i < 0x200; i++)
    ioReadable[i] = false;
  for(i = 0x20c; i < 0x300; i++)
    ioReadable[i] = false;
  for(i = 0x304; i < 0x400; i++)
    ioReadable[i] = false;*/

  if(romSize < 0x1fe2000) {
  	*((uint16a *)&rom[0x1fe209c]) = 0xdffa; // SWI 0xFA
  	*((uint16a *)&rom[0x1fe209e]) = 0x4770; // BX LR
  } else {
#ifdef VBAM_USE_AGB_PRINT
    agbPrintEnable(false);
#endif
  }
}

void CPUReset()
{
  if(gbaSaveType == 0) {
    if(eepromInUse)
      gbaSaveType = 3;
    else
      switch(saveType) {
      case 1:
        gbaSaveType = 1;
        break;
      case 2:
        gbaSaveType = 2;
        break;
      }
  }
  rtcReset();
  // clean io memory
  memset(ioMem.b, 0, 0x400);
  // clean OAM, palette, picture, & vram
  gLcd.resetAll(useBios, skipBios);

  lineMix = &gLcd.pix[240 * ioMem.VCOUNT];
  gGba.dma.reset();
  ioMem.TM0D     = 0x0000;
  ioMem.TM0CNT   = 0x0000;
  ioMem.TM1D     = 0x0000;
  ioMem.TM1CNT   = 0x0000;
  ioMem.TM2D     = 0x0000;
  ioMem.TM2CNT   = 0x0000;
  ioMem.TM3D     = 0x0000;
  ioMem.TM3CNT   = 0x0000;
  P1       = 0x03FF;
  gGba.cpu.reset(cpuIsMultiBoot, useBios, skipBios);

  //UPDATE_REG(0x00, DISPCNT);
  //UPDATE_REG(0x06, VCOUNT);
  //UPDATE_REG(0x20, BG2PA);
  //UPDATE_REG(0x26, BG2PD);
  //UPDATE_REG(0x30, BG3PA);
  //UPDATE_REG(0x36, BG3PD);
  //UPDATE_REG(0x130, P1);
  UPDATE_REG(0x88, 0x200);

#ifdef VBAM_USE_HOLDTYPE
  holdType = 0;
#endif

  biosProtected[0] = 0x00;
  biosProtected[1] = 0xf0;
  biosProtected[2] = 0x29;
  biosProtected[3] = 0xe1;

  lcdTicks = (useBios && !skipBios) ? 1008 : 208;
  gGba.timers.timer0On = false;
  gGba.timers.timer0Ticks = 0;
  gGba.timers.timer0Reload = 0;
  gGba.timers.timer0ClockReload  = 0;
  gGba.timers.timer1On = false;
  gGba.timers.timer1Ticks = 0;
  gGba.timers.timer1Reload = 0;
  gGba.timers.timer1ClockReload  = 0;
  gGba.timers.timer2On = false;
  gGba.timers.timer2Ticks = 0;
  gGba.timers.timer2Reload = 0;
  gGba.timers.timer2ClockReload  = 0;
  gGba.timers.timer3On = false;
  gGba.timers.timer3Ticks = 0;
  gGba.timers.timer3Reload = 0;
  gGba.timers.timer3ClockReload  = 0;
  cpuSaveGameFunc = flashSaveDecide;
  renderLine = mode0RenderLine;
  fxOn = false;
  windowOn = false;
  saveType = 0;

  CPUUpdateRenderBuffers(true);

  /*for(int i = 0; i < 256; i++) {
    map[i].address = (u8 *)&dummyAddress;
    map[i].mask = 0;
  }

  map[0].address = bios;
  map[0].mask = 0x3FFF;
  map[2].address = workRAM;
  map[2].mask = 0x3FFFF;
  map[3].address = internalRAM;
  map[3].mask = 0x7FFF;
  map[4].address = ioMem;
  map[4].mask = 0x3FF;
  map[5].address = paletteRAM;
  map[5].mask = 0x3FF;
  map[6].address = vram;
  map[6].mask = 0x1FFFF;
  map[7].address = oam;
  map[7].mask = 0x3FF;
  map[8].address = rom;
  map[8].mask = 0x1FFFFFF;
  map[9].address = rom;
  map[9].mask = 0x1FFFFFF;
  map[10].address = rom;
  map[10].mask = 0x1FFFFFF;
  map[12].address = rom;
  map[12].mask = 0x1FFFFFF;
  map[14].address = flashSaveMemory;
  map[14].mask = 0xFFFF;*/

  eepromReset();
  flashReset();

  soundReset();

  CPUUpdateWindow0();
  CPUUpdateWindow1();

  // make sure registers are correctly initialized if not using BIOS
  if(!useBios) {
    if(cpuIsMultiBoot)
      BIOS_RegisterRamReset(gGba.cpu, 0xfe);
    else
      BIOS_RegisterRamReset(gGba.cpu, 0xff);
  } else {
    if(cpuIsMultiBoot)
      BIOS_RegisterRamReset(gGba.cpu, 0xfe);
  }

  switch(cpuSaveType) {
  case 0: // automatic
    cpuSramEnabled = true;
    cpuFlashEnabled = true;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = false;
    saveType = gbaSaveType = 0;
    break;
  case 1: // EEPROM
    cpuSramEnabled = false;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = false;
    saveType = gbaSaveType = 3;
    // EEPROM usage is automatically detected
    break;
  case 2: // SRAM
    cpuSramEnabled = true;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = false;
    cpuEEPROMSensorEnabled = false;
    cpuSaveGameFunc = sramDelayedWrite; // to insure we detect the write
    saveType = gbaSaveType = 1;
    break;
  case 3: // FLASH
    cpuSramEnabled = false;
    cpuFlashEnabled = true;
    cpuEEPROMEnabled = false;
    cpuEEPROMSensorEnabled = false;
    cpuSaveGameFunc = flashDelayedWrite; // to insure we detect the write
    saveType = gbaSaveType = 2;
    break;
  case 4: // EEPROM+Sensor
    cpuSramEnabled = false;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = true;
    cpuEEPROMSensorEnabled = true;
    // EEPROM usage is automatically detected
    saveType = gbaSaveType = 3;
    break;
  case 5: // NONE
    cpuSramEnabled = false;
    cpuFlashEnabled = false;
    cpuEEPROMEnabled = false;
    cpuEEPROMSensorEnabled = false;
    // no save at all
    saveType = gbaSaveType = 5;
    break;
  }

  gGba.cpu.ARM_PREFETCH();

  systemSaveUpdateCounter = SYSTEM_SAVE_NOT_UPDATED;

  gGba.dma.cpuDmaHack = false;

  //SWITicks = 0;
}

void CPUInterrupt(ARM7TDMI &cpu)
{
	cpu.interrupt();

  //  if(!holdState)
  biosProtected[0] = 0x02;
  biosProtected[1] = 0xc0;
  biosProtected[2] = 0x5e;
  biosProtected[3] = 0xe5;
}

void CPULoop(bool renderGfx, bool processGfx, bool renderAudio)
{
	auto cpu = gGba.cpu;
	auto &holdState = cpu.holdState;
	auto &armIrqEnable = cpu.armIrqEnable;
#ifdef VBAM_USE_SWITICKS
	auto &SWITicks = cpu.SWITicks;
#endif
#ifdef VBAM_USE_IRQTICKS
	auto &IRQTicks = cpu.IRQTicks;
#endif
	auto &IE = ioMem.IE;
	auto &IF = ioMem.IF;
	auto &IME = ioMem.IME;
  int clockTicks;
  int timerOverflow = 0;
  // variable used by the CPU core
  cpu.cpuTotalTicks = 0;

  // shuffle2: what's the purpose?
  if(gba_link_enabled)
    cpu.cpuNextEvent = 1;

  bool cpuBreakLoop = false;
  cpu.cpuNextEvent = CPUUpdateTicks(cpu);
  /*if(cpu.cpuNextEvent > ticks)
    cpu.cpuNextEvent = ticks;*/

  do {
#ifndef FINAL_VERSION
    if(systemDebug) {
      if(systemDebug >= 10 && !holdState) {
      	cpu.updateCPSR();
#ifdef BKPT_SUPPORT
		if (debugger_last)
		{
			winlog("R00=%08x R01=%08x R02=%08x R03=%08x R04=%08x R05=%08x R06=%08x R07=%08x R08=%08x R09=%08x R10=%08x R11=%08x R12=%08x R13=%08x R14=%08x R15=%08x R16=%08x R17=%08x\n",
                 oldreg[0], oldreg[1], oldreg[2], oldreg[3], oldreg[4], oldreg[5],
                 oldreg[6], oldreg[7], oldreg[8], oldreg[9], oldreg[10], oldreg[11],
                 oldreg[12], oldreg[13], oldreg[14], oldreg[15], oldreg[16],
                 oldreg[17]);
		}
#endif
        /*winlog("R00=%08x R01=%08x R02=%08x R03=%08x R04=%08x R05=%08x R06=%08x R07=%08x R08=%08x R09=%08x R10=%08x R11=%08x R12=%08x R13=%08x R14=%08x R15=%08x R16=%08x R17=%08x\n",
                 reg[0].I, reg[1].I, reg[2].I, reg[3].I, reg[4].I, reg[5].I,
                 reg[6].I, reg[7].I, reg[8].I, reg[9].I, reg[10].I, reg[11].I,
                 reg[12].I, reg[13].I, reg[14].I, reg[15].I, reg[16].I,
                 reg[17].I);*/
      } else if(!holdState) {
        //winlog("PC=%08x\n", armNextPC);
      }
    }
#endif /* FINAL_VERSION */

    if(!holdState
#ifdef VBAM_USE_SWITICKS
    		&& !SWITicks
#endif
    		) {
      if(cpu.armState) {
        if (!armExecute(cpu))
        {
					#ifdef BKPT_SUPPORT
        	gCpu = cpu;
          return;
					#endif
        }
      } else {
        if (!thumbExecute(cpu))
        {
					#ifdef BKPT_SUPPORT
        	gCpu = cpu;
          return;
					#endif
        }
      }
      clockTicks = 0;
    } else
      clockTicks = CPUUpdateTicks(cpu);

    cpu.cpuTotalTicks += clockTicks;

    if(cpu.cpuTotalTicks >= cpu.cpuNextEvent) {
      int remainingTicks = cpu.cpuTotalTicks - cpu.cpuNextEvent;

#ifdef VBAM_USE_SWITICKS
      if (SWITicks)
      {
        SWITicks-=clockTicks;
        if (SWITicks<0)
          SWITicks = 0;
      }
#endif

      clockTicks = cpu.cpuNextEvent;
      cpu.cpuTotalTicks = 0;
      gGba.dma.cpuDmaHack = false;

    updateLoop:

#ifdef VBAM_USE_IRQTICKS
      if (IRQTicks)
      {
          IRQTicks -= clockTicks;
        if (IRQTicks<0)
          IRQTicks = 0;
      }
#endif

      lcdTicks -= clockTicks;


      if(lcdTicks <= 0) {
        if(ioMem.DISPSTAT & 1) { // V-BLANK
          // if in V-Blank mode, keep computing...
          if(ioMem.DISPSTAT & 2) {
            lcdTicks += 1008;
            ioMem.VCOUNT++; //lineMix += 240;
            //UPDATE_REG(0x06, VCOUNT);
            ioMem.DISPSTAT &= 0xFFFD;
            //UPDATE_REG(0x04, DISPSTAT);
            CPUCompareVCOUNT(cpu);
          } else {
            lcdTicks += 224;
            ioMem.DISPSTAT |= 2;
            //UPDATE_REG(0x04, DISPSTAT);
            if(ioMem.DISPSTAT & 16) {
              IF |= 2;
              //UPDATE_REG(0x202, IF);
            }
          }

          if(ioMem.VCOUNT >= 228) { //Reaching last line
          	ioMem.DISPSTAT &= 0xFFFC;
            //UPDATE_REG(0x04, DISPSTAT);
          	ioMem.VCOUNT = 0; lineMix = gLcd.pix;
            //UPDATE_REG(0x06, VCOUNT);
            CPUCompareVCOUNT(cpu);
          }
        } else {

          if(ioMem.DISPSTAT & 2) {
            // if in H-Blank, leave it and move to drawing mode
          	ioMem.VCOUNT++; lineMix += 240;
            //UPDATE_REG(0x06, VCOUNT);

            lcdTicks += 1008;
            ioMem.DISPSTAT &= 0xFFFD;
            if(ioMem.VCOUNT == 160) {
            	// update input
              // TODO: motion sensor
              /*if(cpuEEPROMSensorEnabled)
                systemUpdateMotionSensor();*/
              //UPDATE_REG(0x130, P1);
              u16 P1CNT = READ16LE(((u16 *)&ioMem.b[0x132]));
              // this seems wrong, but there are cases where the game
              // can enter the stop state without requesting an IRQ from
              // the joypad.
              if((P1CNT & 0x4000) || gGba.stopState) {
                u16 p1 = (0x3FF ^ P1) & 0x3FF;
                if(P1CNT & 0x8000) {
                  if(p1 == (P1CNT & 0x3FF)) {
                    IF |= 0x1000;
                    //UPDATE_REG(0x202, IF);
                  }
                } else {
                  if(p1 & P1CNT) {
                    IF |= 0x1000;
                    //UPDATE_REG(0x202, IF);
                  }
                }
              }

              //u32 ext = (joy >> 10);
              // If no (m) code is enabled, apply the cheats at each LCDline
              /*if((cheatsEnabled) && (mastercode==0))
                remainingTicks += cheatsCheckKeys(cpu, P1^0x3FF, ext);*/

              ioMem.DISPSTAT |= 1;
              ioMem.DISPSTAT &= 0xFFFD;
              //UPDATE_REG(0x04, DISPSTAT);
              if(ioMem.DISPSTAT & 0x0008) {
                IF |= 1;
                //UPDATE_REG(0x202, IF);
              }
              CPUCheckDMA(cpu, 1, 0x0f);

              cpuBreakLoop = 1; // stop when frame ready
            }

            //UPDATE_REG(0x04, DISPSTAT);
            CPUCompareVCOUNT(cpu);

          } else {
            if(processGfx)
            {
            	/*if(trackOAM)
            	{
            		if(oamUpdated)
            		{
            			//systemMessage(0, "OAM updated, line %d", (int)VCOUNT);
            			//oamUpdated = 0;
            		}
            	}
            	else
            	{
            	}*/

              (*renderLine)(lineMix, ioMem);
              /*switch(systemColorDepth) {
				#ifdef SUPPORT_PIX_16BIT
                case 16:
                {
                	if(!directColorLookup)
                	{
										for(int x = 0; x < 240; x++)
										{
											//lineMix[x] = systemColorMap.map16[lineMix[x]];
										}
                	}
                }
                break;
				#endif
				#ifdef SUPPORT_PIX_32BIT
                case 24:
                {
                  u8 *dest = (u8 *)pix + 240 * VCOUNT * 3;
                  for(int x = 0; x < 240;) {
                	for(int i = 0; i < 16; i++) {
                      *((u32 *)dest) = systemColorMap32[lineMix[x++] & 0xFFFF];
                      dest += 3;
                	}
                  }
                }
                break;
                case 32:
                {
                  u32 *dest = (u32 *)pix + 240 * (VCOUNT);
                  for(int x = 0; x < 240; ) {
                	for(int i = 0; i < 16; i++)
                      *dest++ = systemColorMap32[lineMix[x++] & 0xFFFF];

                  }
                }
                break;
				#endif
              }*/
            }
            if(ioMem.VCOUNT == 159 && likely(renderGfx))
            {
            	if(likely(processGfx) && !directColorLookup)
            	{
            		for(int x = 0; x < 240*160; x++)
            		{
            			gLcd.pix[x] = systemColorMap.map16[gLcd.pix[x]];
            		}
            	}
            	if(likely(renderGfx))
            		systemDrawScreen();
            }
            // entering H-Blank
            ioMem.DISPSTAT |= 2;
            //UPDATE_REG(0x04, DISPSTAT);
            lcdTicks += 224;
            CPUCheckDMA(cpu, 2, 0x0f);
            if(ioMem.DISPSTAT & 16) {
              IF |= 2;
              //UPDATE_REG(0x202, IF);
            }
          }
        }
      }

	    // we shouldn't be doing sound in stop state, but we loose synchronization
      // if sound is disabled, so in stop state, soundTick will just produce
      // mute sound
      soundTicks -= clockTicks;
      if(soundTicks <= 0) {
        psoundTickfn(renderAudio);
        soundTicks += SOUND_CLOCK_TICKS;
      }

      if(!gGba.stopState) {
      	auto &timerOnOffDelay = gGba.timers.timerOnOffDelay;
      	auto &timer0Value = gGba.timers.timer0Value;
      	auto &timer0On = gGba.timers.timer0On;
      	auto &timer0Ticks = gGba.timers.timer0Ticks;
      	auto &timer0ClockReload  = gGba.timers.timer0ClockReload;
      	auto &timer0Reload  = gGba.timers.timer0Reload;
      	auto &timer1Value = gGba.timers.timer1Value;
      	auto &timer1On = gGba.timers.timer1On;
      	auto &timer1Ticks = gGba.timers.timer1Ticks;
      	auto &timer1ClockReload  = gGba.timers.timer1ClockReload;
      	auto &timer1Reload  = gGba.timers.timer1Reload;
      	auto &timer2Value = gGba.timers.timer2Value;
      	auto &timer2On = gGba.timers.timer2On;
      	auto &timer2Ticks = gGba.timers.timer2Ticks;
      	auto &timer2ClockReload  = gGba.timers.timer2ClockReload;
      	auto &timer2Reload  = gGba.timers.timer2Reload;
      	auto &timer3Value = gGba.timers.timer3Value;
      	auto &timer3On = gGba.timers.timer3On;
      	auto &timer3Ticks = gGba.timers.timer3Ticks;
      	auto &timer3ClockReload  = gGba.timers.timer3ClockReload;
      	auto &timer3Reload  = gGba.timers.timer3Reload;

        if(timer0On) {
          timer0Ticks -= clockTicks;
          if(timer0Ticks <= 0) {
            timer0Ticks += (0x10000 - timer0Reload) << timer0ClockReload;
            timerOverflow |= 1;
            soundTimerOverflow(cpu, 0);
            if(ioMem.TM0CNT & 0x40) {
              IF |= 0x08;
              //UPDATE_REG(0x202, IF);
            }
          }
          ioMem.TM0D = 0xFFFF - (timer0Ticks >> timer0ClockReload);
          //UPDATE_REG(0x100, TM0D);
        }

        if(timer1On) {
          if(ioMem.TM1CNT & 4) {
            if(timerOverflow & 1) {
            	ioMem.TM1D++;
              if(ioMem.TM1D == 0) {
              	ioMem.TM1D += timer1Reload;
                timerOverflow |= 2;
                soundTimerOverflow(cpu, 1);
                if(ioMem.TM1CNT & 0x40) {
                  IF |= 0x10;
                  //UPDATE_REG(0x202, IF);
                }
              }
              //UPDATE_REG(0x104, TM1D);
            }
          } else {
            timer1Ticks -= clockTicks;
            if(timer1Ticks <= 0) {
              timer1Ticks += (0x10000 - timer1Reload) << timer1ClockReload;
              timerOverflow |= 2;
              soundTimerOverflow(cpu, 1);
              if(ioMem.TM1CNT & 0x40) {
                IF |= 0x10;
                //UPDATE_REG(0x202, IF);
              }
            }
            ioMem.TM1D = 0xFFFF - (timer1Ticks >> timer1ClockReload);
            //UPDATE_REG(0x104, TM1D);
          }
        }

        if(timer2On) {
          if(ioMem.TM2CNT & 4) {
            if(timerOverflow & 2) {
            	ioMem.TM2D++;
              if(ioMem.TM2D == 0) {
              	ioMem.TM2D += timer2Reload;
                timerOverflow |= 4;
                if(ioMem.TM2CNT & 0x40) {
                  IF |= 0x20;
                  //UPDATE_REG(0x202, IF);
                }
              }
              //UPDATE_REG(0x108, TM2D);
            }
          } else {
            timer2Ticks -= clockTicks;
            if(timer2Ticks <= 0) {
              timer2Ticks += (0x10000 - timer2Reload) << timer2ClockReload;
              timerOverflow |= 4;
              if(ioMem.TM2CNT & 0x40) {
                IF |= 0x20;
                //UPDATE_REG(0x202, IF);
              }
            }
            ioMem.TM2D = 0xFFFF - (timer2Ticks >> timer2ClockReload);
            //UPDATE_REG(0x108, TM2D);
          }
        }

        if(timer3On) {
          if(ioMem.TM3CNT & 4) {
            if(timerOverflow & 4) {
            	ioMem.TM3D++;
              if(ioMem.TM3D == 0) {
              	ioMem.TM3D += timer3Reload;
                if(ioMem.TM3CNT & 0x40) {
                  IF |= 0x40;
                  //UPDATE_REG(0x202, IF);
                }
              }
              //UPDATE_REG(0x10C, TM3D);
            }
          } else {
              timer3Ticks -= clockTicks;
            if(timer3Ticks <= 0) {
              timer3Ticks += (0x10000 - timer3Reload) << timer3ClockReload;
              if(ioMem.TM3CNT & 0x40) {
                IF |= 0x40;
                //UPDATE_REG(0x202, IF);
              }
            }
            ioMem.TM3D = 0xFFFF - (timer3Ticks >> timer3ClockReload);
            //UPDATE_REG(0x10C, TM3D);
          }
        }
      }

      timerOverflow = 0;



#ifdef PROFILING
      profilingTicks -= clockTicks;
      if(profilingTicks <= 0) {
        profilingTicks += profilingTicksReload;
        if(profilSegment) {
	  profile_segment *seg = profilSegment;
	  do {
	    u16 *b = (u16 *)seg->sbuf;
	    int pc = ((reg[15].I - seg->s_lowpc) * seg->s_scale)/0x10000;
	    if(pc >= 0 && pc < seg->ssiz) {
            b[pc]++;
	      break;
          }

	    seg = seg->next;
	  } while(seg);
        }
      }
#endif

      //ticks -= clockTicks;

	  /*if (gba_joybus_enabled)
		  JoyBusUpdate(clockTicks);*/

	  if (gba_link_enabled)
	  {
		  LinkUpdate(clockTicks);
	  }

      cpu.cpuNextEvent = CPUUpdateTicks(cpu);

      if(gGba.dma.cpuDmaTicksToUpdate > 0) {
        if(gGba.dma.cpuDmaTicksToUpdate > cpu.cpuNextEvent)
          clockTicks = cpu.cpuNextEvent;
        else
          clockTicks = gGba.dma.cpuDmaTicksToUpdate;
        gGba.dma.cpuDmaTicksToUpdate -= clockTicks;
        if(gGba.dma.cpuDmaTicksToUpdate < 0)
        	gGba.dma.cpuDmaTicksToUpdate = 0;
        gGba.dma.cpuDmaHack = true;
        goto updateLoop;
      }

	  // shuffle2: what's the purpose?
	  if(gba_link_enabled)
  	       cpu.cpuNextEvent = 1;

      if(IF && (IME & 1) && armIrqEnable) {
        int res = IF & IE;
        if(gGba.stopState)
          res &= 0x3080;
        if(res) {
          if (gGba.intState)
          {
#ifdef VBAM_USE_IRQTICKS
            if (!IRQTicks)
#endif
            {
              CPUInterrupt(cpu);
              gGba.intState = false;
              holdState = false;
              gGba.stopState = false;
#ifdef VBAM_USE_HOLDTYPE
              holdType = 0;
#endif
            }
          }
          else
          {
            if (!holdState)
            {
            	gGba.intState = true;
#ifdef VBAM_USE_IRQTICKS
              IRQTicks=7;
              if (cpu.cpuNextEvent> IRQTicks)
                cpu.cpuNextEvent = IRQTicks;
#else
							if (cpu.cpuNextEvent> 7)
								cpu.cpuNextEvent = 7;
#endif
            }
            else
            {
              CPUInterrupt(cpu);
              holdState = false;
              gGba.stopState = false;
#ifdef VBAM_USE_HOLDTYPE
              holdType = 0;
#endif
            }
          }

#ifdef VBAM_USE_SWITICKS
          // Stops the SWI Ticks emulation if an IRQ is executed
          //(to avoid problems with nested IRQ/SWI)
          if (SWITicks)
            SWITicks = 0;
#endif
        }
      }

      if(remainingTicks > 0) {
        if(remainingTicks > cpu.cpuNextEvent)
          clockTicks = cpu.cpuNextEvent;
        else
          clockTicks = remainingTicks;
        remainingTicks -= clockTicks;
        if(remainingTicks < 0)
          remainingTicks = 0;
        goto updateLoop;
      }

      if (gGba.timers.timerOnOffDelay)
          applyTimer(cpu);

      /*if(cpu.cpuNextEvent > ticks)
        cpu.cpuNextEvent = ticks;*/

      /*if(ticks <= 0 || cpuBreakLoop)
        break;*/

    }
  } while(!cpuBreakLoop);
  gGba.cpu = cpu;
}



#if 0
struct EmulatedSystem GBASystem = {
  // emuMain
  CPULoop,
  // emuReset
  CPUReset,
  // emuCleanUp
  CPUCleanUp,
  // emuReadBattery
  CPUReadBatteryFile,
  // emuWriteBattery
  CPUWriteBatteryFile,
  // emuReadState
  CPUReadState,
  // emuWriteState
  CPUWriteState,
  // emuReadMemState
  CPUReadMemState,
  // emuWriteMemState
  CPUWriteMemState,
  // emuWritePNG
  //CPUWritePNGFile,
  // emuWriteBMP
  //CPUWriteBMPFile,,
  // emuUpdateCPSR
  CPUUpdateCPSR,
  // emuHasDebugger
  true,
  // emuCount
/*#ifdef FINAL_VERSION
  250000
#else
  5000
#endif*/
};
#endif
