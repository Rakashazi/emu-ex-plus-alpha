/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-b.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1230a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b000, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1230b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b000, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1231a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b008, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1231b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b008, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1232a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b010, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1232b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b010, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1233a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b018, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1233b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b018, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1234a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b020, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1234b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b020, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1235a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b028, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1235b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b028, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1236a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b030, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1236b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b030, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1237a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b038, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1237b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b038, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1238a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b039, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 6;
}

void cpu_op_1238b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b039, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1239a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03a, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1239b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03a, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1240a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03b, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1240b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03b, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1241a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03c, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 4;
}

void cpu_op_1241b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b03c, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1242a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b040, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1242b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b040, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1243a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b048, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1243b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b048, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1244a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b050, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1244b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b050, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1245a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b058, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1245b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b058, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1246a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b060, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1246b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b060, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1247a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b068, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1247b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b068, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1248a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b070, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1248b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b070, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1249a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b078, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1249b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b078, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1250a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b079, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 6;
}

void cpu_op_1250b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b079, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1251a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07a, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1251b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07a, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1252a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07b, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1252b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07b, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1253a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07c, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 4;
}

void cpu_op_1253b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b07c, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1254a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b080, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1254b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b080, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1255a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b088, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1255b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b088, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1256a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b090, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1256b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b090, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1257a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b098, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1257b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b098, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1258a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0a0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1258b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0a0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1259a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0a8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1259b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0a8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1260a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0b0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1260b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b0b0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1261a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0b8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1261b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0b8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1262a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0b9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_1262b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0b9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1263a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0ba, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1263b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0ba, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1264a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0bb, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1264b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0bb, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1265a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0bc, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_1265b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b0bc, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1266a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0c0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_1266b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0c0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1267a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0c8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_1267b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0c8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1268a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0d0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_1268b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0d0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1269a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0d8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_1269b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0d8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1270a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0e0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 2;
}

void cpu_op_1270b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0e0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1271a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0e8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1271b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0e8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1272a(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0f0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1272b(t_ipc *ipc) /* CMPA */ {
  /* mask f1f8, bits b0f0, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1273a(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0f8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1273b(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0f8, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1274a(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0f9, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 6;
}

void cpu_op_1274b(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0f9, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1275a(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fa, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1275b(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fa, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1276a(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fb, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1276b(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fb, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1277a(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fc, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;
  PC+= 4;
}

void cpu_op_1277b(t_ipc *ipc) /* CMPA */ {
  /* mask f1ff, bits b0fc, mnemonic 16, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 12, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)(sint16)srcdata;

  VFLAG = (((sint32)(sint16)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = (uint32)(sint32)(sint16)srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1278a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b100, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1278b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b100, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1279a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b110, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1279b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b110, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1280a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b118, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1280b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b118, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1281a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b120, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1281b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b120, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1282a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b128, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1282b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b128, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1283a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b130, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1283b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b130, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1284a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b138, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1284b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b138, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1285a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b139, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = dstdata;

  outdata^= srcdata;
  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1285b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b139, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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

void cpu_op_1286a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b140, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1286b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b140, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1287a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b150, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1287b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b150, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1288a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b158, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1288b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b158, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1289a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b160, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1289b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b160, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1290a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b168, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1290b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b168, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1291a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b170, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1291b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b170, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1292a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b178, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1292b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b178, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1293a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b179, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = dstdata;

  outdata^= srcdata;
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1293b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b179, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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

void cpu_op_1294a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b180, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1294b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b180, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  DATAREG(dstreg) = outdata;

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 2;
}

void cpu_op_1295a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b190, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1295b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b190, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1296a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b198, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1296b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b198, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1297a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1a0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_1297b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1a0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1298a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1a8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1298b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1a8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1299a(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1b0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1299b(t_ipc *ipc) /* EOR */ {
  /* mask f1f8, bits b1b0, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1300a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b1b8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1300b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b1b8, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);

  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  CFLAG = 0;
  PC+= 4;
}

void cpu_op_1301a(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b1b9, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = dstdata;

  outdata^= srcdata;
  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1301b(t_ipc *ipc) /* EOR */ {
  /* mask f1ff, bits b1b9, mnemonic 5, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1302a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b108, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;
  PC+= 2;
}

void cpu_op_1302b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b108, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1303a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b148, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;
  PC+= 2;
}

void cpu_op_1303b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b148, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1304a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b188, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1304b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b188, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 3, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1305a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1c0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1305b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1c0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1306a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1c8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1306b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1c8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1307a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1d0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1307b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1d0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1308a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1d8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1308b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1d8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1309a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1e0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 2;
}

void cpu_op_1309b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1e0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1310a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1e8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1310b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1e8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1311a(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1f0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1311b(t_ipc *ipc) /* CMP */ {
  /* mask f1f8, bits b1f0, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1312a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1f8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1312b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1f8, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1313a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1f9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_1313b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1f9, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1314a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fa, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1314b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fa, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1315a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fb, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 4;
}

void cpu_op_1315b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fb, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1316a(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fc, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;
  PC+= 6;
}

void cpu_op_1316b(t_ipc *ipc) /* CMP */ {
  /* mask f1ff, bits b1fc, mnemonic 15, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 13, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

