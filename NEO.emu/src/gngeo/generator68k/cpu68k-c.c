/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-c.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1317a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c000, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1317b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c000, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1318a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c010, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1318b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c010, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1319a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c018, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1319b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c018, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1320a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c020, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1320b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c020, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1321a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c028, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1321b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c028, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1322a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c030, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1322b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c030, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1323a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c038, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1323b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c038, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1324a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c039, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 6;
}

void cpu_op_1324b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c039, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_1325a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03a, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1325b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03a, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1326a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03b, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1326b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03b, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1327a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03c, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1327b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c03c, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1328a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c040, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1328b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c040, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1329a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c050, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1329b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c050, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1330a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c058, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1330b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c058, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1331a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c060, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1331b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c060, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1332a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c068, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1332b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c068, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1333a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c070, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1333b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c070, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1334a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c078, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1334b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c078, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1335a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c079, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 6;
}

void cpu_op_1335b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c079, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_1336a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07a, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1336b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07a, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1337a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07b, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1337b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07b, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1338a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07c, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1338b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c07c, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1339a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c080, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1339b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c080, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1340a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c090, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1340b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c090, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1341a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c098, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1341b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c098, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1342a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1342b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1343a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1343b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1344a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1344b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c0b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1345a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1345b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1346a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1346b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1347a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0ba, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1347b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0ba, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1348a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0bb, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1348b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0bb, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1349a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0bc, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1349b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c0bc, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1350a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c110, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1350b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c110, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1351a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c118, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1351b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c118, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1352a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c120, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1352b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c120, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1353a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c128, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1353b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c128, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1354a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c130, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1354b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c130, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1355a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c138, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1355b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c138, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1356a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c139, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata&= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1356b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c139, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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

void cpu_op_1357a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c150, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1357b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c150, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1358a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c158, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1358b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c158, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1359a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c160, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1359b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c160, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1360a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c168, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1360b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c168, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1361a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c170, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1361b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c170, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1362a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c178, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1362b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c178, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1363a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c179, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata&= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1363b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c179, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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

void cpu_op_1364a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c190, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1364b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c190, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1365a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c198, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1365b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c198, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1366a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_1366b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1a0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1367a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1367b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1a8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1368a(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1368b(t_ipc *ipc) /* AND */ {
  /* mask f1f8, bits c1b0, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1369a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c1b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1369b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c1b8, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1370a(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c1b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata&= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1370b(t_ipc *ipc) /* AND */ {
  /* mask f1ff, bits c1b9, mnemonic 3, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1371a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0c0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1371b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0c0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1372a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0d0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1372b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0d0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1373a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0d8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1373b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0d8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1374a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0e0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1374b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0e0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1375a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0e8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1375b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0e8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1376a(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0f0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1376b(t_ipc *ipc) /* MULU */ {
  /* mask f1f8, bits c0f0, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1377a(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0f8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1377b(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0f8, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1378a(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0f9, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1378b(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0f9, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_1379a(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fa, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1379b(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fa, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1380a(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fb, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1380b(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fb, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1381a(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fc, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1381b(t_ipc *ipc) /* MULU */ {
  /* mask f1ff, bits c0fc, mnemonic 13, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (uint32)srcdata * (uint32)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1382a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1c0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1382b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1c0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1383a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1d0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1383b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1d0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1384a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1d8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1384b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1d8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1385a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1e0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1385b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1e0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1386a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1e8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1386b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1e8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1387a(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1f0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1387b(t_ipc *ipc) /* MULS */ {
  /* mask f1f8, bits c1f0, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1388a(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1f8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1388b(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1f8, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1389a(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1f9, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1389b(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1f9, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 6;
}

void cpu_op_1390a(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fa, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1390b(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fa, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1391a(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fb, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1391b(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fb, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1392a(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fc, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1392b(t_ipc *ipc) /* MULS */ {
  /* mask f1ff, bits c1fc, mnemonic 14, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);

  uint32 outdata = (sint32)(sint16)srcdata * (sint32)(sint16)dstdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1393a(t_ipc *ipc) /* ABCD */ {
  /* mask f1f8, bits c100, mnemonic 35, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata;

  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) + XFLAG;
  uint16 precalc = dstdata + srcdata + XFLAG;
  uint16 outdata_tmp = precalc;

  if (outdata_low > 0x09)
    outdata_tmp+= 0x06;
  if (outdata_tmp > 0x90) {
    outdata_tmp+= 0x60;
  } else {
  }
  outdata = outdata_tmp;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1393b(t_ipc *ipc) /* ABCD */ {
  /* mask f1f8, bits c100, mnemonic 35, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata;

  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) + XFLAG;
  uint16 precalc = dstdata + srcdata + XFLAG;
  uint16 outdata_tmp = precalc;

  if (outdata_low > 0x09)
    outdata_tmp+= 0x06;
  if (outdata_tmp > 0x90) {
    outdata_tmp+= 0x60;
    CFLAG = 1;
    XFLAG = 1;
  } else {
    CFLAG = 0;
    XFLAG = 0;
  }
  outdata = outdata_tmp;
  if (outdata) ZFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  VFLAG = ((precalc & 1<<7) == 0) && (outdata & 1<<7);
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1394a(t_ipc *ipc) /* ABCD */ {
  /* mask f1f8, bits c108, mnemonic 35, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata;

  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) + XFLAG;
  uint16 precalc = dstdata + srcdata + XFLAG;
  uint16 outdata_tmp = precalc;

  if (outdata_low > 0x09)
    outdata_tmp+= 0x06;
  if (outdata_tmp > 0x90) {
    outdata_tmp+= 0x60;
  } else {
  }
  outdata = outdata_tmp;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1394b(t_ipc *ipc) /* ABCD */ {
  /* mask f1f8, bits c108, mnemonic 35, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata;

  uint8 outdata_low = (dstdata & 0xF) + (srcdata & 0xF) + XFLAG;
  uint16 precalc = dstdata + srcdata + XFLAG;
  uint16 outdata_tmp = precalc;

  if (outdata_low > 0x09)
    outdata_tmp+= 0x06;
  if (outdata_tmp > 0x90) {
    outdata_tmp+= 0x60;
    CFLAG = 1;
    XFLAG = 1;
  } else {
    CFLAG = 0;
    XFLAG = 0;
  }
  outdata = outdata_tmp;
  if (outdata) ZFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  VFLAG = ((precalc & 1<<7) == 0) && (outdata & 1<<7);
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1395a(t_ipc *ipc) /* EXG */ {
  /* mask f1f8, bits c140, mnemonic 42, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);

  DATAREG(dstreg) = srcdata;
  DATAREG(srcreg) = dstdata;
  PC+= 2;
}

void cpu_op_1396a(t_ipc *ipc) /* EXG */ {
  /* mask f1f8, bits c148, mnemonic 42, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);

  ADDRREG(dstreg) = srcdata;
  ADDRREG(srcreg) = dstdata;
  PC+= 2;
}

void cpu_op_1397a(t_ipc *ipc) /* EXG */ {
  /* mask f1f8, bits c188, mnemonic 42, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 0, dtype 1, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);

  ADDRREG(dstreg) = srcdata;
  DATAREG(srcreg) = dstdata;
  PC+= 2;
}

