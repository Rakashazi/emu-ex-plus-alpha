/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-e.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1485a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e000, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1485b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e000, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1486a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e040, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1486b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e040, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1487a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e080, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1487b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e080, mnemonic 66, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1488a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e008, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1488b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e008, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1489a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e048, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1489b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e048, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1490a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e088, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1490b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e088, mnemonic 67, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1491a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e010, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1491b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e010, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1492a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e050, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1492b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e050, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1493a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e090, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1493b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e090, mnemonic 68, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1494a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e018, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1494b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e018, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1495a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e058, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1495b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e058, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1496a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e098, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1496b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e098, mnemonic 69, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1497a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e000, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1497b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e000, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1498a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e040, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1498b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e040, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1499a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e080, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1499b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e080, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1500a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e008, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1500b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e008, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1501a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e048, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1501b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e048, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1502a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e088, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1502b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e088, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1503a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e010, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1503b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e010, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1504a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e050, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1504b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e050, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1505a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e090, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1505b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e090, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1506a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e018, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1506b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e018, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1507a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e058, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1507b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e058, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1508a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e098, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1508b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e098, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1509a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e020, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1509b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e020, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = ((sint8)dstdata) >> (count > 7 ? 7 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1510a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e060, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1510b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e060, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1511a(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e0a0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1511b(t_ipc *ipc) /* ASR */ {
  /* mask f1f8, bits e0a0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = ((sint32)dstdata) >> (count > 31 ? 31 : count);

  DATAREG(dstreg) = outdata;

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1512a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e028, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1512b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e028, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1513a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e068, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1513b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e068, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1514a(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e0a8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1514b(t_ipc *ipc) /* LSR */ {
  /* mask f1f8, bits e0a8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1515a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e030, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1515b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e030, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1516a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e070, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1516b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e070, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1517a(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e0b0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1517b(t_ipc *ipc) /* ROXR */ {
  /* mask f1f8, bits e0b0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1518a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e038, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1518b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e038, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1519a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e078, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1519b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e078, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1520a(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e0b8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1520b(t_ipc *ipc) /* ROR */ {
  /* mask f1f8, bits e0b8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1521a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e100, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1521b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e100, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint8 mask = 0xff << (7-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1522a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e140, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1522b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e140, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1523a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e180, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1523b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e180, mnemonic 70, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint32 mask = 0xffffffff <<(31-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1524a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e108, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1524b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e108, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1525a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e148, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1525b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e148, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1526a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e188, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1526b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e188, mnemonic 71, priv 0, endblk 0, imm_notzero -1, used 0     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1527a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e110, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1527b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e110, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1528a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e150, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1528b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e150, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1529a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e190, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1529b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e190, mnemonic 72, priv 0, endblk 0, imm_notzero -1, used 1     set -1, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1530a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e118, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1530b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e118, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 1, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1531a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e158, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1531b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e158, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 2, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1532a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e198, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1532b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e198, mnemonic 73, priv 0, endblk 0, imm_notzero -1, used 0     set -2, size 3, stype 15, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  unsigned int srcdata = ipc->src;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1533a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e100, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1533b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e100, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint8 mask = 0xff << (7-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1534a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e140, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1534b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e140, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1535a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e180, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1535b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e180, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint32 mask = 0xffffffff <<(31-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1536a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e108, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1536b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e108, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1537a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e148, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1537b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e148, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1538a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e188, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1538b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e188, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1539a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e110, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1539b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e110, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1540a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e150, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1540b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e150, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1541a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e190, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1541b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e190, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1542a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e118, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1542b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e118, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1543a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e158, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1543b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e158, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1544a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e198, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1544b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e198, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 14, dtype 0, sbitpos 0, dbitpos 0, immvalue 8 */
  unsigned int srcdata = 8;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1545a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e120, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1545b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e120, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint8 mask = 0xff << (7-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1546a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e160, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1546b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e160, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1547a(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e1a0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1547b(t_ipc *ipc) /* ASL */ {
  /* mask f1f8, bits e1a0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint32 mask = 0xffffffff <<(31-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1548a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e128, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1548b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e128, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 count = srcdata & 63;
  uint8 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1549a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e168, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1549b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e168, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1550a(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e1a8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1550b(t_ipc *ipc) /* LSL */ {
  /* mask f1f8, bits e1a8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 count = srcdata & 63;
  uint32 outdata = count >= bits ? 0 : (dstdata << count);

  DATAREG(dstreg) = outdata;

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1551a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e130, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1551b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e130, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1552a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e170, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1552b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e170, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1553a(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e1b0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1553b(t_ipc *ipc) /* ROXL */ {
  /* mask f1f8, bits e1b0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1554a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e138, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;
  PC+= 2;
}

void cpu_op_1554b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e138, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 1, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint8 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint8 dstdata = DATAREG(dstreg);
  uint8 bits = 8;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint8 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint8)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1555a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e178, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;
  PC+= 2;
}

void cpu_op_1555b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e178, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint16 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint16 dstdata = DATAREG(dstreg);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = (DATAREG(dstreg) & ~0xffff) | outdata;

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1556a(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e1b8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;
  PC+= 2;
}

void cpu_op_1556b(t_ipc *ipc) /* ROL */ {
  /* mask f1f8, bits e1b8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 3, stype 0, dtype 0, sbitpos 9, dbitpos 0, immvalue 0 */
  int srcreg = (ipc->opcode >> 9) & 7;
  uint32 srcdata = DATAREG(srcreg);
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstdata = DATAREG(dstreg);
  uint8 bits = 32;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint32 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  DATAREG(dstreg) = outdata;

  CFLAG = cflag;
  NFLAG = ((sint32)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1557a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0d0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1557b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0d0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1558a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0d8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1558b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0d8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1559a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0e0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1559b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0e0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1560a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0e8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1560b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0e8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1561a(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0f0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1561b(t_ipc *ipc) /* ASR */ {
  /* mask fff8, bits e0f0, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1562a(t_ipc *ipc) /* ASR */ {
  /* mask ffff, bits e0f8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1562b(t_ipc *ipc) /* ASR */ {
  /* mask ffff, bits e0f8, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1563a(t_ipc *ipc) /* ASR */ {
  /* mask ffff, bits e0f9, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1563b(t_ipc *ipc) /* ASR */ {
  /* mask ffff, bits e0f9, mnemonic 66, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = ((sint16)dstdata) >> (count > 15 ? 15 : count);

  storeword(dstaddr, outdata);

  if (!srcdata)
    CFLAG = 0;
  else if (srcdata >= bits) {
    CFLAG = dstdata>>(bits-1);
    XFLAG = dstdata>>(bits-1);
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1564a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2d0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1564b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2d0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1565a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2d8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1565b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2d8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1566a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2e0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1566b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2e0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1567a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2e8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1567b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2e8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1568a(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2f0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1568b(t_ipc *ipc) /* LSR */ {
  /* mask fff8, bits e2f0, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1569a(t_ipc *ipc) /* LSR */ {
  /* mask ffff, bits e2f8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1569b(t_ipc *ipc) /* LSR */ {
  /* mask ffff, bits e2f8, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1570a(t_ipc *ipc) /* LSR */ {
  /* mask ffff, bits e2f9, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1570b(t_ipc *ipc) /* LSR */ {
  /* mask ffff, bits e2f9, mnemonic 67, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = dstdata >> (count > (bits-1) ? (bits-1) : count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
    XFLAG = (count == bits) ? dstdata>>(bits-1) : 0;
  } else {
    CFLAG = dstdata>>(count-1) & 1;
    XFLAG = dstdata>>(count-1) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1571a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4d0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1571b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4d0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1572a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4d8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1572b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4d8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1573a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4e0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1573b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4e0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1574a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4e8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1574b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4e8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1575a(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4f0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1575b(t_ipc *ipc) /* ROXR */ {
  /* mask fff8, bits e4f0, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1576a(t_ipc *ipc) /* ROXR */ {
  /* mask ffff, bits e4f8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1576b(t_ipc *ipc) /* ROXR */ {
  /* mask ffff, bits e4f8, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1577a(t_ipc *ipc) /* ROXR */ {
  /* mask ffff, bits e4f9, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1577b(t_ipc *ipc) /* ROXR */ {
  /* mask ffff, bits e4f9, mnemonic 68, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (xflag)
      outdata |= 1<<(bits-1);
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 6;
}

void cpu_op_1578a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6d0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1578b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6d0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1579a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6d8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1579b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6d8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1580a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6e0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1580b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6e0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1581a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6e8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1581b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6e8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1582a(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6f0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1582b(t_ipc *ipc) /* ROR */ {
  /* mask fff8, bits e6f0, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1583a(t_ipc *ipc) /* ROR */ {
  /* mask ffff, bits e6f8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1583b(t_ipc *ipc) /* ROR */ {
  /* mask ffff, bits e6f8, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1584a(t_ipc *ipc) /* ROR */ {
  /* mask ffff, bits e6f9, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1584b(t_ipc *ipc) /* ROR */ {
  /* mask ffff, bits e6f9, mnemonic 69, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1;
    outdata>>= 1;
    if (cflag)
      outdata |= 1<<(bits-1);
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 6;
}

void cpu_op_1585a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1d0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1585b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1d0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1586a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1d8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1586b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1d8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1587a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1e0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1587b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1e0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1588a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1e8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1588b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1e8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1589a(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1f0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1589b(t_ipc *ipc) /* ASL */ {
  /* mask fff8, bits e1f0, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1590a(t_ipc *ipc) /* ASL */ {
  /* mask ffff, bits e1f8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1590b(t_ipc *ipc) /* ASL */ {
  /* mask ffff, bits e1f8, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1591a(t_ipc *ipc) /* ASL */ {
  /* mask ffff, bits e1f9, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1591b(t_ipc *ipc) /* ASL */ {
  /* mask ffff, bits e1f9, mnemonic 70, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
    VFLAG = !dstdata;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
    {
      uint16 mask = 0xffff << (15-count);
      VFLAG = ((dstdata & mask) != mask) && ((dstdata & mask) != 0);
    }
  }
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1592a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3d0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1592b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3d0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1593a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3d8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1593b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3d8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1594a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3e0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1594b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3e0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 2;
}

void cpu_op_1595a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3e8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1595b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3e8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1596a(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3f0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1596b(t_ipc *ipc) /* LSL */ {
  /* mask fff8, bits e3f0, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1597a(t_ipc *ipc) /* LSL */ {
  /* mask ffff, bits e3f8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1597b(t_ipc *ipc) /* LSL */ {
  /* mask ffff, bits e3f8, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 4;
}

void cpu_op_1598a(t_ipc *ipc) /* LSL */ {
  /* mask ffff, bits e3f9, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1598b(t_ipc *ipc) /* LSL */ {
  /* mask ffff, bits e3f9, mnemonic 71, priv 0, endblk 0, imm_notzero 0, used 0     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 count = srcdata & 63;
  uint16 outdata = count >= bits ? 0 : (dstdata << count);

  storeword(dstaddr, outdata);

  if (!count)
    CFLAG = 0;
  else if (count >= bits) {
    CFLAG = (count == bits) ? dstdata & 1 : 0;
    XFLAG = (count == bits) ? dstdata & 1 : 0;
  } else {
    CFLAG = dstdata>>(bits-count) & 1;
    XFLAG = dstdata>>(bits-count) & 1;
  }
  VFLAG = 0;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  PC+= 6;
}

void cpu_op_1599a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5d0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1599b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5d0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1600a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5d8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1600b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5d8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1601a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5e0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1601b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5e0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1602a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5e8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1602b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5e8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1603a(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5f0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1603b(t_ipc *ipc) /* ROXL */ {
  /* mask fff8, bits e5f0, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1604a(t_ipc *ipc) /* ROXL */ {
  /* mask ffff, bits e5f8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1604b(t_ipc *ipc) /* ROXL */ {
  /* mask ffff, bits e5f8, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1605a(t_ipc *ipc) /* ROXL */ {
  /* mask ffff, bits e5f9, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1605b(t_ipc *ipc) /* ROXL */ {
  /* mask ffff, bits e5f9, mnemonic 72, priv 0, endblk 0, imm_notzero 0, used 1     set -1, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = CFLAG;
  uint8 xflag = XFLAG;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    outdata |= xflag;
    xflag = cflag;
    loop--;
  }
  storeword(dstaddr, outdata);

  XFLAG = xflag;
  CFLAG = xflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 6;
}

void cpu_op_1606a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7d0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1606b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7d0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 2, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1607a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7d8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1607b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7d8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 3, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (ADDRREG(dstreg)+=2, ADDRREG(dstreg)-2);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1608a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7e0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 2;
}

void cpu_op_1608b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7e0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 4, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = ADDRREG(dstreg)-=2;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 2;
}

void cpu_op_1609a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7e8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1609b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7e8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 5, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + (sint32)(sint16)ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1610a(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7f0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1610b(t_ipc *ipc) /* ROL */ {
  /* mask fff8, bits e7f0, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 6, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  int dstreg = (ipc->opcode >> 0) & 7;
  uint32 dstaddr = (sint32)ADDRREG(dstreg) + idxval_dst(ipc);
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1611a(t_ipc *ipc) /* ROL */ {
  /* mask ffff, bits e7f8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 4;
}

void cpu_op_1611b(t_ipc *ipc) /* ROL */ {
  /* mask ffff, bits e7f8, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 7, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 4;
}

void cpu_op_1612a(t_ipc *ipc) /* ROL */ {
  /* mask ffff, bits e7f9, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);
  PC+= 6;
}

void cpu_op_1612b(t_ipc *ipc) /* ROL */ {
  /* mask ffff, bits e7f9, mnemonic 73, priv 0, endblk 0, imm_notzero 0, used 0     set -2, size 2, stype 14, dtype 8, sbitpos 0, dbitpos 0, immvalue 1 */
  unsigned int srcdata = 1;
  uint32 dstaddr = ipc->dst;
  uint16 dstdata = fetchword(dstaddr);
  uint8 bits = 16;
  uint8 loop = srcdata & 63;
  uint8 cflag = 0;
  uint16 outdata = dstdata;

  while(loop) {
    cflag = outdata & 1<<(bits-1) ? 1 : 0;
    outdata<<= 1;
    if (cflag)
      outdata |= 1;
    loop--;
  }
  storeword(dstaddr, outdata);

  CFLAG = cflag;
  NFLAG = ((sint16)outdata) < 0;
  ZFLAG = !outdata;
  VFLAG = 0;
  PC+= 6;
}

