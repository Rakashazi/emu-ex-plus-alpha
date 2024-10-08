#ifndef __MDFN_PCE_HUC6280_H
#define __MDFN_PCE_HUC6280_H

#include <trio/trio.h>

using namespace Mednafen;

namespace MDFN_IEN_PCE
{
class alignas(128) HuC6280
{
	public:

	typedef void (MDFN_FASTCALL *writefunc)(uint32 A, uint8 V);
	typedef uint8 (MDFN_FASTCALL *readfunc)(uint32 A);
	typedef int32 (MDFN_FASTCALL *ehfunc)(const int32 timestamp);

	static constexpr unsigned N_FLAG = 0x80;
	static constexpr unsigned V_FLAG = 0x40;
	static constexpr unsigned T_FLAG = 0x20;
	static constexpr unsigned B_FLAG = 0x10;
	static constexpr unsigned D_FLAG = 0x08;
	static constexpr unsigned I_FLAG = 0x04;
	static constexpr unsigned Z_FLAG = 0x02;
	static constexpr unsigned C_FLAG = 0x01;

	// If emulate_wai is true, then the "0xCB" opcode will be handled by waiting for the next high-level event, NOT 
	// for the IRQ line to be asserted as on a 65816.
	// It's mainly a hack intended for less CPU-intensive HES playback.
	HuC6280() MDFN_COLD;
	~HuC6280() MDFN_COLD;

	void Init(const bool emulate_wai) MDFN_COLD;

	void Reset(void) MDFN_COLD;
	void Power(void) MDFN_COLD;

	static constexpr unsigned IQIRQ1 = 0x002;
	static constexpr unsigned IQIRQ2 = 0x001;
	static constexpr unsigned IQTIMER = 0x004;
	static constexpr unsigned IQRESET = 0x020;

	INLINE void IRQBegin(int w)
	{
	 IRQlow |= w;
	}

	INLINE void IRQEnd(int w)
	{
	 IRQlow &= ~w;
	}

	void TimerSync(void);

	INLINE uint8 GetIODataBuffer(void)
	{
	 return(IODataBuffer);
	}

	INLINE void SetIODataBuffer(uint8 v)
	{
	 IODataBuffer = v;
	}

	uint8 TimerRead(unsigned int address, bool peek = false);
	void TimerWrite(unsigned int address, uint8 V);

	uint8 IRQStatusRead(unsigned int address, bool peek = false);
	void IRQStatusWrite(unsigned int address, uint8 V);

	void StateAction(StateMem *sm, const unsigned load, const bool data_only);

	template<bool DebugMode>
	NO_INLINE void RunSub(void);

	void Run(const bool StepMode = false);

	INLINE void Exit(void)
	{
	 runrunrun = 0;
	}

	INLINE void SyncAndResetTimestamp(uint32 ts_base = 0)
	{
         TimerSync();

	 timer_lastts = ts_base;
	 timestamp = ts_base;
	}

	INLINE bool InBlockMove(void)
	{
	 return(in_block_move);
	}

	void StealCycle(void);
	void StealCycles(const int count);
        void StealMasterCycles(const int count);

	void SetEvent(const int32 cycles) NO_INLINE
	{
	 next_user_event = cycles;
	 CalcNextEvent();
	}

	INLINE void SetEventHandler(ehfunc new_EventHandler)
	{
	 EventHandler = new_EventHandler;
	}

	INLINE uint32 Timestamp(void)
	{
	 return(timestamp);
	}

	//
	// Debugger support methods:
	//
	INLINE void SetCPUHook(bool (*new_CPUHook)(uint32), void (*new_ADDBT)(uint32, uint32, uint32))
	{
	 CPUHook = new_CPUHook;
	 ADDBT = new_ADDBT;
	}

	INLINE void LoadShadow(const HuC6280 &state)
	{
	 //EmulateWAI = state.EmulateWAI;

         PC = state.PC;
         A = state.A;
         X = state.X;
         Y = state.Y;
         S = state.S;
         P = state.P;

         PIMaskCache = state.PIMaskCache;

         MPR[0] = state.MPR[0];
         MPR[1] = state.MPR[1];
         MPR[2] = state.MPR[2];
         MPR[3] = state.MPR[3];
         MPR[4] = state.MPR[4];
         MPR[5] = state.MPR[5];
         MPR[6] = state.MPR[6];
         MPR[7] = state.MPR[7];

         for(int x = 0; x < 9; x++)
          SetMPR(x, MPR[x & 0x7]);

         IRQlow = state.IRQlow;
         IRQSample = state.IRQSample;
         IFlagSample = state.IFlagSample;

         speed = state.speed;
         speed_shift_cache = state.speed_shift_cache;
         timestamp = state.timestamp;

         IRQMask = state.IRQMask;

	//
	//

	 timer_status = 0;

         next_user_event = 0x1FFFFFFF;
	 next_event = 0x1FFFFFFF;

	 //IRQlow = 0;
	 //IRQSample = 0;
	 //IFlagSample = HuC6280::I_FLAG;
	}

	enum
	{
	 GSREG_PC = 0,
	 GSREG_A,
	 GSREG_X,
	 GSREG_Y,
	 GSREG_SP,
	 GSREG_P,
	 GSREG_MPR0,
	 GSREG_MPR1,
	 GSREG_MPR2,
	 GSREG_MPR3,
	 GSREG_MPR4,
	 GSREG_MPR5,
	 GSREG_MPR6,
	 GSREG_MPR7,
	 GSREG_SPD,
	 GSREG_IRQM,
	 GSREG_TIMS,
	 GSREG_TIMV,
	 GSREG_TIML,
	 GSREG_TIMD,
	 GSREG_STAMP
	};

	INLINE uint32 GetRegister(const unsigned int id, char *special = NULL, const uint32 special_len = 0)
	{
	 uint32 value = 0xDEADBEEF;

	 switch(id)
	 {
	  case GSREG_PC:
		value = PC & 0xFFFF;
		break;

	  case GSREG_A:
		value = A;
		break;

	  case GSREG_X:
		value = X;
		break;

	  case GSREG_Y:
		value = Y;
		break;

	  case GSREG_SP:
		value = S;
		break;

	  case GSREG_P:
		value = P;	
		if(special)
		{
	 	trio_snprintf(special, special_len, "N: %d, V: %d, T: %d, D: %d, I: %d, Z: %d, C: %d", (int)(bool)(value & N_FLAG),
	        	(int)(bool)(value & V_FLAG),
		        (int)(bool)(value & T_FLAG),
			(int)(bool)(value & D_FLAG),
			(int)(bool)(value & I_FLAG),
		        (int)(bool)(value & Z_FLAG),
		        (int)(bool)(value & C_FLAG));
		}
		break;

  	case GSREG_SPD:
		value = speed;
		if(special)
		{
		 trio_snprintf(special, special_len, "%s(%s)", speed ? "High" : "Low", speed ? "7.16MHz" : "1.79MHz");
		}
		break;

	  case GSREG_MPR0:
	  case GSREG_MPR1:
	  case GSREG_MPR2:
	  case GSREG_MPR3:
	  case GSREG_MPR4:
	  case GSREG_MPR5:
	  case GSREG_MPR6:
	  case GSREG_MPR7:
		value = MPR[id - GSREG_MPR0];

		if(special)
		{
		 trio_snprintf(special, special_len, "0x%02X * 0x2000 = 0x%06X", value, (uint32)value * 0x2000);
		}
		break;

	  case GSREG_IRQM:
		value = IRQMask ^ 0x7;

		if(special)
		{
		 trio_snprintf(special, special_len, "IRQ2: %s, IRQ1: %s, Timer: %s", (value & IQIRQ2) ? "Disabled" : "Enabled",
				(value & IQIRQ1) ? "Disabled" : "Enabled", (value & IQTIMER) ? "Disabled" : "Enabled");
		}
		break;

	  case GSREG_TIMS:
		value = timer_status;

		if(special)
		{
		 trio_snprintf(special, special_len, "%s", (value & 1) ? "Enabled" : "Disabled");
		}
		break;

	  case GSREG_TIMV:
		value = (uint8)timer_value;
		break;

	  case GSREG_TIML:
		value = timer_load;
		if(special)
		{
		 uint32 meowval = (value + 1) * 1024;
	  	 trio_snprintf(special, special_len, "(%d + 1) * 1024 = %d; 7,159,090.90... Hz / %d = %f Hz", value, meowval, meowval, (double)7159090.909090909091 / meowval);
		}
		break;

	  case GSREG_TIMD:
		value = timer_div;
		break;

	  case GSREG_STAMP:
		value = timestamp;
		break;
	 }
	 return(value);
	}

	//uint32 GetRegister(const unsigned int id, char *special = NULL, const uint32 special_len = 0);
	void SetRegister(const unsigned int id, uint32 value);

        INLINE void PokePhysical(uint32 address, uint8 data, bool hl = false)
        {
	 address &= 0x1FFFFF;

         if(hl)
	 {
	  // FIXME: This is a very evil hack.
	  if(FastMap[address >> 13])
	   FastMap[address >> 13][address & 0x1FFF] = data;
	 }
	 else
          WriteMap[address >> 13](address, data);
        }

        INLINE void PokeLogical(uint16 address, uint8 data, bool hl = false)
        {
         uint8 wmpr = MPR[address >> 13];

         PokePhysical((wmpr << 13) | (address & 0x1FFF), data, hl);
        }

	INLINE uint8 PeekPhysical(uint32 address)
	{
	 address &= 0x1FFFFF;

	 return(ReadMap[address >> 13](address));
	}

	INLINE uint8 PeekLogical(uint16 address)
	{
         uint8 wmpr = MPR[address >> 13];

	 return(PeekPhysical((wmpr << 13) | (address & 0x1FFF)));
	}
	//
	// End Debugger Support Methods
	//

	INLINE void SetFastRead(unsigned int i, uint8 *ptr)
	{
	 assert(i < 0x100);
	 FastMap[i] = ptr;
	}

        INLINE readfunc GetReadHandler(unsigned int i)
	{
	 assert(i < 0x100);
	 return(ReadMap[i]);
	}


	INLINE void SetReadHandler(unsigned int i, readfunc func)
	{
	 assert(i < 0x100);
	 ReadMap[i] = func;
	}

	INLINE void SetWriteHandler(unsigned int i, writefunc func)
	{
	 assert(i < 0x100);
	 WriteMap[i] = func;
	}

	// If external debugging code uses this function, then SetFastRead() must not be used with a pointer other than NULL for it to work
	// properly.
	INLINE uint32 GetLastLogicalReadAddr(void)
	{
	 return(LastLogicalReadAddr);
	}

	INLINE uint32 GetLastLogicalWriteAddr(void)
	{
	 return(LastLogicalWriteAddr);
	}

	private:

        void FlushMPRCache(void);

        INLINE void SetMPR(int i, int v)
        {
         MPR[i] = v;
         FastPageR[i] = FastMap[v] ? ((uintptr_t)FastMap[v] - i * 8192) : 0;
        }


	// Logical
	INLINE uint8 RdMem(unsigned int address)
	{
	 if(FastPageR[address >> 13])
	  return *(uint8*)(FastPageR[address >> 13] + address);

	 LastLogicalReadAddr = address;

	 uint8 wmpr = MPR[address >> 13];
	 return ReadMap[wmpr]((wmpr << 13) | (address & 0x1FFF));
	}

	// Logical
	INLINE uint8 RdOp(unsigned int address)
	{
	 return RdMem(address);
	}

	// Logical
	INLINE void WrMem(unsigned int address, uint8 V)
	{
	 uint8 wmpr = MPR[address >> 13];

	 LastLogicalWriteAddr = address;

	 WriteMap[wmpr]((wmpr << 13) | (address & 0x1FFF), V);
	}

	// Used for ST0, ST1, ST2
	// Must not modify address(upper bit is abused for ST0/ST1/ST2 handling).
	INLINE void WrMemPhysical(uint32 address, uint8 data)
	{
	 WriteMap[(address >> 13) & 0xFF](address, data);
	}

	INLINE void REDOPIMCACHE(void)
	{ 
	 PIMaskCache = (P & I_FLAG) ? 0 : ~0; 
	}

	INLINE void REDOSPEEDCACHE(void)
	{
	 speed_shift_cache = (speed ^ 1) << 1;
	}

	void LastCycle(void);

	INLINE void ADDCYC(int x)
	{
	 int master = (x * 3) << speed_shift_cache;

	 timestamp += master;
	 next_event -= master;
	 next_user_event -= master;

	 if(next_event <= 0)
	  HappySync();
	}

        INLINE void ADDCYC_MASTER(int master)
        {
         timestamp += master;
         next_event -= master;
         next_user_event -= master;

         if(next_event <= 0)
          HappySync();
        }


	void HappySync(void);

	INLINE void CalcNextEvent(void)
	{
	 next_event = timer_div;

	 if(next_event > next_user_event)
	  next_event = next_user_event;
	}

	void X_ZN(const uint8);
	void X_ZNT(const uint8);

	void PUSH(const uint8 V);
	uint8 POP(void);

	template<bool DebugMode>
	void JR(const bool cond, const bool BBRS = false);

	template<bool DebugMode>
	void BBRi(const uint8 val, const unsigned int bitto);

	template<bool DebugMode>
	void BBSi(const uint8 val, const unsigned int bitto);

	private:
	uint32 timestamp;
	int32 next_event;	// Next event, period.  Timer, user, ALIENS ARE INVADING SAVE ME HELP
	int32 next_user_event;
	int32 timer_lastts;
	ehfunc EventHandler;

        uint32 PC;		// Program Counter(16-bit, but as a 32-bit variable for performance reasons)
        uint8 A;		// Accumulator
	uint8 X;		// X Index register
	uint8 Y;		// Y Indes register
	uint8 S;		// Stack Pointer
	uint8 P;		// Processor Flags/Status Register
	uint8 IRQMask;
	uint32 PIMaskCache;	// Will be = 0 if (P & I_FLAG) is set, ~0 if (P & I_FLAG) is clear.
        uint32 IRQlow;          /* Simulated IRQ pin held low(or is it high?).
                                   And other junk hooked on for speed reasons.*/
	int32 IRQSample;
	int32 IFlagSample;
	uint8 MPR[9];		// 8, + 1 for PC overflow from $ffff to $10000

	uint8 lastop;
	uint8 speed;
	uint8 speed_shift_cache;

	uint8 IODataBuffer;

	bool timer_inreload;
	uint8 timer_status;
	int32 timer_value, timer_load;
	int32 timer_div;

	int32 runrunrun;	// Don't change to bool(main possibles values are -1, 0, 1).

	enum
	{
	 IBM_TIA = 1,
	 IBM_TAI = 2,
	 IBM_TDD = 3,
	 IBM_TII = 4,
	 IBM_TIN = 5
	};
	uint32 in_block_move;
	uint32 bmt_alternate;
	uint16 bmt_src, bmt_dest, bmt_length;

	uint32 LastLogicalReadAddr;		// Some helper variables for debugging code(external)
	uint32 LastLogicalWriteAddr;		// to know where the read/write occurred in the 16-bit logical space.

	uintptr_t FastPageR[9];	// Biased fast page read cache for each 8KiB in the 16-bit logical address space
				// (Reloaded on corresponding MPR change)

	uint8 *FastMap[0x100];		// Direct pointers to memory for mapped RAM and ROM for faster reads.
	readfunc ReadMap[0x100];	// Read handler pointers for each 8KiB in the 21-bit physical address space.
	writefunc WriteMap[0x100];	// Write handler pointers for each 8KiB in the 21-bit physical address space.

	bool (*CPUHook)(uint32);
	void (*ADDBT)(uint32, uint32, uint32);

	bool EmulateWAI;		// For speed hacks
};

}
#endif
