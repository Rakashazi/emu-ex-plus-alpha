/* Generator is (c) James Ponder, 1997-2001 http://www.squish.net/generator/ */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "generator.h"
#include "cpu68k.h"

/* forward references */

void diss68k_getoperand(char *text, t_ipc * ipc, t_iib * iib, t_type type);

/* functions */

int diss68k_gettext(t_ipc * ipc, char *text)
{
  t_iib *iib;
  char *p;
  const char *c;
  char src[64], dst[64];
  char mnemonic[64];

  *text = '\0';

  iib = cpu68k_iibtable[ipc->opcode];

  if (iib == NULL)
    return 0;

  diss68k_getoperand(dst, ipc, iib, tp_dst);
  diss68k_getoperand(src, ipc, iib, tp_src);

  if ((iib->mnemonic == i_Bcc) || (iib->mnemonic == i_BSR) ||
      (iib->mnemonic == i_DBcc)) {
    sprintf(src, "$%08x", (unsigned)ipc->src);
  }

  strcpy(mnemonic, mnemonic_table[iib->mnemonic].name);

  if ((p = strstr(mnemonic, "cc")) != NULL) {
    if (iib->mnemonic == i_Bcc && iib->cc == 0) {
      p[0] = 'R';
      p[1] = 'A';
    } else {
      c = condition_table[iib->cc];
      strcpy(p, c);
    }
  }

  switch (iib->size) {
  case sz_byte:
    strcat(mnemonic, ".B");
    break;
  case sz_word:
    strcat(mnemonic, ".W");
    break;
  case sz_long:
    strcat(mnemonic, ".L");
    break;
  default:
    break;
  }

  sprintf(text, "%-10s %s%s%s %d", mnemonic, src, dst[0] ? "," : "", dst,iib->funcnum);

  return 1;
}

void diss68k_getoperand(char *text, t_ipc * ipc, t_iib * iib, t_type type)
{
  int bitpos;
  uint32 val;

  if (type == tp_src) {
    bitpos = iib->sbitpos;
    val = ipc->src;
  } else {
    bitpos = iib->dbitpos;
    val = ipc->dst;
  }

  switch (type == tp_src ? iib->stype : iib->dtype) {
  case dt_Dreg:
    sprintf(text, "D%d", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Areg:
    sprintf(text, "A%d", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Aind:
    sprintf(text, "(A%d)", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Ainc:
    sprintf(text, "(A%d)+", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Adec:
    sprintf(text, "-(A%d)", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Adis:
    sprintf(text, "$%04x(A%d)", (uint16)val, (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Aidx:
    sprintf(text, "$%02x(A%d,Rx.X)", (uint8)val, (ipc->opcode >> bitpos) & 7);
    break;
  case dt_AbsW:
    sprintf(text, "$%08x", (unsigned)val);
    break;
  case dt_AbsL:
    sprintf(text, "$%08x", (unsigned)val);
    break;
  case dt_Pdis:
    sprintf(text, "$%08x(pc)", (unsigned)val);
    break;
  case dt_Pidx:
    sprintf(text, "$%08x(pc, Rx.X)", (unsigned)val);
    break;
  case dt_ImmB:
    sprintf(text, "#$%02x", (unsigned)val);
    break;
  case dt_ImmW:
    sprintf(text, "#$%04x", (unsigned)val);
    break;
  case dt_ImmL:
    sprintf(text, "#$%08x", (unsigned)val);
    break;
  case dt_ImmS:
    sprintf(text, "#%d", iib->immvalue);
    break;
  case dt_Imm3:
    sprintf(text, "#%d", (ipc->opcode >> bitpos) & 7);
    break;
  case dt_Imm4:
    sprintf(text, "#%d", (ipc->opcode >> bitpos) & 15);
    break;
  case dt_Imm8:
    sprintf(text, "#%d", (ipc->opcode >> bitpos) & 255);
    break;
  case dt_Imm8s:
    sprintf(text, "#%d", (signed int)val);
    break;
  default:
    strcpy(text, "");
    break;
  }
}

int diss68k_getdumpline(uint32 addr68k, uint8 *addr, char *dumpline)
{
  t_ipc ipc;
  t_iib *iibp = cpu68k_iibtable[LOCENDIAN16(*(uint16 *)addr)];
  int words, i;
  char dissline[64], *p;

  if (addr68k < 256) {
    sprintf(dissline, "dc.l $%08x", (unsigned)LOCENDIAN32(*(uint32 *)addr));
    words = 2;
  } else {
    cpu68k_ipc(addr68k, addr, iibp, &ipc);
    if (!diss68k_gettext(&ipc, dissline))
      strcpy(dissline, "Illegal Instruction");
    words = ipc.wordlen;
  }

  p = dumpline;
  p += sprintf(p, "%6x : %04x ", (unsigned)addr68k, (addr[0] << 8) + addr[1]);
  for (i = 1; i < words; i++) {
    p += sprintf(p, "%04x ", (addr[i * 2] << 8) + addr[i * 2 + 1]);
  }
  for (i = 29 - strlen(dumpline); i > 0; i--) {
    *p++ = ' ';
  }
  p += sprintf(p, ": ");
  for (i = 0; i < words; i++) {
    if (isalnum(addr[i * 2])) {
      *p++ = addr[i * 2];
    } else
      *p++ = '.';
    if (isalnum(addr[i * 2 + 1])) {
      *p++ = addr[i * 2 + 1];
    } else
      *p++ = '.';
  }
  *p = '\0';
  for (i = 39 - strlen(dumpline); i > 0; i--) {
    *p++ = ' ';
  }
  if (iibp) {
    sprintf(p, " : %4d : %s\n", iibp->funcnum, dissline);
  } else {
    sprintf(p, " :      : %s\n", dissline);
  }

  return words;
}
