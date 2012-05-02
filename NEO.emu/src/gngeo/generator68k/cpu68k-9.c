/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-9.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1143a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9000, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1143b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9000, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1144a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9008, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1144b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9008, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1145a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9010, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1145b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9010, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1146a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9018, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1146b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9018, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1147a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9020, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1147b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9020, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1148a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9028, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1148b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9028, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1149a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9030, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1149b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9030, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1150a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9038, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1150b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9038, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1151a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9039, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 6;
}

void cpu_op_1151b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9039, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1152a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903a, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1152b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903a, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1153a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903b, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1153b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903b, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1154a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903c, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 4;
}

void cpu_op_1154b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 903c, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 11, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint8 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1155a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9040, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1155b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9040, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1156a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9048, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1156b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9048, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1157a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9050, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1157b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9050, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1158a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9058, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1158b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9058, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1159a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9060, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1159b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9060, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1160a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9068, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1160b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9068, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1161a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9070, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1161b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9070, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1162a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9078, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1162b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9078, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1163a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9079, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 6;
}

void cpu_op_1163b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9079, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1164a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907a, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1164b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907a, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1165a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907b, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1165b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907b, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1166a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907c, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 4;
}

void cpu_op_1166b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 907c, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1167a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9080, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1167b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9080, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1168a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9088, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1168b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9088, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 1, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1169a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9090, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1169b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9090, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 2, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1170a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9098, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1170b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9098, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 3, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1171a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1171b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 4, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1172a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1172b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 5, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1173a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1173b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 90b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 6, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1174a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1174b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 7, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1175a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1175b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 8, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1176a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90ba, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1176b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90ba, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 9, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1177a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90bb, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1177b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90bb, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 10, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1178a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90bc, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1178b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 90bc, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 13, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
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

void cpu_op_1179a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9110, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1179b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9110, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1180a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9118, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1180b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9118, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1181a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9120, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1181b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9120, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1182a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9128, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1182b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9128, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1183a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9130, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1183b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9130, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1184a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9138, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1184b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9138, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1185a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9139, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1185b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9139, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
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

void cpu_op_1186a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9150, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1186b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9150, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1187a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9158, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1187b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9158, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1188a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9160, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1188b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9160, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1189a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9168, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1189b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9168, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1190a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9170, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1190b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9170, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1191a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9178, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1191b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9178, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1192a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9179, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1192b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 9179, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
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

void cpu_op_1193a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9190, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1193b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9190, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1194a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9198, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1194b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 9198, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1195a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_1195b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 2;
}

void cpu_op_1196a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1196b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1197a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1197b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 91b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
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
  PC+= 4;
}

void cpu_op_1198a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 91b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1198b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 91b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1199a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 91b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1199b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 91b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  uint32 dstaddr = ipc->dst;
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

void cpu_op_1200a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90c0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1201a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90c8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1202a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90d0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1203a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90d8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=2, ADDRREG(srcreg)-2);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1204a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90e0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1205a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90e8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1206a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 90f0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1207a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 90f8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1208a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 90f9, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1209a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 90fa, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1210a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 90fb, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1211a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 90fc, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 12, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint16 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1212a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91c0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 0, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1213a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91c8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 1, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = ADDRREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1214a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91d0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 2, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1215a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91d8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 3, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)+=4, ADDRREG(srcreg)-4);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1216a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91e0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 4, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1217a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91e8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 5, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1218a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 91f0, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 6, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1219a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 91f8, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 7, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1220a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 91f9, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 8, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1221a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 91fa, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 9, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1222a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 91fb, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 10, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcaddr = idxval_src(ipc);
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 4;
}

void cpu_op_1223a(t_ipc *ipc) /* SUBA */ {
  /* mask f1ff, bits 91fc, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 13, dtype 1, sbitpos 0, dbitpos 9, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 6;
}

void cpu_op_1224a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9100, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata - XFLAG;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1224b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9100, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata - XFLAG;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  {
    int Sm = (sint8)srcdata < 0;
    int Dm = (sint8)dstdata < 0;
    int Rm = (sint8)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint8)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

void cpu_op_1225a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9140, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata- XFLAG;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1225b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9140, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 2, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata- XFLAG;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  {
    int Sm = (sint16)srcdata < 0;
    int Dm = (sint16)dstdata < 0;
    int Rm = (sint16)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint16)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

void cpu_op_1226a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9180, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata- XFLAG;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1226b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9180, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 3, stype 0, dtype 0, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata- XFLAG;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  {
    int Sm = (sint32)srcdata < 0;
    int Dm = (sint32)dstdata < 0;
    int Rm = (sint32)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint32)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

void cpu_op_1227a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9108, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata - XFLAG;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1227b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9108, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 1, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 srcdata = fetchbyte(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata - XFLAG;

  storebyte(dstaddr, outdata);

  VFLAG = (((sint8)srcdata < 0) != ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  {
    int Sm = (sint8)srcdata < 0;
    int Dm = (sint8)dstdata < 0;
    int Rm = (sint8)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint8)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

void cpu_op_1228a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9148, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 2, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata- XFLAG;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1228b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9148, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 2, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=2;
  uint16 srcdata = fetchword(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata- XFLAG;

  storeword(dstaddr, outdata);

  VFLAG = (((sint16)srcdata < 0) != ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  {
    int Sm = (sint16)srcdata < 0;
    int Dm = (sint16)dstdata < 0;
    int Rm = (sint16)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint16)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

void cpu_op_1229a(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9188, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 3, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata- XFLAG;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_1229b(t_ipc *ipc) /* SUBX */ {
  /* mask f1f8, bits 9188, mnemonic 9, priv 0, endblk 0, imm_notzero 0, used 5     set -1, size 3, stype 4, dtype 4, sbitpos 0, dbitpos 9, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg)-=4;
  uint32 srcdata = fetchlong(srcaddr);
  int dstreg = (ipc->opcode >> 9) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata- XFLAG;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));

  VFLAG = (((sint32)srcdata < 0) != ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  {
    int Sm = (sint32)srcdata < 0;
    int Dm = (sint32)dstdata < 0;
    int Rm = (sint32)outdata < 0;
    XFLAG = CFLAG = (Sm && !Dm) || (Rm && (!Dm || Sm));
  }
  NFLAG = ((sint32)outdata) < 0;
  if (outdata) ZFLAG = 0;
  PC+= 2;
}

