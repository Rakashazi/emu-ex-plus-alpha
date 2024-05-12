#ifndef VBAM_CORE_GBA_GBACPU_H_
#define VBAM_CORE_GBA_GBACPU_H_

#include <cstdint>

#include "core/base/system.h"
#include "core/gba/gbaCheats.h"
#include "core/gba/gbaGlobals.h"

extern int armExecute(ARM7TDMI &cpu) __attribute__((hot));
extern int thumbExecute(ARM7TDMI &cpu) __attribute__((hot));

#if defined(__i386__) || defined(__x86_64__)
#define INSN_REGPARM __attribute__((regparm(1)))
#else
#define INSN_REGPARM /*nothing*/
#endif

#ifdef __GNUC__
#define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

constexpr bool CONFIG_TRIGGER_ARM_STATE_EVENT = 0;

static void UPDATE_REG(auto *gba, auto address, auto value)
{
  WRITE16LE(((uint16_t*)&(gba)->mem.ioMem.b[address]), value);
}

extern uint32_t mastercode;
extern void CPUSoftwareInterrupt(ARM7TDMI &cpu, int comment);

#define busPrefetchCount cpu.busPrefetchCount
#define busPrefetch cpu.busPrefetch
#define memoryWait cpu.memoryWait
#define memoryWait32 cpu.memoryWait32
#define memoryWaitSeq cpu.memoryWaitSeq
#define memoryWaitSeq32 cpu.memoryWaitSeq32

// Waitstates when accessing data
inline int dataTicksAccess16(ARM7TDMI &cpu, uint32_t address) // DATA 8/16bits NON SEQ
{
  int addr = (address >> 24) & 15;
  int value = memoryWait[addr];

  if ((addr >= 0x08) || (addr < 0x02)) {
    busPrefetchCount = 0;
    busPrefetch = false;
  } else if (busPrefetch) {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
  }

  return value;
}

inline int dataTicksAccess32(ARM7TDMI &cpu, uint32_t address) // DATA 32bits NON SEQ
{
  int addr = (address >> 24) & 15;
  int value = memoryWait32[addr];

  if ((addr >= 0x08) || (addr < 0x02)) {
    busPrefetchCount = 0;
    busPrefetch = false;
  } else if (busPrefetch) {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq16(ARM7TDMI &cpu, uint32_t address)// DATA 8/16bits SEQ
{
  int addr = (address >> 24) & 15;
  int value = memoryWaitSeq[addr];

  if ((addr >= 0x08) || (addr < 0x02)) {
    busPrefetchCount = 0;
    busPrefetch = false;
  } else if (busPrefetch) {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq32(ARM7TDMI &cpu, uint32_t address)// DATA 32bits SEQ
{
  int addr = (address >> 24) & 15;
  int value =  memoryWaitSeq32[addr];

  if ((addr >= 0x08) || (addr < 0x02)) {
    busPrefetchCount = 0;
    busPrefetch = false;
  } else if (busPrefetch) {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount + 1) << waitState) - 1;
  }

  return value;
}

// Waitstates when executing opcode
inline int codeTicksAccess16(ARM7TDMI &cpu, uint32_t address) // THUMB NON SEQ
{
  int addr = (address >> 24) & 15;

  if ((addr >= 0x08) && (addr <= 0x0D)) {
    if (busPrefetchCount & 0x1) {
      if (busPrefetchCount & 0x2) {
        busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
      return memoryWaitSeq[addr] - 1;
    } else {
      busPrefetchCount = 0;
      return memoryWait[addr];
    }
  } else {
    busPrefetchCount = 0;
    return memoryWait[addr];
  }
}

inline int codeTicksAccess32(ARM7TDMI &cpu, uint32_t address) // ARM NON SEQ
{
  int addr = (address >> 24) & 15;

  if ((addr >= 0x08) && (addr <= 0x0D)) {
    if (busPrefetchCount & 0x1) {
      if (busPrefetchCount & 0x2) {
        busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
      return memoryWaitSeq[addr] - 1;
    } else {
      busPrefetchCount = 0;
      return memoryWait32[addr];
    }
  } else {
    busPrefetchCount = 0;
    return memoryWait32[addr];
  }
}

inline int codeTicksAccessSeq16(ARM7TDMI &cpu, uint32_t address) // THUMB SEQ
{
  int addr = (address >> 24) & 15;

  if ((addr >= 0x08) && (addr <= 0x0D)) {
    if (busPrefetchCount & 0x1) {
      busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
      return 0;
    } else if (busPrefetchCount > 0xFF) {
      busPrefetchCount = 0;
      return memoryWait[addr];
    } else
      return memoryWaitSeq[addr];
  } else {
    busPrefetchCount = 0;
    return memoryWaitSeq[addr];
  }
}

inline int codeTicksAccessSeq32(ARM7TDMI &cpu, uint32_t address) // ARM SEQ
{
  int addr = (address >> 24) & 15;

  if ((addr >= 0x08) && (addr <= 0x0D)) {
    if (busPrefetchCount & 0x1) {
      if (busPrefetchCount & 0x2) {
        busPrefetchCount = ((busPrefetchCount & 0xFF) >> 2) | (busPrefetchCount & 0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount & 0xFF) >> 1) | (busPrefetchCount & 0xFFFFFF00);
      return memoryWaitSeq[addr];
    } else if (busPrefetchCount > 0xFF) {
      busPrefetchCount = 0;
      return memoryWait32[addr];
    } else
      return memoryWaitSeq32[addr];
  } else {
    return memoryWaitSeq32[addr];
  }
}

// Emulates the Cheat System (m) code
inline void cpuMasterCodeCheck(ARM7TDMI &cpu)
{
  if ((mastercode) && (mastercode == cpu.armNextPC)) {
    uint32_t joy = 0;
    if (systemReadJoypads())
      joy = systemReadJoypad(-1);
    uint32_t ext = (joy >> 10);
    cpu.cpuTotalTicks += cheatsCheckKeys(cpu, P1 ^ 0x3FF, ext);
  }
}

#undef busPrefetchCount
#undef busPrefetch
#undef memoryWait
#undef memoryWait32
#undef memoryWaitSeq
#undef memoryWaitSeq32

#endif // VBAM_CORE_GBA_GBACPU_H_
