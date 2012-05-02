/* asm-common.h -- dynamic assembler support
 *
 * Copyright (C) 1999, 2000 Ian Piumarta <ian.piumarta@inria.fr>
 *
 * This file is part of CCG.
 *
 * CCG is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * CCG is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the file COPYING for more details.
 *
 * Last edited: Thu Jan 13 12:00:32 2000 by piumarta (Ian Piumarta) on pingu
 */

#ifndef __ccg_asm_common_h
#define __ccg_asm_common_h_

#include <stdio.h>

#include "asm-cache.h"

#ifndef _ASM_LOCALPC
  static int   asm_pass;	/* 0 (single pass) or 1/2 (two pass) */
  static insn *asm_pc;
#endif

#define ASMFAIL(MSG) asmFail(MSG, __FILE__, __LINE__, __FUNCTION__)

#ifndef  _ASM_APP_1
# define _ASM_APP_1	{ asm_pass= 0; {
#endif
#ifndef  _ASM_NOAPP_1
# define _ASM_NOAPP_1	}}
#endif
#ifndef  _ASM_APP_2
# define _ASM_APP_2	{ insn *asm_lwm= asm_pc; for (asm_pass= 1; ((asm_pass < 3) && (asm_pc= asm_lwm)); ++asm_pass) {
#endif
#ifndef  _ASM_NOAPP_2
# define _ASM_NOAPP_2	}}
#endif

#ifndef  _ASM_ORG
# define _ASM_ORG(O)	(asm_pc= (insn *)(O))
#endif
#ifndef  _ASM_LBL
# define _ASM_LBL(V)	static insn *V= 0
#endif
#ifndef  _ASM_DEF
# define _ASM_DEF(V)	(V= (((asm_pass==2)&&(asm_pc!=(V))) ? (insn *)ASMFAIL("phase error") : asm_pc))
#endif

static int asmFail(char *msg, char *file, int line, char *function)
{
  fprintf(stderr, "%s: In function `%s':\n", file, function);
  fprintf(stderr, "%s:%d: %s\n", file, line, msg);
  abort();
  return 0;
}

/* integer predicates */

#define _MASK(N)	((u_int32_t)((1<<(N)))-1)
#define _siP(N,I)	(!((((u_int32_t)(I))^(((u_int32_t)(I))<<1))&~_MASK(N)))
#define _uiP(N,I)	(!(((u_int32_t)(I))&~_MASK(N)))

#endif /* __ccg_asm_common_h */
