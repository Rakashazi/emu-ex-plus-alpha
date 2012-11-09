#include <math.h>
#include <memory.h>
#include <stdlib.h>

#include "GBA.h"
#include "bios.h"
#include "GBAinline.h"
#include "Globals.h"

static const s16 sineTable[256] = {
  (s16)0x0000, (s16)0x0192, (s16)0x0323, (s16)0x04B5, (s16)0x0645, (s16)0x07D5, (s16)0x0964, (s16)0x0AF1,
  (s16)0x0C7C, (s16)0x0E05, (s16)0x0F8C, (s16)0x1111, (s16)0x1294, (s16)0x1413, (s16)0x158F, (s16)0x1708,
  (s16)0x187D, (s16)0x19EF, (s16)0x1B5D, (s16)0x1CC6, (s16)0x1E2B, (s16)0x1F8B, (s16)0x20E7, (s16)0x223D,
  (s16)0x238E, (s16)0x24DA, (s16)0x261F, (s16)0x275F, (s16)0x2899, (s16)0x29CD, (s16)0x2AFA, (s16)0x2C21,
  (s16)0x2D41, (s16)0x2E5A, (s16)0x2F6B, (s16)0x3076, (s16)0x3179, (s16)0x3274, (s16)0x3367, (s16)0x3453,
  (s16)0x3536, (s16)0x3612, (s16)0x36E5, (s16)0x37AF, (s16)0x3871, (s16)0x392A, (s16)0x39DA, (s16)0x3A82,
  (s16)0x3B20, (s16)0x3BB6, (s16)0x3C42, (s16)0x3CC5, (s16)0x3D3E, (s16)0x3DAE, (s16)0x3E14, (s16)0x3E71,
  (s16)0x3EC5, (s16)0x3F0E, (s16)0x3F4E, (s16)0x3F84, (s16)0x3FB1, (s16)0x3FD3, (s16)0x3FEC, (s16)0x3FFB,
  (s16)0x4000, (s16)0x3FFB, (s16)0x3FEC, (s16)0x3FD3, (s16)0x3FB1, (s16)0x3F84, (s16)0x3F4E, (s16)0x3F0E,
  (s16)0x3EC5, (s16)0x3E71, (s16)0x3E14, (s16)0x3DAE, (s16)0x3D3E, (s16)0x3CC5, (s16)0x3C42, (s16)0x3BB6,
  (s16)0x3B20, (s16)0x3A82, (s16)0x39DA, (s16)0x392A, (s16)0x3871, (s16)0x37AF, (s16)0x36E5, (s16)0x3612,
  (s16)0x3536, (s16)0x3453, (s16)0x3367, (s16)0x3274, (s16)0x3179, (s16)0x3076, (s16)0x2F6B, (s16)0x2E5A,
  (s16)0x2D41, (s16)0x2C21, (s16)0x2AFA, (s16)0x29CD, (s16)0x2899, (s16)0x275F, (s16)0x261F, (s16)0x24DA,
  (s16)0x238E, (s16)0x223D, (s16)0x20E7, (s16)0x1F8B, (s16)0x1E2B, (s16)0x1CC6, (s16)0x1B5D, (s16)0x19EF,
  (s16)0x187D, (s16)0x1708, (s16)0x158F, (s16)0x1413, (s16)0x1294, (s16)0x1111, (s16)0x0F8C, (s16)0x0E05,
  (s16)0x0C7C, (s16)0x0AF1, (s16)0x0964, (s16)0x07D5, (s16)0x0645, (s16)0x04B5, (s16)0x0323, (s16)0x0192,
  (s16)0x0000, (s16)0xFE6E, (s16)0xFCDD, (s16)0xFB4B, (s16)0xF9BB, (s16)0xF82B, (s16)0xF69C, (s16)0xF50F,
  (s16)0xF384, (s16)0xF1FB, (s16)0xF074, (s16)0xEEEF, (s16)0xED6C, (s16)0xEBED, (s16)0xEA71, (s16)0xE8F8,
  (s16)0xE783, (s16)0xE611, (s16)0xE4A3, (s16)0xE33A, (s16)0xE1D5, (s16)0xE075, (s16)0xDF19, (s16)0xDDC3,
  (s16)0xDC72, (s16)0xDB26, (s16)0xD9E1, (s16)0xD8A1, (s16)0xD767, (s16)0xD633, (s16)0xD506, (s16)0xD3DF,
  (s16)0xD2BF, (s16)0xD1A6, (s16)0xD095, (s16)0xCF8A, (s16)0xCE87, (s16)0xCD8C, (s16)0xCC99, (s16)0xCBAD,
  (s16)0xCACA, (s16)0xC9EE, (s16)0xC91B, (s16)0xC851, (s16)0xC78F, (s16)0xC6D6, (s16)0xC626, (s16)0xC57E,
  (s16)0xC4E0, (s16)0xC44A, (s16)0xC3BE, (s16)0xC33B, (s16)0xC2C2, (s16)0xC252, (s16)0xC1EC, (s16)0xC18F,
  (s16)0xC13B, (s16)0xC0F2, (s16)0xC0B2, (s16)0xC07C, (s16)0xC04F, (s16)0xC02D, (s16)0xC014, (s16)0xC005,
  (s16)0xC000, (s16)0xC005, (s16)0xC014, (s16)0xC02D, (s16)0xC04F, (s16)0xC07C, (s16)0xC0B2, (s16)0xC0F2,
  (s16)0xC13B, (s16)0xC18F, (s16)0xC1EC, (s16)0xC252, (s16)0xC2C2, (s16)0xC33B, (s16)0xC3BE, (s16)0xC44A,
  (s16)0xC4E0, (s16)0xC57E, (s16)0xC626, (s16)0xC6D6, (s16)0xC78F, (s16)0xC851, (s16)0xC91B, (s16)0xC9EE,
  (s16)0xCACA, (s16)0xCBAD, (s16)0xCC99, (s16)0xCD8C, (s16)0xCE87, (s16)0xCF8A, (s16)0xD095, (s16)0xD1A6,
  (s16)0xD2BF, (s16)0xD3DF, (s16)0xD506, (s16)0xD633, (s16)0xD767, (s16)0xD8A1, (s16)0xD9E1, (s16)0xDB26,
  (s16)0xDC72, (s16)0xDDC3, (s16)0xDF19, (s16)0xE075, (s16)0xE1D5, (s16)0xE33A, (s16)0xE4A3, (s16)0xE611,
  (s16)0xE783, (s16)0xE8F8, (s16)0xEA71, (s16)0xEBED, (s16)0xED6C, (s16)0xEEEF, (s16)0xF074, (s16)0xF1FB,
  (s16)0xF384, (s16)0xF50F, (s16)0xF69C, (s16)0xF82B, (s16)0xF9BB, (s16)0xFB4B, (s16)0xFCDD, (s16)0xFE6E
};

void BIOS_ArcTan(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("ArcTan: %08x (VCOUNT=%2d)\n",
        reg[0].I,
        VCOUNT);
  }
#endif

  s32 a =  -(((s32)(reg[0].I*reg[0].I)) >> 14);
  s32 b = ((0xA9 * a) >> 14) + 0x390;
  b = ((b * a) >> 14) + 0x91C;
  b = ((b * a) >> 14) + 0xFB6;
  b = ((b * a) >> 14) + 0x16AA;
  b = ((b * a) >> 14) + 0x2081;
  b = ((b * a) >> 14) + 0x3651;
  b = ((b * a) >> 14) + 0xA2F9;
  a = ((s32)reg[0].I * b) >> 16;
  reg[0].I = a;

#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("ArcTan: return=%08x\n",
        reg[0].I);
  }
#endif
}

void BIOS_ArcTan2(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("ArcTan2: %08x,%08x (VCOUNT=%2d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  s32 x = reg[0].I;
  s32 y = reg[1].I;
  u32 res = 0;
  if (y == 0) {
    res = ((x>>16) & 0x8000);
  } else {
    if (x == 0) {
      res = ((y>>16) & 0x8000) + 0x4000;
    } else {
		if ((abs(x) > abs(y)) || ((abs(x) == abs(y)) && (!((x<0) && (y<0))))) {
        reg[1].I = x;
        reg[0].I = y << 14;
        BIOS_Div(cpu);
        BIOS_ArcTan(cpu);
        if (x < 0)
          res = 0x8000 + reg[0].I;
        else
          res = (((y>>16) & 0x8000)<<1) + reg[0].I;
      } else {
        reg[0].I = x << 14;
        BIOS_Div(cpu);
        BIOS_ArcTan(cpu);
        res = (0x4000 + ((y>>16) & 0x8000)) - reg[0].I;
      }
    }
  }
  reg[0].I = res;

#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("ArcTan2: return=%08x\n",
        reg[0].I);
  }
#endif
}

void BIOS_BitUnPack(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("BitUnPack: %08x,%08x,%08x (VCOUNT=%2d)\n",
        reg[0].I,
        reg[1].I,
        reg[2].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;
  u32 header = reg[2].I;

  int len = CPUReadHalfWord(cpu, header);
    // check address
  if(((source & 0xe000000) == 0) ||
     ((source + len) & 0xe000000) == 0)
    return;

  int bits = CPUReadByte(cpu, header+2);
  int revbits = 8 - bits;
  // u32 value = 0;
  u32 base = CPUReadMemory(cpu, header+4);
  bool addBase = (base & 0x80000000) ? true : false;
  base &= 0x7fffffff;
  int dataSize = CPUReadByte(cpu, header+3);

  int data = 0;
  int bitwritecount = 0;
  while(1) {
    len -= 1;
    if(len < 0)
      break;
    int mask = 0xff >> revbits;
    u8 b = CPUReadByte(cpu, source);
    source++;
    int bitcount = 0;
    while(1) {
      if(bitcount >= 8)
        break;
      u32 d = b & mask;
      u32 temp = d >> bitcount;
      if(d || addBase) {
        temp += base;
      }
      data |= temp << bitwritecount;
      bitwritecount += dataSize;
      if(bitwritecount >= 32) {
        CPUWriteMemory(cpu, dest, data);
        dest += 4;
        data = 0;
        bitwritecount = 0;
      }
      mask <<= bits;
      bitcount += bits;
    }
  }
}

void BIOS_GetBiosChecksum(ARM7TDMI &cpu)
{
  cpu.reg[0].I=0xBAAE187F;
}

void BIOS_BgAffineSet(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("BgAffineSet: %08x,%08x,%08x (VCOUNT=%2d)\n",
        reg[0].I,
        reg[1].I,
        reg[2].I,
        VCOUNT);
  }
#endif

  u32 src = reg[0].I;
  u32 dest = reg[1].I;
  int num = reg[2].I;

  for(int i = 0; i < num; i++) {
    s32 cx = CPUReadMemory(cpu, src);
    src+=4;
    s32 cy = CPUReadMemory(cpu, src);
    src+=4;
    s16 dispx = CPUReadHalfWord(cpu, src);
    src+=2;
    s16 dispy = CPUReadHalfWord(cpu, src);
    src+=2;
    s16 rx = CPUReadHalfWord(cpu, src);
    src+=2;
    s16 ry = CPUReadHalfWord(cpu, src);
    src+=2;
    u16 theta = CPUReadHalfWord(cpu, src)>>8;
    src+=4; // keep structure alignment
    s32 a = sineTable[(theta+0x40)&255];
    s32 b = sineTable[theta];

    s16 dx =  (rx * a)>>14;
    s16 dmx = (rx * b)>>14;
    s16 dy =  (ry * b)>>14;
    s16 dmy = (ry * a)>>14;

    CPUWriteHalfWord(cpu, dest, dx);
    dest += 2;
    CPUWriteHalfWord(cpu, dest, -dmx);
    dest += 2;
    CPUWriteHalfWord(cpu, dest, dy);
    dest += 2;
    CPUWriteHalfWord(cpu, dest, dmy);
    dest += 2;

    s32 startx = cx - dx * dispx + dmx * dispy;
    s32 starty = cy - dy * dispx - dmy * dispy;

    CPUWriteMemory(cpu, dest, startx);
    dest += 4;
    CPUWriteMemory(cpu, dest, starty);
    dest += 4;
  }
}

void BIOS_CpuSet(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("CpuSet: 0x%08x,0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I, reg[1].I,
        reg[2].I, VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;
  u32 cnt = reg[2].I;

  if(((source & 0xe000000) == 0) ||
     ((source + (((cnt << 11)>>9) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int count = cnt & 0x1FFFFF;

  // 32-bit ?
  if((cnt >> 26) & 1) {
    // needed for 32-bit mode!
    source &= 0xFFFFFFFC;
    dest &= 0xFFFFFFFC;
    // fill ?
    if((cnt >> 24) & 1) {
        u32 value = (source>0x0EFFFFFF ? 0x1CAD1CAD : CPUReadMemory(cpu, source));
      while(count) {
        CPUWriteMemory(cpu, dest, value);
        dest += 4;
        count--;
      }
    } else {
      // copy
      while(count) {
        CPUWriteMemory(cpu, dest, (source>0x0EFFFFFF ? 0x1CAD1CAD : CPUReadMemory(cpu, source)));
        source += 4;
        dest += 4;
        count--;
      }
    }
  } else {
    // 16-bit fill?
    if((cnt >> 24) & 1) {
      u16 value = (source>0x0EFFFFFF ? 0x1CAD : CPUReadHalfWord(cpu, source));
      while(count) {
        CPUWriteHalfWord(cpu, dest, value);
        dest += 2;
        count--;
      }
    } else {
      // copy
      while(count) {
        CPUWriteHalfWord(cpu, dest, (source>0x0EFFFFFF ? 0x1CAD : CPUReadHalfWord(cpu, source)));
        source += 2;
        dest += 2;
        count--;
      }
    }
  }
}

void BIOS_CpuFastSet(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("CpuFastSet: 0x%08x,0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I, reg[1].I,
        reg[2].I, VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;
  u32 cnt = reg[2].I;

  if(((source & 0xe000000) == 0) ||
     ((source + (((cnt << 11)>>9) & 0x1fffff)) & 0xe000000) == 0)
    return;

  // needed for 32-bit mode!
  source &= 0xFFFFFFFC;
  dest &= 0xFFFFFFFC;

  int count = cnt & 0x1FFFFF;

  // fill?
  if((cnt >> 24) & 1) {
    while(count > 0) {
      // BIOS always transfers 32 bytes at a time
      u32 value = (source>0x0EFFFFFF ? 0xBAFFFFFB : CPUReadMemory(cpu, source));
      for(int i = 0; i < 8; i++) {
        CPUWriteMemory(cpu, dest, value);
        dest += 4;
      }
      count -= 8;
    }
  } else {
    // copy
    while(count > 0) {
      // BIOS always transfers 32 bytes at a time
      for(int i = 0; i < 8; i++) {
        CPUWriteMemory(cpu, dest, (source>0x0EFFFFFF ? 0xBAFFFFFB :CPUReadMemory(cpu, source)));
        source += 4;
        dest += 4;
      }
      count -= 8;
    }
  }
}

void BIOS_Diff8bitUnFilterWram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Diff8bitUnFilterWram: 0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I,
        reg[1].I, VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     (((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0))
    return;

  int len = header >> 8;

  u8 data = CPUReadByte(cpu, source++);
  CPUWriteByte(cpu, dest++, data);
  len--;

  while(len > 0) {
    u8 diff = CPUReadByte(cpu, source++);
    data += diff;
    CPUWriteByte(cpu, dest++, data);
    len--;
  }
}

void BIOS_Diff8bitUnFilterVram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Diff8bitUnFilterVram: 0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I,
        reg[1].I, VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int len = header >> 8;

  u8 data = CPUReadByte(cpu, source++);
  u16 writeData = data;
  int shift = 8;
  int bytes = 1;

  while(len >= 2) {
    u8 diff = CPUReadByte(cpu, source++);
    data += diff;
    writeData |= (data << shift);
    bytes++;
    shift += 8;
    if(bytes == 2) {
      CPUWriteHalfWord(cpu, dest, writeData);
      dest += 2;
      len -= 2;
      bytes = 0;
      writeData = 0;
      shift = 0;
    }
  }
}

void BIOS_Diff16bitUnFilter(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Diff16bitUnFilter: 0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I,
        reg[1].I, VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int len = header >> 8;

  u16 data = CPUReadHalfWord(cpu, source);
  source += 2;
  CPUWriteHalfWord(cpu, dest, data);
  dest += 2;
  len -= 2;

  while(len >= 2) {
    u16 diff = CPUReadHalfWord(cpu, source);
    source += 2;
    data += diff;
    CPUWriteHalfWord(cpu, dest, data);
    dest += 2;
    len -= 2;
  }
}

void BIOS_Div(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Div: 0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  int number = reg[0].I;
  int denom = reg[1].I;

  if(denom != 0) {
    reg[0].I = number / denom;
    reg[1].I = number % denom;
    s32 temp = (s32)reg[0].I;
    reg[3].I = temp < 0 ? (u32)-temp : (u32)temp;
  }
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Div: return=0x%08x,0x%08x,0x%08x\n",
        reg[0].I,
        reg[1].I,
        reg[3].I);
  }
#endif
}

void BIOS_DivARM(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("DivARM: 0x%08x, (VCOUNT=%d)\n",
        reg[0].I,
        VCOUNT);
  }
#endif

  u32 temp = reg[0].I;
  reg[0].I = reg[1].I;
  reg[1].I = temp;
  BIOS_Div(cpu);
}

void BIOS_HuffUnComp(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("HuffUnComp: 0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  u8 treeSize = CPUReadByte(cpu, source++);

  u32 treeStart = source;

  source += ((treeSize+1)<<1)-1; // minus because we already skipped one byte

  int len = header >> 8;

  u32 mask = 0x80000000;
  u32 data = CPUReadMemory(cpu, source);
  source += 4;

  int pos = 0;
  u8 rootNode = CPUReadByte(cpu, treeStart);
  u8 currentNode = rootNode;
  bool writeData = false;
  int byteShift = 0;
  int byteCount = 0;
  u32 writeValue = 0;

  if((header & 0x0F) == 8) {
    while(len > 0) {
      // take left
      if(pos == 0)
        pos++;
      else
        pos += (((currentNode & 0x3F)+1)<<1);

      if(data & mask) {
        // right
        if(currentNode & 0x40)
          writeData = true;
        currentNode = CPUReadByte(cpu, treeStart+pos+1);
      } else {
        // left
        if(currentNode & 0x80)
          writeData = true;
        currentNode = CPUReadByte(cpu, treeStart+pos);
      }

      if(writeData) {
        writeValue |= (currentNode << byteShift);
        byteCount++;
        byteShift += 8;

        pos = 0;
        currentNode = rootNode;
        writeData = false;

        if(byteCount == 4) {
          byteCount = 0;
          byteShift = 0;
          CPUWriteMemory(cpu, dest, writeValue);
          writeValue = 0;
          dest += 4;
          len -= 4;
        }
      }
      mask >>= 1;
      if(mask == 0) {
        mask = 0x80000000;
        data = CPUReadMemory(cpu, source);
        source += 4;
      }
    }
  } else {
    int halfLen = 0;
    int value = 0;
    while(len > 0) {
      // take left
      if(pos == 0)
        pos++;
      else
        pos += (((currentNode & 0x3F)+1)<<1);

      if((data & mask)) {
        // right
        if(currentNode & 0x40)
          writeData = true;
        currentNode = CPUReadByte(cpu, treeStart+pos+1);
      } else {
        // left
        if(currentNode & 0x80)
          writeData = true;
        currentNode = CPUReadByte(cpu, treeStart+pos);
      }

      if(writeData) {
        if(halfLen == 0)
          value |= currentNode;
        else
          value |= (currentNode<<4);

        halfLen += 4;
        if(halfLen == 8) {
          writeValue |= (value << byteShift);
          byteCount++;
          byteShift += 8;

          halfLen = 0;
          value = 0;

          if(byteCount == 4) {
            byteCount = 0;
            byteShift = 0;
            CPUWriteMemory(cpu, dest, writeValue);
            dest += 4;
            writeValue = 0;
            len -= 4;
          }
        }
        pos = 0;
        currentNode = rootNode;
        writeData = false;
      }
      mask >>= 1;
      if(mask == 0) {
        mask = 0x80000000;
        data = CPUReadMemory(cpu, source);
        source += 4;
      }
    }
  }
}

void BIOS_LZ77UnCompVram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("LZ77UnCompVram: 0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int byteCount = 0;
  int byteShift = 0;
  u32 writeValue = 0;

  int len = header >> 8;

  while(len > 0) {
    u8 d = CPUReadByte(cpu, source++);

    if(d) {
      for(int i = 0; i < 8; i++) {
        if(d & 0x80) {
          u16 data = CPUReadByte(cpu, source++) << 8;
          data |= CPUReadByte(cpu, source++);
          int length = (data >> 12) + 3;
          int offset = (data & 0x0FFF);
          u32 windowOffset = dest + byteCount - offset - 1;
          for(int i2 = 0; i2 < length; i2++) {
            writeValue |= (CPUReadByte(cpu, windowOffset++) << byteShift);
            byteShift += 8;
            byteCount++;

            if(byteCount == 2) {
              CPUWriteHalfWord(cpu, dest, writeValue);
              dest += 2;
              byteCount = 0;
              byteShift = 0;
              writeValue = 0;
            }
            len--;
            if(len == 0)
              return;
          }
        } else {
          writeValue |= (CPUReadByte(cpu, source++) << byteShift);
          byteShift += 8;
          byteCount++;
          if(byteCount == 2) {
            CPUWriteHalfWord(cpu, dest, writeValue);
            dest += 2;
            byteCount = 0;
            byteShift = 0;
            writeValue = 0;
          }
          len--;
          if(len == 0)
            return;
        }
        d <<= 1;
      }
    } else {
      for(int i = 0; i < 8; i++) {
        writeValue |= (CPUReadByte(cpu, source++) << byteShift);
        byteShift += 8;
        byteCount++;
        if(byteCount == 2) {
          CPUWriteHalfWord(cpu, dest, writeValue);
          dest += 2;
          byteShift = 0;
          byteCount = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return;
      }
    }
  }
}

void BIOS_LZ77UnCompWram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("LZ77UnCompWram: 0x%08x,0x%08x (VCOUNT=%d)\n", reg[0].I, reg[1].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int len = header >> 8;

  while(len > 0) {
    u8 d = CPUReadByte(cpu, source++);

    if(d) {
      for(int i = 0; i < 8; i++) {
        if(d & 0x80) {
          u16 data = CPUReadByte(cpu, source++) << 8;
          data |= CPUReadByte(cpu, source++);
          int length = (data >> 12) + 3;
          int offset = (data & 0x0FFF);
          u32 windowOffset = dest - offset - 1;
          for(int i2 = 0; i2 < length; i2++) {
            CPUWriteByte(cpu, dest++, CPUReadByte(cpu, windowOffset++));
            len--;
            if(len == 0)
              return;
          }
        } else {
          CPUWriteByte(cpu, dest++, CPUReadByte(cpu, source++));
          len--;
          if(len == 0)
            return;
        }
        d <<= 1;
      }
    } else {
      for(int i = 0; i < 8; i++) {
        CPUWriteByte(cpu, dest++, CPUReadByte(cpu, source++));
        len--;
        if(len == 0)
          return;
      }
    }
  }
}

void BIOS_ObjAffineSet(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("ObjAffineSet: 0x%08x,0x%08x,0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        reg[2].I,
        reg[3].I,
        VCOUNT);
  }
#endif

  u32 src = reg[0].I;
  u32 dest = reg[1].I;
  int num = reg[2].I;
  int offset = reg[3].I;

  for(int i = 0; i < num; i++) {
    s16 rx = CPUReadHalfWord(cpu, src);
    src+=2;
    s16 ry = CPUReadHalfWord(cpu, src);
    src+=2;
    u16 theta = CPUReadHalfWord(cpu, src)>>8;
    src+=4; // keep structure alignment

    s32 a = (s32)sineTable[(theta+0x40)&255];
    s32 b = (s32)sineTable[theta];

    s16 dx =  ((s32)rx * a)>>14;
    s16 dmx = ((s32)rx * b)>>14;
    s16 dy =  ((s32)ry * b)>>14;
    s16 dmy = ((s32)ry * a)>>14;

    CPUWriteHalfWord(cpu, dest, dx);
    dest += offset;
    CPUWriteHalfWord(cpu, dest, -dmx);
    dest += offset;
    CPUWriteHalfWord(cpu, dest, dy);
    dest += offset;
    CPUWriteHalfWord(cpu, dest, dmy);
    dest += offset;
  }
}

void BIOS_RegisterRamReset(ARM7TDMI &cpu, u32 flags)
{
  // no need to trace here. this is only called directly from GBA.cpp
  // to emulate bios initialization

  CPUUpdateRegister(cpu, 0x0, 0x80);

  if(flags) {
    if(flags & 0x01) {
      // clear work RAM
      memset(cpu.gba->mem.workRAM, 0, 0x40000);
    }
    if(flags & 0x02) {
      // clear internal RAM
      memset(cpu.gba->mem.internalRAM, 0, 0x7e00); // don't clear 0x7e00-0x7fff
    }
    cpu.gba->lcd.registerRamReset(flags);
    /*if(flags & 0x04) {
      // clear palette RAM
      memset(paletteRAM, 0, 0x400);
    }
    if(flags & 0x08) {
      // clear VRAM
      memset(vram, 0, 0x18000);
    }
    if(flags & 0x10) {
      // clean OAM
      memset(oam, 0, 0x400);
    }*/

    if(flags & 0x80) {
      int i;
      for(i = 0; i < 0x10; i++)
        CPUUpdateRegister(cpu, 0x200+i*2, 0);

      for(i = 0; i < 0xF; i++)
        CPUUpdateRegister(cpu, 0x4+i*2, 0);

      for(i = 0; i < 0x20; i++)
        CPUUpdateRegister(cpu, 0x20+i*2, 0);

      for(i = 0; i < 0x18; i++)
        CPUUpdateRegister(cpu, 0xb0+i*2, 0);

      CPUUpdateRegister(cpu, 0x130, 0);
      CPUUpdateRegister(cpu, 0x20, 0x100);
      CPUUpdateRegister(cpu, 0x30, 0x100);
      CPUUpdateRegister(cpu, 0x26, 0x100);
      CPUUpdateRegister(cpu, 0x36, 0x100);
    }

    if(flags & 0x20) {
      int i;
      for(i = 0; i < 8; i++)
        CPUUpdateRegister(cpu, 0x110+i*2, 0);
      CPUUpdateRegister(cpu, 0x134, 0x8000);
      for(i = 0; i < 7; i++)
        CPUUpdateRegister(cpu, 0x140+i*2, 0);
    }

    if(flags & 0x40) {
      int i;
      CPUWriteByte(cpu, 0x4000084, 0);
      CPUWriteByte(cpu, 0x4000084, 0x80);
      CPUWriteMemory(cpu, 0x4000080, 0x880e0000);
      CPUUpdateRegister(cpu, 0x88, CPUReadHalfWord(cpu, 0x4000088)&0x3ff);
      CPUWriteByte(cpu, 0x4000070, 0x70);
      for(i = 0; i < 8; i++)
        CPUUpdateRegister(cpu, 0x90+i*2, 0);
      CPUWriteByte(cpu, 0x4000070, 0);
      for(i = 0; i < 8; i++)
        CPUUpdateRegister(cpu, 0x90+i*2, 0);
      CPUWriteByte(cpu, 0x4000084, 0);
    }
  }
}

void BIOS_RegisterRamReset(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("RegisterRamReset: 0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        VCOUNT);
  }
#endif

  BIOS_RegisterRamReset(cpu, reg[0].I);
}

void BIOS_RLUnCompVram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("RLUnCompVram: 0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source & 0xFFFFFFFC);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int len = header >> 8;
  int byteCount = 0;
  int byteShift = 0;
  u32 writeValue = 0;

  while(len > 0) {
    u8 d = CPUReadByte(cpu, source++);
    int l = d & 0x7F;
    if(d & 0x80) {
      u8 data = CPUReadByte(cpu, source++);
      l += 3;
      for(int i = 0;i < l; i++) {
        writeValue |= (data << byteShift);
        byteShift += 8;
        byteCount++;

        if(byteCount == 2) {
          CPUWriteHalfWord(cpu, dest, writeValue);
          dest += 2;
          byteCount = 0;
          byteShift = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return;
      }
    } else {
      l++;
      for(int i = 0; i < l; i++) {
        writeValue |= (CPUReadByte(cpu, source++) << byteShift);
        byteShift += 8;
        byteCount++;
        if(byteCount == 2) {
          CPUWriteHalfWord(cpu, dest, writeValue);
          dest += 2;
          byteCount = 0;
          byteShift = 0;
          writeValue = 0;
        }
        len--;
        if(len == 0)
          return;
      }
    }
  }
}

void BIOS_RLUnCompWram(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("RLUnCompWram: 0x%08x,0x%08x (VCOUNT=%d)\n",
        reg[0].I,
        reg[1].I,
        VCOUNT);
  }
#endif

  u32 source = reg[0].I;
  u32 dest = reg[1].I;

  u32 header = CPUReadMemory(cpu, source & 0xFFFFFFFC);
  source += 4;

  if(((source & 0xe000000) == 0) ||
     ((source + ((header >> 8) & 0x1fffff)) & 0xe000000) == 0)
    return;

  int len = header >> 8;

  while(len > 0) {
    u8 d = CPUReadByte(cpu, source++);
    int l = d & 0x7F;
    if(d & 0x80) {
      u8 data = CPUReadByte(cpu, source++);
      l += 3;
      for(int i = 0;i < l; i++) {
        CPUWriteByte(cpu, dest++, data);
        len--;
        if(len == 0)
          return;
      }
    } else {
      l++;
      for(int i = 0; i < l; i++) {
        CPUWriteByte(cpu, dest++,  CPUReadByte(cpu, source++));
        len--;
        if(len == 0)
          return;
      }
    }
  }
}

void BIOS_SoftReset(ARM7TDMI &cpu)
{
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("SoftReset: (VCOUNT=%d)\n", VCOUNT);
  }
#endif

  cpu.softReset(cpu.gba->mem.internalRAM[0x7ffa]);
  memset(&cpu.gba->mem.internalRAM[0x7e00], 0, 0x200);

  /*armState = true;
  armMode = 0x1F;
  armIrqEnable = false;
  C_FLAG = V_FLAG = N_FLAG = Z_FLAG = false;
  reg[13].I = 0x03007F00;
  reg[14].I = 0x00000000;
  reg[16].I = 0x00000000;
  reg[R13_IRQ].I = 0x03007FA0;
  reg[R14_IRQ].I = 0x00000000;
  reg[SPSR_IRQ].I = 0x00000000;
  reg[R13_SVC].I = 0x03007FE0;
  reg[R14_SVC].I = 0x00000000;
  reg[SPSR_SVC].I = 0x00000000;
  u8 b = internalRAM[0x7ffa];

  memset(&internalRAM[0x7e00], 0, 0x200);

  if(b) {
    armNextPC = 0x02000000;
    reg[15].I = 0x02000004;
  } else {
    armNextPC = 0x08000000;
    reg[15].I = 0x08000004;
  }*/
}

void BIOS_Sqrt(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Sqrt: %08x (VCOUNT=%2d)\n",
        reg[0].I,
        VCOUNT);
  }
#endif
  reg[0].I = (u32)sqrt((SysDecimal)reg[0].I);
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("Sqrt: return=%08x\n",
        reg[0].I);
  }
#endif
}

void BIOS_MidiKey2Freq(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("MidiKey2Freq: WaveData=%08x mk=%08x fp=%08x\n",
        reg[0].I,
        reg[1].I,
        reg[2].I);
  }
#endif
  int freq = CPUReadMemory(cpu, reg[0].I+4);
  SysDecimal tmp;
  tmp = ((SysDecimal)(180 - reg[1].I)) - ((SysDecimal)reg[2].I / 256.f);
  tmp = pow((SysDecimal)2.f, tmp / 12.f);
  reg[0].I = (int)((SysDecimal)freq / tmp);

#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("MidiKey2Freq: return %08x\n",
        reg[0].I);
  }
#endif
}

void BIOS_SndDriverJmpTableCopy(ARM7TDMI &cpu)
{
	reg_pair (&reg)[45] = cpu.reg;
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_SWI) {
    log("SndDriverJmpTableCopy: dest=%08x\n",
        reg[0].I);
  }
#endif
  for(int i = 0; i < 0x24; i++) {
    CPUWriteMemory(cpu, reg[0].I, 0x9c);
    reg[0].I += 4;
  }
}
