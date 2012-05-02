/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-2.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_319a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_319b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_320a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_320b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_321a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_321b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_322a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_322b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_323a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_323b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_324a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_324b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_325a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_325b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_326a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_326b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_327a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_327b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_328a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_328b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_329a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_329b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_330a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_330b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_331a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_331b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_332a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_332b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_333a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_333b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_334a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_334b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_335a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_335b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_336a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_336b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_337a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_337b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_338a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_338b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_339a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_339b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_340a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_340b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_341a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_341b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_342a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_342b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_343a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_343b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_344a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_344b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_345a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_345b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_346a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_346b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_347a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_347b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_348a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_348b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_349a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_349b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_350a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_350b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_351a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_351b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_352a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_352b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_353a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_353b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_354a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_354b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_355a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_355b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_356a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_356b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_357a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_357b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_358a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_358b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_359a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_359b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_360a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_360b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_361a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_361b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_362a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 4;
}

void cpu_op_362b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_363a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_363b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_364a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_364b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_365a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_365b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_366a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_366b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_367a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_367b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_368a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_368b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_369a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_369b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 20f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_370a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 4;
}

void cpu_op_370b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_371a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_371b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_372a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_372b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 21b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_373a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_373b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 21f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_374a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_374b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 23f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_375a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_375b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_376a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_376b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_377a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_377b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_378a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 4;
}

void cpu_op_378b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_379a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_379b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_380a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_380b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_381a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_381b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_382a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_382b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_383a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_383b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_384a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_384b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_385a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_385b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_386a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_386b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_387a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_387b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_388a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_388b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_389a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_389b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_390a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_390b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_391a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_391b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_392a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_392b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_393a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_393b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_394a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 4;
}

void cpu_op_394b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_395a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_395b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_396a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_396b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_397a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_397b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_398a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_398b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_399a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_399b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_400a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_400b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_401a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_401b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_402a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 4;
}

void cpu_op_402b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_403a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_403b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_404a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_404b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_405a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_405b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_406a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_406b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_407a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_407b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 203c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_408a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_408b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_409a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_409b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 20fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_410a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_410b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 213c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 outdata = srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_411a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_411b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 217c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_412a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_412b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 21bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_413a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_413b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 21fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_414a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_414b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 23fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 outdata = srcdata;

  storelong(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_415a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2040, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_416a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2048, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_417a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2050, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_418a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2058, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_419a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2060, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_420a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2068, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_421a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 2070, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_422a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2078, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_423a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 2079, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_424a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 207a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_425a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 207b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_426a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 207c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 13, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 outdata = srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 6;
}

