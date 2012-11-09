#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdarg.h>
#include <string.h>

#include "GBA.h"
#include "GBAcpu.h"
#include "GBAinline.h"
#include "Globals.h"
#include "EEprom.h"
#include "Flash.h"
#include "Sound.h"
#include "Sram.h"
#include "bios.h"
#include "Cheats.h"
#include "../NLS.h"
#include "elf.h"
#include "../Util.h"
#include "../System.h"
#include "agbprint.h"
#ifdef PROFILING
#include "prof/prof.h"
#endif

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#define THUMB_PREFETCH cpu.THUMB_PREFETCH()
#define THUMB_PREFETCH_NEXT cpu.THUMB_PREFETCH_NEXT()
#define ARM_PREFETCH cpu.ARM_PREFETCH()
#define cpuPrefetch cpu.cpuPrefetch
#define N_FLAG cpu.nFlag()
#define C_FLAG cpu.C_FLAG
#define Z_FLAG cpu.zFlag()
#define V_FLAG cpu.V_FLAG
#define armState cpu.armState
#define armNextPC cpu.armNextPC
#define reg cpu.reg
#define busPrefetchCount cpu.busPrefetchCount
#define busPrefetch cpu.busPrefetch
#define busPrefetchEnable cpu.busPrefetchEnable

static inline ATTRS(always_inline) int calcTicksFromOldPC(ARM7TDMI &cpu, u32 oldArmNextPC)
{
	return codeTicksAccessSeq16(cpu, oldArmNextPC) + 1;
}

///////////////////////////////////////////////////////////////////////////

static INSN_REGPARM int thumbUnknownInsn(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
#ifdef GBA_LOGGING
  if(systemVerbose & VERBOSE_UNDEFINED)
    log("Undefined THUMB instruction %04x at %08x\n", opcode, armNextPC-2);
#endif
  cpu.undefinedException(cpu.gba->mem.ioMem);
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

#ifdef BKPT_SUPPORT
static INSN_REGPARM int thumbBreakpoint(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[15].I -= 2;
  armNextPC -= 2;
  dbgSignal(5, opcode & 255);
  return -1;
}
#endif

// Common macros //////////////////////////////////////////////////////////

#ifdef BKPT_SUPPORT
# define THUMB_CONSOLE_OUTPUT(a,b) do {                     \
    if ((opcode == 0x4000) && (reg[0].I == 0xC0DED00D)) {   \
      dbgOutput((a), (b));                                  \
    }                                                       \
} while (0)
# define UPDATE_OLDREG do {                                 \
    if (debugger_last) {                                    \
        snprintf(oldbuffer, sizeof(oldbuffer), "%08X",      \
                 armState ? reg[15].I - 4 : reg[15].I - 2); \
        int i;						    \
        for (i = 0; i < 18; i++) {                          \
            oldreg[i] = reg[i].I;                           \
        }                                                   \
    }                                                       \
} while (0)
#else
# define THUMB_CONSOLE_OUTPUT(a,b)
# define UPDATE_OLDREG
#endif

#define NEG(i) ((i) >> 31)
#define POS(i) ((~(i)) >> 31)

// C core
#ifndef ADDCARRY
static inline ATTRS(always_inline) void ADDCARRY_func(bool &cFlag, const u32 a, const u32 b, const u32 c)
{
	/*cFlag = (NEG(a) & NEG(b)) |
	            (NEG(a) & POS(c)) |
	            (NEG(b) & POS(c));*/
	cFlag = (unsigned)c < (unsigned)a;
}

 #define ADDCARRY(a, b, c) ADDCARRY_func(C_FLAG, a, b, c);
#endif
#ifndef ADDOVERFLOW
static inline ATTRS(always_inline) void ADDOVERFLOW_func(bool &vFlag, const u32 a, const u32 b, const u32 c)
{
	vFlag = (NEG(a) & NEG(b) & POS(c)) |
	            (POS(a) & POS(b) & NEG(c));
}
 #define ADDOVERFLOW(a, b, c) ADDOVERFLOW_func(V_FLAG, a, b, c);
#endif
#ifndef SUBCARRY
static inline ATTRS(always_inline) void SUBCARRY_func(bool &cFlag, const u32 a, const u32 b, const u32 c)
{
	cFlag = (NEG(a) & POS(b)) |
	            (NEG(a) & POS(c)) |
	            (POS(b) & POS(c));
}
 #define SUBCARRY(a, b, c) SUBCARRY_func(C_FLAG, a, b, c);
#endif
#ifndef SUBOVERFLOW
static inline ATTRS(always_inline) void SUBOVERFLOW_func(bool &vFlag, const u32 a, const u32 b, const u32 c)
{
	vFlag = (NEG(a) & POS(b) & POS(c)) |\
      (POS(a) & NEG(b) & NEG(c));
}
 #define SUBOVERFLOW(a, b, c) SUBOVERFLOW_func(V_FLAG, a, b, c);
#endif
#ifndef ADD_RD_RS_RN
static inline ATTRS(always_inline) void ADD_RD_RS_RN_func(ARM7TDMI &cpu, const u32 source, const u32 dest, const u32 N)
{
	u32 lhs = reg[source].I;
	u32 rhs = reg[N].I;
	u32 res = lhs + rhs;
	ADDCARRY(lhs, rhs, res);
	ADDOVERFLOW(lhs, rhs, res);
	cpu.setNZFlagParam(res);
	reg[dest].I = res;
	//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
}
 #define ADD_RD_RS_RN(N) ADD_RD_RS_RN_func(cpu, source, dest, N);
#endif
#ifndef ADD_RD_RS_O3
static inline ATTRS(always_inline) void ADD_RD_RS_O3_func(ARM7TDMI &cpu, const u32 source, const u32 dest, const u32 N)
{
	u32 lhs = reg[source].I;
	u32 rhs = N;
	u32 res = lhs + rhs;
	ADDCARRY(lhs, rhs, res);
	ADDOVERFLOW(lhs, rhs, res);
	cpu.setNZFlagParam(res);
	reg[dest].I = res;
	//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
}
 #define ADD_RD_RS_O3(N) ADD_RD_RS_O3_func(cpu, source, dest, N);
#endif
#ifndef ADD_RD_RS_O3_0
# define ADD_RD_RS_O3_0 ADD_RD_RS_O3
#endif
#ifndef ADD_RN_O8
static inline ATTRS(always_inline) void ADD_RN_O8_func(ARM7TDMI &cpu, const u32 opcode, const u32 d)
{
	u32 lhs = reg[(d)].I;
	u32 rhs = (opcode & 255);
	u32 res = lhs + rhs;
	ADDCARRY(lhs, rhs, res);
	ADDOVERFLOW(lhs, rhs, res);
	cpu.setNZFlagParam(res);
	reg[(d)].I = res;
}
 #define ADD_RN_O8(d) ADD_RN_O8_func(cpu, opcode, d);
//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef CMN_RD_RS
 #define CMN_RD_RS \
   {\
     u32 lhs = reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs + rhs;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
     cpu.setNZFlagParam(res);\
   }
//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef ADC_RD_RS
 #define ADC_RD_RS \
   {\
     u32 lhs = reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs + rhs + (u32)C_FLAG;\
     ADDCARRY(lhs, rhs, res);\
     ADDOVERFLOW(lhs, rhs, res);\
     cpu.setNZFlagParam(res);\
     reg[dest].I = res;\
   }
//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef SUB_RD_RS_RN
 #define SUB_RD_RS_RN(N) \
   {\
     u32 lhs = reg[source].I;\
     u32 rhs = reg[N].I;\
     u32 res = lhs - rhs;\
     cpu.setNZFlagParam(res);\
     reg[dest].I = res;\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef SUB_RD_RS_O3
 #define SUB_RD_RS_O3(N) \
   {\
     u32 lhs = reg[source].I;\
     u32 rhs = N;\
     u32 res = lhs - rhs;\
     cpu.setNZFlagParam(res);\
     reg[dest].I = res;\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
	//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef SUB_RD_RS_O3_0
# define SUB_RD_RS_O3_0 SUB_RD_RS_O3
#endif
#ifndef SUB_RN_O8
 #define SUB_RN_O8(d) \
   {\
     u32 lhs = reg[(d)].I;\
     u32 rhs = (opcode & 255);\
     u32 res = lhs - rhs;\
     cpu.setNZFlagParam(res);\
     reg[(d)].I = res;\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
	//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef MOV_RN_O8
 #define MOV_RN_O8(d) \
   {\
     reg[d].I = opcode & 255;\
     cpu.setNZFlagParam(reg[d].I);\
   }
	//N_FLAG = false;\ Z_FLAG = (reg[d].I ? false : true);
#endif
#ifndef CMP_RN_O8
 #define CMP_RN_O8(d) \
   {\
     u32 lhs = reg[(d)].I;\
     u32 rhs = (opcode & 255);\
     u32 res = lhs - rhs;\
     cpu.setNZFlagParam(res);\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
  //Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef SBC_RD_RS
 #define SBC_RD_RS \
   {\
     u32 lhs = reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs - rhs - !((u32)C_FLAG);\
     reg[dest].I = res;\
     cpu.setNZFlagParam(res);\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
	//Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef LSL_RD_RM_I5
 #define LSL_RD_RM_I5 \
   {\
     C_FLAG = (reg[source].I >> (32 - shift)) & 1;\
     value = reg[source].I << shift;\
   }
#endif
#ifndef LSL_RD_RS
 #define LSL_RD_RS \
   {\
     C_FLAG = (reg[dest].I >> (32 - value)) & 1;\
     value = reg[dest].I << value;\
   }
#endif
#ifndef LSR_RD_RM_I5
 #define LSR_RD_RM_I5 \
   {\
     C_FLAG = (reg[source].I >> (shift - 1)) & 1;\
     value = reg[source].I >> shift;\
   }
#endif
#ifndef LSR_RD_RS
 #define LSR_RD_RS \
   {\
     C_FLAG = (reg[dest].I >> (value - 1)) & 1;\
     value = reg[dest].I >> value;\
   }
#endif
#ifndef ASR_RD_RM_I5
 #define ASR_RD_RM_I5 \
   {\
     C_FLAG = ((s32)reg[source].I >> (int)(shift - 1)) & 1;\
     value = (s32)reg[source].I >> (int)shift;\
   }
#endif
#ifndef ASR_RD_RS
 #define ASR_RD_RS \
   {\
     C_FLAG = ((s32)reg[dest].I >> (int)(value - 1)) & 1;\
     value = (s32)reg[dest].I >> (int)value;\
   }
#endif
#ifndef ROR_RD_RS
 #define ROR_RD_RS \
   {\
     C_FLAG = (reg[dest].I >> (value - 1)) & 1;\
     value = ((reg[dest].I << (32 - value)) |\
              (reg[dest].I >> value));\
   }
#endif
#ifndef NEG_RD_RS
 #define NEG_RD_RS \
   {\
     u32 lhs = reg[source].I;\
     u32 rhs = 0;\
     u32 res = rhs - lhs;\
     reg[dest].I = res;\
     cpu.setNZFlagParam(res);\
     \
     SUBCARRY(rhs, lhs, res);\
     SUBOVERFLOW(rhs, lhs, res);\
   }
		// Z_FLAG = (res == 0) ? true : false;\ N_FLAG = NEG(res) ? true : false;
#endif
#ifndef CMP_RD_RS
 #define CMP_RD_RS \
   {\
     u32 lhs = reg[dest].I;\
     u32 rhs = value;\
     u32 res = lhs - rhs;\
     cpu.setNZFlagParam(res);\
     \
     SUBCARRY(lhs, rhs, res);\
     SUBOVERFLOW(lhs, rhs, res);\
   }
   //Z_FLAG = (res == 0) ? true : false; //N_FLAG = NEG(res) ? true : false;
#endif
#ifndef IMM5_INSN
 #define IMM5_INSN(OP,N) \
  int dest = opcode & 0x07;\
  int source = (opcode >> 3) & 0x07;\
  u32 value;\
  OP(N);\
  cpu.setNZFlagParam(value);\
  reg[dest].I = value;
  //N_FLAG = (value & 0x80000000 ? true : false);\
  //Z_FLAG = (value ? false : true);
 #define IMM5_INSN_0(OP) \
  int dest = opcode & 0x07;\
  int source = (opcode >> 3) & 0x07;\
  u32 value;\
  OP;\
  cpu.setNZFlagParam(value);\
  reg[dest].I = value;
  //N_FLAG = (value & 0x80000000 ? true : false);\
  //Z_FLAG = (value ? false : true);
 #define IMM5_LSL(N) \
  int shift = N;\
  LSL_RD_RM_I5;
 #define IMM5_LSL_0 \
  value = reg[source].I;
 #define IMM5_LSR(N) \
  int shift = N;\
  LSR_RD_RM_I5;
 #define IMM5_LSR_0 \
  C_FLAG = reg[source].I & 0x80000000;\
  value = 0;
 #define IMM5_ASR(N) \
  int shift = N;\
  ASR_RD_RM_I5;
 #define IMM5_ASR_0 \
  if(reg[source].I & 0x80000000) {\
    value = 0xFFFFFFFF;\
    C_FLAG = true;\
  } else {\
    value = 0;\
    C_FLAG = false;\
  }
#endif
#ifndef THREEARG_INSN
 #define THREEARG_INSN(OP,N) \
  int dest = opcode & 0x07;          \
  int source = (opcode >> 3) & 0x07; \
  OP(N);
#endif

// Shift instructions /////////////////////////////////////////////////////

#define DEFINE_IMM5_INSN(OP,BASE) \
  static INSN_REGPARM int thumb##BASE##_00(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN_0(OP##_0); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_01(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 1); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_02(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 2); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_03(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 3); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_04(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 4); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_05(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 5); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_06(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 6); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_07(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 7); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_08(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 8); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_09(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP, 9); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0A(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,10); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0B(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,11); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0C(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,12); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0D(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,13); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0E(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,14); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_0F(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,15); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_10(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,16); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_11(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,17); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_12(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,18); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_13(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,19); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_14(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,20); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_15(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,21); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_16(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,22); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_17(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,23); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_18(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,24); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_19(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,25); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1A(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,26); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1B(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,27); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1C(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,28); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1D(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,29); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1E(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,30); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1F(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { IMM5_INSN(OP,31); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// LSL Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_LSL,00)
// LSR Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_LSR,08)
// ASR Rd, Rm, #Imm 5
DEFINE_IMM5_INSN(IMM5_ASR,10)

// 3-argument ADD/SUB /////////////////////////////////////////////////////

#define DEFINE_REG3_INSN(OP,BASE) \
  static INSN_REGPARM int thumb##BASE##_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,0); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,1); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,2); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,3); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_4(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,4); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_5(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,5); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_6(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,6); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_7(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

#define DEFINE_IMM3_INSN(OP,BASE) \
  static INSN_REGPARM int thumb##BASE##_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP##_0,0); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,1); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,2); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,3); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_4(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,4); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_5(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,5); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_6(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,6); return calcTicksFromOldPC(cpu, oldArmNextPC); } \
  static INSN_REGPARM int thumb##BASE##_7(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { THREEARG_INSN(OP,7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// ADD Rd, Rs, Rn
DEFINE_REG3_INSN(ADD_RD_RS_RN,18)
// SUB Rd, Rs, Rn
DEFINE_REG3_INSN(SUB_RD_RS_RN,1A)
// ADD Rd, Rs, #Offset3
DEFINE_IMM3_INSN(ADD_RD_RS_O3,1C)
// SUB Rd, Rs, #Offset3
DEFINE_IMM3_INSN(SUB_RD_RS_O3,1E)

// MOV/CMP/ADD/SUB immediate //////////////////////////////////////////////

// MOV R0, #Offset8
static INSN_REGPARM int thumb20(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(0); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R1, #Offset8
static INSN_REGPARM int thumb21(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(1); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R2, #Offset8
static INSN_REGPARM int thumb22(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(2); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R3, #Offset8
static INSN_REGPARM int thumb23(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(3); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R4, #Offset8
static INSN_REGPARM int thumb24(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(4); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R5, #Offset8
static INSN_REGPARM int thumb25(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(5); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R6, #Offset8
static INSN_REGPARM int thumb26(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(6); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// MOV R7, #Offset8
static INSN_REGPARM int thumb27(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { MOV_RN_O8(7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// CMP R0, #Offset8
static INSN_REGPARM int thumb28(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(0); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R1, #Offset8
static INSN_REGPARM int thumb29(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(1); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R2, #Offset8
static INSN_REGPARM int thumb2A(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(2); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R3, #Offset8
static INSN_REGPARM int thumb2B(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(3); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R4, #Offset8
static INSN_REGPARM int thumb2C(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(4); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R5, #Offset8
static INSN_REGPARM int thumb2D(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(5); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R6, #Offset8
static INSN_REGPARM int thumb2E(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(6); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// CMP R7, #Offset8
static INSN_REGPARM int thumb2F(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { CMP_RN_O8(7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// ADD R0,#Offset8
static INSN_REGPARM int thumb30(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(0); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R1,#Offset8
static INSN_REGPARM int thumb31(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(1); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R2,#Offset8
static INSN_REGPARM int thumb32(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(2); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R3,#Offset8
static INSN_REGPARM int thumb33(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(3); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R4,#Offset8
static INSN_REGPARM int thumb34(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(4); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R5,#Offset8
static INSN_REGPARM int thumb35(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(5); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R6,#Offset8
static INSN_REGPARM int thumb36(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(6); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// ADD R7,#Offset8
static INSN_REGPARM int thumb37(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { ADD_RN_O8(7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// SUB R0,#Offset8
static INSN_REGPARM int thumb38(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(0); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R1,#Offset8
static INSN_REGPARM int thumb39(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(1); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R2,#Offset8
static INSN_REGPARM int thumb3A(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(2); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R3,#Offset8
static INSN_REGPARM int thumb3B(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(3); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R4,#Offset8
static INSN_REGPARM int thumb3C(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(4); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R5,#Offset8
static INSN_REGPARM int thumb3D(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(5); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R6,#Offset8
static INSN_REGPARM int thumb3E(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(6); return calcTicksFromOldPC(cpu, oldArmNextPC); }
// SUB R7,#Offset8
static INSN_REGPARM int thumb3F(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC) { SUB_RN_O8(7); return calcTicksFromOldPC(cpu, oldArmNextPC); }

// ALU operations /////////////////////////////////////////////////////////

// AND Rd, Rs
static INSN_REGPARM int thumb40_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  reg[dest].I &= reg[(opcode >> 3)&7].I;
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  THUMB_CONSOLE_OUTPUT(NULL, reg[2].I);
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// EOR Rd, Rs
static INSN_REGPARM int thumb40_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  reg[dest].I ^= reg[(opcode >> 3)&7].I;
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// LSL Rd, Rs
static INSN_REGPARM int thumb40_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value == 32) {
      value = 0;
      C_FLAG = (reg[dest].I & 1);
    } else if(value < 32) {
      LSL_RD_RS;
    } else {
      value = 0;
      C_FLAG = false;
    }
    reg[dest].I = value;
  }
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  return codeTicksAccess16(cpu, armNextPC)+2;
}

// LSR Rd, Rs
static INSN_REGPARM int thumb40_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value == 32) {
      value = 0;
      C_FLAG = (reg[dest].I & 0x80000000);
    } else if(value < 32) {
      LSR_RD_RS;
    } else {
      value = 0;
      C_FLAG = false;
    }
    reg[dest].I = value;
  }
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  return codeTicksAccess16(cpu, armNextPC)+2;
}

// ASR Rd, Rs
static INSN_REGPARM int thumb41_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].B.B0;
  if(value) {
    if(value < 32) {
      ASR_RD_RS;
      reg[dest].I = value;
    } else {
      if(reg[dest].I & 0x80000000){
        reg[dest].I = 0xFFFFFFFF;
        C_FLAG = true;
      } else {
        reg[dest].I = 0x00000000;
        C_FLAG = false;
      }
    }
  }
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  return codeTicksAccess16(cpu, armNextPC)+2;
}

// ADC Rd, Rs
static INSN_REGPARM int thumb41_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 0x07;
  u32 value = reg[(opcode >> 3)&7].I;
  ADC_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// SBC Rd, Rs
static INSN_REGPARM int thumb41_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 0x07;
  u32 value = reg[(opcode >> 3)&7].I;
  SBC_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// ROR Rd, Rs
static INSN_REGPARM int thumb41_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].B.B0;

  if(value) {
    value = value & 0x1f;
    if(value == 0) {
      C_FLAG = (reg[dest].I & 0x80000000);
    } else {
      ROR_RD_RS;
      reg[dest].I = value;
    }
  }
  cpu.setNZFlagParam(reg[dest].I);
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  //Z_FLAG = reg[dest].I ? false : true;
  return codeTicksAccess16(cpu, armNextPC)+2;
}

// TST Rd, Rs
static INSN_REGPARM int thumb42_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u32 value = reg[opcode & 7].I & reg[(opcode >> 3) & 7].I;
  cpu.setNZFlagParam(value);
  //N_FLAG = value & 0x80000000 ? true : false;
  //Z_FLAG = value ? false : true;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// NEG Rd, Rs
static INSN_REGPARM int thumb42_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  int source = (opcode >> 3) & 7;
  NEG_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// CMP Rd, Rs
static INSN_REGPARM int thumb42_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].I;
  CMP_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// CMN Rd, Rs
static INSN_REGPARM int thumb42_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[(opcode >> 3)&7].I;
  CMN_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// ORR Rd, Rs
static INSN_REGPARM int thumb43_0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  reg[dest].I |= reg[(opcode >> 3) & 7].I;
  cpu.setNZFlagParam(reg[dest].I);
  //Z_FLAG = reg[dest].I ? false : true;
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// MUL Rd, Rs
static INSN_REGPARM int thumb43_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int clockTicks = 1;
  int dest = opcode & 7;
  u32 rm = reg[dest].I;
  reg[dest].I = reg[(opcode >> 3) & 7].I * rm;
  if (((s32)rm) < 0)
    rm = ~rm;
  if ((rm & 0xFFFFFF00) == 0)
    clockTicks += 0;
  else if ((rm & 0xFFFF0000) == 0)
    clockTicks += 1;
  else if ((rm & 0xFF000000) == 0)
    clockTicks += 2;
  else
    clockTicks += 3;
  busPrefetchCount = (busPrefetchCount<<clockTicks) | (0xFF>>(8-clockTicks));
  clockTicks += codeTicksAccess16(cpu, armNextPC) + 1;
  cpu.setNZFlagParam(reg[dest].I);
  //Z_FLAG = reg[dest].I ? false : true;
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  return clockTicks;
}

// BIC Rd, Rs
static INSN_REGPARM int thumb43_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  reg[dest].I &= (~reg[(opcode >> 3) & 7].I);
  cpu.setNZFlagParam(reg[dest].I);
  //Z_FLAG = reg[dest].I ? false : true;
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// MVN Rd, Rs
static INSN_REGPARM int thumb43_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  reg[dest].I = ~reg[(opcode >> 3) & 7].I;
  cpu.setNZFlagParam(reg[dest].I);
  //Z_FLAG = reg[dest].I ? false : true;
  //N_FLAG = reg[dest].I & 0x80000000 ? true : false;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// High-register instructions and BX //////////////////////////////////////

// ADD Rd, Hs
static INSN_REGPARM int thumb44_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[opcode&7].I += reg[((opcode>>3)&7)+8].I;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// ADD Hd, Rs
static INSN_REGPARM int thumb44_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[(opcode&7)+8].I += reg[(opcode>>3)&7].I;
  if((opcode&7) == 7) {
    reg[15].I &= 0xFFFFFFFE;
    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH;
    return codeTicksAccessSeq16(cpu, armNextPC)*2
        + codeTicksAccess16(cpu, armNextPC) + 3;
  }
  else
  {
  	return calcTicksFromOldPC(cpu, oldArmNextPC);
  }
}

// ADD Hd, Hs
static INSN_REGPARM int thumb44_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[(opcode&7)+8].I += reg[((opcode>>3)&7)+8].I;
  if((opcode&7) == 7) {
    reg[15].I &= 0xFFFFFFFE;
    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH;
    return codeTicksAccessSeq16(cpu, armNextPC)*2
        + codeTicksAccess16(cpu, armNextPC) + 3;
  }
  else
  {
  	return calcTicksFromOldPC(cpu, oldArmNextPC);
  }
}

// CMP Rd, Hs
static INSN_REGPARM int thumb45_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = opcode & 7;
  u32 value = reg[((opcode>>3)&7)+8].I;
  CMP_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// CMP Hd, Rs
static INSN_REGPARM int thumb45_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = (opcode & 7) + 8;
  u32 value = reg[(opcode>>3)&7].I;
  CMP_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// CMP Hd, Hs
static INSN_REGPARM int thumb45_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int dest = (opcode & 7) + 8;
  u32 value = reg[((opcode>>3)&7)+8].I;
  CMP_RD_RS;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// MOV Rd, Hs
static INSN_REGPARM int thumb46_1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[opcode&7].I = reg[((opcode>>3)&7)+8].I;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// MOV Hd, Rs
static INSN_REGPARM int thumb46_2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[(opcode&7)+8].I = reg[(opcode>>3)&7].I;
  if((opcode&7) == 7) {
    UPDATE_OLDREG;
    reg[15].I &= 0xFFFFFFFE;
    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH;
    return codeTicksAccessSeq16(cpu, armNextPC)*2
        + codeTicksAccess16(cpu, armNextPC) + 3;
  }
  else
  {
  	return calcTicksFromOldPC(cpu, oldArmNextPC);
  }
}

// MOV Hd, Hs
static INSN_REGPARM int thumb46_3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  reg[(opcode&7)+8].I = reg[((opcode>>3)&7)+8].I;
  if((opcode&7) == 7) {
    UPDATE_OLDREG;
    reg[15].I &= 0xFFFFFFFE;
    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH;
    return codeTicksAccessSeq16(cpu, armNextPC)*2
        + codeTicksAccess16(cpu, armNextPC) + 3;
  }
  else
  {
  	return calcTicksFromOldPC(cpu, oldArmNextPC);
  }
}


// BX Rs
static INSN_REGPARM int thumb47(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int base = (opcode >> 3) & 15;
  busPrefetchCount=0;
  UPDATE_OLDREG;
  reg[15].I = reg[base].I;
  if(reg[base].I & 1) {
    armState = false;
    reg[15].I &= 0xFFFFFFFE;
    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH;
    return codeTicksAccessSeq16(cpu, armNextPC)
        + codeTicksAccessSeq16(cpu, armNextPC) + codeTicksAccess16(cpu, armNextPC) + 3;
  } else {
    armState = true;
    reg[15].I &= 0xFFFFFFFC;
    armNextPC = reg[15].I;
    reg[15].I += 4;
    ARM_PREFETCH;
    return codeTicksAccessSeq32(cpu, armNextPC)
        + codeTicksAccessSeq32(cpu, armNextPC) + codeTicksAccess32(cpu, armNextPC) + 3;
    /*if(CONFIG_TRIGGER_ARM_STATE_EVENT)
    	cpu.cpuNextEvent = cpu.cpuTotalTicks + clockTicks;*/
  }
}

// Load/store instructions ////////////////////////////////////////////////

// LDR R0~R7,[PC, #Imm]
static INSN_REGPARM int thumb48(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = (reg[15].I & 0xFFFFFFFC) + ((opcode & 0xFF) << 2);
  reg[regist].I = CPUReadMemoryQuick(cpu, address);
  busPrefetchCount=0;
  return 3 + dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// STR Rd, [Rs, Rn]
static INSN_REGPARM int thumb50(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  CPUWriteMemory(cpu, address, reg[opcode & 7].I);
  return dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// STRH Rd, [Rs, Rn]
static INSN_REGPARM int thumb52(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  CPUWriteHalfWord(cpu, address, reg[opcode&7].W.W0);
  return dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// STRB Rd, [Rs, Rn]
static INSN_REGPARM int thumb54(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode >>6)&7].I;
  CPUWriteByte(cpu, address, reg[opcode & 7].B.B0);
  return dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// LDSB Rd, [Rs, Rn]
static INSN_REGPARM int thumb56(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  reg[opcode&7].I = (s8)CPUReadByte(cpu, address);
  return 3 + dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// LDR Rd, [Rs, Rn]
static INSN_REGPARM int thumb58(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  reg[opcode&7].I = CPUReadMemory(cpu, address);
  return 3 + dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// LDRH Rd, [Rs, Rn]
static INSN_REGPARM int thumb5A(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  reg[opcode&7].I = CPUReadHalfWordNoRot(cpu, address);
  return 3 + dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// LDRB Rd, [Rs, Rn]
static INSN_REGPARM int thumb5C(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  reg[opcode&7].I = CPUReadByte(cpu, address);
  return 3 + dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// LDSH Rd, [Rs, Rn]
static INSN_REGPARM int thumb5E(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + reg[(opcode>>6)&7].I;
  reg[opcode&7].I = (s16)CPUReadHalfWordSigned(cpu, address);
  return 3 + dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// STR Rd, [Rs, #Imm]
static INSN_REGPARM int thumb60(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
  CPUWriteMemory(cpu, address, reg[opcode&7].I);
  return dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// LDR Rd, [Rs, #Imm]
static INSN_REGPARM int thumb68(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<2);
  reg[opcode&7].I = CPUReadMemory(cpu, address);
  return 3 + dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// STRB Rd, [Rs, #Imm]
static INSN_REGPARM int thumb70(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31));
  CPUWriteByte(cpu, address, reg[opcode&7].B.B0);
  return dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// LDRB Rd, [Rs, #Imm]
static INSN_REGPARM int thumb78(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31));
  reg[opcode&7].I = CPUReadByte(cpu, address);
  return 3 + dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// STRH Rd, [Rs, #Imm]
static INSN_REGPARM int thumb80(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
  CPUWriteHalfWord(cpu, address, reg[opcode&7].W.W0);
  return dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// LDRH Rd, [Rs, #Imm]
static INSN_REGPARM int thumb88(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[(opcode>>3)&7].I + (((opcode>>6)&31)<<1);
  reg[opcode&7].I = CPUReadHalfWordNoRot(cpu, address);
  return 3 + dataTicksAccess16(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// STR R0~R7, [SP, #Imm]
static INSN_REGPARM int thumb90(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[13].I + ((opcode&255)<<2);
  CPUWriteMemory(cpu, address, reg[regist].I);
  return dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC) + 2;
}

// LDR R0~R7, [SP, #Imm]
static INSN_REGPARM int thumb98(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[13].I + ((opcode&255)<<2);
  reg[regist].I = CPUReadMemoryQuick(cpu, address);
  return 3 + dataTicksAccess32(cpu, address) + codeTicksAccess16(cpu, armNextPC);
}

// PC/stack-related ///////////////////////////////////////////////////////

// ADD R0~R7, PC, Imm
static INSN_REGPARM int thumbA0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  reg[regist].I = (reg[15].I & 0xFFFFFFFC) + ((opcode&255)<<2);
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// ADD R0~R7, SP, Imm
static INSN_REGPARM int thumbA8(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  reg[regist].I = reg[13].I + ((opcode&255)<<2);
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// ADD SP, Imm
static INSN_REGPARM int thumbB0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int offset = (opcode & 127) << 2;
  if(opcode & 0x80)
    offset = -offset;
  reg[13].I += offset;
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// Push and pop ///////////////////////////////////////////////////////////

static inline ATTRS(always_inline) int PUSH_REG_func(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC,
		int &count, u32 &address, const u32 val, const u32 r)
{
	if (opcode & (val)) {
	    CPUWriteMemory(cpu, address, reg[(r)].I);
	    int clockTicks = 0;
	    if (!count) {
	        clockTicks += 1 + dataTicksAccess32(cpu, address);
	    } else {
	        clockTicks += 1 + dataTicksAccessSeq32(cpu, address);
	    }
	    count++;
	    address += 4;
	    return clockTicks;
	  }
	{
		return calcTicksFromOldPC(cpu, oldArmNextPC);
	}
}

#define PUSH_REG(val, r) \
PUSH_REG_func(cpu, opcode, clockTicks, count, address, val, r);

static inline ATTRS(always_inline) int POP_REG_func(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC,
		int &count, u32 &address, const u32 val, const u32 r)
{
	if (opcode & (val)) {
	    reg[(r)].I = CPUReadMemory(cpu, address);
	    int clockTicks = 0;
	    if (!count) {
	        clockTicks += 1 + dataTicksAccess32(cpu, address);
	    } else {
	        clockTicks += 1 + dataTicksAccessSeq32(cpu, address);
	    }
	    count++;
	    address += 4;
	    return clockTicks;
	  }
	else
	{
		return calcTicksFromOldPC(cpu, oldArmNextPC);
	}
}

#define POP_REG(val, r) \
POP_REG_func(cpu, opcode, clockTicks, count, address, val, r);

// PUSH {Rlist}
static INSN_REGPARM int thumbB4(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  int count = 0;
  u32 temp = reg[13].I - 4 * cpuBitsSet[opcode & 0xff];
  u32 address = temp & 0xFFFFFFFC;
  int clockTicks = 0;
  clockTicks += PUSH_REG(1, 0);
  clockTicks += PUSH_REG(2, 1);
  clockTicks += PUSH_REG(4, 2);
  clockTicks += PUSH_REG(8, 3);
  clockTicks += PUSH_REG(16, 4);
  clockTicks += PUSH_REG(32, 5);
  clockTicks += PUSH_REG(64, 6);
  clockTicks += PUSH_REG(128, 7);
  clockTicks += 1 + codeTicksAccess16(cpu, armNextPC);
  reg[13].I = temp;
  return clockTicks;
}

// PUSH {Rlist, LR}
static INSN_REGPARM int thumbB5(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  int count = 0;
  u32 temp = reg[13].I - 4 - 4 * cpuBitsSet[opcode & 0xff];
  u32 address = temp & 0xFFFFFFFC;
  int clockTicks = 0;
  clockTicks += PUSH_REG(1, 0);
  clockTicks += PUSH_REG(2, 1);
  clockTicks += PUSH_REG(4, 2);
  clockTicks += PUSH_REG(8, 3);
  clockTicks += PUSH_REG(16, 4);
  clockTicks += PUSH_REG(32, 5);
  clockTicks += PUSH_REG(64, 6);
  clockTicks += PUSH_REG(128, 7);
  clockTicks += PUSH_REG(256, 14);
  clockTicks += 1 + codeTicksAccess16(cpu, armNextPC);
  reg[13].I = temp;
  return clockTicks;
}

// POP {Rlist}
static INSN_REGPARM int thumbBC(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  int count = 0;
  u32 address = reg[13].I & 0xFFFFFFFC;
  u32 temp = reg[13].I + 4*cpuBitsSet[opcode & 0xFF];
  int clockTicks = 0;
  clockTicks += POP_REG(1, 0);
  clockTicks += POP_REG(2, 1);
  clockTicks += POP_REG(4, 2);
  clockTicks += POP_REG(8, 3);
  clockTicks += POP_REG(16, 4);
  clockTicks += POP_REG(32, 5);
  clockTicks += POP_REG(64, 6);
  clockTicks += POP_REG(128, 7);
  reg[13].I = temp;
  clockTicks = 2 + codeTicksAccess16(cpu, armNextPC);
  return clockTicks;
}

// POP {Rlist, PC}
static INSN_REGPARM int thumbBD(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  int count = 0;
  u32 address = reg[13].I & 0xFFFFFFFC;
  u32 temp = reg[13].I + 4 + 4*cpuBitsSet[opcode & 0xFF];
  int clockTicks = 0;
  clockTicks += POP_REG(1, 0);
  clockTicks += POP_REG(2, 1);
  clockTicks += POP_REG(4, 2);
  clockTicks += POP_REG(8, 3);
  clockTicks += POP_REG(16, 4);
  clockTicks += POP_REG(32, 5);
  clockTicks += POP_REG(64, 6);
  clockTicks += POP_REG(128, 7);
  reg[15].I = (CPUReadMemory(cpu, address) & 0xFFFFFFFE);
  if (!count) {
    clockTicks += 1 + dataTicksAccess32(cpu, address);
  } else {
    clockTicks += 1 + dataTicksAccessSeq32(cpu, address);
  }
  count++;
  armNextPC = reg[15].I;
  reg[15].I += 2;
  reg[13].I = temp;
  THUMB_PREFETCH;
  busPrefetchCount = 0;
  clockTicks += 3 + codeTicksAccess16(cpu, armNextPC) + codeTicksAccess16(cpu, armNextPC);
  return clockTicks;
}

// Load/store multiple ////////////////////////////////////////////////////

// TODO: ticks not actually used from THUMB_STM_REG & HUMB_LDM_REG
static inline ATTRS(always_inline) int THUMB_STM_REG_func(ARM7TDMI &cpu, u32 opcode,
		int &count, u32 &address, const u32 temp, const u32 val, const u32 r, const u32 b)
{
	if(opcode & (val)) {
	    CPUWriteMemory(cpu, address, reg[(r)].I);
	    reg[(b)].I = temp;
	    int clockTicks = 0;
	    if (!count) {
	        clockTicks += 1 + dataTicksAccess32(cpu, address);
	    } else {
	        clockTicks += 1 + dataTicksAccessSeq32(cpu, address);
	    }
	    count++;
	    address += 4;
	    return clockTicks;
	  }
	return 0;
}

#define THUMB_STM_REG(val,r,b) \
THUMB_STM_REG_func(cpu, opcode, count, address, temp, val, r, b);

static inline ATTRS(always_inline) int THUMB_LDM_REG_func(ARM7TDMI &cpu, u32 opcode,
		int &count, u32 &address, const u32 val, const u32 r)
{
	if(opcode & (val)) {
	    reg[(r)].I = CPUReadMemoryNoRot(cpu, address);
	    int clockTicks = 0;
	    if (!count) {
	        clockTicks += 1 + dataTicksAccess32(cpu, address);
	    } else {
	        clockTicks += 1 + dataTicksAccessSeq32(cpu, address);
	    }
	    count++;
	    address += 4;
	    return clockTicks;
	  }
	return 0;
}

#define THUMB_LDM_REG(val,r) \
THUMB_LDM_REG_func(cpu, opcode, count, address, val, r);

// STM R0~7!, {Rlist}
static INSN_REGPARM int thumbC0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[regist].I & 0xFFFFFFFC;
  u32 temp = reg[regist].I + 4*cpuBitsSet[opcode & 0xff];
  int count = 0;
  // store
  THUMB_STM_REG(1, 0, regist);
  THUMB_STM_REG(2, 1, regist);
  THUMB_STM_REG(4, 2, regist);
  THUMB_STM_REG(8, 3, regist);
  THUMB_STM_REG(16, 4, regist);
  THUMB_STM_REG(32, 5, regist);
  THUMB_STM_REG(64, 6, regist);
  THUMB_STM_REG(128, 7, regist);
  return 1 + codeTicksAccess16(cpu, armNextPC);
}

// LDM R0~R7!, {Rlist}
static INSN_REGPARM int thumbC8(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u8 regist = (opcode >> 8) & 7;
  if (busPrefetchCount == 0)
    busPrefetch = busPrefetchEnable;
  u32 address = reg[regist].I & 0xFFFFFFFC;
  u32 temp = reg[regist].I + 4*cpuBitsSet[opcode & 0xFF];
  int count = 0;
  // load
  THUMB_LDM_REG(1, 0);
  THUMB_LDM_REG(2, 1);
  THUMB_LDM_REG(4, 2);
  THUMB_LDM_REG(8, 3);
  THUMB_LDM_REG(16, 4);
  THUMB_LDM_REG(32, 5);
  THUMB_LDM_REG(64, 6);
  THUMB_LDM_REG(128, 7);
  if(!(opcode & (1<<regist)))
    reg[regist].I = temp;
  return 2 + codeTicksAccess16(cpu, armNextPC);
}

// Conditional branches ///////////////////////////////////////////////////

// B
static INSN_REGPARM int thumbBInst(ARM7TDMI &cpu, u32 opcode)
{
  reg[15].I += ((s8)(opcode & 0xFF)) << 1;
  armNextPC = reg[15].I;
  reg[15].I += 2;
  THUMB_PREFETCH;
  int clockTicks = codeTicksAccessSeq16(cpu, armNextPC) + codeTicksAccessSeq16(cpu, armNextPC) +
      codeTicksAccess16(cpu, armNextPC)+3;
  busPrefetchCount=0;
  return clockTicks;
}

// BEQ offset
static INSN_REGPARM int thumbD0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(Z_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BNE offset
static INSN_REGPARM int thumbD1(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!Z_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BCS offset
static INSN_REGPARM int thumbD2(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;

  if(C_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BCC offset
static INSN_REGPARM int thumbD3(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!C_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BMI offset
static INSN_REGPARM int thumbD4(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(N_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BPL offset
static INSN_REGPARM int thumbD5(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!N_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BVS offset
static INSN_REGPARM int thumbD6(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(V_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BVC offset
static INSN_REGPARM int thumbD7(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!V_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BHI offset
static INSN_REGPARM int thumbD8(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(C_FLAG && !Z_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BLS offset
static INSN_REGPARM int thumbD9(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!C_FLAG || Z_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BGE offset
static INSN_REGPARM int thumbDA(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(N_FLAG == V_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BLT offset
static INSN_REGPARM int thumbDB(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(N_FLAG != V_FLAG) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BGT offset
static INSN_REGPARM int thumbDC(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(!Z_FLAG && (N_FLAG == V_FLAG)) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// BLE offset
static INSN_REGPARM int thumbDD(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  UPDATE_OLDREG;
  if(Z_FLAG || (N_FLAG != V_FLAG)) {
  	return thumbBInst(cpu, opcode);
  }
  return calcTicksFromOldPC(cpu, oldArmNextPC);
}

// SWI, B, BL /////////////////////////////////////////////////////////////

// SWI #comment
static INSN_REGPARM int thumbDF(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  u32 address = 0;
  int clockTicks = codeTicksAccessSeq16(cpu, address) + codeTicksAccessSeq16(cpu, address) +
      codeTicksAccess16(cpu, address)+3;
  busPrefetchCount=0;
  CPUSoftwareInterrupt(cpu, opcode & 0xFF);
  return clockTicks;
}

// B offset
static INSN_REGPARM int thumbE0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int offset = (opcode & 0x3FF) << 1;
  if(opcode & 0x0400)
    offset |= 0xFFFFF800;
  reg[15].I += offset;
  armNextPC = reg[15].I;
  reg[15].I += 2;
  THUMB_PREFETCH;
  int clockTicks = codeTicksAccessSeq16(cpu, armNextPC) + codeTicksAccessSeq16(cpu, armNextPC) +
      codeTicksAccess16(cpu, armNextPC) + 3;
  busPrefetchCount=0;
  return clockTicks;
}

// BLL #offset (forward)
static INSN_REGPARM int thumbF0(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int offset = (opcode & 0x7FF);
  reg[14].I = reg[15].I + (offset << 12);
  return codeTicksAccessSeq16(cpu, armNextPC) + 1;
}

// BLL #offset (backward)
static INSN_REGPARM int thumbF4(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int offset = (opcode & 0x7FF);
  reg[14].I = reg[15].I + ((offset << 12) | 0xFF800000);
  return codeTicksAccessSeq16(cpu, armNextPC) + 1;
}

// BLH #offset
static INSN_REGPARM int thumbF8(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC)
{
  int offset = (opcode & 0x7FF);
  u32 temp = reg[15].I-2;
  reg[15].I = (reg[14].I + (offset<<1))&0xFFFFFFFE;
  armNextPC = reg[15].I;
  reg[15].I += 2;
  reg[14].I = temp|1;
  THUMB_PREFETCH;
  int clockTicks = codeTicksAccessSeq16(cpu, armNextPC) +
      codeTicksAccess16(cpu, armNextPC) + codeTicksAccessSeq16(cpu, armNextPC) + 3;
  busPrefetchCount = 0;
  return clockTicks;
}

// Instruction table //////////////////////////////////////////////////////

typedef /*INSN_REGPARM*/ int (*insnfunc_t)(ARM7TDMI &cpu, u32 opcode, u32 oldArmNextPC);
#define thumbUI thumbUnknownInsn
#ifdef BKPT_SUPPORT
 #define thumbBP thumbBreakpoint
#else
 #define thumbBP thumbUnknownInsn
#endif
static const insnfunc_t thumbInsnTable[1024] = {
  thumb00_00,thumb00_01,thumb00_02,thumb00_03,thumb00_04,thumb00_05,thumb00_06,thumb00_07,  // 00
  thumb00_08,thumb00_09,thumb00_0A,thumb00_0B,thumb00_0C,thumb00_0D,thumb00_0E,thumb00_0F,
  thumb00_10,thumb00_11,thumb00_12,thumb00_13,thumb00_14,thumb00_15,thumb00_16,thumb00_17,
  thumb00_18,thumb00_19,thumb00_1A,thumb00_1B,thumb00_1C,thumb00_1D,thumb00_1E,thumb00_1F,
  thumb08_00,thumb08_01,thumb08_02,thumb08_03,thumb08_04,thumb08_05,thumb08_06,thumb08_07,  // 08
  thumb08_08,thumb08_09,thumb08_0A,thumb08_0B,thumb08_0C,thumb08_0D,thumb08_0E,thumb08_0F,
  thumb08_10,thumb08_11,thumb08_12,thumb08_13,thumb08_14,thumb08_15,thumb08_16,thumb08_17,
  thumb08_18,thumb08_19,thumb08_1A,thumb08_1B,thumb08_1C,thumb08_1D,thumb08_1E,thumb08_1F,
  thumb10_00,thumb10_01,thumb10_02,thumb10_03,thumb10_04,thumb10_05,thumb10_06,thumb10_07,  // 10
  thumb10_08,thumb10_09,thumb10_0A,thumb10_0B,thumb10_0C,thumb10_0D,thumb10_0E,thumb10_0F,
  thumb10_10,thumb10_11,thumb10_12,thumb10_13,thumb10_14,thumb10_15,thumb10_16,thumb10_17,
  thumb10_18,thumb10_19,thumb10_1A,thumb10_1B,thumb10_1C,thumb10_1D,thumb10_1E,thumb10_1F,
  thumb18_0,thumb18_1,thumb18_2,thumb18_3,thumb18_4,thumb18_5,thumb18_6,thumb18_7,          // 18
  thumb1A_0,thumb1A_1,thumb1A_2,thumb1A_3,thumb1A_4,thumb1A_5,thumb1A_6,thumb1A_7,
  thumb1C_0,thumb1C_1,thumb1C_2,thumb1C_3,thumb1C_4,thumb1C_5,thumb1C_6,thumb1C_7,
  thumb1E_0,thumb1E_1,thumb1E_2,thumb1E_3,thumb1E_4,thumb1E_5,thumb1E_6,thumb1E_7,
  thumb20,thumb20,thumb20,thumb20,thumb21,thumb21,thumb21,thumb21,  // 20
  thumb22,thumb22,thumb22,thumb22,thumb23,thumb23,thumb23,thumb23,
  thumb24,thumb24,thumb24,thumb24,thumb25,thumb25,thumb25,thumb25,
  thumb26,thumb26,thumb26,thumb26,thumb27,thumb27,thumb27,thumb27,
  thumb28,thumb28,thumb28,thumb28,thumb29,thumb29,thumb29,thumb29,  // 28
  thumb2A,thumb2A,thumb2A,thumb2A,thumb2B,thumb2B,thumb2B,thumb2B,
  thumb2C,thumb2C,thumb2C,thumb2C,thumb2D,thumb2D,thumb2D,thumb2D,
  thumb2E,thumb2E,thumb2E,thumb2E,thumb2F,thumb2F,thumb2F,thumb2F,
  thumb30,thumb30,thumb30,thumb30,thumb31,thumb31,thumb31,thumb31,  // 30
  thumb32,thumb32,thumb32,thumb32,thumb33,thumb33,thumb33,thumb33,
  thumb34,thumb34,thumb34,thumb34,thumb35,thumb35,thumb35,thumb35,
  thumb36,thumb36,thumb36,thumb36,thumb37,thumb37,thumb37,thumb37,
  thumb38,thumb38,thumb38,thumb38,thumb39,thumb39,thumb39,thumb39,  // 38
  thumb3A,thumb3A,thumb3A,thumb3A,thumb3B,thumb3B,thumb3B,thumb3B,
  thumb3C,thumb3C,thumb3C,thumb3C,thumb3D,thumb3D,thumb3D,thumb3D,
  thumb3E,thumb3E,thumb3E,thumb3E,thumb3F,thumb3F,thumb3F,thumb3F,
  thumb40_0,thumb40_1,thumb40_2,thumb40_3,thumb41_0,thumb41_1,thumb41_2,thumb41_3,  // 40
  thumb42_0,thumb42_1,thumb42_2,thumb42_3,thumb43_0,thumb43_1,thumb43_2,thumb43_3,
  thumbUI,thumb44_1,thumb44_2,thumb44_3,thumbUI,thumb45_1,thumb45_2,thumb45_3,
  thumbUI,thumb46_1,thumb46_2,thumb46_3,thumb47,thumb47,thumbUI,thumbUI,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,  // 48
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,thumb48,
  thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,thumb50,  // 50
  thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,thumb52,
  thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,thumb54,
  thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,thumb56,
  thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,thumb58,  // 58
  thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,thumb5A,
  thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,thumb5C,
  thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,thumb5E,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,  // 60
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,thumb60,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,  // 68
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,thumb68,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,  // 70
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,thumb70,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,  // 78
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,thumb78,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,  // 80
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,thumb80,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,  // 88
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,thumb88,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,  // 90
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,thumb90,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,  // 98
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,thumb98,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,  // A0
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,thumbA0,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,  // A8
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,thumbA8,
  thumbB0,thumbB0,thumbB0,thumbB0,thumbUI,thumbUI,thumbUI,thumbUI,  // B0
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbB4,thumbB4,thumbB4,thumbB4,thumbB5,thumbB5,thumbB5,thumbB5,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // B8
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbBC,thumbBC,thumbBC,thumbBC,thumbBD,thumbBD,thumbBD,thumbBD,
  thumbBP,thumbBP,thumbBP,thumbBP,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,  // C0
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,thumbC0,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,  // C8
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,thumbC8,
  thumbD0,thumbD0,thumbD0,thumbD0,thumbD1,thumbD1,thumbD1,thumbD1,  // D0
  thumbD2,thumbD2,thumbD2,thumbD2,thumbD3,thumbD3,thumbD3,thumbD3,
  thumbD4,thumbD4,thumbD4,thumbD4,thumbD5,thumbD5,thumbD5,thumbD5,
  thumbD6,thumbD6,thumbD6,thumbD6,thumbD7,thumbD7,thumbD7,thumbD7,
  thumbD8,thumbD8,thumbD8,thumbD8,thumbD9,thumbD9,thumbD9,thumbD9,  // D8
  thumbDA,thumbDA,thumbDA,thumbDA,thumbDB,thumbDB,thumbDB,thumbDB,
  thumbDC,thumbDC,thumbDC,thumbDC,thumbDD,thumbDD,thumbDD,thumbDD,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbDF,thumbDF,thumbDF,thumbDF,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,  // E0
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,thumbE0,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,  // E8
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,thumbUI,
  thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,  // F0
  thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,thumbF0,
  thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
  thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,thumbF4,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,  // F8
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
  thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,thumbF8,
};

// Wrapper routine (execution loop) ///////////////////////////////////////

int thumbExecute(ARM7TDMI &cpu)
{
	//ARM7TDMI cpu = cpuO;
	int &cpuNextEvent = cpu.cpuNextEvent;
	int &cpuTotalTicks = cpu.cpuTotalTicks;
  do {
	  if( cheatsEnabled ) {
		  cpuMasterCodeCheck(cpu);
	  }

    //if ((armNextPC & 0x0803FFFF) == 0x08020000)
    //    busPrefetchCount=0x100;

    u32 opcode = cpu.prefetchThumbOpcode();

    busPrefetch = false;
    // TODO: check if used
    /*if (busPrefetchCount & 0xFFFFFF00)
      busPrefetchCount = 0x100 | (busPrefetchCount & 0xFF);*/
    u32 oldArmNextPC = armNextPC;
#ifndef FINAL_VERSION
    if(armNextPC == stop) {
      armNextPC++;
    }
#endif

    armNextPC = reg[15].I;
    reg[15].I += 2;
    THUMB_PREFETCH_NEXT;

    int clockTicks = (*thumbInsnTable[opcode>>6])(cpu, opcode, oldArmNextPC);

		#ifdef BKPT_SUPPORT
    if (clockTicks < 0)
    {
    	//cpuO = cpu;
      return 0;
    }
		#endif
    cpuTotalTicks += clockTicks;

  } while (cpuTotalTicks < cpuNextEvent &&
  		(!CONFIG_TRIGGER_ARM_STATE_EVENT && !armState)
  		//&& !cpu.holdState
#ifdef VBAM_USE_SWITICKS
  		&& !cpu.SWITicks
#endif
  		);
  //cpuO = cpu;
  return 1;
}
