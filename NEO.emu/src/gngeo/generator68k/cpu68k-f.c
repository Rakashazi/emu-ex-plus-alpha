/*****************************************************************************/
/*     Generator - Sega Genesis emulation - (c) James Ponder 1997-2001       */
/*****************************************************************************/
/*                                                                           */
/* cpu68k-f.c                                                                */
/*                                                                           */
/*****************************************************************************/

#include "cpu68k-inline.h"

void cpu_op_1614a(t_ipc *ipc) /* LINE15 */ {
  /* mask f000, bits f000, mnemonic 75, priv 0, endblk -1, imm_notzero 0, used -1     set 0, size 0, stype 19, dtype 20, sbitpos 0, dbitpos 0, immvalue 0 */

  reg68k_internal_vector(V_LINE15, PC);
}

