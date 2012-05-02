/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-6.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1032a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6000, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = 1;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1033a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6200, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1034a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6300, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG || ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1035a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6400, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1036a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6500, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1037a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6600, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1038a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6700, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1039a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6800, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1040a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6900, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1041a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6a00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1042a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6b00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1043a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6c00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1044a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6d00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1045a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6e00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1046a(t_ipc *ipc) /* Bcc */ {
  /* mask ffff, bits 6f00, mnemonic 62, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 4;
}

void cpu_op_1047a(t_ipc *ipc) /* BSR */ {
  /* mask ffff, bits 6100, mnemonic 63, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 2, stype 12, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;

  ADDRREG(7)-= 4;
  storelong(ADDRREG(7), PC+4);
  PC = srcdata;
}

void cpu_op_1048a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6000, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = 1;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1049a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6200, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !(CFLAG || ZFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1050a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6300, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG || ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1051a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6400, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1052a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6500, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = CFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1053a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6600, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1054a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6700, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1055a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6800, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1056a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6900, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = VFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1057a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6a00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1058a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6b00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = NFLAG;

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1059a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6c00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1060a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6d00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1061a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6e00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = !ZFLAG && (NFLAG == VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1062a(t_ipc *ipc) /* Bcc */ {
  /* mask ff00, bits 6f00, mnemonic 62, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;
  uint8 cc = ZFLAG || (NFLAG != VFLAG);

  if (cc)
    PC = srcdata;
  else
    PC+= 2;
}

void cpu_op_1063a(t_ipc *ipc) /* BSR */ {
  /* mask ff00, bits 6100, mnemonic 63, priv 0, endblk -1, imm_notzero -1, used -1     set 0, size 1, stype 17, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */
  uint32 srcdata = ipc->src;

  ADDRREG(7)-= 4;
  storelong(ADDRREG(7), PC+2);
  PC = srcdata;
}

