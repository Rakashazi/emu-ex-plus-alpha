/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-3.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_427a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_427b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3000, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_428a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_428b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3080, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_429a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_429b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_430a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_430b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3100, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_431a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_431b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3140, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_432a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_432b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3180, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_433a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_433b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_434a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_434b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33c0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_435a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_435b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3008, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_436a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_436b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3088, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_437a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_437b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_438a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_438b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3108, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_439a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_439b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3148, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_440a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_440b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3188, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_441a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_441b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_442a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_442b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33c8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_443a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_443b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3010, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_444a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_444b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3090, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_445a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_445b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_446a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_446b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3110, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_447a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_447b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3150, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_448a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_448b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3190, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_449a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_449b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_450a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_450b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33d0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_451a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_451b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3018, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_452a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_452b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3098, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_453a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_453b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_454a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_454b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3118, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_455a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_455b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3158, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_456a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_456b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3198, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_457a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_457b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_458a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_458b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33d8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_459a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_459b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3020, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_460a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_460b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_461a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_461b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_462a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_462b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3120, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_463a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_463b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3160, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_464a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_464b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31a0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_465a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_465b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_466a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_466b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33e0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_467a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_467b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3028, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_468a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_468b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_469a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_469b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_470a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_470b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3128, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_471a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_471b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3168, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_472a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_472b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31a8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_473a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_473b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_474a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_474b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33e8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_475a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_475b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3030, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_476a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_476b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_477a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_477b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 30f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_478a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_478b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3130, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_479a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_479b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 3170, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_480a(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_480b(t_ipc *ipc) /* MOVE */ {
  /* mask f1f8, bits 31b0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_481a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_481b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 31f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_482a(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_482b(t_ipc *ipc) /* MOVE */ {
  /* mask fff8, bits 33f0, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_483a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_483b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3038, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_484a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_484b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_485a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_485b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_486a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_486b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3138, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_487a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_487b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3178, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_488a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_488b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31b8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_489a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_489b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_490a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_490b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33f8, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_491a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 6;
}

void cpu_op_491b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3039, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_492a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_492b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_493a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_493b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_494a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_494b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3139, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_495a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_495b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 3179, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_496a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_496b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31b9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_497a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_497b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_498a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_498b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33f9, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_499a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_499b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_500a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_500b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_501a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_501b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_502a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_502b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_503a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_503b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317a, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_504a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_504b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31ba, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_505a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_505b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_506a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_506b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fa, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_507a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_507b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_508a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_508b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_509a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_509b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_510a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_510b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_511a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_511b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317b, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_512a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_512b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31bb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_513a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_513b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_514a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_514b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fb, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_515a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_515b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 303c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 outdata = srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_516a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_516b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_517a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_517b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 30fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_518a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_518b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 313c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_519a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_519b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 317c, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_520a(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_520b(t_ipc *ipc) /* MOVE */ {
  /* mask f1ff, bits 31bc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_521a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_521b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 31fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_522a(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_522b(t_ipc *ipc) /* MOVE */ {
  /* mask ffff, bits 33fc, mnemonic 21, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 outdata = srcdata;

  storeword(dstaddr, outdata);

  VFLAG = 0;
  CFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_523a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3040, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_524a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3048, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_525a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3050, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_526a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3058, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_527a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3060, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_528a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3068, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_529a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1f8, bits 3070, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_530a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1ff, bits 3078, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_531a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1ff, bits 3079, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 6;
}

void cpu_op_532a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1ff, bits 307a, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_533a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1ff, bits 307b, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_534a(t_ipc *ipc) /* MOVEA */ {
  /* mask f1ff, bits 307c, mnemonic 22, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 12, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;

  ADDRREG(dstreg) = (sint32)(sint16)srcdata;
  PC+= 4;
}

