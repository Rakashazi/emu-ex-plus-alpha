#ifndef M68KOPS__HEADER
#define M68KOPS__HEADER

/* ======================================================================== */
/* ============================ OPCODE HANDLERS =========================== */
/* ======================================================================== */

#include "m68kcpu.h"

#ifdef BUILD_OP_TABLE

#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
#define NUM_CPU_TYPES 4
#else
#define NUM_CPU_TYPES 1
#endif

/* Build the opcode handler table */
void m68ki_build_opcode_table(void);

extern void (*m68ki_instruction_jump_table[0x10000])(M68KCPU &m68ki_cpu); /* opcode handler jump table */

#endif

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */

#endif /* M68KOPS__HEADER */


