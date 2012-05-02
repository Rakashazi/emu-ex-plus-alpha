/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-7.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1064a(t_ipc *ipc) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1064b(t_ipc *ipc) /* MOVE */ {
  /* mask f100, bits 7000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 18, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  signed int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

