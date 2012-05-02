/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-0.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_0a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0000, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_0b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0000, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0010, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0010, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_2a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0018, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_2b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0018, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_3a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0020, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_3b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0020, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_4a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0028, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_4b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0028, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_5a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0030, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_5b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0030, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_6a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0038, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_6b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0038, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_7a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0039, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_7b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0039, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata|= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_8a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0040, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_8b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0040, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_9a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0050, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_9b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0050, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_10a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0058, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_10b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0058, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_11a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0060, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_11b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0060, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_12a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0068, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_12b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0068, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_13a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0070, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_13b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0070, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_14a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0078, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_14b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0078, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_15a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0079, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_15b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 0079, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata|= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_16a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0080, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_16b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0080, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_17a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0090, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_17b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0090, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_18a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0098, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_18b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 0098, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_19a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00a0, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_19b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00a0, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_20a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00a8, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_20b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00a8, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_21a(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00b0, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_21b(t_ipc *ipc) /* OR */ {
  /* mask fff8, bits 00b0, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_22a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 00b8, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_22b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 00b8, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_23a(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 00b9, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_23b(t_ipc *ipc) /* OR */ {
  /* mask ffff, bits 00b9, mnemonic 1, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata|= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 10;
}

void cpu_op_24a(t_ipc *ipc) /* ORSR */ {
  /* mask ffff, bits 003c, mnemonic 2, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 11, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  SR|= srcdata;
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_25a(t_ipc *ipc) /* ORSR */ {
  /* mask ffff, bits 007c, mnemonic 2, priv -1, endblk 0, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  if (!SFLAG)
    reg68k_internal_vector(V_PRIVILEGE, PC+4);

  SR|= srcdata;
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_26a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0200, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_26b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0200, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_27a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0210, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_27b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0210, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_28a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0218, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_28b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0218, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_29a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0220, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_29b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0220, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_30a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0228, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_30b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0228, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_31a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0230, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_31b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0230, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_32a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0238, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_32b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0238, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_33a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0239, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_33b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0239, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_34a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0240, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_34b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0240, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_35a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0250, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_35b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0250, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_36a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0258, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_36b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0258, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_37a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0260, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_37b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0260, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_38a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0268, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_38b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0268, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_39a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0270, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_39b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0270, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_40a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0278, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_40b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0278, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_41a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0279, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_41b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 0279, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_42a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0280, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_42b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0280, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_43a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0290, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_43b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0290, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_44a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0298, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_44b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 0298, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_45a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_45b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_46a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_46b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_47a(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_47b(t_ipc *ipc) /* AND */ {
  /* mask fff8, bits 02b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_48a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 02b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_48b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 02b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_49a(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 02b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_49b(t_ipc *ipc) /* AND */ {
  /* mask ffff, bits 02b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 10;
}

void cpu_op_50a(t_ipc *ipc) /* ANDSR */ {
  /* mask ffff, bits 023c, mnemonic 4, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 11, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  SR = (SR & 0xFF00) | (SR & srcdata);
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_51a(t_ipc *ipc) /* ANDSR */ {
  /* mask ffff, bits 027c, mnemonic 4, priv -1, endblk 0, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  if (!SFLAG)
    reg68k_internal_vector(V_PRIVILEGE, PC+4);

  SR&= srcdata;
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_52a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0400, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_52b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0400, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_53a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0410, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_53b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0410, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_54a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0418, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_54b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0418, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_55a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0420, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_55b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0420, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_56a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0428, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_56b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0428, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_57a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0430, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_57b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0430, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_58a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0438, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_58b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0438, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_59a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0439, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_59b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0439, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_60a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0440, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_60b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0440, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_61a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0450, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_61b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0450, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_62a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0458, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_62b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0458, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_63a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0460, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_63b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0460, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_64a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0468, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_64b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0468, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_65a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0470, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_65b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0470, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_66a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0478, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_66b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0478, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_67a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0479, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_67b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 0479, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_68a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0480, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_68b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0480, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_69a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0490, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_69b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0490, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_70a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0498, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_70b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 0498, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_71a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_71b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_72a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_72b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_73a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_73b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 04b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_74a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 04b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_74b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 04b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_75a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 04b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_75b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 04b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_76a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0600, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_76b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0600, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_77a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0610, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_77b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0610, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_78a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0618, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_78b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0618, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_79a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0620, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_79b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0620, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_80a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0628, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_80b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0628, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_81a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0630, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_81b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0630, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_82a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0638, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_82b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0638, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_83a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0639, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_83b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0639, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_84a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0640, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_84b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0640, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_85a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0650, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_85b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0650, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_86a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0658, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_86b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0658, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_87a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0660, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_87b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0660, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_88a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0668, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_88b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0668, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_89a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0670, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_89b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0670, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_90a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0678, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_90b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0678, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_91a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0679, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_91b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 0679, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_92a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0680, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_92b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0680, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_93a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0690, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_93b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0690, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_94a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0698, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_94b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 0698, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_95a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06a0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_95b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06a0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_96a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06a8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_96b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06a8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_97a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06b0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_97b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 06b0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_98a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 06b8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_98b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 06b8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_99a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 06b9, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_99b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 06b9, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_100a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0810, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_100b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0810, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_101a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0818, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_101b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0818, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_102a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0820, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_102b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0820, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_103a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0828, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_103b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0828, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_104a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0830, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_104b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0830, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_105a(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 0838, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_105b(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 0838, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_106a(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 0839, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_106b(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 0839, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_107a(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 083a, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 9, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_107b(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 083a, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 9, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_108a(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 083b, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 10, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_108b(t_ipc *ipc) /* BTST */ {
  /* mask ffff, bits 083b, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 10, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_109a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0850, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_109b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0850, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_110a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0858, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_110b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0858, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_111a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0860, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_111b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0860, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_112a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0868, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_112b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0868, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_113a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0870, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_113b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0870, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_114a(t_ipc *ipc) /* BCHG */ {
  /* mask ffff, bits 0878, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_114b(t_ipc *ipc) /* BCHG */ {
  /* mask ffff, bits 0878, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_115a(t_ipc *ipc) /* BCHG */ {
  /* mask ffff, bits 0879, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_115b(t_ipc *ipc) /* BCHG */ {
  /* mask ffff, bits 0879, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_116a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0890, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_116b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0890, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_117a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0898, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_117b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0898, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_118a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08a0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_118b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08a0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_119a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08a8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_119b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08a8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_120a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08b0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_120b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 08b0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_121a(t_ipc *ipc) /* BCLR */ {
  /* mask ffff, bits 08b8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_121b(t_ipc *ipc) /* BCLR */ {
  /* mask ffff, bits 08b8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_122a(t_ipc *ipc) /* BCLR */ {
  /* mask ffff, bits 08b9, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_122b(t_ipc *ipc) /* BCLR */ {
  /* mask ffff, bits 08b9, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_123a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08d0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_123b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08d0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_124a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08d8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_124b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08d8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_125a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08e0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_125b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08e0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_126a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08e8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_126b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08e8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_127a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08f0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_127b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08f0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_128a(t_ipc *ipc) /* BSET */ {
  /* mask ffff, bits 08f8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_128b(t_ipc *ipc) /* BSET */ {
  /* mask ffff, bits 08f8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_129a(t_ipc *ipc) /* BSET */ {
  /* mask ffff, bits 08f9, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_129b(t_ipc *ipc) /* BSET */ {
  /* mask ffff, bits 08f9, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 8;
}

void cpu_op_130a(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0800, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_130b(t_ipc *ipc) /* BTST */ {
  /* mask fff8, bits 0800, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_131a(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0840, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata ^ bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_131b(t_ipc *ipc) /* BCHG */ {
  /* mask fff8, bits 0840, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata ^ bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_132a(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0880, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata & ~bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_132b(t_ipc *ipc) /* BCLR */ {
  /* mask fff8, bits 0880, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata & ~bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_133a(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08c0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata | bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_133b(t_ipc *ipc) /* BSET */ {
  /* mask fff8, bits 08c0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata | bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_134a(t_ipc *ipc) /* EORSR */ {
  /* mask ffff, bits 0a3c, mnemonic 6, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 11, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  SR^= srcdata;
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_135a(t_ipc *ipc) /* EORSR */ {
  /* mask ffff, bits 0a7c, mnemonic 6, priv -1, endblk 0, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  unsigned int sr = regs.sr.sr_struct.s;

  if (!SFLAG)
    reg68k_internal_vector(V_PRIVILEGE, PC+4);

  SR^= srcdata;
  if (sr != (uint8)regs.sr.sr_struct.s) {
    /* mode change, swap SP and A7 */
    ADDRREG(7)^= SP; SP^= ADDRREG(7); ADDRREG(7)^= SP;
  }
  PC+= 4;
}

void cpu_op_136a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a00, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_136b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a00, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_137a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a10, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_137b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a10, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_138a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a18, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_138b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a18, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_139a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a20, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_139b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a20, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_140a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a28, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_140b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a28, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_141a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a30, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_141b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a30, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_142a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a38, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_142b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a38, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_143a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a39, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_143b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a39, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_144a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a40, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_144b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a40, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_145a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a50, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_145b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a50, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_146a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a58, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_146b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a58, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_147a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a60, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_147b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a60, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_148a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a68, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_148b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a68, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_149a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a70, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_149b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a70, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_150a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a78, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_150b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a78, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_151a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a79, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_151b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0a79, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_152a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a80, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_152b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a80, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_153a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a90, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_153b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a90, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_154a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a98, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_154b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0a98, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_155a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0aa0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 6;
}

void cpu_op_155b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0aa0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_156a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0aa8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_156b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0aa8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_157a(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0ab0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_157b(t_ipc *ipc) /* EOR */ {
  /* mask fff8, bits 0ab0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_158a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0ab8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 8;
}

void cpu_op_158b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0ab8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 8;
}

void cpu_op_159a(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0ab9, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 10;
}

void cpu_op_159b(t_ipc *ipc) /* EOR */ {
  /* mask ffff, bits 0ab9, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 10;
}

void cpu_op_160a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c00, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_160b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c00, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_161a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c10, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_161b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c10, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_162a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c18, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_162b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c18, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_163a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c20, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_163b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c20, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_164a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c28, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 6;
}

void cpu_op_164b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c28, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_165a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c30, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 6;
}

void cpu_op_165b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c30, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_166a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c38, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 6;
}

void cpu_op_166b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c38, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_167a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c39, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 8;
}

void cpu_op_167b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c39, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint8 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_168a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c40, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_168b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c40, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_169a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c50, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_169b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c50, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_170a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c58, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_170b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c58, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_171a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c60, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_171b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c60, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_172a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c68, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 6;
}

void cpu_op_172b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c68, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_173a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c70, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 6;
}

void cpu_op_173b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c70, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_174a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c78, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 6;
}

void cpu_op_174b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c78, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_175a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c79, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 8;
}

void cpu_op_175b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0c79, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint16 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_176a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c80, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_176b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c80, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_177a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c90, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_177b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c90, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 2, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_178a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c98, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_178b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0c98, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 3, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_179a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0ca0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_179b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0ca0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 4, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_180a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0ca8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 8;
}

void cpu_op_180b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0ca8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 5, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_181a(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0cb0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 8;
}

void cpu_op_181b(t_ipc *ipc) /* CMP */ {
  /* mask fff8, bits 0cb0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 6, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_182a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0cb8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 8;
}

void cpu_op_182b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0cb8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 7, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 8;
}

void cpu_op_183a(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0cb9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 10;
}

void cpu_op_183b(t_ipc *ipc) /* CMP */ {
  /* mask ffff, bits 0cb9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 8, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 10;
}

void cpu_op_184a(t_ipc *ipc) /* MOVEPMR */ {
  /* mask f1f8, bits 0108, mnemonic 23, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (fetchbyte(srcaddr) << 8) + fetchbyte(srcaddr+2);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_185a(t_ipc *ipc) /* MOVEPMR */ {
  /* mask f1f8, bits 0148, mnemonic 23, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (fetchbyte(srcaddr) << 24) | (fetchbyte(srcaddr+2) << 16) | 
    (fetchbyte(srcaddr+4) << 8) | fetchbyte(srcaddr+6);

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_186a(t_ipc *ipc) /* MOVEPRM */ {
  /* mask f1f8, bits 0188, mnemonic 24, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;

  storebyte(dstaddr, (srcdata >> 8) & 0xFF);
  storebyte(dstaddr+2, srcdata & 0xFF);
  PC+= 4;
}

void cpu_op_187a(t_ipc *ipc) /* MOVEPRM */ {
  /* mask f1f8, bits 01c8, mnemonic 24, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;

  storebyte(dstaddr, (srcdata >> 24) & 0xFF);
  storebyte(dstaddr+2, (srcdata >> 16) & 0xFF);
  storebyte(dstaddr+4, (srcdata >> 8) & 0xFF);
  storebyte(dstaddr+6, srcdata & 0xFF);
  PC+= 4;
}

void cpu_op_188a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0110, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_188b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0110, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_189a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0118, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_189b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0118, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_190a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0120, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_190b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0120, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_191a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0128, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_191b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0128, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_192a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0130, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_192b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0130, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_193a(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 0138, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_193b(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 0138, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_194a(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 0139, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_194b(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 0139, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_195a(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013a, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 9, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_195b(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013a, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 9, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_196a(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013b, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 10, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_196b(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013b, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 10, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_197a(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013c, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 11, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint8 dstdata = ipc->dst;
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_197b(t_ipc *ipc) /* BTST */ {
  /* mask f1ff, bits 013c, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 11, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint8 dstdata = ipc->dst;
  uint32 bitpos = 1<<(srcdata & 7);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_198a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0150, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_198b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0150, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_199a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0158, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_199b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0158, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_200a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0160, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_200b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0160, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_201a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0168, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_201b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0168, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_202a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0170, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_202b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0170, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_203a(t_ipc *ipc) /* BCHG */ {
  /* mask f1ff, bits 0178, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_203b(t_ipc *ipc) /* BCHG */ {
  /* mask f1ff, bits 0178, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_204a(t_ipc *ipc) /* BCHG */ {
  /* mask f1ff, bits 0179, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_204b(t_ipc *ipc) /* BCHG */ {
  /* mask f1ff, bits 0179, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata ^ bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_205a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0190, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_205b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0190, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_206a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0198, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_206b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0198, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_207a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01a0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_207b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01a0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_208a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01a8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_208b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01a8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_209a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01b0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_209b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 01b0, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_210a(t_ipc *ipc) /* BCLR */ {
  /* mask f1ff, bits 01b8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_210b(t_ipc *ipc) /* BCLR */ {
  /* mask f1ff, bits 01b8, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_211a(t_ipc *ipc) /* BCLR */ {
  /* mask f1ff, bits 01b9, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_211b(t_ipc *ipc) /* BCLR */ {
  /* mask f1ff, bits 01b9, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata & ~bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_212a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01d0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_212b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01d0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_213a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01d8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_213b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01d8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_214a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01e0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_214b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01e0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_215a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01e8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_215b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01e8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_216a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01f0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_216b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01f0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_217a(t_ipc *ipc) /* BSET */ {
  /* mask f1ff, bits 01f8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_217b(t_ipc *ipc) /* BSET */ {
  /* mask f1ff, bits 01f8, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 4;
}

void cpu_op_218a(t_ipc *ipc) /* BSET */ {
  /* mask f1ff, bits 01f9, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_218b(t_ipc *ipc) /* BSET */ {
  /* mask f1ff, bits 01f9, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint32 bitpos = 1<<(srcdata & 7);
  uint8 outdata = dstdata | bitpos;
  storebyte(dstaddr, outdata);

  ZFLAG = !(dstdata & bitpos);
  PC+= 6;
}

void cpu_op_219a(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0100, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_219b(t_ipc *ipc) /* BTST */ {
  /* mask f1f8, bits 0100, mnemonic 17, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_220a(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0140, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata ^ bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_220b(t_ipc *ipc) /* BCHG */ {
  /* mask f1f8, bits 0140, mnemonic 18, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata ^ bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_221a(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0180, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata & ~bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_221b(t_ipc *ipc) /* BCLR */ {
  /* mask f1f8, bits 0180, mnemonic 19, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata & ~bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_222a(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01c0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata | bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

void cpu_op_222b(t_ipc *ipc) /* BSET */ {
  /* mask f1f8, bits 01c0, mnemonic 20, priv 0, endblk 0, imm_notzero 0, used 0     set 4, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 bitpos = 1<<(srcdata & 31);
  uint32 outdata = dstdata | bitpos;
  DATAREG(dstreg) = outdata;

  ZFLAG = !(dstdata & bitpos);
  PC+= 2;
}

