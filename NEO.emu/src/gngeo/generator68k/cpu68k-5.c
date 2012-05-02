/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-5.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_784a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5000, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_784b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5000, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_785a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5010, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_785b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5010, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_786a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5018, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_786b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5018, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_787a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5020, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_787b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5020, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_788a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5028, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_788b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5028, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_789a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5030, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_789b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5030, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_790a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5038, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_790b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5038, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_791a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5039, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_791b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5039, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_792a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5040, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_792b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5040, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_793a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5050, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_793b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5050, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_794a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5058, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_794b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5058, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_795a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5060, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_795b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5060, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_796a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5068, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_796b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5068, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_797a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5070, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_797b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5070, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_798a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5078, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_798b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5078, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_799a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5079, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_799b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 5079, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_800a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5080, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_800b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5080, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_801a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5090, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_801b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5090, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_802a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5098, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_802b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 5098, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_803a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50a0, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_803b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50a0, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 2;
}

void cpu_op_804a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50a8, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_804b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50a8, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_805a(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50b0, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_805b(t_ipc *ipc) /* ADD */ {
  /* mask f1f8, bits 50b0, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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
  PC+= 4;
}

void cpu_op_806a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 50b8, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_806b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 50b8, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_807a(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 50b9, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_807b(t_ipc *ipc) /* ADD */ {
  /* mask f1ff, bits 50b9, mnemonic 10, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_808a(t_ipc *ipc) /* ADDA */ {
  /* mask f1f8, bits 5048, mnemonic 11, priv 0, endblk 0, imm_notzero -1, used 0     set 0, size 2, stype 15, dtype 1, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_809a(t_ipc *ipc) /* ADDA */ {
  /* mask f1f8, bits 5088, mnemonic 11, priv 0, endblk 0, imm_notzero -1, used 0     set 0, size 3, stype 15, dtype 1, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_810a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5100, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_810b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5100, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_811a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5110, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_811b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5110, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_812a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5118, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_812b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5118, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_813a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5120, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_813b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5120, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_814a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5128, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_814b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5128, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_815a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5130, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_815b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5130, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_816a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5138, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_816b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5138, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_817a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5139, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_817b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5139, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_818a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5140, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_818b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5140, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_819a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5150, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_819b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5150, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_820a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5158, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_820b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5158, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_821a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5160, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_821b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5160, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_822a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5168, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_822b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5168, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_823a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5170, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_823b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5170, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_824a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5178, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_824b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5178, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_825a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5179, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_825b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 5179, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_826a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5180, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_826b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5180, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_827a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5190, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_827b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5190, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 2, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_828a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5198, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_828b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 5198, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 3, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_829a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51a0, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_829b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51a0, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 4, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_830a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51a8, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_830b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51a8, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 5, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_831a(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51b0, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_831b(t_ipc *ipc) /* SUB */ {
  /* mask f1f8, bits 51b0, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 6, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_832a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 51b8, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_832b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 51b8, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 7, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_833a(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 51b9, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_833b(t_ipc *ipc) /* SUB */ {
  /* mask f1ff, bits 51b9, mnemonic 7, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 8, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
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

void cpu_op_834a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 5148, mnemonic 8, priv 0, endblk 0, imm_notzero -1, used 0     set 0, size 3, stype 15, dtype 1, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_835a(t_ipc *ipc) /* SUBA */ {
  /* mask f1f8, bits 5188, mnemonic 8, priv 0, endblk 0, imm_notzero -1, used 0     set 0, size 3, stype 15, dtype 1, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_836a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5000, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_836b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5000, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  VFLAG = (((sint8)srcdata < 0) == ((sint8)dstdata < 0)) &&
    (((sint8)dstdata < 0) != ((sint8)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFu - (uint8)dstdata);
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_837a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5010, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_837b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5010, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_838a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5018, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_838b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5018, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_839a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5020, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_839b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5020, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_840a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5028, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_840b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5028, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_841a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5030, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_841b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5030, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_842a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5038, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_842b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5038, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_843a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5039, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata + (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_843b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5039, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_844a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5040, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_844b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5040, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  VFLAG = (((sint16)srcdata < 0) == ((sint16)dstdata < 0)) &&
    (((sint16)dstdata < 0) != ((sint16)outdata < 0));
  XFLAG = CFLAG = srcdata > (0xFFFFu - (uint16)dstdata);
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_845a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5050, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_845b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5050, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_846a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5058, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_846b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5058, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_847a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5060, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_847b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5060, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_848a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5068, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_848b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5068, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_849a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5070, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_849b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5070, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_850a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5078, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_850b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5078, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_851a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5079, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata + (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_851b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 5079, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_852a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5080, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_852b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5080, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  DATAREG(dstreg) = outdata;

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_853a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5090, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_853b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5090, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_854a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5098, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_854b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 5098, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_855a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50a0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=4;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  /* pre-decrement long store must write low 16 bits
     in -2 first, then upper 16 bits in -4 second */
  storeword(dstaddr + 2, (uint16)outdata);
  storeword(dstaddr, (uint16)(outdata >> 16));
  PC+= 2;
}

void cpu_op_855b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50a0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 2;
}

void cpu_op_856a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50a8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_856b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50a8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_857a(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50b0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_857b(t_ipc *ipc) /* ADD */ {
  /* mask fff8, bits 50b0, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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
  PC+= 4;
}

void cpu_op_858a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 50b8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_858b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 50b8, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);

  VFLAG = (((sint32)srcdata < 0) == ((sint32)dstdata < 0)) &&
    (((sint32)dstdata < 0) != ((sint32)outdata < 0));
  XFLAG = CFLAG = srcdata > (uint32)~(uint32)dstdata;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_859a(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 50b9, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_859b(t_ipc *ipc) /* ADD */ {
  /* mask ffff, bits 50b9, mnemonic 10, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
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

void cpu_op_860a(t_ipc *ipc) /* ADDA */ {
  /* mask fff8, bits 5048, mnemonic 11, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 2, stype 14, dtype 1, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint16)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_861a(t_ipc *ipc) /* ADDA */ {
  /* mask fff8, bits 5088, mnemonic 11, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 14, dtype 1, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata + (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_862a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5100, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_862b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5100, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_863a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5110, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_863b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5110, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_864a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5118, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstaddr_tmp = (ADDRREG(dstreg)+= (dstreg == 7 ? 2 : 1), 0);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_864b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5118, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_865a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5120, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)-= (dstreg == 7 ? 2 : 1));
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_865b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5120, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_866a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5128, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_866b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5128, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_867a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5130, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_867b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5130, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_868a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5138, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_868b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5138, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_869a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5139, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint8 dstdata = fetchbyte(dstaddr);
  uint8 outdata = (sint8)dstdata - (sint8)srcdata;

  storebyte(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_869b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5139, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_870a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5140, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_870b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5140, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_871a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5150, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_871b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5150, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_872a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5158, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_872b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5158, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_873a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5160, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_873b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5160, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_874a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5168, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_874b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5168, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_875a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5170, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_875b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5170, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_876a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5178, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_876b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5178, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_877a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5179, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint16 outdata = (sint16)dstdata - (sint16)srcdata;

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_877b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 5179, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_878a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5180, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_878b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5180, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
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

void cpu_op_879a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5190, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_879b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5190, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_880a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5198, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=4, ADDRREG(dstreg)-4);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_880b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 5198, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_881a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_881b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51a0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_882a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_882b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51a8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_883a(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_883b(t_ipc *ipc) /* SUB */ {
  /* mask fff8, bits 51b0, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_884a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 51b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_884b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 51b8, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_885a(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 51b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  uint32 dstaddr = ipc->dst;
  uint32 dstdata = fetchlong(dstaddr);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  storelong(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_885b(t_ipc *ipc) /* SUB */ {
  /* mask ffff, bits 51b9, mnemonic 7, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
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

void cpu_op_886a(t_ipc *ipc) /* SUBA */ {
  /* mask fff8, bits 5148, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 14, dtype 1, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_887a(t_ipc *ipc) /* SUBA */ {
  /* mask fff8, bits 5188, mnemonic 8, priv 0, endblk 0, imm_notzero 0, used 0     set 0, size 3, stype 14, dtype 1, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = ADDRREG(dstreg);
  uint32 outdata = (sint32)dstdata - (sint32)srcdata;

  ADDRREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_888a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_889a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_890a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_891a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_892a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_893a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_894a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_895a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_896a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59c0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_897a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ac0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_898a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5bc0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_899a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5cc0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_900a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5dc0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_901a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ec0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_902a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5fc0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_903a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_904a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_905a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_906a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_907a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_908a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_909a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_910a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_911a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59d0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_912a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ad0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_913a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5bd0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_914a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5cd0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_915a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5dd0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_916a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ed0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_917a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5fd0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_918a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_919a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_920a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_921a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_922a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_923a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_924a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_925a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_926a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59d8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_927a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ad8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_928a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5bd8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_929a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5cd8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_930a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5dd8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_931a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ed8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_932a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5fd8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_933a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_934a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_935a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_936a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_937a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_938a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_939a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_940a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_941a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59e0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_942a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ae0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_943a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5be0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_944a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ce0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_945a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5de0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_946a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ee0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_947a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5fe0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_948a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_949a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_950a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_951a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_952a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_953a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_954a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_955a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_956a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59e8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_957a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ae8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_958a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5be8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_959a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ce8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_960a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5de8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_961a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ee8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_962a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5fe8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_963a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 50f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_964a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 52f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_965a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 53f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_966a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 54f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_967a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 55f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_968a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 56f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_969a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 57f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_970a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 58f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_971a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 59f0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_972a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5af0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_973a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5bf0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_974a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5cf0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_975a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5df0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_976a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ef0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_977a(t_ipc *ipc) /* Scc */ {
  /* mask fff8, bits 5ff0, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_978a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 50f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_979a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 52f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_980a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 53f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_981a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 54f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_982a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 55f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_983a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 56f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_984a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 57f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_985a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 58f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_986a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 59f8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_987a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5af8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_988a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5bf8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_989a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5cf8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_990a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5df8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_991a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5ef8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_992a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5ff8, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_993a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 50f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = 1;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_994a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 52f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_995a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 53f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = CFLAG || ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_996a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 54f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_997a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 55f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = CFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_998a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 56f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_999a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 57f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = ZFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1000a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 58f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1001a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 59f9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = VFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1002a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5af9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1003a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5bf9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = NFLAG;
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1004a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5cf9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1005a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5df9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1006a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5ef9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1007a(t_ipc *ipc) /* Scc */ {
  /* mask ffff, bits 5ff9, mnemonic 58, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);
  uint8 outdata = cc ? (uint8)(-1) : 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1008a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51c0, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 0, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint8 outdata = 0;

  DATAREG(srcreg) = (DATAREG(srcreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1009a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51d0, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 2, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_1010a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51d8, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 3, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = ADDRREG(srcreg);
  uint32 srcaddr_tmp = (ADDRREG(srcreg)+= (srcreg == 7 ? 2 : 1), 0);
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_1011a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51e0, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 4, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (ADDRREG(srcreg)-= (srcreg == 7 ? 2 : 1));
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 2;
}

void cpu_op_1012a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51e8, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 5, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + (sint32)(sint16)ipc->src;
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_1013a(t_ipc *ipc) /* SF */ {
  /* mask fff8, bits 51f0, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 6, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 0) & 7;
  uint32 srcaddr = (sint32)ADDRREG(srcreg) + idxval_src(ipc);
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_1014a(t_ipc *ipc) /* SF */ {
  /* mask ffff, bits 51f8, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 7, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 4;
}

void cpu_op_1015a(t_ipc *ipc) /* SF */ {
  /* mask ffff, bits 51f9, mnemonic 59, priv 0, endblk 0, imm_notzero 0, used -1     set 0, size 1, stype 8, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcaddr = ipc->src;
  uint8 outdata = 0;

  storebyte(srcaddr, outdata);
  PC+= 6;
}

void cpu_op_1016a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 50c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = 1;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1017a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 52c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !(CFLAG || ZFLAG);

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1018a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 53c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = CFLAG || ZFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1019a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 54c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !CFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1020a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 55c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = CFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1021a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 56c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !ZFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1022a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 57c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = ZFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1023a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 58c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !VFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1024a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 59c8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = VFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1025a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5ac8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !NFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1026a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5bc8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = NFLAG;

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1027a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5cc8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = (NFLAG == VFLAG);

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1028a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5dc8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = (NFLAG != VFLAG);

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1029a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5ec8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1030a(t_ipc *ipc) /* DBcc */ {
  /* mask fff8, bits 5fc8, mnemonic 60, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 cc = ZFLAG || (NFLAG != VFLAG);

  if (!cc) {
    dstdata-= 1;
    DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF)
| (dstdata & 0xFFFF);
    if ((sint16)dstdata != -1)
      PC = srcdata;
    else
      PC+= 4;
  } else
    PC+= 4;
}

void cpu_op_1031a(t_ipc *ipc) /* DBRA */ {
  /* mask fff8, bits 51c8, mnemonic 61, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 0, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);

  dstdata-= 1;
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xFFFF) | (dstdata & 0xFFFF);
  if ((sint16)dstdata != -1)
    PC = srcdata;
  else
    PC+= 4;
}

