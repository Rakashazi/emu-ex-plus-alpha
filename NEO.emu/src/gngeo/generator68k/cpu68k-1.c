/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-1.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_223a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_223b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_224a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_224b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_225a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_225b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_226a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_226b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_227a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_227b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_228a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_228b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_229a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_229b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_230a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_230b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_231a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_231b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_232a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_232b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_233a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_233b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_234a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_234b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_235a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_235b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_236a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_236b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_237a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_237b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_238a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_238b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_239a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_239b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_240a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_240b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_241a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_241b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_242a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_242b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_243a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_243b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_244a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_244b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_245a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_245b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_246a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_246b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_247a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_247b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_248a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_248b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_249a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_249b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_250a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_250b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_251a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_251b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_252a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_252b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_253a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_253b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_254a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_254b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_255a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_255b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_256a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_256b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_257a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_257b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_258a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_258b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_259a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_259b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_260a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_260b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_261a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_261b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_262a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_262b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_263a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_263b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_264a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_264b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_265a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_265b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_266a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_266b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_267a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_267b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_268a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_268b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_269a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_269b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_270a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_270b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_271a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_271b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_272a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_272b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_273a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_273b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 10f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_274a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_274b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_275a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_275b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 1170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_276a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_276b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 11b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_277a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_277b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 11f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_278a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_278b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 13f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_279a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_279b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_280a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_280b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_281a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_281b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_282a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_282b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_283a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_283b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_284a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_284b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_285a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_285b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_286a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_286b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_287a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 6;
}

void cpu_op_287b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_288a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_288b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_289a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_289b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_290a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_290b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_291a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_291b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 1179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_292a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_292b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_293a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_293b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_294a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_294b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_295a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_295b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_296a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_296b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_297a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_297b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_298a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_298b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_299a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_299b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_300a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_300b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_301a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_301b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_302a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_302b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_303a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_303b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_304a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_304b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_305a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_305b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_306a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_306b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_307a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_307b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_308a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_308b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_309a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_309b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_310a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_310b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_311a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_311b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 103c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_312a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_312b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_313a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_313b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 10fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_314a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_314b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 113c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_315a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_315b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 117c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_316a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_316b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 11bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_317a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_317b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 11fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_318a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_318b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 13fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 outdata = srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

