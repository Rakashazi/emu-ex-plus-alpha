/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Z80/R800.c,v $
**
** $Revision: 1.40 $
**
** $Date: 2009-07-01 05:00:23 $
**
** Author: Daniel Vik
**
** Description: Emulation of the Z80/R800 processor
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/

#include "R800.h"
#include <stdlib.h>
#include <stdio.h>

typedef void (*Opcode)(R800*);
typedef void (*OpcodeNn)(R800*, UInt16);

static UInt8  ZSXYTable[256];
static UInt8  ZSPXYTable[256];
static UInt8  ZSPHTable[256];
static UInt16 DAATable[0x800];


static void cb(R800* r800);
static void dd(R800* r800);
static void ed(R800* r800);
static void fd(R800* r800);
static void dd_cb(R800* r800);
static void fd_cb(R800* r800);

#define INT_LOW   0
#define INT_EDGE  1
#define INT_HIGH  2

#define DLY_MEM       0
#define DLY_MEMOP     1
#define DLY_MEMPAGE   2
#define DLY_PREIO     3
#define DLY_POSTIO    4
#define DLY_M1        5
#define DLY_XD        6
#define DLY_IM        7
#define DLY_IM2       8
#define DLY_NMI       9
#define DLY_PARALLEL  10
#define DLY_BLOCK     11
#define DLY_ADD8      12
#define DLY_ADD16     13
#define DLY_BIT       14
#define DLY_CALL      15
#define DLY_DJNZ      16
#define DLY_EXSPHL    17
#define DLY_INC       18
#define DLY_INC16     19
#define DLY_INOUT     20
#define DLY_LD        21
#define DLY_LDI       22
#define DLY_MUL8      23
#define DLY_MUL16     24
#define DLY_PUSH      25
#define DLY_RLD       26
#define DLY_RET       27
#define DLY_S1990VDP  28
#define DLY_T9769VDP  29
#define DLY_LDSPHL    30
#define DLY_BITIX     31

#define delayMem(r800)      { r800->systemTime += r800->delay[DLY_MEM];      }
#define delayMemOp(r800)    { r800->systemTime += r800->delay[DLY_MEMOP];    }
#define delayMemPage(r800)  { r800->systemTime += r800->delay[DLY_MEMPAGE];  }
#define delayPreIo(r800)    { r800->systemTime += r800->delay[DLY_PREIO];    }
#define delayPostIo(r800)   { r800->systemTime += r800->delay[DLY_POSTIO];   }
#define delayM1(r800)       { r800->systemTime += r800->delay[DLY_M1];       }
#define delayXD(r800)       { r800->systemTime += r800->delay[DLY_XD];       }
#define delayIm(r800)       { r800->systemTime += r800->delay[DLY_IM];       }
#define delayIm2(r800)      { r800->systemTime += r800->delay[DLY_IM2];      }
#define delayNmi(r800)      { r800->systemTime += r800->delay[DLY_NMI];      }
#define delayParallel(r800) { r800->systemTime += r800->delay[DLY_PARALLEL]; }
#define delayBlock(r800)    { r800->systemTime += r800->delay[DLY_BLOCK];    }
#define delayAdd8(r800)     { r800->systemTime += r800->delay[DLY_ADD8];     }
#define delayAdd16(r800)    { r800->systemTime += r800->delay[DLY_ADD16];    }
#define delayBit(r800)      { r800->systemTime += r800->delay[DLY_BIT];      }
#define delayCall(r800)     { r800->systemTime += r800->delay[DLY_CALL];     }
#define delayDjnz(r800)     { r800->systemTime += r800->delay[DLY_DJNZ];     }
#define delayExSpHl(r800)   { r800->systemTime += r800->delay[DLY_EXSPHL];   }
#define delayInc(r800)      { r800->systemTime += r800->delay[DLY_INC];      }
#define delayInc16(r800)    { r800->systemTime += r800->delay[DLY_INC16];    }
#define delayInOut(r800)    { r800->systemTime += r800->delay[DLY_INOUT];    }
#define delayLd(r800)       { r800->systemTime += r800->delay[DLY_LD];       }
#define delayLdi(r800)      { r800->systemTime += r800->delay[DLY_LDI];      }
#define delayMul8(r800)     { r800->systemTime += r800->delay[DLY_MUL8];     }
#define delayMul16(r800)    { r800->systemTime += r800->delay[DLY_MUL16];    }
#define delayPush(r800)     { r800->systemTime += r800->delay[DLY_PUSH];     }
#define delayRet(r800)      { r800->systemTime += r800->delay[DLY_RET];      }
#define delayRld(r800)      { r800->systemTime += r800->delay[DLY_RLD];      }
#define delayT9769(r800)    { r800->systemTime += r800->delay[DLY_T9769VDP]; }
#define delayLdSpHl(r800)   { r800->systemTime += r800->delay[DLY_LDSPHL];   }
#define delayBitIx(r800)    { r800->systemTime += r800->delay[DLY_BITIX];    }

/*
#define delayVdpIO(r800, port) do {                                          \
    if ((port & 0xfffc) == 0x98) {                                           \
        delayT9769(r800);                                                    \
    }                                                                        \
    if ((port & 0xf8) == 0x98) {                                             \
        if (r800->cpuMode == CPU_R800) {                                     \
            if (r800->systemTime - r800->vdpTime < r800->delay[DLY_S1990VDP])\
                r800->systemTime = r800->vdpTime + r800->delay[DLY_S1990VDP];\
            r800->vdpTime = r800->systemTime;                                \
        }                                                                    \
    }                                                                        \
} while (0)
*/

#define delayVdpIO(r800, port) do {                                          \
    if ((port & 0xfc) == 0x98) {                                             \
        delayT9769(r800);                                                    \
    }                                                                        \
    if ((port & 0xf8) == 0x98) {                                             \
        if (r800->cpuMode == CPU_R800) {                                     \
            if (r800->systemTime - r800->vdpTime < r800->delay[DLY_S1990VDP])\
                r800->systemTime = r800->vdpTime + r800->delay[DLY_S1990VDP];\
            r800->vdpTime = r800->systemTime;                                \
        }                                                                    \
    }                                                                        \
} while (0)

static UInt8 readPort(R800* r800, UInt16 port) {
    UInt8 value;

    r800->regs.SH.W = port + 1;
    delayPreIo(r800);

    delayVdpIO(r800, port);

    value = r800->readIoPort(r800->ref, port);
    delayPostIo(r800);

    return value;
}

static void writePort(R800* r800, UInt16 port, UInt8 value) {
    r800->regs.SH.W = port + 1;
    delayPreIo(r800);

    delayVdpIO(r800, port);
    r800->writeIoPort(r800->ref, port, value);
    delayPostIo(r800);

}

static UInt8 readMem(R800* r800, UInt16 address) {
    delayMem(r800);
    r800->cachePage = 0xffff;
    return r800->readMemory(r800->ref, address);
}

static UInt8 readOpcode(R800* r800, UInt16 address) {
    delayMemOp(r800);
    if ((address >> 8) ^ r800->cachePage) {
        r800->cachePage = address >> 8;
        delayMemPage(r800);
    }
    return r800->readMemory(r800->ref, address);
}

static void writeMem(R800* r800, UInt16 address, UInt8 value) {
    delayMem(r800);
    r800->cachePage = 0xffff;
    r800->writeMemory(r800->ref, address, value);
}

static void INC(R800* r800, UInt8* reg) {
    UInt8 regVal = ++(*reg);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | ZSXYTable[regVal] |
        (regVal == 0x80 ? V_FLAG : 0) |
        (!(regVal & 0x0f) ? H_FLAG : 0);
}

static void DEC(R800* r800, UInt8* reg) {
    UInt8 regVal = --(*reg);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | ZSXYTable[regVal] | 
        N_FLAG | (regVal == 0x7f ? V_FLAG : 0) |
        ((regVal & 0x0f) == 0x0f ? H_FLAG : 0);
}

static void ADD(R800* r800, UInt8 reg) {
    int rv = r800->regs.AF.B.h + reg;
    r800->regs.AF.B.l = ZSXYTable[rv & 0xff] | ((rv >> 8) & C_FLAG) |
        ((r800->regs.AF.B.h ^ rv ^ reg) & H_FLAG) |
        ((((reg ^ r800->regs.AF.B.h ^ 0x80) & (reg ^ rv)) >> 5) & V_FLAG);
    r800->regs.AF.B.h = rv;
}

static void ADDW(R800* r800, UInt16* reg1, UInt16 reg2) { //DIFF

    int rv = *reg1 + reg2;

    r800->regs.SH.W   = *reg1 + 1;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | V_FLAG)) |
        (((*reg1 ^ reg2 ^ rv) >> 8) & H_FLAG) |
        ((rv >> 16) & C_FLAG) |
        ((rv >> 8) & (X_FLAG | Y_FLAG));
    *reg1 = rv;
    delayAdd16(r800);
}

static void ADC(R800* r800, UInt8 reg) {
    int rv = r800->regs.AF.B.h + reg + (r800->regs.AF.B.l & C_FLAG);
    r800->regs.AF.B.l = ZSXYTable[rv & 0xff] | ((rv >> 8) & C_FLAG) |
        ((r800->regs.AF.B.h ^ rv ^ reg) & H_FLAG) |
        ((((reg ^ r800->regs.AF.B.h ^ 0x80) & (reg ^ rv)) >> 5) & V_FLAG);
    r800->regs.AF.B.h = rv;
}

static void ADCW(R800* r800, UInt16 reg) {
    int rv = r800->regs.HL.W + reg + (r800->regs.AF.B.l & C_FLAG);

    r800->regs.SH.W   = r800->regs.HL.W + 1;
    r800->regs.AF.B.l = (((r800->regs.HL.W ^ reg ^ rv) >> 8) & H_FLAG) | 
        ((rv >> 16) & C_FLAG) | ((rv & 0xffff) ? 0 : Z_FLAG) |
        ((((reg ^ r800->regs.HL.W ^ 0x8000) & (reg ^ rv)) >> 13) & V_FLAG) |
        ((rv >> 8) & (S_FLAG | X_FLAG | Y_FLAG));
    r800->regs.HL.W = rv;
    delayAdd16(r800);
}

static void SUB(R800* r800, UInt8 reg) {
    int regVal = r800->regs.AF.B.h;
    int rv = regVal - reg;
    r800->regs.AF.B.l = ZSXYTable[rv & 0xff] | ((rv >> 8) & C_FLAG) |
        ((regVal ^ rv ^ reg) & H_FLAG) | N_FLAG |
        ((((reg ^ regVal) & (rv ^ regVal)) >> 5) & V_FLAG);
    r800->regs.AF.B.h = rv;
} 

static void SBC(R800* r800, UInt8 reg) {
    int regVal = r800->regs.AF.B.h;
    int rv = regVal - reg - (r800->regs.AF.B.l & C_FLAG);
    r800->regs.AF.B.l = ZSXYTable[rv & 0xff] | ((rv >> 8) & C_FLAG) |
        ((regVal ^ rv ^ reg) & H_FLAG) | N_FLAG |
        ((((reg ^ regVal) & (rv ^ regVal)) >> 5) & V_FLAG);
    r800->regs.AF.B.h = rv;
}

static void SBCW(R800* r800, UInt16 reg) {
    int regVal = r800->regs.HL.W;
    int rv = regVal - reg - (r800->regs.AF.B.l & C_FLAG);
    r800->regs.SH.W   = regVal + 1;
    r800->regs.AF.B.l = (((regVal ^ reg ^ rv) >> 8) & H_FLAG) | N_FLAG |
        ((rv >> 16) & C_FLAG) | ((rv & 0xffff) ? 0 : Z_FLAG) | 
        ((((reg ^ regVal) & (regVal ^ rv)) >> 13) & V_FLAG) |
        ((rv >> 8) & (S_FLAG | X_FLAG | Y_FLAG));
    r800->regs.HL.W = rv;
    delayAdd16(r800);
}

static void CP(R800* r800, UInt8 reg) {
    int regVal = r800->regs.AF.B.h;
    int rv = regVal - reg;
    r800->regs.AF.B.l = (ZSPXYTable[rv & 0xff] & (Z_FLAG | S_FLAG)) | 
        ((rv >> 8) & C_FLAG) |
        ((regVal ^ rv ^ reg) & H_FLAG) | N_FLAG |
        ((((reg ^ regVal) & (rv ^ regVal)) >> 5) & V_FLAG) |
        (reg & (X_FLAG | Y_FLAG));
}

static void AND(R800* r800, UInt8 reg) {
    r800->regs.AF.B.h &= reg;
    r800->regs.AF.B.l = ZSPXYTable[r800->regs.AF.B.h] | H_FLAG;
} 

static void OR(R800* r800, UInt8 reg) {
    r800->regs.AF.B.h |= reg;
    r800->regs.AF.B.l = ZSPXYTable[r800->regs.AF.B.h];
} 

static void XOR(R800* r800, UInt8 reg) {
    r800->regs.AF.B.h ^= reg;
    r800->regs.AF.B.l = ZSPXYTable[r800->regs.AF.B.h];
}

static void MULU(R800* r800, UInt8 reg) { // Diff on mask // RuMSX: (S_FLAG & V_FLAG)
    r800->regs.HL.W = (Int16)r800->regs.AF.B.h * reg;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (N_FLAG | H_FLAG)) |
        (r800->regs.HL.W ? 0 : Z_FLAG) | ((r800->regs.HL.W >> 15) & C_FLAG);
    delayMul8(r800);
}

static void MULUW(R800* r800, UInt16 reg) { // Diff on mask // RuMSX: (S_FLAG & V_FLAG)
    UInt32 rv = (UInt32)r800->regs.HL.W * reg;
    r800->regs.DE.W = (UInt16)(rv >> 16);
    r800->regs.HL.W = (UInt16)(rv & 0xffff);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (N_FLAG | H_FLAG)) |
        (rv ? 0 : Z_FLAG) | (UInt8)((rv >> 31) & C_FLAG);
    delayMul16(r800);
}

static void SLA(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = regVal << 1;
    r800->regs.AF.B.l = ZSPXYTable[regVal] | ((*reg >> 7) & C_FLAG);
    *reg = regVal;
}

static void SLL(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = (regVal << 1) | 1;
    r800->regs.AF.B.l = ZSPXYTable[regVal] | ((*reg >> 7) & C_FLAG);
    *reg = regVal;
}

static void SRA(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = (regVal >> 1) | (regVal & 0x80);
    r800->regs.AF.B.l = ZSPXYTable[regVal] | (*reg & C_FLAG);
    *reg = regVal;
}

static void SRL(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = regVal >> 1;
    r800->regs.AF.B.l = ZSPXYTable[regVal] | (*reg & C_FLAG);
    *reg = regVal;
}

static void RL(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = (regVal << 1) | (r800->regs.AF.B.l & 0x01);
    r800->regs.AF.B.l = ZSPXYTable[regVal] | ((*reg >> 7) & C_FLAG);
    *reg = regVal;
}

static void RLC(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal= (regVal << 1) | (regVal >> 7);
    r800->regs.AF.B.l = ZSPXYTable[regVal] | (regVal & C_FLAG);
    *reg = regVal;
}

static void RR(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal = (regVal >> 1) | (r800->regs.AF.B.l << 7);
    r800->regs.AF.B.l = ZSPXYTable[regVal] | (*reg & C_FLAG);
    *reg = regVal;
}

static void RRC(R800* r800, UInt8* reg) {
    UInt8 regVal = *reg;
    regVal= (regVal >> 1) | (regVal << 7);
    r800->regs.AF.B.l = ZSPXYTable[regVal] | ((regVal >> 7) & C_FLAG);
    *reg = regVal;
}

static void BIT(R800* r800, UInt8 bit, UInt8 reg) {
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) |
        (reg & (X_FLAG | Y_FLAG)) | ZSPHTable[reg & (1 << bit)];
}

static void RES(R800* r800, UInt8 bit, UInt8* reg) {
    *reg &= ~(1 << bit);
}

static void SET(R800* r800, UInt8 bit, UInt8* reg) {
    *reg |= 1 << bit;
}

static void JR(R800* r800) {
    RegisterPair addr;

    addr.W = r800->regs.PC.W + 1 + (Int8)readOpcode(r800, r800->regs.PC.W);
    r800->regs.PC.W = addr.W;
    r800->regs.SH.W = addr.W;
    delayAdd8(r800);
}

static void SKIP_JR(R800* r800) {
    readOpcode(r800, r800->regs.PC.W++);
}

static void JP(R800* r800) {
    RegisterPair addr;

    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.PC.W = addr.W;
    r800->regs.SH.W = addr.W;
}

static void SKIP_JP(R800* r800) {
    RegisterPair addr;

    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.SH.W = addr.W;
}

static void CALL(R800* r800) {
    RegisterPair addr;

    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    delayCall(r800);
#ifdef ENABLE_CALLSTACK
    r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
    writeMem(r800, --r800->regs.SP.W, r800->regs.PC.B.h);
    writeMem(r800, --r800->regs.SP.W, r800->regs.PC.B.l);
    r800->regs.PC.W = addr.W;
    r800->regs.SH.W = addr.W;
}

static void SKIP_CALL(R800* r800) {
    RegisterPair addr;

    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.SH.W = addr.W;
}

static void RET(R800* r800) { 
    RegisterPair addr;
    addr.B.l = readMem(r800, r800->regs.SP.W++);
    addr.B.h = readMem(r800, r800->regs.SP.W++);
    r800->regs.PC.W = addr.W;
    r800->regs.SH.W = addr.W;
#ifdef ENABLE_CALLSTACK
    if (r800->callstack[(r800->callstackSize - 1) & 0xff] == addr.W) {
        r800->callstackSize--;
    }
#endif
}

static void PUSH(R800* r800, UInt16* reg) {
    RegisterPair* pair = (RegisterPair*)reg;
    delayPush(r800);
    writeMem(r800, --r800->regs.SP.W, pair->B.h);
    writeMem(r800, --r800->regs.SP.W, pair->B.l);
}

static void POP(R800* r800, UInt16* reg) {
    RegisterPair* pair = (RegisterPair*)reg;
    pair->B.l = readMem(r800, r800->regs.SP.W++);
    pair->B.h = readMem(r800, r800->regs.SP.W++);
}

static void RST(R800* r800, UInt16 vector) {
#ifdef ENABLE_CALLSTACK
    r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
    PUSH(r800, &r800->regs.PC.W);
    r800->regs.PC.W = vector;
    r800->regs.SH.W = vector;
}

static void EX_SP(R800* r800, UInt16* reg) {
    RegisterPair* pair = (RegisterPair*)reg;
    RegisterPair addr;

    addr.B.l = readMem(r800, r800->regs.SP.W++);
    addr.B.h = readMem(r800, r800->regs.SP.W);
    writeMem(r800, r800->regs.SP.W--, pair->B.h);
    writeMem(r800, r800->regs.SP.W,   pair->B.l);
    pair->W   = addr.W;
    r800->regs.SH.W = addr.W;
    delayExSpHl(r800);
}

static void M1(R800* r800) { 
    UInt8 value = r800->regs.R;
    r800->regs.R = (value & 0x80) | ((value + 1) & 0x7f); 
    delayM1(r800);
}

static void M1_nodelay(R800* r800) { 
    UInt8 value = r800->regs.R;
    r800->regs.R = (value & 0x80) | ((value + 1) & 0x7f); 
}


static void nop(R800* r800) {
}

static void ld_bc_word(R800* r800) {
    r800->regs.BC.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.BC.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_de_word(R800* r800) {
    r800->regs.DE.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.DE.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_hl_word(R800* r800) {
    r800->regs.HL.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.HL.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_ix_word(R800* r800) {
    r800->regs.IX.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.IX.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_iy_word(R800* r800) {
    r800->regs.IY.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.IY.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_sp_word(R800* r800) {
    r800->regs.SP.B.l = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.SP.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_sp_hl(R800* r800) { 
    delayLdSpHl(r800);                  // white cat append
    r800->regs.SP.W = r800->regs.HL.W; 
}

static void ld_sp_ix(R800* r800) { 
    delayLdSpHl(r800);                  // white cat append
    r800->regs.SP.W = r800->regs.IX.W; 
}

static void ld_sp_iy(R800* r800) { 
    delayLdSpHl(r800);                  // white cat append
    r800->regs.SP.W = r800->regs.IY.W; 
}

static void ld_xbc_a(R800* r800) {
    writeMem(r800, r800->regs.BC.W, r800->regs.AF.B.h);
}

static void ld_xde_a(R800* r800) {
    writeMem(r800, r800->regs.DE.W, r800->regs.AF.B.h);
}

static void ld_xhl_a(R800* r800) {
    writeMem(r800, r800->regs.HL.W, r800->regs.AF.B.h);
}

static void ld_a_xbc(R800* r800) {
    r800->regs.AF.B.h = readMem(r800, r800->regs.BC.W);
}

static void ld_a_xde(R800* r800) {
    r800->regs.AF.B.h = readMem(r800, r800->regs.DE.W);
}

static void ld_xhl_byte(R800* r800) {
    writeMem(r800, r800->regs.HL.W, readOpcode(r800, r800->regs.PC.W++));
}

static void ld_i_a(R800* r800) {
    delayLd(r800);
    r800->regs.I = r800->regs.AF.B.h;
}

static void ld_r_a(R800* r800) {
    delayLd(r800);
    r800->regs.R = r800->regs.R2 = r800->regs.AF.B.h;
}

static void ld_a_i(R800* r800) {
    delayLd(r800);
    r800->regs.AF.B.h = r800->regs.I;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSXYTable[r800->regs.AF.B.h] | (r800->regs.iff2 << 2);
    
    if (r800->cpuMode == CPU_Z80 && ((r800->intState == INT_LOW && r800->regs.iff1) || r800->nmiEdge)) r800->regs.AF.B.l &= 0xfb;
}

static void ld_a_r(R800* r800) {
    delayLd(r800);
    r800->regs.AF.B.h = (r800->regs.R & 0x7f) | (r800->regs.R2 & 0x80);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSXYTable[r800->regs.AF.B.h] | (r800->regs.iff2 << 2);
    
    if (r800->cpuMode == CPU_Z80 && ((r800->intState == INT_LOW && r800->regs.iff1) || r800->nmiEdge)) r800->regs.AF.B.l &= 0xfb;
}

static void inc_bc(R800* r800) {
    r800->regs.BC.W++; delayInc16(r800);
}

static void inc_de(R800* r800) {
    r800->regs.DE.W++; delayInc16(r800);
}

static void inc_hl(R800* r800) {
    r800->regs.HL.W++; delayInc16(r800);
}

static void inc_ix(R800* r800) {
    r800->regs.IX.W++; delayInc16(r800);
}

static void inc_iy(R800* r800) {
    r800->regs.IY.W++; delayInc16(r800);
}

static void inc_sp(R800* r800) {
    r800->regs.SP.W++; delayInc16(r800);
}

static void inc_a(R800* r800) {
    INC(r800, &r800->regs.AF.B.h);
}

static void inc_b(R800* r800) {
    INC(r800, &r800->regs.BC.B.h);
}

static void inc_c(R800* r800) {
    INC(r800, &r800->regs.BC.B.l);
}

static void inc_d(R800* r800) {
    INC(r800, &r800->regs.DE.B.h);
}

static void inc_e(R800* r800) {
    INC(r800, &r800->regs.DE.B.l);
}

static void inc_h(R800* r800) {
    INC(r800, &r800->regs.HL.B.h);
}

static void inc_l(R800* r800) {
    INC(r800, &r800->regs.HL.B.l);
}

static void inc_ixh(R800* r800) { 
    INC(r800, &r800->regs.IX.B.h); 
}

static void inc_ixl(R800* r800) { 
    INC(r800, &r800->regs.IX.B.l); 
}

static void inc_iyh(R800* r800) { 
    INC(r800, &r800->regs.IY.B.h); 
}


static void inc_iyl(R800* r800) { 
    INC(r800, &r800->regs.IY.B.l); 
}

static void inc_xhl(R800* r800) {
    UInt8 value = readMem(r800, r800->regs.HL.W);
    INC(r800, &value);
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, value);
}

static void inc_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value;
    delayAdd8(r800);
    value = readMem(r800, addr);
    INC(r800, &value);
    delayInc(r800);
    writeMem(r800, addr, value);
    r800->regs.SH.W = addr;
}

static void inc_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value;
    delayAdd8(r800);
    value = readMem(r800, addr);
    INC(r800, &value);
    delayInc(r800);
    writeMem(r800, addr, value);
    r800->regs.SH.W = addr;
}

static void dec_bc(R800* r800) {
    r800->regs.BC.W--; delayInc16(r800);
}

static void dec_de(R800* r800) {
    r800->regs.DE.W--; delayInc16(r800);
}

static void dec_hl(R800* r800) {
    r800->regs.HL.W--; delayInc16(r800);
}

static void dec_ix(R800* r800) {
    r800->regs.IX.W--; delayInc16(r800);
}

static void dec_iy(R800* r800) {
    r800->regs.IY.W--; delayInc16(r800);
}

static void dec_sp(R800* r800) {
    r800->regs.SP.W--; delayInc16(r800);
}

static void dec_a(R800* r800) {
    DEC(r800, &r800->regs.AF.B.h);
}

static void dec_b(R800* r800) {
    DEC(r800, &r800->regs.BC.B.h);
}

static void dec_c(R800* r800) {
    DEC(r800, &r800->regs.BC.B.l);
}

static void dec_d(R800* r800) {
    DEC(r800, &r800->regs.DE.B.h);
}

static void dec_e(R800* r800) {
    DEC(r800, &r800->regs.DE.B.l);
}

static void dec_h(R800* r800) {
    DEC(r800, &r800->regs.HL.B.h);
}

static void dec_l(R800* r800) {
    DEC(r800, &r800->regs.HL.B.l);
}

static void dec_ixh(R800* r800) { 
    DEC(r800, &r800->regs.IX.B.h); 
}

static void dec_ixl(R800* r800) { 
    DEC(r800, &r800->regs.IX.B.l); 
}

static void dec_iyh(R800* r800) { 
    DEC(r800, &r800->regs.IY.B.h); 
}

static void dec_iyl(R800* r800) { 
    DEC(r800, &r800->regs.IY.B.l); 
}

static void dec_xhl(R800* r800) {
    UInt8 value = readMem(r800, r800->regs.HL.W);
    DEC(r800, &value);
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, value);
}

static void dec_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value;
    delayAdd8(r800);
    value = readMem(r800, addr);
    DEC(r800, &value);
    delayInc(r800);
    writeMem(r800, addr, value);
    r800->regs.SH.W = addr;
}

static void dec_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value;
    delayAdd8(r800);
    value = readMem(r800, addr);
    DEC(r800, &value);
    delayInc(r800);
    writeMem(r800, addr, value);
    r800->regs.SH.W = addr;
}

static void ld_a_a(R800* r800) { 
}

static void ld_a_b(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.BC.B.h; 
}

static void ld_a_c(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.BC.B.l; 
}

static void ld_a_d(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.DE.B.h; 
}

static void ld_a_e(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.DE.B.l; 
}

static void ld_a_h(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.HL.B.h; 
}

static void ld_a_l(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.HL.B.l; 
}

static void ld_a_ixh(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.IX.B.h; 
}

static void ld_a_ixl(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.IX.B.l; 
}

static void ld_a_iyh(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.IY.B.h; 
}

static void ld_a_iyl(R800* r800) { 
    r800->regs.AF.B.h = r800->regs.IY.B.l; 
}

static void ld_a_xhl(R800* r800) { 
    r800->regs.AF.B.h = readMem(r800, r800->regs.HL.W); 
}

static void ld_a_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.AF.B.h = readMem(r800, addr);
}

static void ld_a_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.AF.B.h = readMem(r800, addr);
}

static void ld_xiy_a(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.AF.B.h);
}

static void ld_xix_a(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.AF.B.h);
}

static void ld_b_a(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.AF.B.h; 
}

static void ld_b_b(R800* r800) { 
#ifdef ENABLE_ASMSX_DEBUG_COMMANDS
    char debugString[256];
    UInt16 addr = r800->regs.PC.W;
    UInt16 bpAddr = 0;
    UInt8  size;
    UInt16 page = 0xffff;
    UInt16 slot = 0xffff;

    if (r800->readMemory(r800->ref, addr) != 24) {
        return;
    }
    addr++;

    size = r800->readMemory(r800->ref, addr++);
    switch (size) {
    case 0:
        bpAddr = addr;
        break;
    case 2:
        bpAddr = r800->readMemory(r800->ref, addr++);
        bpAddr |= r800->readMemory(r800->ref, addr++) << 8;
        break;
    case 3:
        slot = r800->readMemory(r800->ref, addr++);
        bpAddr = r800->readMemory(r800->ref, addr++);
        bpAddr |= r800->readMemory(r800->ref, addr++) << 8;
        break;
    case 4:
        slot = r800->readMemory(r800->ref, addr++);
        page = r800->readMemory(r800->ref, addr++);
        bpAddr = r800->readMemory(r800->ref, addr++);
        bpAddr |= r800->readMemory(r800->ref, addr++) << 8;
        break;
    default:
        return;
    }

    sprintf(debugString, "%.4x %.4x %.4x", slot, page, bpAddr);
    r800->debugCb(r800->ref, ASDBG_SETBP, debugString);
#endif
}

static void ld_b_c(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.BC.B.l; 
}

static void ld_b_d(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.DE.B.h; 
}

static void ld_b_e(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.DE.B.l; 
}

static void ld_b_h(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.HL.B.h; 
}

static void ld_b_l(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.HL.B.l; 
}

static void ld_b_ixh(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.IX.B.h; 
}

static void ld_b_ixl(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.IX.B.l; 
}

static void ld_b_iyh(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.IY.B.h; 
}

static void ld_b_iyl(R800* r800) { 
    r800->regs.BC.B.h = r800->regs.IY.B.l; 
}

static void ld_b_xhl(R800* r800) { 
    r800->regs.BC.B.h = readMem(r800, r800->regs.HL.W); 
}

static void ld_b_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.BC.B.h = readMem(r800, addr);
}

static void ld_b_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.BC.B.h = readMem(r800, addr);
}

static void ld_xix_b(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.BC.B.h);
}

static void ld_xiy_b(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.BC.B.h);
}

static void ld_c_a(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.AF.B.h; 
}

static void ld_c_b(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.BC.B.h; 
}

static void ld_c_c(R800* r800) { 
#ifdef ENABLE_TRAP_CALLBACK
    UInt16 addr = r800->regs.PC.W;
    UInt8  value;

    if (r800->readMemory(r800->ref, addr) != 24) {
        return;
    }
    addr++;

    value = r800->readMemory(r800->ref, addr++);
    
    r800->trapCb(r800->ref, value);
#endif
}

static void ld_c_d(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.DE.B.h; 
}

static void ld_c_e(R800* r800) {
    r800->regs.BC.B.l = r800->regs.DE.B.l;
}

static void ld_c_h(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.HL.B.h; 
}

static void ld_c_l(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.HL.B.l; 
}

static void ld_c_ixh(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.IX.B.h; 
}

static void ld_c_ixl(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.IX.B.l; 
}

static void ld_c_iyh(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.IY.B.h; 
}

static void ld_c_iyl(R800* r800) { 
    r800->regs.BC.B.l = r800->regs.IY.B.l; 
}

static void ld_c_xhl(R800* r800) { 
    r800->regs.BC.B.l = readMem(r800, r800->regs.HL.W); 
}

static void ld_c_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.BC.B.l = readMem(r800, addr);
}

static void ld_c_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.BC.B.l = readMem(r800, addr);
}

static void ld_xix_c(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.BC.B.l);
}

static void ld_xiy_c(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.BC.B.l);
}

static void ld_d_a(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.AF.B.h; 
}

static void ld_d_b(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.BC.B.h; 
}

static void ld_d_c(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.BC.B.l; 
}

static void ld_d_d(R800* r800) { 
#ifdef ENABLE_ASMSX_DEBUG_COMMANDS
    char debugString[256];
    UInt16 addr = r800->regs.PC.W;
    UInt16 end;
    char* ptr = debugString;

    if (r800->readMemory(r800->ref, addr) != 24) {
        return;
    }
    addr++;
    end = addr + 1 + (Int8)r800->readMemory(r800->ref, addr);
    addr++;

    if (end - addr > 127) {
        return;
    }
    else if (end - addr > 4 &&
             r800->readMemory(r800->ref, addr + 0) == 100 &&
             r800->readMemory(r800->ref, addr + 1) == 100 &&
             r800->readMemory(r800->ref, addr + 2) == 0   &&
             r800->readMemory(r800->ref, addr + 3) == 0)
    {
        addr += 4;
    }
    
    while (addr < end) {
        *ptr++ = (char)r800->readMemory(r800->ref, addr++);
    }

    if (ptr > debugString && ptr[-1] != 'n') {
        *ptr++ = '\n';
    }

    *ptr = 0;

    r800->debugCb(r800->ref, ASDBG_TRACE, debugString);
#endif
}

static void ld_d_e(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.DE.B.l; 
}

static void ld_d_h(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.HL.B.h; 
}

static void ld_d_l(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.HL.B.l; 
}

static void ld_d_ixh(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.IX.B.h; 
}

static void ld_d_ixl(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.IX.B.l; 
}

static void ld_d_iyh(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.IY.B.h; 
}

static void ld_d_iyl(R800* r800) { 
    r800->regs.DE.B.h = r800->regs.IY.B.l; 
}

static void ld_d_xhl(R800* r800) { 
    r800->regs.DE.B.h = readMem(r800, r800->regs.HL.W); 
}

static void ld_d_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.DE.B.h = readMem(r800, addr);
}

static void ld_d_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.DE.B.h = readMem(r800, addr);
}

static void ld_xix_d(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.DE.B.h);

}
static void ld_xiy_d(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.DE.B.h);
}

static void ld_e_a(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.AF.B.h; 
}

static void ld_e_b(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.BC.B.h; 
}

static void ld_e_c(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.BC.B.l; 
}

static void ld_e_d(R800* r800) {
    r800->regs.DE.B.l = r800->regs.DE.B.h; 
}

static void ld_e_e(R800* r800) { 
}

static void ld_e_h(R800* r800) {
    r800->regs.DE.B.l = r800->regs.HL.B.h; 
}

static void ld_e_l(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.HL.B.l; 
}

static void ld_e_ixh(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.IX.B.h; 
}

static void ld_e_ixl(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.IX.B.l; 
}

static void ld_e_iyh(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.IY.B.h; 
}

static void ld_e_iyl(R800* r800) { 
    r800->regs.DE.B.l = r800->regs.IY.B.l; 
}

static void ld_e_xhl(R800* r800) { 
    r800->regs.DE.B.l = readMem(r800, r800->regs.HL.W); 
}

static void ld_e_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.DE.B.l = readMem(r800, addr);
}

static void ld_e_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.DE.B.l = readMem(r800, addr);
}

static void ld_xix_e(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.DE.B.l);
}

static void ld_xiy_e(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.DE.B.l);
}

static void ld_h_a(R800* r800) { 
    r800->regs.HL.B.h = r800->regs.AF.B.h;
}

static void ld_h_b(R800* r800) { 
    r800->regs.HL.B.h = r800->regs.BC.B.h;
}

static void ld_h_c(R800* r800) { 
    r800->regs.HL.B.h = r800->regs.BC.B.l;
}

static void ld_h_d(R800* r800) {
    r800->regs.HL.B.h = r800->regs.DE.B.h;
}

static void ld_h_e(R800* r800) {
    r800->regs.HL.B.h = r800->regs.DE.B.l; 
}

static void ld_h_h(R800* r800) { 
    r800->regs.HL.B.h = r800->regs.HL.B.h; 
}

static void ld_h_l(R800* r800) { 
    r800->regs.HL.B.h = r800->regs.HL.B.l; 
}

static void ld_h_ixh(R800* r800) {
    r800->regs.HL.B.h = r800->regs.IX.B.h; 
}

static void ld_h_ixl(R800* r800) {
    r800->regs.HL.B.h = r800->regs.IX.B.l;
}

static void ld_h_iyh(R800* r800) {
    r800->regs.HL.B.h = r800->regs.IY.B.h; 
}

static void ld_h_iyl(R800* r800) {
    r800->regs.HL.B.h = r800->regs.IY.B.l; 
}

static void ld_h_xhl(R800* r800) {
    r800->regs.HL.B.h = readMem(r800, r800->regs.HL.W); 
}

static void ld_h_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.HL.B.h = readMem(r800, addr);
}

static void ld_h_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.HL.B.h = readMem(r800, addr);
}

static void ld_xix_h(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.HL.B.h);
}

static void ld_xiy_h(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.HL.B.h);
}

static void ld_l_a(R800* r800) { 
    r800->regs.HL.B.l = r800->regs.AF.B.h; 
}

static void ld_l_b(R800* r800) {
    r800->regs.HL.B.l = r800->regs.BC.B.h; 
}

static void ld_l_c(R800* r800) {
    r800->regs.HL.B.l = r800->regs.BC.B.l;
}

static void ld_l_d(R800* r800) {
    r800->regs.HL.B.l = r800->regs.DE.B.h;
}

static void ld_l_e(R800* r800) {
    r800->regs.HL.B.l = r800->regs.DE.B.l;
}

static void ld_l_h(R800* r800) {
    r800->regs.HL.B.l = r800->regs.HL.B.h;
}

static void ld_l_l(R800* r800) {
    r800->regs.HL.B.l = r800->regs.HL.B.l;
}

static void ld_l_ixh(R800* r800) {
    r800->regs.HL.B.l = r800->regs.IX.B.h;
}

static void ld_l_ixl(R800* r800) {
    r800->regs.HL.B.l = r800->regs.IX.B.l;
}

static void ld_l_iyh(R800* r800) {
    r800->regs.HL.B.l = r800->regs.IY.B.h;
}

static void ld_l_iyl(R800* r800) {
    r800->regs.HL.B.l = r800->regs.IY.B.l; 
}

static void ld_l_xhl(R800* r800) {
    r800->regs.HL.B.l = readMem(r800, r800->regs.HL.W); 
}

static void ld_l_xix(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.HL.B.l = readMem(r800, addr);
}

static void ld_l_xiy(R800* r800) { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    r800->regs.HL.B.l = readMem(r800, addr);
}

static void ld_xix_l(R800* r800) { 
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.HL.B.l);
}

static void ld_xiy_l(R800* r800)   { 
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800);
    r800->regs.SH.W = addr;
    writeMem(r800, addr, r800->regs.HL.B.l);
}

static void ld_ixh_a(R800* r800) { 
    r800->regs.IX.B.h = r800->regs.AF.B.h;
}

static void ld_ixh_b(R800* r800) {
    r800->regs.IX.B.h = r800->regs.BC.B.h;
}

static void ld_ixh_c(R800* r800) {
    r800->regs.IX.B.h = r800->regs.BC.B.l;
}

static void ld_ixh_d(R800* r800) {
    r800->regs.IX.B.h = r800->regs.DE.B.h; 
}

static void ld_ixh_e(R800* r800) {
    r800->regs.IX.B.h = r800->regs.DE.B.l;
}

static void ld_ixh_ixh(R800* r800) {
}

static void ld_ixh_ixl(R800* r800) {
    r800->regs.IX.B.h = r800->regs.IX.B.l;
}

static void ld_ixl_a(R800* r800) { 
    r800->regs.IX.B.l = r800->regs.AF.B.h;
}

static void ld_ixl_b(R800* r800) {
    r800->regs.IX.B.l = r800->regs.BC.B.h;
}

static void ld_ixl_c(R800* r800) {
    r800->regs.IX.B.l = r800->regs.BC.B.l; 
}

static void ld_ixl_d(R800* r800) { 
    r800->regs.IX.B.l = r800->regs.DE.B.h;
}

static void ld_ixl_e(R800* r800) {
    r800->regs.IX.B.l = r800->regs.DE.B.l;
}

static void ld_ixl_ixh(R800* r800) {
    r800->regs.IX.B.l = r800->regs.IX.B.h;
}

static void ld_ixl_ixl(R800* r800) {
}

static void ld_iyh_a(R800* r800) {
    r800->regs.IY.B.h = r800->regs.AF.B.h;
}

static void ld_iyh_b(R800* r800) {
    r800->regs.IY.B.h = r800->regs.BC.B.h;
}

static void ld_iyh_c(R800* r800) { 
    r800->regs.IY.B.h = r800->regs.BC.B.l;
}

static void ld_iyh_d(R800* r800) {
    r800->regs.IY.B.h = r800->regs.DE.B.h; 
}

static void ld_iyh_e(R800* r800) {
    r800->regs.IY.B.h = r800->regs.DE.B.l; 
}

static void ld_iyh_iyh(R800* r800) {
}

static void ld_iyh_iyl(R800* r800) {
    r800->regs.IY.B.h = r800->regs.IY.B.l; 
}

static void ld_iyl_a(R800* r800) {
    r800->regs.IY.B.l = r800->regs.AF.B.h;
}

static void ld_iyl_b(R800* r800) { 
    r800->regs.IY.B.l = r800->regs.BC.B.h;
}

static void ld_iyl_c(R800* r800) { 
    r800->regs.IY.B.l = r800->regs.BC.B.l;
}

static void ld_iyl_d(R800* r800) { 
    r800->regs.IY.B.l = r800->regs.DE.B.h;
}

static void ld_iyl_e(R800* r800) { 
    r800->regs.IY.B.l = r800->regs.DE.B.l;
}

static void ld_iyl_iyh(R800* r800) { 
    r800->regs.IY.B.l = r800->regs.IY.B.h;
}

static void ld_iyl_iyl(R800* r800) {
}

static void ld_xhl_b(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.BC.B.h);
}

static void ld_xhl_c(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.BC.B.l); 
}

static void ld_xhl_d(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.DE.B.h);
}

static void ld_xhl_e(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.DE.B.l);
}

static void ld_xhl_h(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.HL.B.h);
}

static void ld_xhl_l(R800* r800) { 
    writeMem(r800, r800->regs.HL.W, r800->regs.HL.B.l);
}

static void ld_a_byte(R800* r800) {
    r800->regs.AF.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_b_byte(R800* r800) {
    r800->regs.BC.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_c_byte(R800* r800) {
    r800->regs.BC.B.l = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_d_byte(R800* r800) {
    r800->regs.DE.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_e_byte(R800* r800) {
    r800->regs.DE.B.l = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_h_byte(R800* r800) {
    r800->regs.HL.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_l_byte(R800* r800) {
    r800->regs.HL.B.l = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_ixh_byte(R800* r800) {

    r800->regs.IX.B.h = readOpcode(r800, r800->regs.PC.W++);

}

static void ld_ixl_byte(R800* r800) {
    r800->regs.IX.B.l = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_iyh_byte(R800* r800) { 
    r800->regs.IY.B.h = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_iyl_byte(R800* r800) { 
    r800->regs.IY.B.l = readOpcode(r800, r800->regs.PC.W++);
}

static void ld_xbyte_a(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.SH.W = r800->regs.AF.B.h << 8;
    writeMem(r800, addr.W, r800->regs.AF.B.h);
}

static void ld_a_xbyte(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.AF.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W + 1;
}


static void ld_xix_byte(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value = readOpcode(r800, r800->regs.PC.W++);
    delayParallel(r800); 
    r800->regs.SH.W = addr;
    writeMem(r800, addr, value);
}

static void ld_xiy_byte(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    UInt8 value = readOpcode(r800, r800->regs.PC.W++);
    delayParallel(r800); 
    r800->regs.SH.W = addr;
    writeMem(r800, addr, value);
}

static void ld_xword_bc(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.BC.B.l);
    writeMem(r800, addr.W,   r800->regs.BC.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_xword_de(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.DE.B.l);
    writeMem(r800, addr.W,   r800->regs.DE.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_xword_hl(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.HL.B.l);
    writeMem(r800, addr.W,   r800->regs.HL.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_xword_ix(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.IX.B.l);
    writeMem(r800, addr.W,   r800->regs.IX.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_xword_iy(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.IY.B.l);
    writeMem(r800, addr.W,   r800->regs.IY.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_xword_sp(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    writeMem(r800, addr.W++, r800->regs.SP.B.l);
    writeMem(r800, addr.W,   r800->regs.SP.B.h);
    r800->regs.SH.W = addr.W;
}

static void ld_bc_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.BC.B.l = readMem(r800, addr.W++);
    r800->regs.BC.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void ld_de_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.DE.B.l = readMem(r800, addr.W++);
    r800->regs.DE.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void ld_hl_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.HL.B.l = readMem(r800, addr.W++);
    r800->regs.HL.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void ld_ix_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.IX.B.l = readMem(r800, addr.W++);
    r800->regs.IX.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void ld_iy_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.IY.B.l = readMem(r800, addr.W++);
    r800->regs.IY.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void ld_sp_xword(R800* r800) {
    RegisterPair addr;
    addr.B.l = readOpcode(r800, r800->regs.PC.W++);
    addr.B.h = readOpcode(r800, r800->regs.PC.W++);
    r800->regs.SP.B.l = readMem(r800, addr.W++);
    r800->regs.SP.B.h = readMem(r800, addr.W);
    r800->regs.SH.W = addr.W;
}

static void add_a_a(R800* r800) {
    ADD(r800, r800->regs.AF.B.h); 
}

static void add_a_b(R800* r800) {
    ADD(r800, r800->regs.BC.B.h);
}

static void add_a_c(R800* r800) {
    ADD(r800, r800->regs.BC.B.l);
}

static void add_a_d(R800* r800) {
    ADD(r800, r800->regs.DE.B.h);
}

static void add_a_e(R800* r800) {
    ADD(r800, r800->regs.DE.B.l);
}

static void add_a_h(R800* r800) {
    ADD(r800, r800->regs.HL.B.h); 
}

static void add_a_l(R800* r800) { 
    ADD(r800, r800->regs.HL.B.l);
}

static void add_a_ixl(R800* r800) {
    ADD(r800, r800->regs.IX.B.l); 
}

static void add_a_ixh(R800* r800) {
    ADD(r800, r800->regs.IX.B.h);
}

static void add_a_iyl(R800* r800) {
    ADD(r800, r800->regs.IY.B.l);
}

static void add_a_iyh(R800* r800) {
    ADD(r800, r800->regs.IY.B.h);
}

static void add_a_byte(R800* r800){
    ADD(r800, readOpcode(r800, r800->regs.PC.W++));
}

static void add_a_xhl(R800* r800) { 
    ADD(r800, readMem(r800, r800->regs.HL.W)); 
}

static void add_a_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    ADD(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void add_a_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    ADD(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void adc_a_a(R800* r800) {
    ADC(r800, r800->regs.AF.B.h);
}

static void adc_a_b(R800* r800) {
    ADC(r800, r800->regs.BC.B.h); 
}

static void adc_a_c(R800* r800) {
    ADC(r800, r800->regs.BC.B.l); 
}

static void adc_a_d(R800* r800) {
    ADC(r800, r800->regs.DE.B.h); 
}

static void adc_a_e(R800* r800) {
    ADC(r800, r800->regs.DE.B.l);
}

static void adc_a_h(R800* r800) {
    ADC(r800, r800->regs.HL.B.h);
}

static void adc_a_l(R800* r800) {
    ADC(r800, r800->regs.HL.B.l);
}

static void adc_a_ixl(R800* r800) {
    ADC(r800, r800->regs.IX.B.l);
}

static void adc_a_ixh(R800* r800) {
    ADC(r800, r800->regs.IX.B.h);
}

static void adc_a_iyl(R800* r800) {
    ADC(r800, r800->regs.IY.B.l);
}

static void adc_a_iyh(R800* r800) { 
    ADC(r800, r800->regs.IY.B.h);
}

static void adc_a_byte(R800* r800) {
    ADC(r800, readOpcode(r800, r800->regs.PC.W++)); 
}

static void adc_a_xhl(R800* r800) {
    ADC(r800, readMem(r800, r800->regs.HL.W));
}

static void adc_a_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    r800->regs.SH.W = addr;
    ADC(r800, readMem(r800, addr));
}

static void adc_a_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    r800->regs.SH.W = addr;
    ADC(r800, readMem(r800, addr));
}

static void adc_hl_bc(R800* r800) {
    ADCW(r800, r800->regs.BC.W);
}

static void adc_hl_de(R800* r800) { 
    ADCW(r800, r800->regs.DE.W);
}

static void adc_hl_hl(R800* r800) {
    ADCW(r800, r800->regs.HL.W);
}

static void adc_hl_sp(R800* r800) {
    ADCW(r800, r800->regs.SP.W);
}

static void sub_a(R800* r800) {
    SUB(r800, r800->regs.AF.B.h);
}

static void sub_b(R800* r800) {
    SUB(r800, r800->regs.BC.B.h); 
}

static void sub_c(R800* r800) { 
    SUB(r800, r800->regs.BC.B.l); 
}

static void sub_d(R800* r800) {
    SUB(r800, r800->regs.DE.B.h); 
}

static void sub_e(R800* r800) {
    SUB(r800, r800->regs.DE.B.l); 
}

static void sub_h(R800* r800) {
    SUB(r800, r800->regs.HL.B.h);
}

static void sub_l(R800* r800) {
    SUB(r800, r800->regs.HL.B.l);
}

static void sub_ixl(R800* r800) {
    SUB(r800, r800->regs.IX.B.l); 
}

static void sub_ixh(R800* r800) {
    SUB(r800, r800->regs.IX.B.h);
}

static void sub_iyl(R800* r800) {
    SUB(r800, r800->regs.IY.B.l);
}

static void sub_iyh(R800* r800) {
    SUB(r800, r800->regs.IY.B.h);
}

static void sub_byte(R800* r800){
    SUB(r800, readOpcode(r800, r800->regs.PC.W++)); 
}

static void sub_xhl(R800* r800) { 
    SUB(r800, readMem(r800, r800->regs.HL.W)); 
}

static void sub_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    r800->regs.SH.W = addr;
    SUB(r800, readMem(r800, addr));
}

static void sub_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    r800->regs.SH.W = addr;
    SUB(r800, readMem(r800, addr));
}

static void neg(R800* r800) {
    UInt8 regVal = r800->regs.AF.B.h;
    r800->regs.AF.B.h = 0;
    SUB(r800, regVal);
}

static void sbc_a_a(R800* r800) {
    SBC(r800, r800->regs.AF.B.h); 
}

static void sbc_a_b(R800* r800) {
    SBC(r800, r800->regs.BC.B.h); 
}

static void sbc_a_c(R800* r800) {
    SBC(r800, r800->regs.BC.B.l); 
}

static void sbc_a_d(R800* r800) {
    SBC(r800, r800->regs.DE.B.h);
}

static void sbc_a_e(R800* r800) {
    SBC(r800, r800->regs.DE.B.l);
}

static void sbc_a_h(R800* r800) {
    SBC(r800, r800->regs.HL.B.h);
}

static void sbc_a_l(R800* r800) {
    SBC(r800, r800->regs.HL.B.l);
}

static void sbc_a_ixl(R800* r800) {
    SBC(r800, r800->regs.IX.B.l);
}

static void sbc_a_ixh(R800* r800) {
    SBC(r800, r800->regs.IX.B.h);
}

static void sbc_a_iyl(R800* r800) {
    SBC(r800, r800->regs.IY.B.l);
}

static void sbc_a_iyh(R800* r800) { 
    SBC(r800, r800->regs.IY.B.h);
}

static void sbc_a_byte(R800* r800){ 
    SBC(r800, readOpcode(r800, r800->regs.PC.W++));
}

static void sbc_a_xhl(R800* r800) { 
    SBC(r800, readMem(r800, r800->regs.HL.W)); 
}

static void sbc_a_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    SBC(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void sbc_a_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    SBC(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void sbc_hl_bc(R800* r800) { SBCW(r800, r800->regs.BC.W);
}

static void sbc_hl_de(R800* r800) { SBCW(r800, r800->regs.DE.W); 
}

static void sbc_hl_hl(R800* r800) { SBCW(r800, r800->regs.HL.W);
}

static void sbc_hl_sp(R800* r800) { SBCW(r800, r800->regs.SP.W);
}

static void cp_a(R800* r800) {
    CP(r800, r800->regs.AF.B.h);
}

static void cp_b(R800* r800) {
    CP(r800, r800->regs.BC.B.h);
}

static void cp_c(R800* r800) {
    CP(r800, r800->regs.BC.B.l);
}

static void cp_d(R800* r800) {
    CP(r800, r800->regs.DE.B.h);
}

static void cp_e(R800* r800) {
    CP(r800, r800->regs.DE.B.l);
}

static void cp_h(R800* r800) {
    CP(r800, r800->regs.HL.B.h);
}

static void cp_l(R800* r800) {
    CP(r800, r800->regs.HL.B.l);
}

static void cp_ixl(R800* r800) {
    CP(r800, r800->regs.IX.B.l);
}

static void cp_ixh(R800* r800) {
    CP(r800, r800->regs.IX.B.h);
}

static void cp_iyl(R800* r800) { 
    CP(r800, r800->regs.IY.B.l);
}

static void cp_iyh(R800* r800) {
    CP(r800, r800->regs.IY.B.h);
}

static void cp_byte(R800* r800){
    CP(r800, readOpcode(r800, r800->regs.PC.W++)); 
}

static void cp_xhl(R800* r800) { 
    CP(r800, readMem(r800, r800->regs.HL.W)); 
}

static void cp_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    CP(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void cp_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    CP(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void and_a(R800* r800) {
    AND(r800, r800->regs.AF.B.h); 
}

static void and_b(R800* r800) {
    AND(r800, r800->regs.BC.B.h); 
}

static void and_c(R800* r800) {
    AND(r800, r800->regs.BC.B.l);
}

static void and_d(R800* r800) {
    AND(r800, r800->regs.DE.B.h);
}

static void and_e(R800* r800) {
    AND(r800, r800->regs.DE.B.l); 
}

static void and_h(R800* r800) {
    AND(r800, r800->regs.HL.B.h);
}

static void and_l(R800* r800) { 
    AND(r800, r800->regs.HL.B.l); 
}

static void and_ixl(R800* r800) {
    AND(r800, r800->regs.IX.B.l);
}

static void and_ixh(R800* r800) {
    AND(r800, r800->regs.IX.B.h); 
}

static void and_iyl(R800* r800) {
    AND(r800, r800->regs.IY.B.l); 
}

static void and_iyh(R800* r800) {
    AND(r800, r800->regs.IY.B.h); 
}

static void and_byte(R800* r800){
    AND(r800, readOpcode(r800, r800->regs.PC.W++)); 
}

static void and_xhl(R800* r800) { 
    AND(r800, readMem(r800, r800->regs.HL.W));
}

static void and_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    AND(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void and_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    AND(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void or_a(R800* r800) {
    OR(r800, r800->regs.AF.B.h);
}

static void or_b(R800* r800) {
    OR(r800, r800->regs.BC.B.h);
}

static void or_c(R800* r800) {
    OR(r800, r800->regs.BC.B.l);
}

static void or_d(R800* r800) {
    OR(r800, r800->regs.DE.B.h);
}

static void or_e(R800* r800) {
    OR(r800, r800->regs.DE.B.l);
}

static void or_h(R800* r800) {
    OR(r800, r800->regs.HL.B.h); 
}

static void or_l(R800* r800) {
    OR(r800, r800->regs.HL.B.l); 
}

static void or_ixl(R800* r800) {
    OR(r800, r800->regs.IX.B.l); 
}

static void or_ixh(R800* r800) {
    OR(r800, r800->regs.IX.B.h);
}

static void or_iyl(R800* r800) {
    OR(r800, r800->regs.IY.B.l); 
}

static void or_iyh(R800* r800) {
    OR(r800, r800->regs.IY.B.h); 
}

static void or_byte(R800* r800){
    OR(r800, readOpcode(r800, r800->regs.PC.W++)); 
}

static void or_xhl(R800* r800) { 
    OR(r800, readMem(r800, r800->regs.HL.W));
}

static void or_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    OR(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void or_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    OR(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void xor_a(R800* r800) { 
    XOR(r800, r800->regs.AF.B.h); 
}

static void xor_b(R800* r800) {
    XOR(r800, r800->regs.BC.B.h); 
}

static void xor_c(R800* r800) { 
    XOR(r800, r800->regs.BC.B.l); 
}

static void xor_d(R800* r800) { 
    XOR(r800, r800->regs.DE.B.h);
}

static void xor_e(R800* r800) {
    XOR(r800, r800->regs.DE.B.l);
}

static void xor_h(R800* r800) {
    XOR(r800, r800->regs.HL.B.h);
}

static void xor_l(R800* r800) {
    XOR(r800, r800->regs.HL.B.l);
}

static void xor_ixl(R800* r800) {
    XOR(r800, r800->regs.IX.B.l); 
}

static void xor_ixh(R800* r800) { 
    XOR(r800, r800->regs.IX.B.h); 
}

static void xor_iyl(R800* r800) {
    XOR(r800, r800->regs.IY.B.l); 
}

static void xor_iyh(R800* r800) { 
    XOR(r800, r800->regs.IY.B.h);
}

static void xor_byte(R800* r800){
    XOR(r800, readOpcode(r800, r800->regs.PC.W++));
}

static void xor_xhl(R800* r800) {
    XOR(r800, readMem(r800, r800->regs.HL.W));
}

static void xor_xix(R800* r800) {
    UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    XOR(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void xor_xiy(R800* r800) {
    UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    delayAdd8(r800); 
    XOR(r800, readMem(r800, addr));
    r800->regs.SH.W = addr;
}

static void add_hl_bc(R800* r800) {
    ADDW(r800, &r800->regs.HL.W, r800->regs.BC.W);
}

static void add_hl_de(R800* r800) {
    ADDW(r800, &r800->regs.HL.W, r800->regs.DE.W);
}

static void add_hl_hl(R800* r800) {
    ADDW(r800, &r800->regs.HL.W, r800->regs.HL.W);
}

static void add_hl_sp(R800* r800) {
    ADDW(r800, &r800->regs.HL.W, r800->regs.SP.W);
}

static void add_ix_bc(R800* r800) {
    ADDW(r800, &r800->regs.IX.W, r800->regs.BC.W);
}

static void add_ix_de(R800* r800) {
    ADDW(r800, &r800->regs.IX.W, r800->regs.DE.W);
}

static void add_ix_ix(R800* r800) {
    ADDW(r800, &r800->regs.IX.W, r800->regs.IX.W);
}

static void add_ix_sp(R800* r800) {
    ADDW(r800, &r800->regs.IX.W, r800->regs.SP.W);
}

static void add_iy_bc(R800* r800) {
    ADDW(r800, &r800->regs.IY.W, r800->regs.BC.W);
}

static void add_iy_de(R800* r800) {
    ADDW(r800, &r800->regs.IY.W, r800->regs.DE.W);
}

static void add_iy_iy(R800* r800) {
    ADDW(r800, &r800->regs.IY.W, r800->regs.IY.W);
}

static void add_iy_sp(R800* r800) {
    ADDW(r800, &r800->regs.IY.W, r800->regs.SP.W);
}

static void mulu_xhl(R800* r800) { 
}

static void mulu_a(R800* r800) { 
}

static void mulu_b(R800* r800) { 
    if (r800->cpuMode == CPU_R800) MULU(r800, r800->regs.BC.B.h);
}

static void mulu_c(R800* r800) {
    if (r800->cpuMode == CPU_R800) MULU(r800, r800->regs.BC.B.l); 
}

static void mulu_d(R800* r800) {
    if (r800->cpuMode == CPU_R800) MULU(r800, r800->regs.DE.B.h); 
}

static void mulu_e(R800* r800) {
    if (r800->cpuMode == CPU_R800) MULU(r800, r800->regs.DE.B.l);
}

static void mulu_h(R800* r800) { 
}

static void mulu_l(R800* r800) { 
}

static void muluw_bc(R800* r800) { 
    if (r800->cpuMode == CPU_R800) MULUW(r800, r800->regs.BC.W);
}

static void muluw_de(R800* r800) {
}

static void muluw_hl(R800* r800) {
}

static void muluw_sp(R800* r800) {
    if (r800->cpuMode == CPU_R800) MULUW(r800, r800->regs.SP.W); 
}

static void sla_a(R800* r800) { 
    SLA(r800, &r800->regs.AF.B.h); 
}

static void sla_b(R800* r800) { 
    SLA(r800, &r800->regs.BC.B.h); 
}

static void sla_c(R800* r800) {
    SLA(r800, &r800->regs.BC.B.l); 
}

static void sla_d(R800* r800) {
    SLA(r800, &r800->regs.DE.B.h);
}

static void sla_e(R800* r800) { 
    SLA(r800, &r800->regs.DE.B.l); 
}

static void sla_h(R800* r800) {
    SLA(r800, &r800->regs.HL.B.h);
}

static void sla_l(R800* r800) {
    SLA(r800, &r800->regs.HL.B.l); 
}

static void sla_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SLA(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SLA_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SLA(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void sla_xnn  (R800* r800, UInt16 addr) {
    SLA_XNN(r800, addr);
}

static void sla_xnn_a(R800* r800, UInt16 addr) { 
    r800->regs.AF.B.h = SLA_XNN(r800, addr);
}

static void sla_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SLA_XNN(r800, addr);
}

static void sla_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = SLA_XNN(r800, addr);
}

static void sla_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SLA_XNN(r800, addr);
}

static void sla_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SLA_XNN(r800, addr);
}

static void sla_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SLA_XNN(r800, addr);
}

static void sla_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SLA_XNN(r800, addr);
}

static void sll_a(R800* r800) {
    SLL(r800, &r800->regs.AF.B.h); 
}

static void sll_b(R800* r800) {
    SLL(r800, &r800->regs.BC.B.h); 
}

static void sll_c(R800* r800) {
    SLL(r800, &r800->regs.BC.B.l);
}

static void sll_d(R800* r800) {
    SLL(r800, &r800->regs.DE.B.h);
}

static void sll_e(R800* r800) {
    SLL(r800, &r800->regs.DE.B.l);
}

static void sll_h(R800* r800) {
    SLL(r800, &r800->regs.HL.B.h);
}

static void sll_l(R800* r800) {
    SLL(r800, &r800->regs.HL.B.l); 
}

static void sll_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SLL(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SLL_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SLL(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void sll_xnn  (R800* r800, UInt16 addr) { 
    SLL_XNN(r800, addr);
}

static void sll_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SLL_XNN(r800, addr);
}

static void sll_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SLL_XNN(r800, addr);
}

static void sll_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SLL_XNN(r800, addr);
}

static void sll_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SLL_XNN(r800, addr);
}

static void sll_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SLL_XNN(r800, addr);
}

static void sll_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SLL_XNN(r800, addr);
}

static void sll_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SLL_XNN(r800, addr); 
}

static void sra_a(R800* r800) {    
    SRA(r800, &r800->regs.AF.B.h);
}

static void sra_b(R800* r800) { 
    SRA(r800, &r800->regs.BC.B.h);
}

static void sra_c(R800* r800) { 
    SRA(r800, &r800->regs.BC.B.l);
}

static void sra_d(R800* r800) { 
    SRA(r800, &r800->regs.DE.B.h);
}

static void sra_e(R800* r800) { 
    SRA(r800, &r800->regs.DE.B.l); 
}

static void sra_h(R800* r800) {
    SRA(r800, &r800->regs.HL.B.h);
}

static void sra_l(R800* r800) {
    SRA(r800, &r800->regs.HL.B.l); 
}

static void sra_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SRA(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SRA_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SRA(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void sra_xnn  (R800* r800, UInt16 addr) {
    SRA_XNN(r800, addr); 
}

static void sra_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SRA_XNN(r800, addr); 
}

static void sra_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SRA_XNN(r800, addr); 
}

static void sra_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SRA_XNN(r800, addr); 
}

static void sra_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SRA_XNN(r800, addr); 
}

static void sra_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = SRA_XNN(r800, addr); 
}

static void sra_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SRA_XNN(r800, addr); 
}

static void sra_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SRA_XNN(r800, addr);
}

static void srl_a(R800* r800) { 
    SRL(r800, &r800->regs.AF.B.h); 
}

static void srl_b(R800* r800) {
    SRL(r800, &r800->regs.BC.B.h); 
}

static void srl_c(R800* r800) { 
    SRL(r800, &r800->regs.BC.B.l); 
}

static void srl_d(R800* r800) {
    SRL(r800, &r800->regs.DE.B.h);
}

static void srl_e(R800* r800) {
    SRL(r800, &r800->regs.DE.B.l); 
}

static void srl_h(R800* r800) {
    SRL(r800, &r800->regs.HL.B.h); 
}

static void srl_l(R800* r800) {
    SRL(r800, &r800->regs.HL.B.l); 
}

static void srl_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SRL(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SRL_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SRL(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void srl_xnn  (R800* r800, UInt16 addr) {
    SRL_XNN(r800, addr);
}

static void srl_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SRL_XNN(r800, addr);
}

static void srl_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SRL_XNN(r800, addr); 
}

static void srl_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SRL_XNN(r800, addr); 
}

static void srl_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SRL_XNN(r800, addr);
}

static void srl_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = SRL_XNN(r800, addr); 
}

static void srl_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SRL_XNN(r800, addr);
}

static void srl_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SRL_XNN(r800, addr);
}

static void rl_a(R800* r800) {
    RL(r800, &r800->regs.AF.B.h);
}

static void rl_b(R800* r800) {
    RL(r800, &r800->regs.BC.B.h);
}

static void rl_c(R800* r800) { 
    RL(r800, &r800->regs.BC.B.l); 
}

static void rl_d(R800* r800) {
    RL(r800, &r800->regs.DE.B.h);
}

static void rl_e(R800* r800) { 
    RL(r800, &r800->regs.DE.B.l);
}

static void rl_h(R800* r800) {
    RL(r800, &r800->regs.HL.B.h);
}

static void rl_l(R800* r800) { 
    RL(r800, &r800->regs.HL.B.l);
}

static void rl_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RL(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RL_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RL(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void rl_xnn  (R800* r800, UInt16 addr) {
    RL_XNN(r800, addr);
}

static void rl_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RL_XNN(r800, addr);
}

static void rl_xnn_b(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.h = RL_XNN(r800, addr); 
}

static void rl_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = RL_XNN(r800, addr); 
}

static void rl_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RL_XNN(r800, addr);
}

static void rl_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = RL_XNN(r800, addr);
}

static void rl_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RL_XNN(r800, addr);
}

static void rl_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RL_XNN(r800, addr);
}

static void rlc_a(R800* r800) {
    RLC(r800, &r800->regs.AF.B.h);
}

static void rlc_b(R800* r800) {
    RLC(r800, &r800->regs.BC.B.h);
}

static void rlc_c(R800* r800) { 
    RLC(r800, &r800->regs.BC.B.l);
}

static void rlc_d(R800* r800) {
    RLC(r800, &r800->regs.DE.B.h);
}

static void rlc_e(R800* r800) { 
    RLC(r800, &r800->regs.DE.B.l);
}

static void rlc_h(R800* r800) {
    RLC(r800, &r800->regs.HL.B.h);
}

static void rlc_l(R800* r800) { 
    RLC(r800, &r800->regs.HL.B.l);
}

static void rlc_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RLC(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RLC_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RLC(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void rlc_xnn  (R800* r800, UInt16 addr) { 
    RLC_XNN(r800, addr);
}

static void rlc_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RLC_XNN(r800, addr);
}

static void rlc_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RLC_XNN(r800, addr);
}

static void rlc_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RLC_XNN(r800, addr); 
}

static void rlc_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RLC_XNN(r800, addr); 
}

static void rlc_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RLC_XNN(r800, addr); 
}

static void rlc_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RLC_XNN(r800, addr);
}

static void rlc_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RLC_XNN(r800, addr); 
}

static void rr_a(R800* r800) {
    RR(r800, &r800->regs.AF.B.h);
}

static void rr_b(R800* r800) {
    RR(r800, &r800->regs.BC.B.h); 
}

static void rr_c(R800* r800) {
    RR(r800, &r800->regs.BC.B.l);
}

static void rr_d(R800* r800) {
    RR(r800, &r800->regs.DE.B.h);
}

static void rr_e(R800* r800) {
    RR(r800, &r800->regs.DE.B.l);
}

static void rr_h(R800* r800) {
    RR(r800, &r800->regs.HL.B.h);
}

static void rr_l(R800* r800) { 
    RR(r800, &r800->regs.HL.B.l);
}

static void rr_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RR(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RR_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RR(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void rr_xnn  (R800* r800, UInt16 addr) {
    RR_XNN(r800, addr);
}

static void rr_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RR_XNN(r800, addr);
}

static void rr_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RR_XNN(r800, addr);
}

static void rr_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RR_XNN(r800, addr);
}

static void rr_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RR_XNN(r800, addr);
}

static void rr_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RR_XNN(r800, addr); 
}

static void rr_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RR_XNN(r800, addr);
}

static void rr_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RR_XNN(r800, addr);
}

static void rrc_a(R800* r800) {
    RRC(r800, &r800->regs.AF.B.h);
}

static void rrc_b(R800* r800) {
    RRC(r800, &r800->regs.BC.B.h);
}

static void rrc_c(R800* r800) {
    RRC(r800, &r800->regs.BC.B.l);
}

static void rrc_d(R800* r800) {
    RRC(r800, &r800->regs.DE.B.h);
}

static void rrc_e(R800* r800) {
    RRC(r800, &r800->regs.DE.B.l);
}

static void rrc_h(R800* r800) { 
    RRC(r800, &r800->regs.HL.B.h);
}

static void rrc_l(R800* r800) { 
    RRC(r800, &r800->regs.HL.B.l); 
}

static void rrc_xhl(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RRC(r800, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RRC_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RRC(r800, &val);
    delayBit(r800);                 // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void rrc_xnn  (R800* r800, UInt16 addr) {
    RRC_XNN(r800, addr);
}

static void rrc_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RRC_XNN(r800, addr); 
}

static void rrc_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RRC_XNN(r800, addr);
}

static void rrc_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RRC_XNN(r800, addr);
}

static void rrc_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RRC_XNN(r800, addr);
}

static void rrc_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RRC_XNN(r800, addr);
}

static void rrc_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RRC_XNN(r800, addr);
}

static void rrc_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = RRC_XNN(r800, addr);
}

static void bit_0_a(R800* r800) { 
    BIT(r800, 0, r800->regs.AF.B.h);
}

static void bit_0_b(R800* r800) {
    BIT(r800, 0, r800->regs.BC.B.h);
}

static void bit_0_c(R800* r800) {
    BIT(r800, 0, r800->regs.BC.B.l);
}

static void bit_0_d(R800* r800) {
    BIT(r800, 0, r800->regs.DE.B.h); 
}

static void bit_0_e(R800* r800) {
    BIT(r800, 0, r800->regs.DE.B.l);
}

static void bit_0_h(R800* r800) { 
    BIT(r800, 0, r800->regs.HL.B.h);
}

static void bit_0_l(R800* r800) {
    BIT(r800, 0, r800->regs.HL.B.l); 
}

static void bit_0_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 0)];
}

static void bit_0_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 0)];
}

static void bit_1_a(R800* r800) {
    BIT(r800, 1, r800->regs.AF.B.h);
}

static void bit_1_b(R800* r800) {
    BIT(r800, 1, r800->regs.BC.B.h);
}

static void bit_1_c(R800* r800) {
    BIT(r800, 1, r800->regs.BC.B.l);
}

static void bit_1_d(R800* r800) {
    BIT(r800, 1, r800->regs.DE.B.h);
}

static void bit_1_e(R800* r800) {
    BIT(r800, 1, r800->regs.DE.B.l);
}

static void bit_1_h(R800* r800) {
    BIT(r800, 1, r800->regs.HL.B.h);
}

static void bit_1_l(R800* r800) {
    BIT(r800, 1, r800->regs.HL.B.l);
}

static void bit_1_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 1)];
}

static void bit_1_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 1)];
}

static void bit_2_a(R800* r800) {
    BIT(r800, 2, r800->regs.AF.B.h); 
}

static void bit_2_b(R800* r800) {
    BIT(r800, 2, r800->regs.BC.B.h);
}

static void bit_2_c(R800* r800) {
    BIT(r800, 2, r800->regs.BC.B.l);
}

static void bit_2_d(R800* r800) {
    BIT(r800, 2, r800->regs.DE.B.h);
}

static void bit_2_e(R800* r800) {
    BIT(r800, 2, r800->regs.DE.B.l);
}

static void bit_2_h(R800* r800) {
    BIT(r800, 2, r800->regs.HL.B.h);
}

static void bit_2_l(R800* r800) { 
    BIT(r800, 2, r800->regs.HL.B.l);
}

static void bit_2_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 2)];
}

static void bit_2_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 2)];
}

static void bit_3_a(R800* r800) {
    BIT(r800, 3, r800->regs.AF.B.h);
}

static void bit_3_b(R800* r800) {
    BIT(r800, 3, r800->regs.BC.B.h);
}

static void bit_3_c(R800* r800) { 
    BIT(r800, 3, r800->regs.BC.B.l);
}

static void bit_3_d(R800* r800) {
    BIT(r800, 3, r800->regs.DE.B.h); 
}

static void bit_3_e(R800* r800) {
    BIT(r800, 3, r800->regs.DE.B.l);
}

static void bit_3_h(R800* r800) { 
    BIT(r800, 3, r800->regs.HL.B.h);
}

static void bit_3_l(R800* r800) { 
    BIT(r800, 3, r800->regs.HL.B.l);
}

static void bit_3_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 3)];
}

static void bit_3_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 3)];
}

static void bit_4_a(R800* r800) {
    BIT(r800, 4, r800->regs.AF.B.h);
}

static void bit_4_b(R800* r800) { 
    BIT(r800, 4, r800->regs.BC.B.h);
}

static void bit_4_c(R800* r800) { 
    BIT(r800, 4, r800->regs.BC.B.l);
}

static void bit_4_d(R800* r800) {
    BIT(r800, 4, r800->regs.DE.B.h);
}

static void bit_4_e(R800* r800) {
    BIT(r800, 4, r800->regs.DE.B.l);
}

static void bit_4_h(R800* r800) {
    BIT(r800, 4, r800->regs.HL.B.h);
}

static void bit_4_l(R800* r800) { 
    BIT(r800, 4, r800->regs.HL.B.l);
}

static void bit_4_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 4)];
}

static void bit_4_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 4)];
}

static void bit_5_a(R800* r800) {
    BIT(r800, 5, r800->regs.AF.B.h); 
}

static void bit_5_b(R800* r800) {
    BIT(r800, 5, r800->regs.BC.B.h); 
}

static void bit_5_c(R800* r800) { 
    BIT(r800, 5, r800->regs.BC.B.l); 
}

static void bit_5_d(R800* r800) {
    BIT(r800, 5, r800->regs.DE.B.h); 
}

static void bit_5_e(R800* r800) {
    BIT(r800, 5, r800->regs.DE.B.l); 
}

static void bit_5_h(R800* r800) {
    BIT(r800, 5, r800->regs.HL.B.h);
}

static void bit_5_l(R800* r800) { 
    BIT(r800, 5, r800->regs.HL.B.l); 
}

static void bit_5_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 5)];
}

static void bit_5_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 5)];
}

static void bit_6_a(R800* r800) { 
    BIT(r800, 6, r800->regs.AF.B.h);
}

static void bit_6_b(R800* r800) {
    BIT(r800, 6, r800->regs.BC.B.h);
}

static void bit_6_c(R800* r800) {
    BIT(r800, 6, r800->regs.BC.B.l); 
}

static void bit_6_d(R800* r800) { 
    BIT(r800, 6, r800->regs.DE.B.h);
}

static void bit_6_e(R800* r800) {
    BIT(r800, 6, r800->regs.DE.B.l);
}

static void bit_6_h(R800* r800) {
    BIT(r800, 6, r800->regs.HL.B.h);
}

static void bit_6_l(R800* r800) {
    BIT(r800, 6, r800->regs.HL.B.l);
}

static void bit_6_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 6)];
}

static void bit_6_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 6)];
}

static void bit_7_a(R800* r800) { 
    BIT(r800, 7, r800->regs.AF.B.h); 
}

static void bit_7_b(R800* r800) { 
    BIT(r800, 7, r800->regs.BC.B.h);
}

static void bit_7_c(R800* r800) {
    BIT(r800, 7, r800->regs.BC.B.l);
}

static void bit_7_d(R800* r800) {
    BIT(r800, 7, r800->regs.DE.B.h);
}

static void bit_7_e(R800* r800) {
    BIT(r800, 7, r800->regs.DE.B.l); 
}

static void bit_7_h(R800* r800) {
    BIT(r800, 7, r800->regs.HL.B.h);
}

static void bit_7_l(R800* r800) {
    BIT(r800, 7, r800->regs.HL.B.l); 
}

static void bit_7_xhl(R800* r800) {
    delayBit(r800);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) | 
        ZSPHTable[readMem(r800, r800->regs.HL.W) & (1 << 7)];
}

static void bit_7_xnn(R800* r800, UInt16 addr) { 
    delayBitIx(r800);           // white cat append
    r800->regs.SH.W   = addr;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        (r800->regs.SH.B.h & (X_FLAG | Y_FLAG)) |
        ZSPHTable[readMem(r800, addr) & (1 << 7)];
}

static void res_0_a(R800* r800) {
    RES(r800, 0, &r800->regs.AF.B.h);
}

static void res_0_b(R800* r800) {
    RES(r800, 0, &r800->regs.BC.B.h);
}

static void res_0_c(R800* r800) {
    RES(r800, 0, &r800->regs.BC.B.l);
}

static void res_0_d(R800* r800) {
    RES(r800, 0, &r800->regs.DE.B.h);
}

static void res_0_e(R800* r800) {
    RES(r800, 0, &r800->regs.DE.B.l);
}

static void res_0_h(R800* r800) {
    RES(r800, 0, &r800->regs.HL.B.h);
}

static void res_0_l(R800* r800) {
    RES(r800, 0, &r800->regs.HL.B.l);
}

static void res_0_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 0, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_0_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 0, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_0_xnn  (R800* r800, UInt16 addr) {
    RES_0_XNN(r800, addr);
}

static void res_0_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_0_XNN(r800, addr);
}

static void res_0_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_0_XNN(r800, addr);
}

static void res_0_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RES_0_XNN(r800, addr);
}

static void res_0_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_0_XNN(r800, addr);
}

static void res_0_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RES_0_XNN(r800, addr);
}

static void res_0_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RES_0_XNN(r800, addr);
}

static void res_0_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = RES_0_XNN(r800, addr); 
}

static void res_1_a(R800* r800) { 
    RES(r800, 1, &r800->regs.AF.B.h);
}

static void res_1_b(R800* r800) {
    RES(r800, 1, &r800->regs.BC.B.h);
}

static void res_1_c(R800* r800) {
    RES(r800, 1, &r800->regs.BC.B.l); 
}

static void res_1_d(R800* r800) {
    RES(r800, 1, &r800->regs.DE.B.h); 
}

static void res_1_e(R800* r800) {
    RES(r800, 1, &r800->regs.DE.B.l); 
}

static void res_1_h(R800* r800) {
    RES(r800, 1, &r800->regs.HL.B.h);
}

static void res_1_l(R800* r800) {
    RES(r800, 1, &r800->regs.HL.B.l); 
}

static void res_1_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 1, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_1_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 1, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_1_xnn  (R800* r800, UInt16 addr) {
    RES_1_XNN(r800, addr); 
}

static void res_1_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_1_XNN(r800, addr);
}

static void res_1_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_1_XNN(r800, addr);
}

static void res_1_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = RES_1_XNN(r800, addr); 
}

static void res_1_xnn_d(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.h = RES_1_XNN(r800, addr);
}

static void res_1_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = RES_1_XNN(r800, addr);
}

static void res_1_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = RES_1_XNN(r800, addr);
}

static void res_1_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = RES_1_XNN(r800, addr);
}

static void res_2_a(R800* r800) {
    RES(r800, 2, &r800->regs.AF.B.h);
}

static void res_2_b(R800* r800) {
    RES(r800, 2, &r800->regs.BC.B.h);
}

static void res_2_c(R800* r800) {
    RES(r800, 2, &r800->regs.BC.B.l);
}

static void res_2_d(R800* r800) {
    RES(r800, 2, &r800->regs.DE.B.h);
}

static void res_2_e(R800* r800) { 
    RES(r800, 2, &r800->regs.DE.B.l); 
}

static void res_2_h(R800* r800) {
    RES(r800, 2, &r800->regs.HL.B.h); 
}

static void res_2_l(R800* r800) {
    RES(r800, 2, &r800->regs.HL.B.l);
}

static void res_2_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 2, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_2_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 2, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_2_xnn  (R800* r800, UInt16 addr) {
    RES_2_XNN(r800, addr); 
}

static void res_2_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_2_XNN(r800, addr);
}

static void res_2_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_2_XNN(r800, addr);
}

static void res_2_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RES_2_XNN(r800, addr);
}

static void res_2_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_2_XNN(r800, addr);
}

static void res_2_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RES_2_XNN(r800, addr);
}

static void res_2_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RES_2_XNN(r800, addr); 
}

static void res_2_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RES_2_XNN(r800, addr);
}

static void res_3_a(R800* r800) {
    RES(r800, 3, &r800->regs.AF.B.h);
}

static void res_3_b(R800* r800) {
    RES(r800, 3, &r800->regs.BC.B.h);
}

static void res_3_c(R800* r800) { 
    RES(r800, 3, &r800->regs.BC.B.l);
}

static void res_3_d(R800* r800) {
    RES(r800, 3, &r800->regs.DE.B.h);
}

static void res_3_e(R800* r800) {
    RES(r800, 3, &r800->regs.DE.B.l);
}

static void res_3_h(R800* r800) {
    RES(r800, 3, &r800->regs.HL.B.h);
}

static void res_3_l(R800* r800) {
    RES(r800, 3, &r800->regs.HL.B.l);
}

static void res_3_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 3, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_3_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 3, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_3_xnn  (R800* r800, UInt16 addr) {
    RES_3_XNN(r800, addr);
}

static void res_3_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_3_XNN(r800, addr); 
}

static void res_3_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_3_XNN(r800, addr);
}

static void res_3_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RES_3_XNN(r800, addr); 
}

static void res_3_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_3_XNN(r800, addr);
}

static void res_3_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RES_3_XNN(r800, addr);
}

static void res_3_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = RES_3_XNN(r800, addr); 
}

static void res_3_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RES_3_XNN(r800, addr);
}

static void res_4_a(R800* r800) {
    RES(r800, 4, &r800->regs.AF.B.h);
}

static void res_4_b(R800* r800) {
    RES(r800, 4, &r800->regs.BC.B.h);
}

static void res_4_c(R800* r800) { 
    RES(r800, 4, &r800->regs.BC.B.l);
}

static void res_4_d(R800* r800) {
    RES(r800, 4, &r800->regs.DE.B.h);
}

static void res_4_e(R800* r800) {
    RES(r800, 4, &r800->regs.DE.B.l);
}

static void res_4_h(R800* r800) {
    RES(r800, 4, &r800->regs.HL.B.h);
}

static void res_4_l(R800* r800) {
    RES(r800, 4, &r800->regs.HL.B.l); 
}

static void res_4_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 4, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_4_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 4, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_4_xnn  (R800* r800, UInt16 addr) {
    RES_4_XNN(r800, addr); 
}

static void res_4_xnn_a(R800* r800, UInt16 addr) { 
    r800->regs.AF.B.h = RES_4_XNN(r800, addr); 
}

static void res_4_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_4_XNN(r800, addr);
}

static void res_4_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RES_4_XNN(r800, addr);
}

static void res_4_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_4_XNN(r800, addr);
}

static void res_4_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RES_4_XNN(r800, addr);
}

static void res_4_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RES_4_XNN(r800, addr);
}

static void res_4_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RES_4_XNN(r800, addr);
}

static void res_5_a(R800* r800) {
    RES(r800, 5, &r800->regs.AF.B.h);
}

static void res_5_b(R800* r800) {
    RES(r800, 5, &r800->regs.BC.B.h);
}

static void res_5_c(R800* r800) {
    RES(r800, 5, &r800->regs.BC.B.l);
}

static void res_5_d(R800* r800) {
    RES(r800, 5, &r800->regs.DE.B.h);
}

static void res_5_e(R800* r800) {
    RES(r800, 5, &r800->regs.DE.B.l);
}

static void res_5_h(R800* r800) {
    RES(r800, 5, &r800->regs.HL.B.h);
}

static void res_5_l(R800* r800) {
    RES(r800, 5, &r800->regs.HL.B.l);
}

static void res_5_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 5, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_5_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 5, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_5_xnn  (R800* r800, UInt16 addr) {
    RES_5_XNN(r800, addr);
}

static void res_5_xnn_a(R800* r800, UInt16 addr) { 
    r800->regs.AF.B.h = RES_5_XNN(r800, addr);
}

static void res_5_xnn_b(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.h = RES_5_XNN(r800, addr);
}

static void res_5_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = RES_5_XNN(r800, addr);
}

static void res_5_xnn_d(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.h = RES_5_XNN(r800, addr);
}

static void res_5_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = RES_5_XNN(r800, addr);
}

static void res_5_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = RES_5_XNN(r800, addr);
}

static void res_5_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = RES_5_XNN(r800, addr);
}

static void res_6_a(R800* r800) {
    RES(r800, 6, &r800->regs.AF.B.h);
}

static void res_6_b(R800* r800) {
    RES(r800, 6, &r800->regs.BC.B.h);
}

static void res_6_c(R800* r800) {
    RES(r800, 6, &r800->regs.BC.B.l); 
}

static void res_6_d(R800* r800) { 
    RES(r800, 6, &r800->regs.DE.B.h);
}

static void res_6_e(R800* r800) { 
    RES(r800, 6, &r800->regs.DE.B.l);
}

static void res_6_h(R800* r800) { 
    RES(r800, 6, &r800->regs.HL.B.h);
}

static void res_6_l(R800* r800) { 
    RES(r800, 6, &r800->regs.HL.B.l);
}

static void res_6_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 6, &val); 
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_6_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 6, &val);
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_6_xnn  (R800* r800, UInt16 addr) {
    RES_6_XNN(r800, addr);
}

static void res_6_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_6_XNN(r800, addr);
}

static void res_6_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_6_XNN(r800, addr);
}

static void res_6_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = RES_6_XNN(r800, addr); 
}

static void res_6_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_6_XNN(r800, addr);
}

static void res_6_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = RES_6_XNN(r800, addr);
}

static void res_6_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = RES_6_XNN(r800, addr);
}

static void res_6_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = RES_6_XNN(r800, addr);
}

static void res_7_a(R800* r800) {
    RES(r800, 7, &r800->regs.AF.B.h);
}

static void res_7_b(R800* r800) {
    RES(r800, 7, &r800->regs.BC.B.h);
}

static void res_7_c(R800* r800) {
    RES(r800, 7, &r800->regs.BC.B.l);
}

static void res_7_d(R800* r800) { 
    RES(r800, 7, &r800->regs.DE.B.h);
}

static void res_7_e(R800* r800) {
    RES(r800, 7, &r800->regs.DE.B.l);
}

static void res_7_h(R800* r800) {
    RES(r800, 7, &r800->regs.HL.B.h);
}

static void res_7_l(R800* r800) {
    RES(r800, 7, &r800->regs.HL.B.l); 
}

static void res_7_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    RES(r800, 7, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 RES_7_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    RES(r800, 7, &val);
    delayBit(r800)              // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void res_7_xnn  (R800* r800, UInt16 addr) {
    RES_7_XNN(r800, addr); 
}

static void res_7_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = RES_7_XNN(r800, addr);
}

static void res_7_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = RES_7_XNN(r800, addr); 
}

static void res_7_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = RES_7_XNN(r800, addr); 
}

static void res_7_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = RES_7_XNN(r800, addr);
}

static void res_7_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = RES_7_XNN(r800, addr); 
}

static void res_7_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = RES_7_XNN(r800, addr); 
}

static void res_7_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = RES_7_XNN(r800, addr); 
}

static void set_0_a(R800* r800) {
    SET(r800, 0, &r800->regs.AF.B.h); 
}

static void set_0_b(R800* r800) {
    SET(r800, 0, &r800->regs.BC.B.h);
}

static void set_0_c(R800* r800) {
    SET(r800, 0, &r800->regs.BC.B.l);
}

static void set_0_d(R800* r800) {
    SET(r800, 0, &r800->regs.DE.B.h);
}

static void set_0_e(R800* r800) { 
    SET(r800, 0, &r800->regs.DE.B.l);
}

static void set_0_h(R800* r800) { 
    SET(r800, 0, &r800->regs.HL.B.h);
}

static void set_0_l(R800* r800) {
    SET(r800, 0, &r800->regs.HL.B.l);
}

static void set_0_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 0, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_0_XNN(R800* r800, UInt16 addr) {

    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;

    SET(r800, 0, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);

    writeMem(r800, addr, val);
    return val;
}

static void set_0_xnn  (R800* r800, UInt16 addr) { 
    SET_0_XNN(r800, addr);
}

static void set_0_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_0_XNN(r800, addr);
}

static void set_0_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_0_XNN(r800, addr);
}

static void set_0_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_0_XNN(r800, addr);
}

static void set_0_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_0_XNN(r800, addr);
}

static void set_0_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SET_0_XNN(r800, addr);
}

static void set_0_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = SET_0_XNN(r800, addr); 
}

static void set_0_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SET_0_XNN(r800, addr);
}

static void set_1_a(R800* r800) {
    SET(r800, 1, &r800->regs.AF.B.h);
}

static void set_1_b(R800* r800) {
    SET(r800, 1, &r800->regs.BC.B.h);
}

static void set_1_c(R800* r800) {
    SET(r800, 1, &r800->regs.BC.B.l);
}

static void set_1_d(R800* r800) {
    SET(r800, 1, &r800->regs.DE.B.h);
}

static void set_1_e(R800* r800) {
    SET(r800, 1, &r800->regs.DE.B.l);
}

static void set_1_h(R800* r800) {
    SET(r800, 1, &r800->regs.HL.B.h);
}

static void set_1_l(R800* r800) { 
    SET(r800, 1, &r800->regs.HL.B.l);
}

static void set_1_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 1, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_1_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 1, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_1_xnn  (R800* r800, UInt16 addr) { 
    SET_1_XNN(r800, addr);
}

static void set_1_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_1_XNN(r800, addr); 
}

static void set_1_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_1_XNN(r800, addr);
}

static void set_1_xnn_c(R800* r800, UInt16 addr) { 
    r800->regs.BC.B.l = SET_1_XNN(r800, addr);
}

static void set_1_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_1_XNN(r800, addr);
}

static void set_1_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = SET_1_XNN(r800, addr);
}

static void set_1_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = SET_1_XNN(r800, addr);
}

static void set_1_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SET_1_XNN(r800, addr);
}

static void set_2_a(R800* r800) { 
    SET(r800, 2, &r800->regs.AF.B.h);
}

static void set_2_b(R800* r800) {
    SET(r800, 2, &r800->regs.BC.B.h); 
}

static void set_2_c(R800* r800) {
    SET(r800, 2, &r800->regs.BC.B.l); 
}

static void set_2_d(R800* r800) { 
    SET(r800, 2, &r800->regs.DE.B.h);
}

static void set_2_e(R800* r800) { 
    SET(r800, 2, &r800->regs.DE.B.l);
}

static void set_2_h(R800* r800) {
    SET(r800, 2, &r800->regs.HL.B.h);
}

static void set_2_l(R800* r800) {
    SET(r800, 2, &r800->regs.HL.B.l);
}

static void set_2_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 2, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_2_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 2, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_2_xnn  (R800* r800, UInt16 addr) {
    SET_2_XNN(r800, addr);
}

static void set_2_xnn_a(R800* r800, UInt16 addr) { 
    r800->regs.AF.B.h = SET_2_XNN(r800, addr); 
}

static void set_2_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_2_XNN(r800, addr); 
}

static void set_2_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_2_XNN(r800, addr);
}

static void set_2_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_2_XNN(r800, addr);
}

static void set_2_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = SET_2_XNN(r800, addr);
}

static void set_2_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = SET_2_XNN(r800, addr); 
}

static void set_2_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SET_2_XNN(r800, addr); 
}

static void set_3_a(R800* r800) {
    SET(r800, 3, &r800->regs.AF.B.h);
}

static void set_3_b(R800* r800) {
    SET(r800, 3, &r800->regs.BC.B.h);
}

static void set_3_c(R800* r800) {
    SET(r800, 3, &r800->regs.BC.B.l);
}

static void set_3_d(R800* r800) { 
    SET(r800, 3, &r800->regs.DE.B.h);
}

static void set_3_e(R800* r800) { 
    SET(r800, 3, &r800->regs.DE.B.l);
}

static void set_3_h(R800* r800) { 
    SET(r800, 3, &r800->regs.HL.B.h);
}

static void set_3_l(R800* r800) { 
    SET(r800, 3, &r800->regs.HL.B.l);
}

static void set_3_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 3, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_3_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 3, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_3_xnn  (R800* r800, UInt16 addr) {
    SET_3_XNN(r800, addr);
}

static void set_3_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_3_XNN(r800, addr); 
}

static void set_3_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_3_XNN(r800, addr);
}

static void set_3_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_3_XNN(r800, addr);
}

static void set_3_xnn_d(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.h = SET_3_XNN(r800, addr);
}

static void set_3_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SET_3_XNN(r800, addr);
}

static void set_3_xnn_h(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.h = SET_3_XNN(r800, addr);
}

static void set_3_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SET_3_XNN(r800, addr);
}

static void set_4_a(R800* r800) {
    SET(r800, 4, &r800->regs.AF.B.h);
}

static void set_4_b(R800* r800) {
    SET(r800, 4, &r800->regs.BC.B.h);
}

static void set_4_c(R800* r800) {
    SET(r800, 4, &r800->regs.BC.B.l);
}

static void set_4_d(R800* r800) {
    SET(r800, 4, &r800->regs.DE.B.h);
}

static void set_4_e(R800* r800) {
    SET(r800, 4, &r800->regs.DE.B.l);
}

static void set_4_h(R800* r800) {
    SET(r800, 4, &r800->regs.HL.B.h); 
}

static void set_4_l(R800* r800) {
    SET(r800, 4, &r800->regs.HL.B.l); 
}

static void set_4_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 4, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_4_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 4, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_4_xnn  (R800* r800, UInt16 addr) {
    SET_4_XNN(r800, addr);
}

static void set_4_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_4_XNN(r800, addr);
}

static void set_4_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_4_XNN(r800, addr);
}

static void set_4_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_4_XNN(r800, addr);
}

static void set_4_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_4_XNN(r800, addr);
}

static void set_4_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SET_4_XNN(r800, addr);
}

static void set_4_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SET_4_XNN(r800, addr);
}

static void set_4_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SET_4_XNN(r800, addr);
}

static void set_5_a(R800* r800) { 
    SET(r800, 5, &r800->regs.AF.B.h); 
}

static void set_5_b(R800* r800) {
    SET(r800, 5, &r800->regs.BC.B.h);
}

static void set_5_c(R800* r800) {
    SET(r800, 5, &r800->regs.BC.B.l);
}

static void set_5_d(R800* r800) { 
    SET(r800, 5, &r800->regs.DE.B.h);
}

static void set_5_e(R800* r800) { 
    SET(r800, 5, &r800->regs.DE.B.l); 
}

static void set_5_h(R800* r800) { 
    SET(r800, 5, &r800->regs.HL.B.h); 
}

static void set_5_l(R800* r800) { 
    SET(r800, 5, &r800->regs.HL.B.l);
}

static void set_5_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 5, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_5_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 5, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_5_xnn  (R800* r800, UInt16 addr) {
    SET_5_XNN(r800, addr);
}

static void set_5_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_5_XNN(r800, addr);
}

static void set_5_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_5_XNN(r800, addr);
}

static void set_5_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_5_XNN(r800, addr);
}

static void set_5_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_5_XNN(r800, addr);
}

static void set_5_xnn_e(R800* r800, UInt16 addr) { 
    r800->regs.DE.B.l = SET_5_XNN(r800, addr);
}

static void set_5_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SET_5_XNN(r800, addr);
}

static void set_5_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SET_5_XNN(r800, addr);
}

static void set_6_a(R800* r800) {
    SET(r800, 6, &r800->regs.AF.B.h);
}

static void set_6_b(R800* r800) {
    SET(r800, 6, &r800->regs.BC.B.h);
}

static void set_6_c(R800* r800) {
    SET(r800, 6, &r800->regs.BC.B.l);
}

static void set_6_d(R800* r800) {
    SET(r800, 6, &r800->regs.DE.B.h);
}

static void set_6_e(R800* r800) {
    SET(r800, 6, &r800->regs.DE.B.l); 
}

static void set_6_h(R800* r800) {
    SET(r800, 6, &r800->regs.HL.B.h);
}

static void set_6_l(R800* r800) { 
    SET(r800, 6, &r800->regs.HL.B.l); 
}

static void set_6_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 6, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_6_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 6, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_6_xnn  (R800* r800, UInt16 addr) {
    SET_6_XNN(r800, addr); 
}

static void set_6_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_6_XNN(r800, addr); 
}

static void set_6_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_6_XNN(r800, addr);
}

static void set_6_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_6_XNN(r800, addr);
}

static void set_6_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_6_XNN(r800, addr);
}

static void set_6_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SET_6_XNN(r800, addr); 
}

static void set_6_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SET_6_XNN(r800, addr);
}

static void set_6_xnn_l(R800* r800, UInt16 addr) { 
    r800->regs.HL.B.l = SET_6_XNN(r800, addr);
}

static void set_7_a(R800* r800) {
    SET(r800, 7, &r800->regs.AF.B.h); 
}

static void set_7_b(R800* r800) {
    SET(r800, 7, &r800->regs.BC.B.h); 
}

static void set_7_c(R800* r800) {
    SET(r800, 7, &r800->regs.BC.B.l); 
}

static void set_7_d(R800* r800) {
    SET(r800, 7, &r800->regs.DE.B.h); 
}

static void set_7_e(R800* r800) {
    SET(r800, 7, &r800->regs.DE.B.l);
}

static void set_7_h(R800* r800) {
    SET(r800, 7, &r800->regs.HL.B.h);
}

static void set_7_l(R800* r800) {
    SET(r800, 7, &r800->regs.HL.B.l); 
}

static void set_7_xhl(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    SET(r800, 7, &val); 
    delayInc(r800);
    writeMem(r800, r800->regs.HL.W, val);
}

static UInt8 SET_7_XNN(R800* r800, UInt16 addr) {
    UInt8 val = readMem(r800, addr);
    r800->regs.SH.W = addr;
    SET(r800, 7, &val);
    delayBit(r800);             // white cat append
    delayInc(r800);
    writeMem(r800, addr, val);
    return val;
}

static void set_7_xnn  (R800* r800, UInt16 addr) {
    SET_7_XNN(r800, addr);
}

static void set_7_xnn_a(R800* r800, UInt16 addr) {
    r800->regs.AF.B.h = SET_7_XNN(r800, addr);
}

static void set_7_xnn_b(R800* r800, UInt16 addr) {
    r800->regs.BC.B.h = SET_7_XNN(r800, addr);
}

static void set_7_xnn_c(R800* r800, UInt16 addr) {
    r800->regs.BC.B.l = SET_7_XNN(r800, addr);
}

static void set_7_xnn_d(R800* r800, UInt16 addr) {
    r800->regs.DE.B.h = SET_7_XNN(r800, addr);
}

static void set_7_xnn_e(R800* r800, UInt16 addr) {
    r800->regs.DE.B.l = SET_7_XNN(r800, addr); 
}

static void set_7_xnn_h(R800* r800, UInt16 addr) {
    r800->regs.HL.B.h = SET_7_XNN(r800, addr);
}

static void set_7_xnn_l(R800* r800, UInt16 addr) {
    r800->regs.HL.B.l = SET_7_XNN(r800, addr);
}

static void ex_af_af(R800* r800) {
    UInt16 regVal = r800->regs.AF.W;
    r800->regs.AF.W = r800->regs.AF1.W;
    r800->regs.AF1.W = regVal;
}

static void djnz(R800* r800) {
    delayDjnz(r800);
    r800->regs.BC.B.h--;
    if (r800->regs.BC.B.h != 0) {
        JR(r800);
    }
    else {
        SKIP_JR(r800);
    }
}

static void jr(R800* r800) {
    JR(r800);
}

static void jr_z(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        JR(r800);
    }
    else {
        SKIP_JR(r800);
    }
}

static void jr_nz(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        SKIP_JR(r800);
    }
    else {
        JR(r800);
    }
}

static void jr_c(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        JR(r800);
    }
    else {
        SKIP_JR(r800);
    }
}

static void jr_nc(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        SKIP_JR(r800);
    }
    else {
        JR(r800);
    }
}

static void jp(R800* r800) {
    JP(r800);
}

static void jp_hl(R800* r800) { 
    r800->regs.PC.W = r800->regs.HL.W; 
}

static void jp_ix(R800* r800) { 
    r800->regs.PC.W = r800->regs.IX.W; 
}

static void jp_iy(R800* r800) { 
    r800->regs.PC.W = r800->regs.IY.W; 
}

static void jp_z(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        JP(r800);
    }
    else {
        SKIP_JP(r800);
    }
}

static void jp_nz(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        SKIP_JP(r800);
    }
    else {
        JP(r800);
    }
}

static void jp_c(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        JP(r800);
    }
    else {
        SKIP_JP(r800);
    }
}

static void jp_nc(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        SKIP_JP(r800);
    }
    else {
        JP(r800);
    }
}

static void jp_m(R800* r800) {
    if (r800->regs.AF.B.l & S_FLAG) {
        JP(r800);
    }
    else {
        SKIP_JP(r800);
    }
}

static void jp_p(R800* r800) {
    if (r800->regs.AF.B.l & S_FLAG) {
        SKIP_JP(r800);
    }
    else {
        JP(r800);
    }
}

static void jp_pe(R800* r800) {
    if (r800->regs.AF.B.l & V_FLAG) {
        JP(r800);
    }
    else {
        SKIP_JP(r800);
    }
}

static void jp_po(R800* r800) {
    if (r800->regs.AF.B.l & V_FLAG) {
        SKIP_JP(r800);
    }
    else {
        JP(r800);
    }
}

static void call(R800* r800) {
    CALL(r800);
}

static void call_z(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        CALL(r800);
    }
    else {
        SKIP_CALL(r800);
    }
}

static void call_nz(R800* r800) {
    if (r800->regs.AF.B.l & Z_FLAG) {
        SKIP_CALL(r800);
    }
    else {
        CALL(r800);
    }
}

static void call_c(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        CALL(r800);
    }
    else {
        SKIP_CALL(r800);
    }
}

static void call_nc(R800* r800) {
    if (r800->regs.AF.B.l & C_FLAG) {
        SKIP_CALL(r800);
    }
    else {
        CALL(r800);
    }
}

static void call_m(R800* r800) {
    if (r800->regs.AF.B.l & S_FLAG) {
        CALL(r800);
    }
    else {
        SKIP_CALL(r800);
    }
}

static void call_p(R800* r800) {
    if (r800->regs.AF.B.l & S_FLAG) {
        SKIP_CALL(r800);
    }
    else {
        CALL(r800);
    }
}

static void call_pe(R800* r800) {
    if (r800->regs.AF.B.l & V_FLAG) {
        CALL(r800);
    }
    else {
        SKIP_CALL(r800);
    }
}

static void call_po(R800* r800) {
    if (r800->regs.AF.B.l & V_FLAG) {
        SKIP_CALL(r800);
    }
    else {
        CALL(r800);
    }
}

static void ret(R800* r800) {
    RET(r800);
}

static void ret_c(R800* r800) {
    delayRet(r800);
    if (r800->regs.AF.B.l & C_FLAG) {
        RET(r800);
    }
}

static void ret_nc(R800* r800) {
    delayRet(r800);
    if (!(r800->regs.AF.B.l & C_FLAG)) {
        RET(r800);
    }
}

static void ret_z(R800* r800) {
    delayRet(r800);
    if (r800->regs.AF.B.l & Z_FLAG) {
        RET(r800);
    }
}

static void ret_nz(R800* r800) {
    delayRet(r800);
    if (!(r800->regs.AF.B.l & Z_FLAG)) {
        RET(r800);
    }
}

static void ret_m(R800* r800) {
    delayRet(r800);
    if (r800->regs.AF.B.l & S_FLAG) {
        RET(r800);
    }
}

static void ret_p(R800* r800) {
    delayRet(r800);
    if (!(r800->regs.AF.B.l & S_FLAG)) {
        RET(r800);
    }
}

static void ret_pe(R800* r800) {
    delayRet(r800);
    if (r800->regs.AF.B.l & V_FLAG) {
        RET(r800);
    }
}

static void ret_po(R800* r800) {
    delayRet(r800);
    if (!(r800->regs.AF.B.l & V_FLAG)) {
        RET(r800);
    }
}

static void reti(R800* r800) {
    r800->regs.iff1 = r800->regs.iff2;
    RET(r800);
}

static void retn(R800* r800) {
    r800->regs.iff1 = r800->regs.iff2;
    RET(r800); 
}

static void ex_xsp_hl(R800* r800) { 
    EX_SP(r800, &r800->regs.HL.W);
}

static void ex_xsp_ix(R800* r800) { 
    EX_SP(r800, &r800->regs.IX.W); 
}

static void ex_xsp_iy(R800* r800) { 

    EX_SP(r800, &r800->regs.IY.W); 

}

static void ex_de_hl(R800* r800) {
    UInt16 tmp = r800->regs.DE.W;
    r800->regs.DE.W  = r800->regs.HL.W;
    r800->regs.HL.W  = tmp;
}


static void rlca(R800* r800) {
    UInt8 regVal = r800->regs.AF.B.h;
    r800->regs.AF.B.h = (regVal << 1) | (regVal >> 7);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG)) |
        (r800->regs.AF.B.h & (Y_FLAG | X_FLAG | C_FLAG));
}

static void rrca(R800* r800) {

    UInt8 regVal = r800->regs.AF.B.h;
    r800->regs.AF.B.h = (regVal >> 1) | (regVal << 7);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG)) | 
        (regVal &  C_FLAG) | (r800->regs.AF.B.h & (X_FLAG | Y_FLAG));
}

static void rla(R800* r800) {
    UInt8 regVal = r800->regs.AF.B.h;
    r800->regs.AF.B.h = (regVal << 1) | (r800->regs.AF.B.l & C_FLAG);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG)) |
        ((regVal >> 7) & C_FLAG) | (r800->regs.AF.B.h & (X_FLAG | Y_FLAG));
}

static void rra(R800* r800) {
    UInt8 regVal = r800->regs.AF.B.h;
    r800->regs.AF.B.h = (regVal >> 1) | ((r800->regs.AF.B.l & C_FLAG) << 7);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG)) |
        (regVal & C_FLAG) | (r800->regs.AF.B.h & (X_FLAG | Y_FLAG));
}

static void daa(R800* r800) {
    int regVal = r800->regs.AF.B.l;
    r800->regs.AF.W = DAATable[(int)r800->regs.AF.B.h | ((regVal & 3) << 8) | 
        ((regVal & 0x10) << 6)];
}

static void cpl(R800* r800) {
    r800->regs.AF.B.h ^= 0xff;
    r800->regs.AF.B.l = 
        (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG | C_FLAG)) |
        H_FLAG | N_FLAG | (r800->regs.AF.B.h & (X_FLAG | Y_FLAG));
}

static void scf(R800* r800) {
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG)) |
        C_FLAG | ((r800->regs.AF.B.l | r800->regs.AF.B.h) & (X_FLAG | Y_FLAG));
}

static void ccf(R800* r800) { //DIFF
    r800->regs.AF.B.l = 
        ((r800->regs.AF.B.l & (S_FLAG | Z_FLAG | P_FLAG | C_FLAG)) |
        ((r800->regs.AF.B.l & C_FLAG) << 4) |
        ((r800->regs.AF.B.l | r800->regs.AF.B.h) & (X_FLAG | Y_FLAG))) ^ C_FLAG;
}

static void halt(R800* r800) {
    if ((r800->intState == INT_LOW && r800->regs.iff1) || r800->nmiEdge) {
		r800->regs.halt=0;
    }
	else {
		r800->regs.PC.W--;
		r800->regs.halt=1;
	}
}

static void push_af(R800* r800) {
    PUSH(r800, &r800->regs.AF.W);
}

static void push_bc(R800* r800) {
    PUSH(r800, &r800->regs.BC.W); 
}

static void push_de(R800* r800) {
    PUSH(r800, &r800->regs.DE.W);
}

static void push_hl(R800* r800) {
    PUSH(r800, &r800->regs.HL.W);
}

static void push_ix(R800* r800) {
    PUSH(r800, &r800->regs.IX.W);
}

static void push_iy(R800* r800) { 
    PUSH(r800, &r800->regs.IY.W);
}

static void pop_af(R800* r800) {
    POP(r800, &r800->regs.AF.W);
}

static void pop_bc(R800* r800) {
    POP(r800, &r800->regs.BC.W);
}

static void pop_de(R800* r800) {
    POP(r800, &r800->regs.DE.W);
}

static void pop_hl(R800* r800) {
    POP(r800, &r800->regs.HL.W); 
}

static void pop_ix(R800* r800) {
    POP(r800, &r800->regs.IX.W); 
}

static void pop_iy(R800* r800) {
    POP(r800, &r800->regs.IY.W);
}

static void rst_00(R800* r800) {
    RST(r800, 0x00);
}
static void rst_08(R800* r800) {
    RST(r800, 0x08);
}
static void rst_10(R800* r800) {
    RST(r800, 0x10);
}
static void rst_18(R800* r800) {
    RST(r800, 0x18);
}
static void rst_20(R800* r800) {
    RST(r800, 0x20);
}
static void rst_28(R800* r800) {
    RST(r800, 0x28);
}
static void rst_30(R800* r800) {
    RST(r800, 0x30);
}
static void rst_38(R800* r800) {
    RST(r800, 0x38);
}

static void out_byte_a(R800* r800) {
    RegisterPair port;
    port.B.l = readOpcode(r800, r800->regs.PC.W++);
    port.B.h = r800->regs.AF.B.h;
    writePort(r800, port.W, r800->regs.AF.B.h);
}

static void in_a_byte(R800* r800) {
    RegisterPair port;
    port.B.l = readOpcode(r800, r800->regs.PC.W++);
    port.B.h = r800->regs.AF.B.h;
    r800->regs.AF.B.h = readPort(r800, port.W);
}

static void exx(R800* r800) {
    UInt16 tmp;
    tmp        = r800->regs.BC.W; 
    r800->regs.BC.W  = r800->regs.BC1.W; 
    r800->regs.BC1.W = tmp;
    tmp        = r800->regs.DE.W; 
    r800->regs.DE.W  = r800->regs.DE1.W; 
    r800->regs.DE1.W = tmp;
    tmp        = r800->regs.HL.W; 
    r800->regs.HL.W  = r800->regs.HL1.W; 
    r800->regs.HL1.W = tmp;
}

static void rld(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    r800->regs.SH.W = r800->regs.HL.W + 1;
    delayRld(r800);
    writeMem(r800, r800->regs.HL.W, (val << 4) | (r800->regs.AF.B.h & 0x0f));
    r800->regs.AF.B.h = (r800->regs.AF.B.h & 0xf0) | (val >> 4);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.AF.B.h];
}

static void rrd(R800* r800) {
    UInt8 val = readMem(r800, r800->regs.HL.W);
    r800->regs.SH.W = r800->regs.HL.W + 1;
    delayRld(r800);
    writeMem(r800, r800->regs.HL.W, (val >> 4) | (r800->regs.AF.B.h << 4));
    r800->regs.AF.B.h = (r800->regs.AF.B.h & 0xf0) | (val & 0x0f);
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.AF.B.h];
}

static void di(R800* r800) {
    r800->regs.iff1 = 0;
    r800->regs.iff2 = 0;
}

static void ei(R800* r800) {
/*    if (!r800->regs.iff1) {
        r800->regs.iff2 = 1;
        r800->regs.iff1 = 2;
    }*/
        r800->regs.iff2 = 1;
        r800->regs.iff1 = 1;
		r800->regs.ei_mode=1;
}

static void im_0(R800* r800)  {
    r800->regs.im = 0;
}

static void im_1(R800* r800)  {
    r800->regs.im = 1;
}

static void im_2(R800* r800)  {
    r800->regs.im = 2;
}

static void in_a_c(R800* r800) { 
    r800->regs.AF.B.h = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.AF.B.h]; 
}

static void in_b_c(R800* r800) { 
    r800->regs.BC.B.h = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.BC.B.h]; 
}

static void in_c_c(R800* r800) { 
    r800->regs.BC.B.l = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.BC.B.l]; 
}

static void in_d_c(R800* r800) { 
    r800->regs.DE.B.h = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.DE.B.h]; 
}

static void in_e_c(R800* r800) { 
    r800->regs.DE.B.l = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.DE.B.l]; 
}

static void out_c_a(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.AF.B.h); 
}

static void out_c_b(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.BC.B.h);
}

static void out_c_c(R800* r800)   { 
    writePort(r800, r800->regs.BC.W, r800->regs.BC.B.l);
}

static void out_c_d(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.DE.B.h);
}

static void out_c_e(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.DE.B.l);
}

static void out_c_h(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.HL.B.h);
}

static void out_c_l(R800* r800)   {
    writePort(r800, r800->regs.BC.W, r800->regs.HL.B.l);
}

static void out_c_0(R800* r800)   { 
    writePort(r800, r800->regs.BC.W, 0); 
}

static void in_h_c(R800* r800) { 
    r800->regs.HL.B.h = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.HL.B.h]; 
}

static void in_l_c(R800* r800) { 
    r800->regs.HL.B.l = readPort(r800, r800->regs.BC.W); 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[r800->regs.HL.B.l];
}

static void in_0_c(R800* r800) { 
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ZSPXYTable[readPort(r800, r800->regs.BC.W)]; 
}

static void cpi(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W++);
    UInt8 rv = r800->regs.AF.B.h - val;
    delayBlock(r800);

    r800->regs.BC.W--;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ((r800->regs.AF.B.h ^ val ^ rv) & H_FLAG) | 
        (ZSPXYTable[rv & 0xff] & (Z_FLAG | S_FLAG)) | N_FLAG;
    rv -= (r800->regs.AF.B.l & H_FLAG) >> 4;
    r800->regs.AF.B.l |= ((rv << 4) & Y_FLAG) | (rv & X_FLAG) | 
        (r800->regs.BC.W ? P_FLAG : 0);
}

static void cpir(R800* r800) { 
    cpi(r800);
    if (r800->regs.BC.W && !(r800->regs.AF.B.l & Z_FLAG)) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2;
        r800->instCnt--;
    }
}

static void cpd(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W--);
    UInt8 rv = r800->regs.AF.B.h - val;
    delayBlock(r800);

    r800->regs.BC.W--;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & C_FLAG) | 
        ((r800->regs.AF.B.h ^ val ^ rv) & H_FLAG) | 
        (ZSPXYTable[rv & 0xff] & (Z_FLAG | S_FLAG)) | N_FLAG;
    rv -= (r800->regs.AF.B.l & H_FLAG) >> 4;
    r800->regs.AF.B.l |= ((rv << 4) & Y_FLAG) | (rv & X_FLAG) |
        (r800->regs.BC.W ? P_FLAG : 0);
}

static void cpdr(R800* r800) { 
    cpd(r800);
    if (r800->regs.BC.W && !(r800->regs.AF.B.l & Z_FLAG)) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2;
        r800->instCnt--;
    }
}

static void ldi(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W++);
    writeMem(r800, r800->regs.DE.W++, val);
    delayLdi(r800);

    r800->regs.BC.W--;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | C_FLAG)) |
        (((r800->regs.AF.B.h + val) << 4) & Y_FLAG) | 
        ((r800->regs.AF.B.h + val) & X_FLAG) | (r800->regs.BC.W ? P_FLAG : 0);
}

static void ldir(R800* r800) { 
    ldi(r800);
    if (r800->regs.BC.W != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}

static void ldd(R800* r800) { 
    UInt8 val = readMem(r800, r800->regs.HL.W--);
    writeMem(r800, r800->regs.DE.W--, val);
    delayLdi(r800);

    r800->regs.BC.W--;
    r800->regs.AF.B.l = (r800->regs.AF.B.l & (S_FLAG | Z_FLAG | C_FLAG)) |
        (((r800->regs.AF.B.h + val) << 4) & Y_FLAG) | 
        ((r800->regs.AF.B.h + val) & X_FLAG) | (r800->regs.BC.W ? P_FLAG : 0);
}

static void lddr(R800* r800) { 
    ldd(r800);
    if (r800->regs.BC.W != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}

static void ini(R800* r800) {  // Diff on flags
    UInt8  val;
    UInt16 tmp;
    delayInOut(r800);
    r800->regs.BC.B.h--;
    val = readPort(r800, r800->regs.BC.W);
    writeMem(r800, r800->regs.HL.W++, val);
    r800->regs.AF.B.l = (ZSXYTable[r800->regs.BC.B.h]) |
        ((val >> 6) & N_FLAG);
    tmp = val + ((r800->regs.BC.B.l + 1) & 0xFF);
    r800->regs.AF.B.l |= (tmp >> 8) * (H_FLAG | C_FLAG) |
        (ZSPXYTable[(tmp & 0x07) ^ r800->regs.BC.B.h] & P_FLAG);
}

static void inir(R800* r800) { 
    ini(r800);
    if (r800->regs.BC.B.h != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}


static void ind(R800* r800) {
    UInt8 val;
    UInt16 tmp;
    delayInOut(r800);
    r800->regs.BC.B.h--;
    val = readPort(r800, r800->regs.BC.W);
    writeMem(r800, r800->regs.HL.W--, val);
    r800->regs.AF.B.l = (ZSXYTable[r800->regs.BC.B.h]) | 
        ((val >> 6) & N_FLAG);
    tmp = val + ((r800->regs.BC.B.l - 1) & 0xFF);
    r800->regs.AF.B.l |= (tmp >> 8) * (H_FLAG | C_FLAG) |
        (ZSPXYTable[(tmp & 0x07) ^ r800->regs.BC.B.h] & P_FLAG);
}

static void indr(R800* r800) { 
    ind(r800);
    if (r800->regs.BC.B.h != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}

static void outi(R800* r800) {
    UInt8  val;
    UInt16 tmp;
    delayInOut(r800);
    val = readMem(r800, r800->regs.HL.W++);
    writePort(r800, r800->regs.BC.W, val);
    r800->regs.BC.B.h--;
    r800->regs.AF.B.l = (ZSXYTable[r800->regs.BC.B.h]) |
        ((val >> 6) & N_FLAG);
    tmp = val + r800->regs.HL.B.l;
    r800->regs.AF.B.l |= (tmp >> 8) * (H_FLAG | C_FLAG) |
        (ZSPXYTable[(tmp & 0x07) ^ r800->regs.BC.B.h] & P_FLAG);
}

static void otir(R800* r800) { 
    outi(r800);
    if (r800->regs.BC.B.h != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}

static void outd(R800* r800) {
    UInt8 val;
    UInt16 tmp;
    delayInOut(r800);
    val = readMem(r800, r800->regs.HL.W--);
    writePort(r800, r800->regs.BC.W, val);
    r800->regs.BC.B.h--;
    r800->regs.AF.B.l = (ZSXYTable[r800->regs.BC.B.h]) |
        ((val >> 6) & N_FLAG);
    tmp = val + r800->regs.HL.B.l;
    r800->regs.AF.B.l |= (tmp >> 8) * (H_FLAG | C_FLAG) |
        (ZSPXYTable[(tmp & 0x07) ^ r800->regs.BC.B.h] & P_FLAG);
}

static void otdr(R800* r800) { 
    outd(r800);
    if (r800->regs.BC.B.h != 0) {
        delayBlock(r800); 
        r800->regs.PC.W -= 2; 
        r800->instCnt--;
    }
}

static void patch(R800* r800) { 
    r800->patch(r800->ref, &r800->regs);
}

static Opcode opcodeMain[256] = {
    nop,         ld_bc_word,  ld_xbc_a,    inc_bc,      inc_b,       dec_b,       ld_b_byte,   rlca,
    ex_af_af,    add_hl_bc,   ld_a_xbc,    dec_bc,      inc_c,       dec_c,       ld_c_byte,   rrca,
    djnz,        ld_de_word,  ld_xde_a,    inc_de,      inc_d,       dec_d,       ld_d_byte,   rla,
    jr,          add_hl_de,   ld_a_xde,    dec_de,      inc_e,       dec_e,       ld_e_byte,   rra,
    jr_nz,       ld_hl_word,  ld_xword_hl, inc_hl,      inc_h,       dec_h,       ld_h_byte,   daa,
    jr_z,        add_hl_hl,   ld_hl_xword, dec_hl,      inc_l,       dec_l,       ld_l_byte,   cpl,
    jr_nc,       ld_sp_word,  ld_xbyte_a,  inc_sp,      inc_xhl,     dec_xhl,     ld_xhl_byte, scf,
    jr_c,        add_hl_sp,   ld_a_xbyte,  dec_sp,      inc_a,       dec_a,       ld_a_byte,   ccf,
    ld_b_b,      ld_b_c,      ld_b_d,      ld_b_e,      ld_b_h,      ld_b_l,      ld_b_xhl,    ld_b_a,
    ld_c_b,      ld_c_c,      ld_c_d,      ld_c_e,      ld_c_h,      ld_c_l,      ld_c_xhl,    ld_c_a,
    ld_d_b,      ld_d_c,      ld_d_d,      ld_d_e,      ld_d_h,      ld_d_l,      ld_d_xhl,    ld_d_a,
    ld_e_b,      ld_e_c,      ld_e_d,      ld_e_e,      ld_e_h,      ld_e_l,      ld_e_xhl,    ld_e_a,
    ld_h_b,      ld_h_c,      ld_h_d,      ld_h_e,      ld_h_h,      ld_h_l,      ld_h_xhl,    ld_h_a,
    ld_l_b,      ld_l_c,      ld_l_d,      ld_l_e,      ld_l_h,      ld_l_l,      ld_l_xhl,    ld_l_a,
    ld_xhl_b,    ld_xhl_c,    ld_xhl_d,    ld_xhl_e,    ld_xhl_h,    ld_xhl_l,    halt,        ld_xhl_a,
    ld_a_b,      ld_a_c,      ld_a_d,      ld_a_e,      ld_a_h,      ld_a_l,      ld_a_xhl,    ld_a_a,
    add_a_b,     add_a_c,     add_a_d,     add_a_e,     add_a_h,     add_a_l,     add_a_xhl,   add_a_a,
    adc_a_b,     adc_a_c,     adc_a_d,     adc_a_e,     adc_a_h,     adc_a_l,     adc_a_xhl,   adc_a_a,
    sub_b,       sub_c,       sub_d,       sub_e,       sub_h,       sub_l,       sub_xhl,     sub_a,
    sbc_a_b,     sbc_a_c,     sbc_a_d,     sbc_a_e,     sbc_a_h,     sbc_a_l,     sbc_a_xhl,   sbc_a_a,
    and_b,       and_c,       and_d,       and_e,       and_h,       and_l,       and_xhl,     and_a,
    xor_b,       xor_c,       xor_d,       xor_e,       xor_h,       xor_l,       xor_xhl,     xor_a,
    or_b,        or_c,        or_d,        or_e,        or_h,        or_l,        or_xhl,      or_a,
    cp_b,        cp_c,        cp_d,        cp_e,        cp_h,        cp_l,        cp_xhl,      cp_a,
    ret_nz,      pop_bc,      jp_nz,       jp,          call_nz,     push_bc,     add_a_byte,  rst_00,
    ret_z,       ret,         jp_z,        cb,          call_z,      call,        adc_a_byte,  rst_08,
    ret_nc,      pop_de,      jp_nc,       out_byte_a,  call_nc,     push_de,     sub_byte,    rst_10,
    ret_c,       exx,         jp_c,        in_a_byte,   call_c,      dd,          sbc_a_byte,  rst_18,
    ret_po,      pop_hl,      jp_po,       ex_xsp_hl,   call_po,     push_hl,     and_byte,    rst_20,
    ret_pe,      jp_hl,       jp_pe,       ex_de_hl,    call_pe,     ed,          xor_byte,    rst_28,
    ret_p,       pop_af,      jp_p,        di,          call_p,      push_af,     or_byte,     rst_30,
    ret_m,       ld_sp_hl,    jp_m,        ei,          call_m,      fd,          cp_byte,     rst_38
};

static Opcode opcodeCb[256] = {
    rlc_b,       rlc_c,       rlc_d,       rlc_e,       rlc_h,       rlc_l,       rlc_xhl,     rlc_a,
    rrc_b,       rrc_c,       rrc_d,       rrc_e,       rrc_h,       rrc_l,       rrc_xhl,     rrc_a,
    rl_b,        rl_c,        rl_d,        rl_e,        rl_h,        rl_l,        rl_xhl,      rl_a ,
    rr_b,        rr_c,        rr_d,        rr_e,        rr_h,        rr_l,        rr_xhl,      rr_a ,
    sla_b,       sla_c,       sla_d,       sla_e,       sla_h,       sla_l,       sla_xhl,     sla_a,
    sra_b,       sra_c,       sra_d,       sra_e,       sra_h,       sra_l,       sra_xhl,     sra_a,
    sll_b,       sll_c,       sll_d,       sll_e,       sll_h,       sll_l,       sll_xhl,     sll_a,
    srl_b,       srl_c,       srl_d,       srl_e,       srl_h,       srl_l,       srl_xhl,     srl_a,
    bit_0_b,     bit_0_c,     bit_0_d,     bit_0_e,     bit_0_h,     bit_0_l,     bit_0_xhl,   bit_0_a,
    bit_1_b,     bit_1_c,     bit_1_d,     bit_1_e,     bit_1_h,     bit_1_l,     bit_1_xhl,   bit_1_a,
    bit_2_b,     bit_2_c,     bit_2_d,     bit_2_e,     bit_2_h,     bit_2_l,     bit_2_xhl,   bit_2_a,
    bit_3_b,     bit_3_c,     bit_3_d,     bit_3_e,     bit_3_h,     bit_3_l,     bit_3_xhl,   bit_3_a,
    bit_4_b,     bit_4_c,     bit_4_d,     bit_4_e,     bit_4_h,     bit_4_l,     bit_4_xhl,   bit_4_a,
    bit_5_b,     bit_5_c,     bit_5_d,     bit_5_e,     bit_5_h,     bit_5_l,     bit_5_xhl,   bit_5_a,
    bit_6_b,     bit_6_c,     bit_6_d,     bit_6_e,     bit_6_h,     bit_6_l,     bit_6_xhl,   bit_6_a,
    bit_7_b,     bit_7_c,     bit_7_d,     bit_7_e,     bit_7_h,     bit_7_l,     bit_7_xhl,   bit_7_a,
    res_0_b,     res_0_c,     res_0_d,     res_0_e,     res_0_h,     res_0_l,     res_0_xhl,   res_0_a,
    res_1_b,     res_1_c,     res_1_d,     res_1_e,     res_1_h,     res_1_l,     res_1_xhl,   res_1_a,
    res_2_b,     res_2_c,     res_2_d,     res_2_e,     res_2_h,     res_2_l,     res_2_xhl,   res_2_a,
    res_3_b,     res_3_c,     res_3_d,     res_3_e,     res_3_h,     res_3_l,     res_3_xhl,   res_3_a,
    res_4_b,     res_4_c,     res_4_d,     res_4_e,     res_4_h,     res_4_l,     res_4_xhl,   res_4_a,
    res_5_b,     res_5_c,     res_5_d,     res_5_e,     res_5_h,     res_5_l,     res_5_xhl,   res_5_a,
    res_6_b,     res_6_c,     res_6_d,     res_6_e,     res_6_h,     res_6_l,     res_6_xhl,   res_6_a,
    res_7_b,     res_7_c,     res_7_d,     res_7_e,     res_7_h,     res_7_l,     res_7_xhl,   res_7_a,
    set_0_b,     set_0_c,     set_0_d,     set_0_e,     set_0_h,     set_0_l,     set_0_xhl,   set_0_a,
    set_1_b,     set_1_c,     set_1_d,     set_1_e,     set_1_h,     set_1_l,     set_1_xhl,   set_1_a,
    set_2_b,     set_2_c,     set_2_d,     set_2_e,     set_2_h,     set_2_l,     set_2_xhl,   set_2_a,
    set_3_b,     set_3_c,     set_3_d,     set_3_e,     set_3_h,     set_3_l,     set_3_xhl,   set_3_a,
    set_4_b,     set_4_c,     set_4_d,     set_4_e,     set_4_h,     set_4_l,     set_4_xhl,   set_4_a,
    set_5_b,     set_5_c,     set_5_d,     set_5_e,     set_5_h,     set_5_l,     set_5_xhl,   set_5_a,
    set_6_b,     set_6_c,     set_6_d,     set_6_e,     set_6_h,     set_6_l,     set_6_xhl,   set_6_a,
    set_7_b,     set_7_c,     set_7_d,     set_7_e,     set_7_h,     set_7_l,     set_7_xhl,   set_7_a
};

static Opcode opcodeDd[256] = {
    nop,         ld_bc_word,  ld_xbc_a,    inc_bc,      inc_b,       dec_b,       ld_b_byte,   rlca,
    ex_af_af,    add_ix_bc,   ld_a_xbc,    dec_bc,      inc_c,       dec_c,       ld_c_byte,   rrca,
    djnz,        ld_de_word,  ld_xde_a,    inc_de,      inc_d,       dec_d,       ld_d_byte,   rla,
    jr,          add_ix_de,   ld_a_xde,    dec_de,      inc_e,       dec_e,       ld_e_byte,   rra,
    jr_nz,       ld_ix_word,  ld_xword_ix, inc_ix,      inc_ixh,     dec_ixh,     ld_ixh_byte, daa,
    jr_z,        add_ix_ix,   ld_ix_xword, dec_ix,      inc_ixl,     dec_ixl,     ld_ixl_byte, cpl,
    jr_nc,       ld_sp_word,  ld_xbyte_a,  inc_sp,      inc_xix,     dec_xix,     ld_xix_byte, scf,
    jr_c,        add_ix_sp,   ld_a_xbyte,  dec_sp,      inc_a,       dec_a,       ld_a_byte,   ccf,
    ld_b_b,      ld_b_c,      ld_b_d,      ld_b_e,      ld_b_ixh,    ld_b_ixl,    ld_b_xix,    ld_b_a,
    ld_c_b,      ld_c_c,      ld_c_d,      ld_c_e,      ld_c_ixh,    ld_c_ixl,    ld_c_xix,    ld_c_a,
    ld_d_b,      ld_d_c,      ld_d_d,      ld_d_e,      ld_d_ixh,    ld_d_ixl,    ld_d_xix,    ld_d_a,
    ld_e_b,      ld_e_c,      ld_e_d,      ld_e_e,      ld_e_ixh,    ld_e_ixl,    ld_e_xix,    ld_e_a,
    ld_ixh_b,    ld_ixh_c,    ld_ixh_d,    ld_ixh_e,    ld_ixh_ixh,  ld_ixh_ixl,  ld_h_xix,    ld_ixh_a,
    ld_ixl_b,    ld_ixl_c,    ld_ixl_d,    ld_ixl_e,    ld_ixl_ixh,  ld_ixl_ixl,  ld_l_xix,    ld_ixl_a,
    ld_xix_b,    ld_xix_c,    ld_xix_d,    ld_xix_e,    ld_xix_h,    ld_xix_l,    halt,        ld_xix_a,
    ld_a_b,      ld_a_c,      ld_a_d,      ld_a_e,      ld_a_ixh,    ld_a_ixl,    ld_a_xix,    ld_a_a,
    add_a_b,     add_a_c,     add_a_d,     add_a_e,     add_a_ixh,   add_a_ixl,   add_a_xix,   add_a_a,
    adc_a_b,     adc_a_c,     adc_a_d,     adc_a_e,     adc_a_ixh,   adc_a_ixl,   adc_a_xix,   adc_a_a,
    sub_b,       sub_c,       sub_d,       sub_e,       sub_ixh,     sub_ixl,     sub_xix,     sub_a,
    sbc_a_b,     sbc_a_c,     sbc_a_d,     sbc_a_e,     sbc_a_ixh,   sbc_a_ixl,   sbc_a_xix,   sbc_a_a,
    and_b,       and_c,       and_d,       and_e,       and_ixh,     and_ixl,     and_xix,     and_a,
    xor_b,       xor_c,       xor_d,       xor_e,       xor_ixh,     xor_ixl,     xor_xix,     xor_a,
    or_b,        or_c,        or_d,        or_e,        or_ixh,      or_ixl,      or_xix,      or_a,
    cp_b,        cp_c,        cp_d,        cp_e,        cp_ixh,      cp_ixl,      cp_xix,      cp_a,
    ret_nz,      pop_bc,      jp_nz,       jp,          call_nz,     push_bc,     add_a_byte,  rst_00,
    ret_z,       ret,         jp_z,        dd_cb,       call_z,      call,        adc_a_byte,  rst_08,
    ret_nc,      pop_de,      jp_nc,       out_byte_a,  call_nc,     push_de,     sub_byte,    rst_10,
    ret_c,       exx,         jp_c,        in_a_byte,   call_c,      dd,          sbc_a_byte,  rst_18,
    ret_po,      pop_ix,      jp_po,       ex_xsp_ix,   call_po,     push_ix,     and_byte,    rst_20,
    ret_pe,      jp_ix,       jp_pe,       ex_de_hl,    call_pe,     ed,          xor_byte,    rst_28,
    ret_p,       pop_af,      jp_p,        di,          call_p,      push_af,     or_byte,     rst_30,
    ret_m,       ld_sp_ix,    jp_m,        ei,          call_m,      fd,          cp_byte,     rst_38  
};

static Opcode opcodeEd[256] = {
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    in_b_c,      out_c_b,     sbc_hl_bc,   ld_xword_bc, neg,         retn,        im_0,        ld_i_a,
    in_c_c,      out_c_c,     adc_hl_bc,   ld_bc_xword, neg,         reti,        im_0,        ld_r_a,
    in_d_c,      out_c_d,     sbc_hl_de,   ld_xword_de, neg,         retn,        im_1,        ld_a_i,
    in_e_c,      out_c_e,     adc_hl_de,   ld_de_xword, neg,         retn,        im_2,        ld_a_r,
    in_h_c,      out_c_h,     sbc_hl_hl,   ld_xword_hl, neg,         retn,        im_0,        rrd,
    in_l_c,      out_c_l,     adc_hl_hl,   ld_hl_xword, neg,         retn,        im_0,        rld,
    in_0_c,      out_c_0,     sbc_hl_sp,   ld_xword_sp, neg,         retn,        im_1,        nop,
    in_a_c,      out_c_a,     adc_hl_sp,   ld_sp_xword, neg,         retn,        im_2,        nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    nop,         nop,         nop,         nop,         nop,         nop,         nop,         nop,
    ldi,         cpi,         ini,         outi,        nop,         nop,         nop,         nop,
    ldd,         cpd,         ind,         outd,        nop,         nop,         nop,         nop,
    ldir,        cpir,        inir,        otir,        nop,         nop,         nop,         nop,
    lddr,        cpdr,        indr,        otdr,        nop,         nop,         nop,         nop,
    nop,         mulu_b,      nop,         muluw_bc,    nop,         nop,         nop,         nop,
    nop,         mulu_c,      nop,         nop,         nop,         nop,         nop,         nop,
    nop,         mulu_d,      nop,         muluw_de,    nop,         nop,         nop,         nop,
    nop,         mulu_e,      nop,         nop,         nop,         nop,         nop,         nop,
    nop,         mulu_h,      nop,         muluw_hl,    nop,         nop,         nop,         nop,
    nop,         mulu_l,      nop,         nop,         nop,         nop,         nop,         nop,
    nop,         mulu_xhl,    nop,         muluw_sp,    nop,         nop,         nop,         nop,
    nop,         mulu_a,      nop,         nop,         nop,         nop,         patch,       nop
};

static Opcode opcodeFd[256] = {
    nop,         ld_bc_word,ld_xbc_a,      inc_bc,      inc_b,       dec_b,       ld_b_byte,   rlca,
    ex_af_af,    add_iy_bc,   ld_a_xbc,    dec_bc,      inc_c,       dec_c,       ld_c_byte,   rrca,
    djnz,        ld_de_word,  ld_xde_a,    inc_de,      inc_d,       dec_d,       ld_d_byte,   rla,
    jr,          add_iy_de,   ld_a_xde,    dec_de,      inc_e,       dec_e,       ld_e_byte,   rra,
    jr_nz,       ld_iy_word,  ld_xword_iy, inc_iy,      inc_iyh,     dec_iyh,     ld_iyh_byte, daa,
    jr_z,        add_iy_iy,   ld_iy_xword, dec_iy,      inc_iyl,     dec_iyl,     ld_iyl_byte, cpl,
    jr_nc,       ld_sp_word,  ld_xbyte_a,  inc_sp,      inc_xiy,     dec_xiy,     ld_xiy_byte, scf,
    jr_c,        add_iy_sp,   ld_a_xbyte,  dec_sp,      inc_a,       dec_a,       ld_a_byte,   ccf,
    ld_b_b,      ld_b_c,      ld_b_d,      ld_b_e,      ld_b_iyh,    ld_b_iyl,    ld_b_xiy,    ld_b_a,
    ld_c_b,      ld_c_c,      ld_c_d,      ld_c_e,      ld_c_iyh,    ld_c_iyl,    ld_c_xiy,    ld_c_a,
    ld_d_b,      ld_d_c,      ld_d_d,      ld_d_e,      ld_d_iyh,    ld_d_iyl,    ld_d_xiy,    ld_d_a,
    ld_e_b,      ld_e_c,      ld_e_d,      ld_e_e,      ld_e_iyh,    ld_e_iyl,    ld_e_xiy,    ld_e_a,
    ld_iyh_b,    ld_iyh_c,    ld_iyh_d,    ld_iyh_e,    ld_iyh_iyh,  ld_iyh_iyl,  ld_h_xiy,    ld_iyh_a,
    ld_iyl_b,    ld_iyl_c,    ld_iyl_d,    ld_iyl_e,    ld_iyl_iyh,  ld_iyl_iyl,  ld_l_xiy,    ld_iyl_a,
    ld_xiy_b,    ld_xiy_c,    ld_xiy_d,    ld_xiy_e,    ld_xiy_h,    ld_xiy_l,    halt,        ld_xiy_a,
    ld_a_b,      ld_a_c,      ld_a_d,      ld_a_e,      ld_a_iyh,    ld_a_iyl,    ld_a_xiy,    ld_a_a,
    add_a_b,     add_a_c,     add_a_d,     add_a_e,     add_a_iyh,   add_a_iyl,   add_a_xiy,   add_a_a,
    adc_a_b,     adc_a_c,     adc_a_d,     adc_a_e,     adc_a_iyh,   adc_a_iyl,   adc_a_xiy,   adc_a_a,
    sub_b,       sub_c,       sub_d,       sub_e,       sub_iyh,     sub_iyl,     sub_xiy,     sub_a,
    sbc_a_b,     sbc_a_c,     sbc_a_d,     sbc_a_e,     sbc_a_iyh,   sbc_a_iyl,   sbc_a_xiy,   sbc_a_a,
    and_b,       and_c,       and_d,       and_e,       and_iyh,     and_iyl,     and_xiy,     and_a,
    xor_b,       xor_c,       xor_d,       xor_e,       xor_iyh,     xor_iyl,     xor_xiy,     xor_a,
    or_b,        or_c,        or_d,        or_e,        or_iyh,      or_iyl,      or_xiy,      or_a,
    cp_b,        cp_c,        cp_d,        cp_e,        cp_iyh,      cp_iyl,      cp_xiy,      cp_a,
    ret_nz,      pop_bc,      jp_nz,       jp,          call_nz,     push_bc,     add_a_byte,  rst_00,
    ret_z,       ret,         jp_z,        fd_cb,       call_z,      call,        adc_a_byte,  rst_08,
    ret_nc,      pop_de,      jp_nc,       out_byte_a,  call_nc,     push_de,     sub_byte,    rst_10,
    ret_c,       exx,         jp_c,        in_a_byte,   call_c,      dd,          sbc_a_byte,  rst_18,
    ret_po,      pop_iy,      jp_po,       ex_xsp_iy,   call_po,     push_iy,     and_byte,    rst_20,
    ret_pe,      jp_iy,       jp_pe,       ex_de_hl,    call_pe,     ed,          xor_byte,    rst_28,
    ret_p,       pop_af,      jp_p,        di,          call_p,      push_af,     or_byte,     rst_30,
    ret_m,       ld_sp_iy,    jp_m,        ei,          call_m,      fd,          cp_byte,     rst_38  
};

static OpcodeNn opcodeNnCb[256] = {
    rlc_xnn_b,   rlc_xnn_c,   rlc_xnn_d,   rlc_xnn_e,   rlc_xnn_h,   rlc_xnn_l,   rlc_xnn,     rlc_xnn_a,
    rrc_xnn_b,   rrc_xnn_c,   rrc_xnn_d,   rrc_xnn_e,   rrc_xnn_h,   rrc_xnn_l,   rrc_xnn,     rrc_xnn_a,
    rl_xnn_b,    rl_xnn_c,    rl_xnn_d,    rl_xnn_e,    rl_xnn_h,    rl_xnn_l,    rl_xnn,      rl_xnn_a,
    rr_xnn_b,    rr_xnn_c,    rr_xnn_d,    rr_xnn_e,    rr_xnn_h,    rr_xnn_l,    rr_xnn,      rr_xnn_a,
    sla_xnn_b,   sla_xnn_c,   sla_xnn_d,   sla_xnn_e,   sla_xnn_h,   sla_xnn_l,   sla_xnn,     sla_xnn_a,   
    sra_xnn_b,   sra_xnn_c,   sra_xnn_d,   sra_xnn_e,   sra_xnn_h,   sra_xnn_l,   sra_xnn,     sra_xnn_a,
    sll_xnn_b,   sll_xnn_c,   sll_xnn_d,   sll_xnn_e,   sll_xnn_h,   sll_xnn_l,   sll_xnn,     sll_xnn_a,
    srl_xnn_b,   srl_xnn_c,   srl_xnn_d,   srl_xnn_e,   srl_xnn_h,   srl_xnn_l,   srl_xnn,     srl_xnn_a,
    bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   bit_0_xnn,   
    bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   bit_1_xnn,   
    bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   bit_2_xnn,   
    bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   bit_3_xnn,   
    bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   bit_4_xnn,   
    bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   bit_5_xnn,   
    bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   bit_6_xnn,   
    bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   bit_7_xnn,   
    res_0_xnn_b, res_0_xnn_c, res_0_xnn_d, res_0_xnn_e, res_0_xnn_h, res_0_xnn_l, res_0_xnn,   res_0_xnn_a,
    res_1_xnn_b, res_1_xnn_c, res_1_xnn_d, res_1_xnn_e, res_1_xnn_h, res_1_xnn_l, res_1_xnn,   res_1_xnn_a,
    res_2_xnn_b, res_2_xnn_c, res_2_xnn_d, res_2_xnn_e, res_2_xnn_h, res_2_xnn_l, res_2_xnn,   res_2_xnn_a,
    res_3_xnn_b, res_3_xnn_c, res_3_xnn_d, res_3_xnn_e, res_3_xnn_h, res_3_xnn_l, res_3_xnn,   res_3_xnn_a,
    res_4_xnn_b, res_4_xnn_c, res_4_xnn_d, res_4_xnn_e, res_4_xnn_h, res_4_xnn_l, res_4_xnn,   res_4_xnn_a,
    res_5_xnn_b, res_5_xnn_c, res_5_xnn_d, res_5_xnn_e, res_5_xnn_h, res_5_xnn_l, res_5_xnn,   res_5_xnn_a,
    res_6_xnn_b, res_6_xnn_c, res_6_xnn_d, res_6_xnn_e, res_6_xnn_h, res_6_xnn_l, res_6_xnn,   res_6_xnn_a,
    res_7_xnn_b, res_7_xnn_c, res_7_xnn_d, res_7_xnn_e, res_7_xnn_h, res_7_xnn_l, res_7_xnn,   res_7_xnn_a,
    set_0_xnn_b, set_0_xnn_c, set_0_xnn_d, set_0_xnn_e, set_0_xnn_h, set_0_xnn_l, set_0_xnn,   set_0_xnn_a,
    set_1_xnn_b, set_1_xnn_c, set_1_xnn_d, set_1_xnn_e, set_1_xnn_h, set_1_xnn_l, set_1_xnn,   set_1_xnn_a,
    set_2_xnn_b, set_2_xnn_c, set_2_xnn_d, set_2_xnn_e, set_2_xnn_h, set_2_xnn_l, set_2_xnn,   set_2_xnn_a,
    set_3_xnn_b, set_3_xnn_c, set_3_xnn_d, set_3_xnn_e, set_3_xnn_h, set_3_xnn_l, set_3_xnn,   set_3_xnn_a,
    set_4_xnn_b, set_4_xnn_c, set_4_xnn_d, set_4_xnn_e, set_4_xnn_h, set_4_xnn_l, set_4_xnn,   set_4_xnn_a,
    set_5_xnn_b, set_5_xnn_c, set_5_xnn_d, set_5_xnn_e, set_5_xnn_h, set_5_xnn_l, set_5_xnn,   set_5_xnn_a,
    set_6_xnn_b, set_6_xnn_c, set_6_xnn_d, set_6_xnn_e, set_6_xnn_h, set_6_xnn_l, set_6_xnn,   set_6_xnn_a,
    set_7_xnn_b, set_7_xnn_c, set_7_xnn_d, set_7_xnn_e, set_7_xnn_h, set_7_xnn_l, set_7_xnn,   set_7_xnn_a,
};

static void dd_cb(R800* r800) {
	UInt16 addr = r800->regs.IX.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    int opcode = readOpcode(r800, r800->regs.PC.W++);
	delayM1(r800);
    opcodeNnCb[opcode](r800, addr);
}

static void fd_cb(R800* r800) {
	UInt16 addr = r800->regs.IY.W + (Int8)readOpcode(r800, r800->regs.PC.W++);
    int opcode = readOpcode(r800, r800->regs.PC.W++);
	delayM1(r800);
    opcodeNnCb[opcode](r800, addr);
}

static void cb(R800* r800) {
    int opcode = readOpcode(r800, r800->regs.PC.W++);
    M1(r800);
    opcodeCb[opcode](r800);
}

static void dd(R800* r800) {
    int opcode = readOpcode(r800, r800->regs.PC.W++);
    M1(r800);
    opcodeDd[opcode](r800);
}

static void ed(R800* r800) {
    int opcode = readOpcode(r800, r800->regs.PC.W++);
    M1(r800);
    opcodeEd[opcode](r800);
}

static void fd(R800* r800) {
    int opcode = readOpcode(r800, r800->regs.PC.W++);
    M1(r800);
    opcodeFd[opcode](r800);
}

static void executeInstruction(R800* r800, UInt8 opcode) {
    M1(r800);
    r800->instCnt++;
    opcodeMain[opcode](r800);
}

static UInt8 readMemoryDummy(void* ref, UInt16 address) {
    return 0xff;
}

static void writeMemoryDummy(void* ref, UInt16 address, UInt8 value) {
}

static UInt8 readIoPortDummy(void* ref, UInt16 address) {
    return 0xff;
}

static void writeIoPortDummy(void* ref, UInt16 address, UInt8 value) {
}

static void  patchDummy(void* ref, CpuRegs* regs) {
}

static void  timerCbDummy(void* ref) {
}

static void breakpointCbDummy(void* ref, UInt16 pc) {
}

static void debugCbDummy(void* ref, int command, const char* data) {
}

static void trapCbDummy(void* ref, UInt8 value) {
}

static void r800InitTables() {
    int i;

	for (i = 0; i < 256; ++i) {
        UInt8 flags = i ^ 1;
        flags = flags ^ (flags >> 4);
        flags = flags ^ (flags << 2);
        flags = flags ^ (flags >> 1);
        flags = (flags & V_FLAG) | H_FLAG | (i & (S_FLAG | X_FLAG | Y_FLAG)) |
                (i ? 0 : Z_FLAG);

        ZSXYTable[i]  = flags & (Z_FLAG | S_FLAG | X_FLAG | Y_FLAG);
		ZSPXYTable[i] = flags & (Z_FLAG | S_FLAG | X_FLAG | Y_FLAG | V_FLAG);
		ZSPHTable[i]  = flags & (Z_FLAG | S_FLAG | V_FLAG | H_FLAG);
	}

    for (i = 0; i < 0x800; ++i) {
		int flagC = i & 0x100;
		int flagN = i & 0x200;
		int flagH = i & 0x400;
		UInt8 a = i & 0xff;
		UInt8 hi = a / 16;
		UInt8 lo = a & 15;
		UInt8 diff;
        UInt8 regA;

		if (flagC) {
			diff = ((lo <= 9) && !flagH) ? 0x60 : 0x66;
		} 
        else {
			if (lo >= 10) {
				diff = (hi <= 8) ? 0x06 : 0x66;
			} 
            else {
				if (hi >= 10) {
					diff = flagH ? 0x66 : 0x60;
				} 
                else {
					diff = flagH ? 0x06 : 0x00;
				}
			}
		}
		regA = flagN ? a - diff : a + diff;
		DAATable[i] = (regA << 8) |
                      ZSPXYTable[regA] | 
                      (flagN ? N_FLAG : 0) |
                      (flagC || (lo <= 9 ? hi >= 10 : hi >= 9) ? C_FLAG : 0) |
                      ((flagN ? (flagH && lo <= 5) : lo >= 10) ? H_FLAG : 0);
	}
}

static void r800SwitchCpu(R800* r800) {
    int freqAdjust;

    switch (r800->oldCpuMode) {
    case CPU_Z80:
        r800->regBanks[0] = r800->regs;
        break;
    case CPU_R800:
        r800->regBanks[1] = r800->regs;
        break;
    }

    r800->oldCpuMode = CPU_UNKNOWN;

    switch (r800->cpuMode) {
    case CPU_Z80:
        r800->regs = r800->regBanks[0];
        break;
    case CPU_R800:
        r800->regs = r800->regBanks[1];
        break;
    }

    switch (r800->cpuMode) {
    default:
    case CPU_Z80:
        freqAdjust = R800_MASTER_FREQUENCY / (r800->frequencyZ80 - 1);
        break;
    case CPU_R800:
        freqAdjust = R800_MASTER_FREQUENCY / (r800->frequencyR800 - 1);
        break;
    }

    switch (r800->cpuMode) {
    default:
    case CPU_Z80:
        r800->delay[DLY_MEM]       = freqAdjust * 3;
        r800->delay[DLY_MEMOP]     = freqAdjust * 3;
        r800->delay[DLY_MEMPAGE]   = freqAdjust * 0;
        r800->delay[DLY_PREIO]     = freqAdjust * 1;
        r800->delay[DLY_POSTIO]    = freqAdjust * 3;
        r800->delay[DLY_M1]        = freqAdjust * ((r800->cpuFlags & CPU_ENABLE_M1) ? 2 : 0);
        r800->delay[DLY_XD]        = freqAdjust * 1;
        r800->delay[DLY_IM]        = freqAdjust * 2; // should be 4, but currently will break vdp timing
        r800->delay[DLY_IM2]       = freqAdjust * 19;
        r800->delay[DLY_NMI]       = freqAdjust * 11;
        r800->delay[DLY_PARALLEL]  = freqAdjust * 2;
        r800->delay[DLY_BLOCK]     = freqAdjust * 5;
        r800->delay[DLY_ADD8]      = freqAdjust * 5;
        r800->delay[DLY_ADD16]     = freqAdjust * 7;
        r800->delay[DLY_BIT]       = freqAdjust * 1;
        r800->delay[DLY_CALL]      = freqAdjust * 1;
        r800->delay[DLY_DJNZ]      = freqAdjust * 1;
        r800->delay[DLY_EXSPHL]    = freqAdjust * 3;
        r800->delay[DLY_LD]        = freqAdjust * 1;
        r800->delay[DLY_LDI]       = freqAdjust * 2;
        r800->delay[DLY_INC]       = freqAdjust * 1;
        r800->delay[DLY_INC16]     = freqAdjust * 2;
        r800->delay[DLY_INOUT]     = freqAdjust * 1;
        r800->delay[DLY_MUL8]      = freqAdjust * 0;
        r800->delay[DLY_MUL16]     = freqAdjust * 0;
        r800->delay[DLY_PUSH]      = freqAdjust * 1;
        r800->delay[DLY_RET]       = freqAdjust * 1;
        r800->delay[DLY_RLD]       = freqAdjust * 4;
        r800->delay[DLY_S1990VDP]  = freqAdjust * 0;
        r800->delay[DLY_T9769VDP]  = freqAdjust * ((r800->cpuFlags & CPU_VDP_IO_DELAY) ? 1 : 0);
        r800->delay[DLY_LDSPHL]    = freqAdjust * 2;
        r800->delay[DLY_BITIX]     = freqAdjust * 2;
        break;

    case CPU_R800:
        r800->delay[DLY_MEM]       = freqAdjust * 2;
        r800->delay[DLY_MEMOP]     = freqAdjust * 1;
        r800->delay[DLY_MEMPAGE]   = freqAdjust * 1;
        r800->delay[DLY_PREIO]     = freqAdjust * 0;
        r800->delay[DLY_POSTIO]    = freqAdjust * 3;
        r800->delay[DLY_M1]        = freqAdjust * 0;
        r800->delay[DLY_XD]        = freqAdjust * 0;
        r800->delay[DLY_IM]        = freqAdjust * 0;
        r800->delay[DLY_IM2]       = freqAdjust * 3;
        r800->delay[DLY_NMI]       = freqAdjust * 0;
        r800->delay[DLY_PARALLEL]  = freqAdjust * 0;
        r800->delay[DLY_BLOCK]     = freqAdjust * 1;
        r800->delay[DLY_ADD8]      = freqAdjust * 1;
        r800->delay[DLY_ADD16]     = freqAdjust * 0;
        r800->delay[DLY_BIT]       = freqAdjust * 0;
        r800->delay[DLY_CALL]      = freqAdjust * 0;
        r800->delay[DLY_DJNZ]      = freqAdjust * 0;
        r800->delay[DLY_EXSPHL]    = freqAdjust * 0;
        r800->delay[DLY_LD]        = freqAdjust * 0;
        r800->delay[DLY_LDI]       = freqAdjust * 0;
        r800->delay[DLY_INC]       = freqAdjust * 1;
        r800->delay[DLY_INC16]     = freqAdjust * 0;
        r800->delay[DLY_INOUT]     = freqAdjust * 0;
        r800->delay[DLY_MUL8]      = freqAdjust * 12;
        r800->delay[DLY_MUL16]     = freqAdjust * 34;
        r800->delay[DLY_PUSH]      = freqAdjust * 1;
        r800->delay[DLY_RET]       = freqAdjust * 0;
        r800->delay[DLY_RLD]       = freqAdjust * 1;
        r800->delay[DLY_S1990VDP]  = freqAdjust * 57;
        r800->delay[DLY_T9769VDP]  = freqAdjust * ((r800->cpuFlags & CPU_VDP_IO_DELAY) ? 1 : 0);
        r800->delay[DLY_LDSPHL]    = freqAdjust * 0;
        r800->delay[DLY_BITIX]     = freqAdjust * 0;
        break;
    }
}

R800* r800Create(UInt32 cpuFlags, 
                 R800ReadCb readMemory, R800WriteCb writeMemory,
                 R800ReadCb readIoPort, R800WriteCb writeIoPort, 
                 R800PatchCb patch,     R800TimerCb timerCb,
                 R800BreakptCb bpCb,    R800DebugCb debugCb,
                 R800TrapCb trapCb,
                 void* ref)
{
    R800* r800 = calloc(1, sizeof(R800));
    
    r800->cpuFlags    = cpuFlags;

    r800->readMemory  = readMemory  ? readMemory  : readMemoryDummy;
    r800->writeMemory = writeMemory ? writeMemory : writeMemoryDummy;
    r800->readIoPort  = readIoPort  ? readIoPort  : readIoPortDummy;
    r800->writeIoPort = writeIoPort ? writeIoPort : writeIoPortDummy;
    r800->patch       = patch       ? patch       : patchDummy;
    r800->timerCb     = timerCb     ? timerCb     : timerCbDummy;
    r800->breakpointCb= bpCb        ? bpCb        : breakpointCbDummy;
    r800->debugCb     = debugCb     ? debugCb     : debugCbDummy;
    r800->trapCb      = trapCb      ? trapCb      : trapCbDummy;
    r800->ref         = ref;

    r800->frequencyZ80  = 3579545;
    r800->frequencyR800 = 7159090;

    r800->terminate       = 0;
#ifdef ENABLE_BREAKPOINTS
    r800->breakpointCount = 0;
#endif
    r800->systemTime      = 0;
    r800->cpuMode         = CPU_UNKNOWN;
    r800->oldCpuMode      = CPU_UNKNOWN;

    r800->instCnt         = 0;

    r800Reset(r800, 0);

    return r800;
}

void r800Destroy(R800* r800) {
    free(r800);
}

UInt32 r800GetSystemTime(R800* r800) {
    return r800->systemTime;
}

void r800Reset(R800 *r800, UInt32 cpuTime) {
    static int init = 0;
    if (!init) {
        r800InitTables();
        init = 1;
    }

    r800->regBanks[0].AF.W  = 0xffff;
	r800->regBanks[0].BC.W  = 0xffff;
	r800->regBanks[0].DE.W  = 0xffff;
	r800->regBanks[0].HL.W  = 0xffff;
	r800->regBanks[0].IX.W  = 0xffff;
	r800->regBanks[0].IY.W  = 0xffff;
	r800->regBanks[0].SP.W  = 0xffff;
	r800->regBanks[0].AF1.W = 0xffff;
	r800->regBanks[0].BC1.W = 0xffff;
	r800->regBanks[0].DE1.W = 0xffff;
	r800->regBanks[0].HL1.W = 0xffff;
    r800->regBanks[0].SH.W  = 0xffff;
	r800->regBanks[0].I     = 0x00;
	r800->regBanks[0].R     = 0x00;
	r800->regBanks[0].R2    = 0;
	r800->regBanks[0].PC.W  = 0x0000;

    r800->regBanks[0].iff1  = 0;
    r800->regBanks[0].iff2  = 0;
    r800->regBanks[0].im    = 0;
    r800->regBanks[0].halt  = 0;
    r800->regBanks[0].ei_mode  = 0;

    r800->regBanks[1].AF.W  = 0xffff;
	r800->regBanks[1].BC.W  = 0xffff;
	r800->regBanks[1].DE.W  = 0xffff;
	r800->regBanks[1].HL.W  = 0xffff;
	r800->regBanks[1].IX.W  = 0xffff;
	r800->regBanks[1].IY.W  = 0xffff;
	r800->regBanks[1].SP.W  = 0xffff;
	r800->regBanks[1].AF1.W = 0xffff;
	r800->regBanks[1].BC1.W = 0xffff;
	r800->regBanks[1].DE1.W = 0xffff;
	r800->regBanks[1].HL1.W = 0xffff;
    r800->regBanks[1].SH.W  = 0xffff;
	r800->regBanks[1].I     = 0x00;
	r800->regBanks[1].R     = 0x00;
	r800->regBanks[1].R2    = 0;
	r800->regBanks[1].PC.W  = 0x0000;

    r800->regBanks[1].iff1  = 0;
    r800->regBanks[1].iff2  = 0;
    r800->regBanks[1].im    = 0;
    r800->regBanks[1].halt  = 0;
    r800->regBanks[1].ei_mode  = 0;

    r800SetMode(r800, CPU_Z80);
    r800SwitchCpu(r800);

    r800->dataBus        = 0xff;
    r800->defaultDatabus = 0xff;
    r800->intState       = INT_HIGH;
    r800->nmiState       = INT_HIGH;
    r800->nmiEdge        = 0;

#ifdef ENABLE_CALLSTACK
    r800->callstackSize = 0;
#endif
}

void r800SetDataBus(R800* r800, UInt8 value, UInt8 defaultValue, int setDefault) {
    r800->dataBus = value;
    if (setDefault) {
        r800->defaultDatabus = defaultValue;
    }
}

void r800SetInt(R800* r800) {
    r800->intState = INT_LOW;
}

void r800ClearInt(R800* r800) {
    r800->intState = INT_HIGH;
}

void r800SetNmi(R800* r800) {
    if (r800->nmiState == INT_HIGH) {
        r800->nmiEdge = 1;
    }
    r800->nmiState = INT_LOW;
}

void r800ClearNmi(R800* r800) {
    r800->nmiState = INT_HIGH;
}

CpuMode r800GetMode(R800* r800) {
	return r800->cpuMode;
}

void r800SetFrequency(R800* r800, CpuMode cpuMode, UInt32 frequency) {
    switch (cpuMode) {
    case CPU_Z80:
        r800->frequencyZ80  = frequency;
        break;
    case CPU_R800:
        r800->frequencyR800 = frequency;
        break;
    }

    r800->oldCpuMode = r800->cpuMode;
    r800SwitchCpu(r800);
}

void r800SetMode(R800* r800, CpuMode mode) {
    if (r800->cpuMode == mode) {
        return;
    }

    r800->oldCpuMode = r800->cpuMode;
    r800->cpuMode    = mode;
}

void r800StopExecution(R800* r800) {
    r800->terminate = 1;
}

void r800SetTimeoutAt(R800* r800, SystemTime time)
{
    r800->timeout = time;
}

void r800SetBreakpoint(R800* r800, UInt16 address)
{
#ifdef ENABLE_BREAKPOINTS
    if (r800->breakpoints[address] == 0) {
        r800->breakpoints[address] = 1;
        r800->breakpointCount++;
    }
#endif
}

void r800ClearBreakpoint(R800* r800, UInt16 address)
{
#ifdef ENABLE_BREAKPOINTS
    if (r800->breakpoints[address] != 0) {
        r800->breakpointCount--;
        r800->breakpoints[address] = 0;
    }
#endif
}

void r800Execute(R800* r800) {
    static SystemTime lastRefreshTime = 0;
    while (!r800->terminate) {
        UInt16 address;
        int iff1 = 0;

        if ((Int32)(r800->timeout - r800->systemTime) <= 0) {
            if (r800->timerCb != NULL) {
                r800->timerCb(r800->ref);
            }
        }

        if (r800->oldCpuMode != CPU_UNKNOWN) {
            r800SwitchCpu(r800);
        }

        if (r800->cpuMode == CPU_R800) {
            if (r800->systemTime - lastRefreshTime > 222 * 3) {
                lastRefreshTime = r800->systemTime;
                r800->systemTime += 20 * 3;
            }
        }

#ifdef ENABLE_BREAKPOINTS
        if (r800->breakpointCount > 0) {
            if (r800->breakpoints[r800->regs.PC.W]) {
                if (r800->breakpointCb != NULL) {
                    r800->breakpointCb(r800->ref, r800->regs.PC.W);
                    if (r800->terminate) {
                        break;
                    }
                }
            }
        }
#endif

        executeInstruction(r800, readOpcode(r800, r800->regs.PC.W++));

        if (r800->regs.halt) {
			continue;
        }

		if (r800->regs.ei_mode) {
			r800->regs.ei_mode=0;
			continue;
		}

        if (! ((r800->intState==INT_LOW && r800->regs.iff1)||r800->nmiEdge) ) {
			continue;
        }

        /* If it is NMI... */

        if (r800->nmiEdge) {
            r800->nmiEdge = 0;
#ifdef ENABLE_CALLSTACK
            r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);
//            r800->regs.iff2 = r800->regs.iff1;
            r800->regs.iff1 = 0;
            r800->regs.PC.W = 0x0066;
            M1(r800);
            delayNmi(r800);
            continue;
        }

        r800->regs.iff1 = 0;
        r800->regs.iff2 = 0;

        switch (r800->regs.im) {

        case 0:
            delayIm(r800);
            address = r800->dataBus;
            r800->dataBus = r800->defaultDatabus;
            executeInstruction(r800, (UInt8)(address & 0xff));
            break;

        case 1:
            delayIm(r800);
            executeInstruction(r800, 0xff);
            break;

        case 2:
            address = r800->dataBus | ((Int16)r800->regs.I << 8);
            r800->dataBus = r800->defaultDatabus;
#ifdef ENABLE_CALLSTACK
            r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);
            r800->regs.PC.B.l = r800->readMemory(r800->ref, address++);
            r800->regs.PC.B.h = r800->readMemory(r800->ref, address);
            M1_nodelay(r800);
            delayIm2(r800);
            break;
        }
    }
}

void r800ExecuteUntil(R800* r800, UInt32 endTime) {
    static SystemTime lastRefreshTime = 0;

    while ((Int32)(endTime - r800->systemTime) > 0) {
        UInt16 address;
        int iff1 = 0;

        if (r800->oldCpuMode != CPU_UNKNOWN) {
            r800SwitchCpu(r800);
        }

        if (r800->cpuMode == CPU_R800) {
            if (r800->systemTime - lastRefreshTime > 222 * 3) {
                lastRefreshTime = r800->systemTime;
                r800->systemTime += 12 * 3;
            }
        }

#ifdef ENABLE_BREAKPOINTS
        if (r800->breakpointCount > 0) {
            if (r800->breakpoints[r800->regs.PC.W]) {
                if (r800->breakpointCb != NULL) {
                    r800->breakpointCb(r800->ref, r800->regs.PC.W);
                }
            }
        }
#endif

        executeInstruction(r800, readOpcode(r800, r800->regs.PC.W++));

        if (!r800->regs.halt) { 
            iff1 = r800->regs.iff1 >> 1;
            r800->regs.iff1 >>= iff1;
        }

        if (! ((r800->intState==INT_LOW && r800->regs.iff1)||r800->nmiEdge) ) {
            continue;
        }

        if (r800->regs.halt) { 
            r800->regs.PC.W++;
            r800->regs.halt = 0; 
        }

        /* If it is NMI... */
        if (r800->nmiEdge) {
            r800->nmiEdge = 0;
#ifdef ENABLE_CALLSTACK
            r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);

//            r800->regs.iff2 = r800->regs.iff1;
            r800->regs.iff1 = 0;
            r800->regs.PC.W = 0x0066;
            M1(r800);
            delayNmi(r800);
            continue;
        }

        r800->regs.iff1 = 0;
        r800->regs.iff2 = 0;

        switch (r800->regs.im) {
        case 0:
            delayIm(r800);
            executeInstruction(r800, r800->dataBus);
            r800->dataBus = r800->defaultDatabus;
            break;

        case 1:
            delayIm(r800);
            executeInstruction(r800, 0xff);
            break;

        case 2:
            address = r800->dataBus | ((Int16)r800->regs.I << 8);
            r800->dataBus = r800->defaultDatabus;
#ifdef ENABLE_CALLSTACK
            r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	        r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);

            r800->regs.PC.B.l = r800->readMemory(r800->ref, address++);
            r800->regs.PC.B.h = r800->readMemory(r800->ref, address);
            M1_nodelay(r800);
            delayIm2(r800);
            break;
        }
    }
}

void r800ExecuteInstruction(R800* r800) {
    static SystemTime lastRefreshTime = 0;
    UInt16 address;
    int iff1 = 0;

    if (r800->cpuMode == CPU_R800) {
        if (r800->systemTime - lastRefreshTime > 222 * 3) {
            lastRefreshTime = r800->systemTime;
            r800->systemTime += 12 * 3;
        }
    }

#ifdef ENABLE_BREAKPOINTS
    if (r800->breakpointCount > 0) {
        if (r800->breakpoints[r800->regs.PC.W]) {
            if (r800->breakpointCb != NULL) {
                r800->breakpointCb(r800->ref, r800->regs.PC.W);
            }
        }
    }
#endif

    executeInstruction(r800, readOpcode(r800, r800->regs.PC.W++));

    if (!r800->regs.halt) { 
        iff1 = r800->regs.iff1 >> 1;
        r800->regs.iff1 >>= iff1;
    }

        if (! ((r800->intState==INT_LOW && r800->regs.iff1)||r800->nmiEdge) ) {
        return;
    }

    if (r800->regs.halt) { 
        r800->regs.PC.W++;
        r800->regs.halt = 0; 
    }

    /* If it is NMI... */
    if (r800->nmiEdge) {
        r800->nmiEdge = 0;
#ifdef ENABLE_CALLSTACK
        r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	    r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	    r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);

//        r800->regs.iff2 = r800->regs.iff1;
        r800->regs.iff1 = 0;
        r800->regs.PC.W = 0x0066;
        M1(r800);
        delayNmi(r800);
        return;
    }

    r800->regs.iff1 = 0;
    r800->regs.iff2 = 0;

    switch (r800->regs.im) {
    case 0:
        delayIm(r800);
        executeInstruction(r800, r800->dataBus);
        r800->dataBus = r800->defaultDatabus;
        break;

    case 1:
        delayIm(r800);
        executeInstruction(r800, 0xff);
        break;

    case 2:
        address = r800->dataBus | ((Int16)r800->regs.I << 8);
        r800->dataBus = r800->defaultDatabus;
#ifdef ENABLE_CALLSTACK
        r800->callstack[r800->callstackSize++ & 0xff] = r800->regs.PC.W;
#endif
	    r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.h);
	    r800->writeMemory(r800->ref, --r800->regs.SP.W, r800->regs.PC.B.l);

        r800->regs.PC.B.l = r800->readMemory(r800->ref, address++);
        r800->regs.PC.B.h = r800->readMemory(r800->ref, address);
        M1_nodelay(r800);
        delayIm2(r800);
        break;
    }
}

