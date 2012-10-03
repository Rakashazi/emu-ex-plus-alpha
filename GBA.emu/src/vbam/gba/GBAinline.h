#ifndef GBAINLINE_H
#define GBAINLINE_H

#include "../System.h"
#include "../common/Port.h"
#include "RTC.h"
#include "Sound.h"
#include "agbprint.h"
#include "GBAcpu.h"
#include "GBALink.h"

static const u32  objTilesAddress [3] = {0x010000, 0x014000, 0x014000};

#ifdef VBAM_USE_HOLDTYPE
extern int holdType;
#endif
extern bool cpuSramEnabled;
extern bool cpuFlashEnabled;
extern bool cpuEEPROMEnabled;
extern bool cpuEEPROMSensorEnabled;

// Handlers (TODO)

static inline u32 armRotLoad32(u32 value, u32 address, bool rot = 1)
{
  if(rot && (address & 3))
  {
    int shift = (address & 3) << 3;
    return (value >> shift) | (value << (32 - shift));
  }
  return value;
}

static inline u32 unreadableRead32(ARM7TDMI &cpu, u32 address)
{
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
	bool &armState = cpu.armState;
	auto &reg = cpu.reg;
#ifdef GBA_LOGGING
	if(systemVerbose & VERBOSE_ILLEGAL_READ) {
		log("Illegal word read: %08x at %08x\n", address, armMode ?
			armNextPC - 4 : armNextPC - 2);
	}
#endif

	if(cpuDmaHack) {
		return armRotLoad32(cpuDmaLast, address);
	} else {
		if(armState) {
			return armRotLoad32(CPUReadMemoryQuick(reg[15].I), address);
		} else {
			return armRotLoad32(CPUReadHalfWordQuick(reg[15].I) |
				CPUReadHalfWordQuick(reg[15].I) << 16, address);
		}
	}
}

static inline u32 biosRead32(ARM7TDMI &cpu, u32 address)
{
	auto &reg = cpu.reg;
	if(reg[15].I >> 24) {
		if(address < 0x4000) {
#ifdef GBA_LOGGING
			if(systemVerbose & VERBOSE_ILLEGAL_READ) {
				log("Illegal word read: %08x at %08x\n", address, armMode ?
					armNextPC - 4 : armNextPC - 2);
			}
#endif

			return armRotLoad32(READ32LE(((u32 *)&biosProtected)), address);
		}
		else return unreadableRead32(cpu, address);
	} else
		return armRotLoad32(READ32LE(((u32 *)&bios[address & 0x3FFC])), address);
}

static inline u32 ioMemRead32(ARM7TDMI &cpu, u32 address)
{
  if((address < 0x4000400) && ioReadable[address & 0x3fc]) {
	  if(ioReadable[(address & 0x3fc) + 2]) {
		  if ((address & 0x3fc) == COMM_JOY_RECV_L)
			  UPDATE_REG(COMM_JOYSTAT, READ16LE(&ioMem.b[COMM_JOYSTAT]) & ~JOYSTAT_RECV);
		  return armRotLoad32(READ32LE(((u32 *)&ioMem.b[address & 0x3fC])), address);
	  } else {
	  	return armRotLoad32(READ16LE(((u16 *)&ioMem.b[address & 0x3fc])), address);
	  }
  }
  else
  	return unreadableRead32(cpu, address);
}

static inline u32 vramRead32(ARM7TDMI &cpu, u32 address)
{
	address = (address & 0x1fffc);
	if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
	{
		return 0;
	}
	if ((address & 0x18000) == 0x18000)
		address &= 0x17fff;
	return armRotLoad32(READ32LE(((u32 *)&vram[address])), address);
}

static inline u32 eepromRead32(ARM7TDMI &cpu, u32 address)
{
  if(cpuEEPROMEnabled)
    // no need to swap this
    return eepromRead(address);
  return unreadableRead32(cpu, address);
}

static inline u32 flashRead32(ARM7TDMI &cpu, u32 address)
{
  if(cpuFlashEnabled | cpuSramEnabled)
    // no need to swap this
    return flashRead(address);
  else
  	return unreadableRead32(cpu, address);
}

static inline u32 armRotLoad16(u32 value, u32 address, bool rot = 1)
{
	if(rot && (address & 1))
	{
	   return (value >> 8) | (value << 24);
	}
	return value;
}

static inline u32 unreadableRead16(ARM7TDMI &cpu, u32 address)
{
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
	auto &reg = cpu.reg;
	bool &armState = cpu.armState;
#ifdef GBA_LOGGING
	if(systemVerbose & VERBOSE_ILLEGAL_READ) {
		log("Illegal halfword read: %08x at %08x\n", address, armMode ?
			armNextPC - 4 : armNextPC - 2);
	}
#endif
	if(cpuDmaHack) {
		return armRotLoad16(cpuDmaLast & 0xFFFF, address);
	} else {
		if(armState) {
			return armRotLoad16(CPUReadHalfWordQuick(reg[15].I + (address & 2)), address);
		} else {
			return armRotLoad16(CPUReadHalfWordQuick(reg[15].I), address);
		}
	}
}

static inline u32 biosRead16(ARM7TDMI &cpu, u32 address)
{
	auto &reg = cpu.reg;
  if (reg[15].I >> 24) {
		if(address < 0x4000) {
	#ifdef GBA_LOGGING
			if(systemVerbose & VERBOSE_ILLEGAL_READ) {
				log("Illegal halfword read: %08x at %08x\n", address, armMode ?
					armNextPC - 4 : armNextPC - 2);
			}
	#endif
			return armRotLoad16(READ16LE(((u16 *)&biosProtected[address&2])), address);
		} else return unreadableRead16(cpu, address);
	} else
		return armRotLoad16(READ16LE(((u16 *)&bios[address & 0x3FFE])), address);
}



static inline u32 ioMemRead16(ARM7TDMI &cpu, u32 address)
{
	auto &cpuTotalTicks = cpu.cpuTotalTicks;
	auto &timer0Value = gGba.timers.timer0Value;
	auto &timer0On = gGba.timers.timer0On;
	auto &timer0Ticks = gGba.timers.timer0Ticks;
	auto &timer0Reload = gGba.timers.timer0Reload;
	auto &timer0ClockReload  = gGba.timers.timer0ClockReload;
	auto &timer1Value = gGba.timers.timer1Value;
	auto &timer1On = gGba.timers.timer1On;
	auto &timer1Ticks = gGba.timers.timer1Ticks;
	auto &timer1Reload = gGba.timers.timer1Reload;
	auto &timer1ClockReload  = gGba.timers.timer1ClockReload;
	auto &timer2Value = gGba.timers.timer2Value;
	auto &timer2On = gGba.timers.timer2On;
	auto &timer2Ticks = gGba.timers.timer2Ticks;
	auto &timer2Reload = gGba.timers.timer2Reload;
	auto &timer2ClockReload  = gGba.timers.timer2ClockReload;
	auto &timer3Value = gGba.timers.timer3Value;
	auto &timer3On = gGba.timers.timer3On;
	auto &timer3Ticks = gGba.timers.timer3Ticks;
	auto &timer3Reload = gGba.timers.timer3Reload;
	auto &timer3ClockReload  = gGba.timers.timer3ClockReload;
  if((address < 0x4000400) && ioReadable[address & 0x3fe])
  {
    if (((address & 0x3fe)>0xFF) && ((address & 0x3fe)<0x10E))
    {
      if (((address & 0x3fe) == 0x100) && timer0On)
      	return armRotLoad16(0xFFFF - ((timer0Ticks-cpuTotalTicks) >> timer0ClockReload), address);
      else
        if (((address & 0x3fe) == 0x104) && timer1On && !(ioMem.TM1CNT & 4))
        	return armRotLoad16(0xFFFF - ((timer1Ticks-cpuTotalTicks) >> timer1ClockReload), address);
        else
          if (((address & 0x3fe) == 0x108) && timer2On && !(ioMem.TM2CNT & 4))
          	return armRotLoad16(0xFFFF - ((timer2Ticks-cpuTotalTicks) >> timer2ClockReload), address);
          else
            if (((address & 0x3fe) == 0x10C) && timer3On && !(ioMem.TM3CNT & 4))
            	return armRotLoad16(0xFFFF - ((timer3Ticks-cpuTotalTicks) >> timer3ClockReload), address);
    }
    return armRotLoad16(READ16LE(((u16 *)&ioMem.b[address & 0x3fe])), address);
  }
  else return unreadableRead16(cpu, address);
}

static inline u32 vramRead16(ARM7TDMI &cpu, u32 address)
{
  address = (address & 0x1fffe);
  if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
  {
  	return 0;
  }
  if ((address & 0x18000) == 0x18000)
    address &= 0x17fff;
  return armRotLoad16(READ16LE(((u16 *)&vram[address])), address);
}

static inline u32 rtcRead16(ARM7TDMI &cpu, u32 address)
{
	if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
  	return armRotLoad16(rtcRead(address), address);
  else
  	return armRotLoad16(READ16LE(((u16 *)&rom[address & 0x1FFFFFE])), address);
}

static inline u32 unreadableRead8(ARM7TDMI &cpu, u32 address)
{
	auto &reg = cpu.reg;
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
	bool &armState = cpu.armState;
#ifdef GBA_LOGGING
	if(systemVerbose & VERBOSE_ILLEGAL_READ) {
		log("Illegal byte read: %08x at %08x\n", address, armMode ?
			armNextPC - 4 : armNextPC - 2);
	}
#endif
	if(cpuDmaHack) {
		return cpuDmaLast & 0xFF;
	} else {
		if(armState) {
			return CPUReadByteQuick(reg[15].I+(address & 3));
		} else {
			return CPUReadByteQuick(reg[15].I+(address & 1));
		}
	}
}

static inline u32 biosRead8(ARM7TDMI &cpu, u32 address)
{
	auto &reg = cpu.reg;
	if (reg[15].I >> 24) {
		if(address < 0x4000) {
#ifdef GBA_LOGGING
			if(systemVerbose & VERBOSE_ILLEGAL_READ) {
				log("Illegal byte read: %08x at %08x\n", address, armMode ?
					armNextPC - 4 : armNextPC - 2);
			}
#endif
			return biosProtected[address & 3];
		} else return unreadableRead8(cpu, address);
	}
	return bios[address & 0x3FFF];
}

static inline u32 ioMemRead8(ARM7TDMI &cpu, u32 address)
{
	if((address < 0x4000400) && ioReadable[address & 0x3ff])
		return ioMem.b[address & 0x3ff];
	else return unreadableRead8(cpu, address);
}

static inline u32 vramRead8(ARM7TDMI &cpu, u32 address)
{
  address = (address & 0x1ffff);
  if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
    return 0;
  if ((address & 0x18000) == 0x18000)
    address &= 0x17fff;
  return vram[address];
}

template <bool rot>
static inline u32 CPUReadMemoryBase(ARM7TDMI &cpu, u32 address)
{
	bool &armState = cpu.armState;
	auto &reg = cpu.reg;
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
#ifdef GBA_LOGGING
  if(address & 3) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned word read: %08x at %08x\n", address, armMode ?
        armNextPC - 4 : armNextPC - 2);
    }
  }
#endif

  switch(address >> 24) {
  case 0:
    if(reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal word read: %08x at %08x\n", address, armMode ?
            armNextPC - 4 : armNextPC - 2);
        }
#endif

        return armRotLoad32(READ32LE(((u32 *)&biosProtected)), address, rot);
      }
      else goto unreadable;
    } else
    	return armRotLoad32(READ32LE(((u32 *)&bios[address & 0x3FFC])), address, rot);
    break;
  case 2:
  	return armRotLoad32(READ32LE(((u32 *)&workRAM[address & 0x3FFFC])), address, rot);
    break;
  case 3:
  	return armRotLoad32(READ32LE(((u32 *)&internalRAM[address & 0x7ffC])), address, rot);
    break;
  case 4:
	  if((address < 0x4000400) && ioReadable[address & 0x3fc]) {
		  if(ioReadable[(address & 0x3fc) + 2]) {
			  if ((address & 0x3fc) == COMM_JOY_RECV_L)
				  UPDATE_REG(COMM_JOYSTAT, READ16LE(&ioMem.b[COMM_JOYSTAT]) & ~JOYSTAT_RECV);
			  return armRotLoad32(READ32LE(((u32 *)&ioMem.b[address & 0x3fC])), address, rot);
		  } else {
		  	return armRotLoad32(READ16LE(((u16 *)&ioMem.b[address & 0x3fc])), address, rot);
		  }
	  }
	  else
		  goto unreadable;
	  break;
  case 5:
  	return armRotLoad32(READ32LE(((u32 *)&paletteRAM[address & 0x3fC])), address, rot);
    break;
  case 6:
    address = (address & 0x1fffc);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
    {
    	return 0;
      break;
    }
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    return armRotLoad32(READ32LE(((u32 *)&vram[address])), address, rot);
    break;
  case 7:
  	return armRotLoad32(READ32LE(((u32 *)&oam[address & 0x3FC])), address, rot);
    break;
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
  	return armRotLoad32(READ32LE(((u32 *)&rom[address&0x1FFFFFC])), address, rot);
    break;
  case 13:
    if(cpuEEPROMEnabled)
      // no need to swap this
      return eepromRead(address);
    goto unreadable;
  case 14:
    if(cpuFlashEnabled | cpuSramEnabled)
      // no need to swap this
      return flashRead(address);
    // default
  default:
unreadable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_READ) {
      log("Illegal word read: %08x at %08x\n", address, armMode ?
        armNextPC - 4 : armNextPC - 2);
    }
#endif

    if(cpuDmaHack) {
    	return armRotLoad32(cpuDmaLast, address, rot);
    } else {
      if(armState) {
      	return armRotLoad32(CPUReadMemoryQuick(reg[15].I), address, rot);
      } else {
      	return armRotLoad32(CPUReadHalfWordQuick(reg[15].I) |
          CPUReadHalfWordQuick(reg[15].I) << 16, address, rot);
      }
    }
  }

  /*if(address & 3) {
#ifdef C_CORE
    int shift = (address & 3) << 3;
    value = (value >> shift) | (value << (32 - shift));
#else
#ifdef __GNUC__
    asm("and $3, %%ecx;"
      "shl $3 ,%%ecx;"
      "ror %%cl, %0"
      : "=r" (value)
      : "r" (value), "c" (address));
#else
    __asm {
      mov ecx, address;
      and ecx, 3;
      shl ecx, 3;
      ror [dword ptr value], cl;
    }
#endif
#endif
  }
  return value;*/
}

static inline u32 CPUReadMemory(ARM7TDMI &cpu, u32 address)
{
	return CPUReadMemoryBase<1>(cpu, address);
}

static inline u32 CPUReadMemoryNoRot(ARM7TDMI &cpu, u32 address)
{
	return CPUReadMemoryBase<1>(cpu, address);
}




template <bool rot>
static inline u32 CPUReadHalfWordBase(ARM7TDMI &cpu, u32 address)
{
	auto &armState = cpu.armState;
	auto &cpuNextEvent = cpu.cpuNextEvent;
	auto &cpuTotalTicks = cpu.cpuTotalTicks;
	auto &reg = cpu.reg;
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;
	auto &timer0Value = gGba.timers.timer0Value;
	auto &timer0On = gGba.timers.timer0On;
	auto &timer0Ticks = gGba.timers.timer0Ticks;
	auto &timer0Reload = gGba.timers.timer0Reload;
	auto &timer0ClockReload  = gGba.timers.timer0ClockReload;
	auto &timer1Value = gGba.timers.timer1Value;
	auto &timer1On = gGba.timers.timer1On;
	auto &timer1Ticks = gGba.timers.timer1Ticks;
	auto &timer1Reload = gGba.timers.timer1Reload;
	auto &timer1ClockReload  = gGba.timers.timer1ClockReload;
	auto &timer2Value = gGba.timers.timer2Value;
	auto &timer2On = gGba.timers.timer2On;
	auto &timer2Ticks = gGba.timers.timer2Ticks;
	auto &timer2Reload = gGba.timers.timer2Reload;
	auto &timer2ClockReload  = gGba.timers.timer2ClockReload;
	auto &timer3Value = gGba.timers.timer3Value;
	auto &timer3On = gGba.timers.timer3On;
	auto &timer3Ticks = gGba.timers.timer3Ticks;
	auto &timer3Reload = gGba.timers.timer3Reload;
	auto &timer3ClockReload  = gGba.timers.timer3ClockReload;
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
#ifdef GBA_LOGGING
  if(address & 1) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned halfword read: %08x at %08x\n", address, armMode ?
        armNextPC - 4 : armNextPC - 2);
    }
  }
#endif

  switch(address >> 24) {
  case 0:
    if (reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal halfword read: %08x at %08x\n", address, armMode ?
            armNextPC - 4 : armNextPC - 2);
        }
#endif
        return armRotLoad16(READ16LE(((u16 *)&biosProtected[address&2])), address, rot);
      } else goto unreadable;
    } else
    	return armRotLoad16(READ16LE(((u16 *)&bios[address & 0x3FFE])), address, rot);
    break;
  case 2:
  	return armRotLoad16(READ16LE(((u16 *)&workRAM[address & 0x3FFFE])), address, rot);
    break;
  case 3:
  	return armRotLoad16(READ16LE(((u16 *)&internalRAM[address & 0x7ffe])), address, rot);
    break;
  case 4:
    if((address < 0x4000400) && ioReadable[address & 0x3fe])
    {
      if (((address & 0x3fe)>0xFF) && ((address & 0x3fe)<0x10E))
      {
        if (((address & 0x3fe) == 0x100) && timer0On)
        	return armRotLoad16(0xFFFF - ((timer0Ticks-cpuTotalTicks) >> timer0ClockReload), address, rot);
        else
          if (((address & 0x3fe) == 0x104) && timer1On && !(ioMem.TM1CNT & 4))
          	return armRotLoad16(0xFFFF - ((timer1Ticks-cpuTotalTicks) >> timer1ClockReload), address, rot);
          else
            if (((address & 0x3fe) == 0x108) && timer2On && !(ioMem.TM2CNT & 4))
            	return armRotLoad16(0xFFFF - ((timer2Ticks-cpuTotalTicks) >> timer2ClockReload), address, rot);
            else
              if (((address & 0x3fe) == 0x10C) && timer3On && !(ioMem.TM3CNT & 4))
              	return armRotLoad16(0xFFFF - ((timer3Ticks-cpuTotalTicks) >> timer3ClockReload), address, rot);
      }
      return armRotLoad16(READ16LE(((u16 *)&ioMem.b[address & 0x3fe])), address, rot);
    }
    else goto unreadable;
    break;
  case 5:
  	return armRotLoad16(READ16LE(((u16 *)&paletteRAM[address & 0x3fe])), address, rot);
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
    {
    	return 0;
      break;
    }
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    return armRotLoad16(READ16LE(((u16 *)&vram[address])), address, rot);
    break;
  case 7:
  	return armRotLoad16(READ16LE(((u16 *)&oam[address & 0x3fe])), address, rot);
    break;
  case 8:
  	/*if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
  	  return armRotLoad16(rtcRead(address), address, rot);*/
  case 9 ... 12:
    if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8)
    	return armRotLoad16(rtcRead(address), address, rot);
    else
    	return armRotLoad16(READ16LE(((u16 *)&rom[address & 0x1FFFFFE])), address, rot);
    break;
  case 13:
    if(cpuEEPROMEnabled)
      // no need to swap this
      return  eepromRead(address);
    goto unreadable;
  case 14:
    if(cpuFlashEnabled | cpuSramEnabled)
      // no need to swap this
      return flashRead(address);
    // default
  default:
unreadable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_READ) {
      log("Illegal halfword read: %08x at %08x\n", address, armMode ?
        armNextPC - 4 : armNextPC - 2);
    }
#endif
    if(cpuDmaHack) {
    	return armRotLoad16(cpuDmaLast & 0xFFFF, address, rot);
    } else {
      if(armState) {
      	return armRotLoad16(CPUReadHalfWordQuick(reg[15].I + (address & 2)), address, rot);
      } else {
      	return armRotLoad16(CPUReadHalfWordQuick(reg[15].I), address, rot);
      }
    }
    break;
  }

  /*if(address & 1) {
    value = (value >> 8) | (value << 24);
  }

  return value;*/
}

static inline u32 CPUReadHalfWord(ARM7TDMI &cpu, u32 address)
{
	return CPUReadHalfWordBase<1>(cpu, address);
}

static inline u32 CPUReadHalfWordNoRot(ARM7TDMI &cpu, u32 address)
{
	return CPUReadHalfWordBase<1>(cpu, address);
}

static inline u16 CPUReadHalfWordSigned(ARM7TDMI &cpu, u32 address)
{
  u16 value = CPUReadHalfWord(cpu, address);
  if((address & 1))
    value = (s8)value;
  return value;
}

static inline u8 CPUReadByte(ARM7TDMI &cpu, u32 address)
{
	auto &armState = cpu.armState;
	auto &reg = cpu.reg;
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;
	auto &cpuDmaHack = gGba.dma.cpuDmaHack;
	auto &cpuDmaLast = gGba.dma.cpuDmaLast;
  switch(address >> 24) {
  case 0:
    if (reg[15].I >> 24) {
      if(address < 0x4000) {
#ifdef GBA_LOGGING
        if(systemVerbose & VERBOSE_ILLEGAL_READ) {
          log("Illegal byte read: %08x at %08x\n", address, armMode ?
            armNextPC - 4 : armNextPC - 2);
        }
#endif
        return biosProtected[address & 3];
      } else goto unreadable;
    }
    return bios[address & 0x3FFF];
  case 2:
    return workRAM[address & 0x3FFFF];
  case 3:
    return internalRAM[address & 0x7fff];
  case 4:
    if((address < 0x4000400) && ioReadable[address & 0x3ff])
      return ioMem.b[address & 0x3ff];
    else goto unreadable;
  case 5:
    return paletteRAM[address & 0x3ff];
  case 6:
    address = (address & 0x1ffff);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return 0;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
    return vram[address];
  case 7:
    return oam[address & 0x3ff];
  case 8:
  case 9:
  case 10:
  case 11:
  case 12:
    return rom[address & 0x1FFFFFF];
  case 13:
    if(cpuEEPROMEnabled)
      return eepromRead(address);
    goto unreadable;
  case 14:
    if(cpuSramEnabled | cpuFlashEnabled)
      return flashRead(address);
    if(cpuEEPROMSensorEnabled) {
      switch(address & 0x00008f00) {
  case 0x8200:
    return systemGetSensorX() & 255;
  case 0x8300:
    return (systemGetSensorX() >> 8)|0x80;
  case 0x8400:
    return systemGetSensorY() & 255;
  case 0x8500:
    return systemGetSensorY() >> 8;
      }
    }
    // default
  default:
unreadable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_READ) {
      log("Illegal byte read: %08x at %08x\n", address, armMode ?
        armNextPC - 4 : armNextPC - 2);
    }
#endif
    if(cpuDmaHack) {
      return cpuDmaLast & 0xFF;
    } else {
      if(armState) {
        return CPUReadByteQuick(reg[15].I+(address & 3));
      } else {
        return CPUReadByteQuick(reg[15].I+(address & 1));
      }
    }
    break;
  }
}

static inline void CPUWriteMemory(ARM7TDMI &cpu, u32 address, u32 value)
{
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;

#ifdef GBA_LOGGING
  if(address & 3) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned word write: %08x to %08x from %08x\n",
        value,
        address,
        armMode ? armNextPC - 4 : armNextPC - 2);
    }
  }
#endif

  switch(address >> 24) {
  case 0x02:
#ifdef BKPT_SUPPORT
    if(*((u32 *)&freezeWorkRAM[address & 0x3FFFC]))
      cheatsWriteMemory(address & 0x203FFFC,
      value);
    else
#endif
      WRITE32LE(((u32 *)&workRAM[address & 0x3FFFC]), value);
    break;
  case 0x03:
#ifdef BKPT_SUPPORT
    if(*((u32 *)&freezeInternalRAM[address & 0x7ffc]))
      cheatsWriteMemory(address & 0x3007FFC,
      value);
    else
#endif
      WRITE32LE(((u32 *)&internalRAM[address & 0x7ffC]), value);
    break;
  case 0x04:
    if(address < 0x4000400) {
      CPUUpdateRegister(cpu, (address & 0x3FC), value & 0xFFFF);
      CPUUpdateRegister(cpu, (address & 0x3FC) + 2, (value >> 16));
    } else goto unwritable;
    break;
  case 0x05:
#ifdef BKPT_SUPPORT
    if(*((u32 *)&freezePRAM[address & 0x3fc]))
      cheatsWriteMemory(address & 0x70003FC,
      value);
    else
#endif
      WRITE32LE(((u32 *)&paletteRAM[address & 0x3FC]), value);
    break;
  case 0x06:
    address = (address & 0x1fffc);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;

#ifdef BKPT_SUPPORT
    if(*((u32 *)&freezeVRAM[address]))
      cheatsWriteMemory(address + 0x06000000, value);
    else
#endif

      WRITE32LE(((u32 *)&vram[address]), value);
    break;
  case 0x07:
#ifdef BKPT_SUPPORT
    if(*((u32 *)&freezeOAM[address & 0x3fc]))
      cheatsWriteMemory(address & 0x70003FC,
      value);
    else
#endif
      WRITE32LE(((u32 *)&oam[address & 0x3fc]), value);
      //oamUpdated = 1;
    break;
  case 0x0D:
    if(cpuEEPROMEnabled) {
      eepromWrite(address, value, gGba.dma.cpuDmaCount);
      break;
    }
    goto unwritable;
  case 0x0E:
    if((!eepromInUse) | cpuSramEnabled | cpuFlashEnabled) {
      (*cpuSaveGameFunc)(address, (u8)value);
      break;
    }
    // default
  default:
unwritable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal word write: %08x to %08x from %08x\n",
        value,
        address,
        armMode ? armNextPC - 4 : armNextPC - 2);
    }
#endif
    break;
  }
}

static inline void CPUWriteHalfWord(ARM7TDMI &cpu, u32 address, u16 value)
{
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;
#ifdef GBA_LOGGING
  if(address & 1) {
    if(systemVerbose & VERBOSE_UNALIGNED_MEMORY) {
      log("Unaligned halfword write: %04x to %08x from %08x\n",
        value,
        address,
        armMode ? armNextPC - 4 : armNextPC - 2);
    }
  }
#endif

  switch(address >> 24) {
  case 2:
#ifdef BKPT_SUPPORT
    if(*((u16 *)&freezeWorkRAM[address & 0x3FFFE]))
      cheatsWriteHalfWord(address & 0x203FFFE,
      value);
    else
#endif
      WRITE16LE(((u16 *)&workRAM[address & 0x3FFFE]),value);
    break;
  case 3:
#ifdef BKPT_SUPPORT
    if(*((u16 *)&freezeInternalRAM[address & 0x7ffe]))
      cheatsWriteHalfWord(address & 0x3007ffe,
      value);
    else
#endif
      WRITE16LE(((u16 *)&internalRAM[address & 0x7ffe]), value);
    break;
  case 4:
    if(address < 0x4000400)
      CPUUpdateRegister(cpu, address & 0x3fe, value);
    else goto unwritable;
    break;
  case 5:
#ifdef BKPT_SUPPORT
    if(*((u16 *)&freezePRAM[address & 0x03fe]))
      cheatsWriteHalfWord(address & 0x70003fe,
      value);
    else
#endif
      WRITE16LE(((u16 *)&paletteRAM[address & 0x3fe]), value);
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;
#ifdef BKPT_SUPPORT
    if(*((u16 *)&freezeVRAM[address]))
      cheatsWriteHalfWord(address + 0x06000000,
      value);
    else
#endif
      WRITE16LE(((u16 *)&vram[address]), value);
    break;
  case 7:
#ifdef BKPT_SUPPORT
    if(*((u16 *)&freezeOAM[address & 0x03fe]))
      cheatsWriteHalfWord(address & 0x70003fe,
      value);
    else
#endif
      WRITE16LE(((u16 *)&oam[address & 0x3fe]), value);
      //oamUpdated = 1;
    break;
  case 8:
  case 9:
    if(address == 0x80000c4 || address == 0x80000c6 || address == 0x80000c8) {
      if(!rtcWrite(address, value))
        goto unwritable;
    }
		#ifdef VBAM_USE_AGB_PRINT
    else if(!agbPrintWrite(address, value))
		#endif
    	goto unwritable;
    break;
  case 13:
    if(cpuEEPROMEnabled) {
      eepromWrite(address, (u8)value, gGba.dma.cpuDmaCount);
      break;
    }
    goto unwritable;
  case 14:
    if((!eepromInUse) | cpuSramEnabled | cpuFlashEnabled) {
      (*cpuSaveGameFunc)(address, (u8)value);
      break;
    }
    goto unwritable;
  default:
unwritable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal halfword write: %04x to %08x from %08x\n",
        value,
        address,
        armMode ? armNextPC - 4 : armNextPC - 2);
    }
#endif
    break;
  }
}

static inline void CPUWriteByte(ARM7TDMI &cpu, u32 address, u8 b)
{
	auto &cpuNextEvent = cpu.cpuNextEvent;
	auto &cpuTotalTicks = cpu.cpuTotalTicks;
	auto &holdState = cpu.holdState;
	auto &paletteRAM = gLcd.paletteRAM;
	auto &vram = gLcd.vram;
	auto &oam = gLcd.oam;
  switch(address >> 24) {
  case 2:
#ifdef BKPT_SUPPORT
    if(freezeWorkRAM[address & 0x3FFFF])
      cheatsWriteByte(address & 0x203FFFF, b);
    else
#endif
      workRAM[address & 0x3FFFF] = b;
    break;
  case 3:
#ifdef BKPT_SUPPORT
    if(freezeInternalRAM[address & 0x7fff])
      cheatsWriteByte(address & 0x3007fff, b);
    else
#endif
      internalRAM[address & 0x7fff] = b;
    break;
  case 4:
    if(address < 0x4000400) {
      switch(address & 0x3FF) {
      case 0x60:
      case 0x61:
      case 0x62:
      case 0x63:
      case 0x64:
      case 0x65:
      case 0x68:
      case 0x69:
      case 0x6c:
      case 0x6d:
      case 0x70:
      case 0x71:
      case 0x72:
      case 0x73:
      case 0x74:
      case 0x75:
      case 0x78:
      case 0x79:
      case 0x7c:
      case 0x7d:
      case 0x80:
      case 0x81:
      case 0x84:
      case 0x85:
      case 0x90:
      case 0x91:
      case 0x92:
      case 0x93:
      case 0x94:
      case 0x95:
      case 0x96:
      case 0x97:
      case 0x98:
      case 0x99:
      case 0x9a:
      case 0x9b:
      case 0x9c:
      case 0x9d:
      case 0x9e:
      case 0x9f:
        soundEvent(address&0xFF, b);
        break;
      case 0x301: // HALTCNT, undocumented
        if(b == 0x80)
          gGba.stopState = true;
        holdState = 1;
				#ifdef VBAM_USE_HOLDTYPE
        holdType = -1;
				#endif
        cpuNextEvent = cpuTotalTicks;
        break;
      default: // every other register
        u32 lowerBits = address & 0x3fe;
        if(address & 1) {
          CPUUpdateRegister(cpu, lowerBits, (READ16LE(&ioMem.b[lowerBits]) & 0x00FF) | (b << 8));
        } else {
          CPUUpdateRegister(cpu, lowerBits, (READ16LE(&ioMem.b[lowerBits]) & 0xFF00) | b);
        }
      }
      break;
    } else goto unwritable;
    break;
  case 5:
    // no need to switch
  	*((uint16a *)&paletteRAM[address & 0x3FE]) = (b << 8) | b;
    break;
  case 6:
    address = (address & 0x1fffe);
    if (((ioMem.DISPCNT & 7) >2) && ((address & 0x1C000) == 0x18000))
      return;
    if ((address & 0x18000) == 0x18000)
      address &= 0x17fff;

    // no need to switch
    // byte writes to OBJ VRAM are ignored
    if ((address) < objTilesAddress[((ioMem.DISPCNT&7)+1)>>2])
    {
#ifdef BKPT_SUPPORT
      if(freezeVRAM[address])
        cheatsWriteByte(address + 0x06000000, b);
      else
#endif
      	*((uint16a *)&vram[address]) = (b << 8) | b;
    }
    break;
  case 7:
    // no need to switch
    // byte writes to OAM are ignored
    //    *((u16 *)&oam[address & 0x3FE]) = (b << 8) | b;
    break;
  case 13:
    if(cpuEEPROMEnabled) {
      eepromWrite(address, b, gGba.dma.cpuDmaCount);
      break;
    }
    goto unwritable;
  case 14:
    if ((saveType != 5) && ((!eepromInUse) | cpuSramEnabled | cpuFlashEnabled)) {

      //if(!cpuEEPROMEnabled && (cpuSramEnabled | cpuFlashEnabled)) {

      (*cpuSaveGameFunc)(address, b);
      break;
    }
    // default
  default:
unwritable:
#ifdef GBA_LOGGING
    if(systemVerbose & VERBOSE_ILLEGAL_WRITE) {
      log("Illegal byte write: %02x to %08x from %08x\n",
        b,
        address,
        armMode ? armNextPC - 4 : armNextPC -2 );
    }
#endif
    break;
  }
}

#endif // GBAINLINE_H
