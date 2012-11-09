#ifndef GBACPU_H
#define GBACPU_H

extern int armExecute(ARM7TDMI &cpu) ATTRS(hot);
extern int thumbExecute(ARM7TDMI &cpu) ATTRS(hot);

#ifdef __GNUC__
/*#ifndef __APPLE__
# define INSN_REGPARM //__attribute__((regparm(1)))
#else
# define INSN_REGPARM //nothing
#endif*/
//ATTRS(always_inline) inline
#define INSN_REGPARM ATTRS(hot) inline
# define LIKELY(x) __builtin_expect(!!(x),1)
# define UNLIKELY(x) __builtin_expect(!!(x),0)
#else
# define INSN_REGPARM /*nothing*/
# define LIKELY(x) (x)
# define UNLIKELY(x) (x)
#endif

static const bool CONFIG_TRIGGER_ARM_STATE_EVENT = 0;

#define UPDATE_REG(gba, address, value)\
  {\
    WRITE16LE(((u16 *)&(gba)->mem.ioMem.b[address]),value);\
  }\


extern u32 mastercode;

extern void CPUSoftwareInterrupt(ARM7TDMI &cpu, int comment);


// Waitstates when accessing data
inline int dataTicksAccess16(ARM7TDMI &cpu, u32 address) // DATA 8/16bits NON SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;
  int value =  cpu.memoryWait[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccess32(ARM7TDMI &cpu, u32 address) // DATA 32bits NON SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;
  int value = cpu.memoryWait32[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq16(ARM7TDMI &cpu, u32 address)// DATA 8/16bits SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;
  int value = cpu.memoryWaitSeq[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}

inline int dataTicksAccessSeq32(ARM7TDMI &cpu, u32 address)// DATA 32bits SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;
  int value =  cpu.memoryWaitSeq32[addr];

  if ((addr>=0x08) || (addr < 0x02))
  {
    busPrefetchCount=0;
    busPrefetch=false;
  }
  else if (busPrefetch)
  {
    int waitState = value;
    if (!waitState)
      waitState = 1;
    busPrefetchCount = ((busPrefetchCount+1)<<waitState) - 1;
  }

  return value;
}


// Waitstates when executing opcode
inline int codeTicksAccess16(ARM7TDMI &cpu, u32 address) // THUMB NON SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      if (busPrefetchCount&0x2)
      {
        busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return cpu.memoryWaitSeq[addr]-1;
    }
    else
    {
      busPrefetchCount=0;
      return cpu.memoryWait[addr];
    }
  }
  else
  {
    busPrefetchCount = 0;
    return cpu.memoryWait[addr];
  }
}

inline int codeTicksAccess32(ARM7TDMI &cpu, u32 address) // ARM NON SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      if (busPrefetchCount&0x2)
      {
        busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return cpu.memoryWaitSeq[addr] - 1;
    }
    else
    {
      busPrefetchCount = 0;
      return cpu.memoryWait32[addr];
    }
  }
  else
  {
    busPrefetchCount = 0;
    return cpu.memoryWait32[addr];
  }
}

inline int codeTicksAccessSeq16(ARM7TDMI &cpu, u32 address) // THUMB SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return 0;
    }
    else
    if (busPrefetchCount>0xFF)
    {
      busPrefetchCount=0;
      return cpu.memoryWait[addr];
    }
    else
      return cpu.memoryWaitSeq[addr];
  }
  else
  {
    busPrefetchCount = 0;
    return cpu.memoryWaitSeq[addr];
  }
}

inline int codeTicksAccessSeq32(ARM7TDMI &cpu, u32 address) // ARM SEQ
{
	u32 &busPrefetchCount = cpu.busPrefetchCount;
	bool &busPrefetch = cpu.busPrefetch;
  int addr = (address>>24)&15;

  if ((addr>=0x08) && (addr<=0x0D))
  {
    if (busPrefetchCount&0x1)
    {
      if (busPrefetchCount&0x2)
      {
        busPrefetchCount = ((busPrefetchCount&0xFF)>>2) | (busPrefetchCount&0xFFFFFF00);
        return 0;
      }
      busPrefetchCount = ((busPrefetchCount&0xFF)>>1) | (busPrefetchCount&0xFFFFFF00);
      return cpu.memoryWaitSeq[addr];
    }
    else
    if (busPrefetchCount>0xFF)
    {
      busPrefetchCount=0;
      return cpu.memoryWait32[addr];
    }
    else
      return cpu.memoryWaitSeq32[addr];
  }
  else
  {
    return cpu.memoryWaitSeq32[addr];
  }
}


// Emulates the Cheat System (m) code
inline void cpuMasterCodeCheck(ARM7TDMI &cpu)
{
  if((mastercode) && (mastercode == cpu.armNextPC))
  {
    u32 joy = 0;
    if(systemReadJoypads())
      joy = systemReadJoypad(-1);
    u32 ext = (joy >> 10);
    cpu.cpuTotalTicks += cheatsCheckKeys(cpu, P1^0x3FF, ext);
  }
}

#endif // GBACPU_H
