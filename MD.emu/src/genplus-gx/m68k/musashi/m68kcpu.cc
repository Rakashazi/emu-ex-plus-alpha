/* ======================================================================== */
/* ========================= LICENSING & COPYRIGHT ======================== */
/* ======================================================================== */

#if 0
static const char copyright_notice[] =
"MUSASHI\n"
"Version 3.32 (2007-12-15)\n"
"A portable Motorola M680x0 processor emulation engine.\n"
"Copyright Karl Stenerud.  All rights reserved.\n"
"\n"
"This code may be freely used for non-commercial purpooses as long as this\n"
"copyright notice remains unaltered in the source code and any binary files\n"
"containing this code in compiled form.\n"
"\n"
"All other licensing terms must be negotiated with the author\n"
"(Karl Stenerud).\n"
"\n"
"The latest version of this code can be obtained at:\n"
"http://kstenerud.cjb.net\n"
;
#endif


/* ======================================================================== */
/* ================================= NOTES ================================ */
/* ======================================================================== */



/* ======================================================================== */
/* ================================ INCLUDES ============================== */
/* ======================================================================== */

#include "m68kops.h"
#include "m68kcpu.h"

#include <imagine/logger/logger.h>

#if M68K_EMULATE_040
#include "m68kfpu.c"
extern void m68040_fpu_op0(void);
extern void m68040_fpu_op1(void);
#endif /* M68K_EMULATE_040 */

/* ======================================================================== */
/* ================================= DATA ================================= */
/* ======================================================================== */

#if M68K_EMULATE_TRACE
unsigned m68ki_tracing = 0;
#endif /* M68K_EMULATE_TRACE */

#if M68K_EMULATE_FC
unsigned m68ki_address_space;
#endif /* M68K_EMULATE_FC */

#if M68K_LOG_ENABLE
const char *const m68ki_cpu_names[] =
{
  "Invalid CPU",
  "M68000",
  "M68008",
  "Invalid CPU",
  "M68010",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "M68EC020",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "Invalid CPU",
  "M68020"
};
#endif /* M68K_LOG_ENABLE */

#if M68K_EMULATE_ADDRESS_ERROR == OPT_ON
#include <setjmp.h>
jmp_buf m68ki_aerr_trap;
int emulate_address_error = 0;
#endif /* M68K_EMULATE_ADDRESS_ERROR */

unsigned    m68ki_aerr_address;
unsigned    m68ki_aerr_write_mode;
unsigned    m68ki_aerr_fc;

/* Used by shift & rotate instructions */
const uint8_t m68ki_shift_8_table[65] =
{
  0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff
};
const uint16_t m68ki_shift_16_table[65] =
{
  0x0000, 0x8000, 0xc000, 0xe000, 0xf000, 0xf800, 0xfc00, 0xfe00, 0xff00,
  0xff80, 0xffc0, 0xffe0, 0xfff0, 0xfff8, 0xfffc, 0xfffe, 0xffff, 0xffff,
  0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
  0xffff, 0xffff
};
const unsigned m68ki_shift_32_table[65] =
{
  0x00000000, 0x80000000, 0xc0000000, 0xe0000000, 0xf0000000, 0xf8000000,
  0xfc000000, 0xfe000000, 0xff000000, 0xff800000, 0xffc00000, 0xffe00000,
  0xfff00000, 0xfff80000, 0xfffc0000, 0xfffe0000, 0xffff0000, 0xffff8000,
  0xffffc000, 0xffffe000, 0xfffff000, 0xfffff800, 0xfffffc00, 0xfffffe00,
  0xffffff00, 0xffffff80, 0xffffffc0, 0xffffffe0, 0xfffffff0, 0xfffffff8,
  0xfffffffc, 0xfffffffe, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff
};


/* Number of clock cycles to use for exception processing.
 * I used 4 for any vectors that are undocumented for processing times.
 */
const uint16_t m68ki_exception_cycle_table[256] =
{
   /* 000 */
     40 * M68K_CYCLE_SCALER, /*  0: Reset - Initial Stack Pointer                      */
      4 * M68K_CYCLE_SCALER, /*  1: Reset - Initial Program Counter                    */
     50 * M68K_CYCLE_SCALER, /*  2: Bus Error                             (unemulated) */
     50 * M68K_CYCLE_SCALER, /*  3: Address Error                         (unemulated) */
     34 * M68K_CYCLE_SCALER, /*  4: Illegal Instruction                                */
     38 * M68K_CYCLE_SCALER, /*  5: Divide by Zero -- ASG: changed from 42             */
     40 * M68K_CYCLE_SCALER, /*  6: CHK -- ASG: chanaged from 44                       */
     34 * M68K_CYCLE_SCALER, /*  7: TRAPV                                              */
     34 * M68K_CYCLE_SCALER, /*  8: Privilege Violation                                */
     34 * M68K_CYCLE_SCALER, /*  9: Trace                                              */
      4 * M68K_CYCLE_SCALER, /* 10: 1010                                               */
      4 * M68K_CYCLE_SCALER, /* 11: 1111                                               */
      4 * M68K_CYCLE_SCALER, /* 12: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 13: Coprocessor Protocol Violation        (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 14: Format Error                                       */
     44 * M68K_CYCLE_SCALER, /* 15: Uninitialized Interrupt                            */
      4 * M68K_CYCLE_SCALER, /* 16: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 17: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 18: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 19: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 20: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 21: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 22: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 23: RESERVED                                           */
     44 * M68K_CYCLE_SCALER, /* 24: Spurious Interrupt                                 */
     44 * M68K_CYCLE_SCALER, /* 25: Level 1 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 26: Level 2 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 27: Level 3 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 28: Level 4 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 29: Level 5 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 30: Level 6 Interrupt Autovector                       */
     44 * M68K_CYCLE_SCALER, /* 31: Level 7 Interrupt Autovector                       */
     34 * M68K_CYCLE_SCALER, /* 32: TRAP #0 -- ASG: chanaged from 38                   */
     34 * M68K_CYCLE_SCALER, /* 33: TRAP #1                                            */
     34 * M68K_CYCLE_SCALER, /* 34: TRAP #2                                            */
     34 * M68K_CYCLE_SCALER, /* 35: TRAP #3                                            */
     34 * M68K_CYCLE_SCALER, /* 36: TRAP #4                                            */
     34 * M68K_CYCLE_SCALER, /* 37: TRAP #5                                            */
     34 * M68K_CYCLE_SCALER, /* 38: TRAP #6                                            */
     34 * M68K_CYCLE_SCALER, /* 39: TRAP #7                                            */
     34 * M68K_CYCLE_SCALER, /* 40: TRAP #8                                            */
     34 * M68K_CYCLE_SCALER, /* 41: TRAP #9                                            */
     34 * M68K_CYCLE_SCALER, /* 42: TRAP #10                                           */
     34 * M68K_CYCLE_SCALER, /* 43: TRAP #11                                           */
     34 * M68K_CYCLE_SCALER, /* 44: TRAP #12                                           */
     34 * M68K_CYCLE_SCALER, /* 45: TRAP #13                                           */
     34 * M68K_CYCLE_SCALER, /* 46: TRAP #14                                           */
     34 * M68K_CYCLE_SCALER, /* 47: TRAP #15                                           */
      4 * M68K_CYCLE_SCALER, /* 48: FP Branch or Set on Unknown Condition (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 49: FP Inexact Result                     (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 50: FP Divide by Zero                     (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 51: FP Underflow                          (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 52: FP Operand Error                      (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 53: FP Overflow                           (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 54: FP Signaling NAN                      (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 55: FP Unimplemented Data Type            (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 56: MMU Configuration Error               (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 57: MMU Illegal Operation Error           (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 58: MMU Access Level Violation Error      (unemulated) */
      4 * M68K_CYCLE_SCALER, /* 59: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 60: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 61: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 62: RESERVED                                           */
      4 * M68K_CYCLE_SCALER, /* 63: RESERVED                                           */
         /* 64-255: User Defined                                   */
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,
      4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,4 * M68K_CYCLE_SCALER,


#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  { /* 010 */
     40, /*  0: Reset - Initial Stack Pointer                      */
      4, /*  1: Reset - Initial Program Counter                    */
    126, /*  2: Bus Error                             (unemulated) */
    126, /*  3: Address Error                         (unemulated) */
     38, /*  4: Illegal Instruction                                */
     44, /*  5: Divide by Zero                                     */
     44, /*  6: CHK                                                */
     34, /*  7: TRAPV                                              */
     38, /*  8: Privilege Violation                                */
     38, /*  9: Trace                                              */
      4, /* 10: 1010                                               */
      4, /* 11: 1111                                               */
      4, /* 12: RESERVED                                           */
      4, /* 13: Coprocessor Protocol Violation        (unemulated) */
      4, /* 14: Format Error                                       */
     44, /* 15: Uninitialized Interrupt                            */
      4, /* 16: RESERVED                                           */
      4, /* 17: RESERVED                                           */
      4, /* 18: RESERVED                                           */
      4, /* 19: RESERVED                                           */
      4, /* 20: RESERVED                                           */
      4, /* 21: RESERVED                                           */
      4, /* 22: RESERVED                                           */
      4, /* 23: RESERVED                                           */
     46, /* 24: Spurious Interrupt                                 */
     46, /* 25: Level 1 Interrupt Autovector                       */
     46, /* 26: Level 2 Interrupt Autovector                       */
     46, /* 27: Level 3 Interrupt Autovector                       */
     46, /* 28: Level 4 Interrupt Autovector                       */
     46, /* 29: Level 5 Interrupt Autovector                       */
     46, /* 30: Level 6 Interrupt Autovector                       */
     46, /* 31: Level 7 Interrupt Autovector                       */
     38, /* 32: TRAP #0                                            */
     38, /* 33: TRAP #1                                            */
     38, /* 34: TRAP #2                                            */
     38, /* 35: TRAP #3                                            */
     38, /* 36: TRAP #4                                            */
     38, /* 37: TRAP #5                                            */
     38, /* 38: TRAP #6                                            */
     38, /* 39: TRAP #7                                            */
     38, /* 40: TRAP #8                                            */
     38, /* 41: TRAP #9                                            */
     38, /* 42: TRAP #10                                           */
     38, /* 43: TRAP #11                                           */
     38, /* 44: TRAP #12                                           */
     38, /* 45: TRAP #13                                           */
     38, /* 46: TRAP #14                                           */
     38, /* 47: TRAP #15                                           */
      4, /* 48: FP Branch or Set on Unknown Condition (unemulated) */
      4, /* 49: FP Inexact Result                     (unemulated) */
      4, /* 50: FP Divide by Zero                     (unemulated) */
      4, /* 51: FP Underflow                          (unemulated) */
      4, /* 52: FP Operand Error                      (unemulated) */
      4, /* 53: FP Overflow                           (unemulated) */
      4, /* 54: FP Signaling NAN                      (unemulated) */
      4, /* 55: FP Unimplemented Data Type            (unemulated) */
      4, /* 56: MMU Configuration Error               (unemulated) */
      4, /* 57: MMU Illegal Operation Error           (unemulated) */
      4, /* 58: MMU Access Level Violation Error      (unemulated) */
      4, /* 59: RESERVED                                           */
      4, /* 60: RESERVED                                           */
      4, /* 61: RESERVED                                           */
      4, /* 62: RESERVED                                           */
      4, /* 63: RESERVED                                           */
         /* 64-255: User Defined                                   */
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
  },
  { /* 020 */
      4, /*  0: Reset - Initial Stack Pointer                      */
      4, /*  1: Reset - Initial Program Counter                    */
     50, /*  2: Bus Error                             (unemulated) */
     50, /*  3: Address Error                         (unemulated) */
     20, /*  4: Illegal Instruction                                */
     38, /*  5: Divide by Zero                                     */
     40, /*  6: CHK                                                */
     20, /*  7: TRAPV                                              */
     34, /*  8: Privilege Violation                                */
     25, /*  9: Trace                                              */
     20, /* 10: 1010                                               */
     20, /* 11: 1111                                               */
      4, /* 12: RESERVED                                           */
      4, /* 13: Coprocessor Protocol Violation        (unemulated) */
      4, /* 14: Format Error                                       */
     30, /* 15: Uninitialized Interrupt                            */
      4, /* 16: RESERVED                                           */
      4, /* 17: RESERVED                                           */
      4, /* 18: RESERVED                                           */
      4, /* 19: RESERVED                                           */
      4, /* 20: RESERVED                                           */
      4, /* 21: RESERVED                                           */
      4, /* 22: RESERVED                                           */
      4, /* 23: RESERVED                                           */
     30, /* 24: Spurious Interrupt                                 */
     30, /* 25: Level 1 Interrupt Autovector                       */
     30, /* 26: Level 2 Interrupt Autovector                       */
     30, /* 27: Level 3 Interrupt Autovector                       */
     30, /* 28: Level 4 Interrupt Autovector                       */
     30, /* 29: Level 5 Interrupt Autovector                       */
     30, /* 30: Level 6 Interrupt Autovector                       */
     30, /* 31: Level 7 Interrupt Autovector                       */
     20, /* 32: TRAP #0                                            */
     20, /* 33: TRAP #1                                            */
     20, /* 34: TRAP #2                                            */
     20, /* 35: TRAP #3                                            */
     20, /* 36: TRAP #4                                            */
     20, /* 37: TRAP #5                                            */
     20, /* 38: TRAP #6                                            */
     20, /* 39: TRAP #7                                            */
     20, /* 40: TRAP #8                                            */
     20, /* 41: TRAP #9                                            */
     20, /* 42: TRAP #10                                           */
     20, /* 43: TRAP #11                                           */
     20, /* 44: TRAP #12                                           */
     20, /* 45: TRAP #13                                           */
     20, /* 46: TRAP #14                                           */
     20, /* 47: TRAP #15                                           */
      4, /* 48: FP Branch or Set on Unknown Condition (unemulated) */
      4, /* 49: FP Inexact Result                     (unemulated) */
      4, /* 50: FP Divide by Zero                     (unemulated) */
      4, /* 51: FP Underflow                          (unemulated) */
      4, /* 52: FP Operand Error                      (unemulated) */
      4, /* 53: FP Overflow                           (unemulated) */
      4, /* 54: FP Signaling NAN                      (unemulated) */
      4, /* 55: FP Unimplemented Data Type            (unemulated) */
      4, /* 56: MMU Configuration Error               (unemulated) */
      4, /* 57: MMU Illegal Operation Error           (unemulated) */
      4, /* 58: MMU Access Level Violation Error      (unemulated) */
      4, /* 59: RESERVED                                           */
      4, /* 60: RESERVED                                           */
      4, /* 61: RESERVED                                           */
      4, /* 62: RESERVED                                           */
      4, /* 63: RESERVED                                           */
         /* 64-255: User Defined                                   */
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
  },
  { /* 040 */ // TODO: these values are not correct
      4, /*  0: Reset - Initial Stack Pointer                      */
      4, /*  1: Reset - Initial Program Counter                    */
     50, /*  2: Bus Error                             (unemulated) */
     50, /*  3: Address Error                         (unemulated) */
     20, /*  4: Illegal Instruction                                */
     38, /*  5: Divide by Zero                                     */
     40, /*  6: CHK                                                */
     20, /*  7: TRAPV                                              */
     34, /*  8: Privilege Violation                                */
     25, /*  9: Trace                                              */
     20, /* 10: 1010                                               */
     20, /* 11: 1111                                               */
      4, /* 12: RESERVED                                           */
      4, /* 13: Coprocessor Protocol Violation        (unemulated) */
      4, /* 14: Format Error                                       */
     30, /* 15: Uninitialized Interrupt                            */
      4, /* 16: RESERVED                                           */
      4, /* 17: RESERVED                                           */
      4, /* 18: RESERVED                                           */
      4, /* 19: RESERVED                                           */
      4, /* 20: RESERVED                                           */
      4, /* 21: RESERVED                                           */
      4, /* 22: RESERVED                                           */
      4, /* 23: RESERVED                                           */
     30, /* 24: Spurious Interrupt                                 */
     30, /* 25: Level 1 Interrupt Autovector                       */
     30, /* 26: Level 2 Interrupt Autovector                       */
     30, /* 27: Level 3 Interrupt Autovector                       */
     30, /* 28: Level 4 Interrupt Autovector                       */
     30, /* 29: Level 5 Interrupt Autovector                       */
     30, /* 30: Level 6 Interrupt Autovector                       */
     30, /* 31: Level 7 Interrupt Autovector                       */
     20, /* 32: TRAP #0                                            */
     20, /* 33: TRAP #1                                            */
     20, /* 34: TRAP #2                                            */
     20, /* 35: TRAP #3                                            */
     20, /* 36: TRAP #4                                            */
     20, /* 37: TRAP #5                                            */
     20, /* 38: TRAP #6                                            */
     20, /* 39: TRAP #7                                            */
     20, /* 40: TRAP #8                                            */
     20, /* 41: TRAP #9                                            */
     20, /* 42: TRAP #10                                           */
     20, /* 43: TRAP #11                                           */
     20, /* 44: TRAP #12                                           */
     20, /* 45: TRAP #13                                           */
     20, /* 46: TRAP #14                                           */
     20, /* 47: TRAP #15                                           */
      4, /* 48: FP Branch or Set on Unknown Condition (unemulated) */
      4, /* 49: FP Inexact Result                     (unemulated) */
      4, /* 50: FP Divide by Zero                     (unemulated) */
      4, /* 51: FP Underflow                          (unemulated) */
      4, /* 52: FP Operand Error                      (unemulated) */
      4, /* 53: FP Overflow                           (unemulated) */
      4, /* 54: FP Signaling NAN                      (unemulated) */
      4, /* 55: FP Unimplemented Data Type            (unemulated) */
      4, /* 56: MMU Configuration Error               (unemulated) */
      4, /* 57: MMU Illegal Operation Error           (unemulated) */
      4, /* 58: MMU Access Level Violation Error      (unemulated) */
      4, /* 59: RESERVED                                           */
      4, /* 60: RESERVED                                           */
      4, /* 61: RESERVED                                           */
      4, /* 62: RESERVED                                           */
      4, /* 63: RESERVED                                           */
         /* 64-255: User Defined                                   */
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
      4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4
  }
#endif
};

#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
const uint8_t m68ki_ea_idx_cycle_table[64] =
{
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0, /* ..01.000 no memory indirect, base NULL             */
   5, /* ..01..01 memory indirect,    base NULL, outer NULL */
   7, /* ..01..10 memory indirect,    base NULL, outer 16   */
   7, /* ..01..11 memory indirect,    base NULL, outer 32   */
   0,  5,  7,  7,  0,  5,  7,  7,  0,  5,  7,  7,
   2, /* ..10.000 no memory indirect, base 16               */
   7, /* ..10..01 memory indirect,    base 16,   outer NULL */
   9, /* ..10..10 memory indirect,    base 16,   outer 16   */
   9, /* ..10..11 memory indirect,    base 16,   outer 32   */
   0,  7,  9,  9,  0,  7,  9,  9,  0,  7,  9,  9,
   6, /* ..11.000 no memory indirect, base 32               */
  11, /* ..11..01 memory indirect,    base 32,   outer NULL */
  13, /* ..11..10 memory indirect,    base 32,   outer 16   */
  13, /* ..11..11 memory indirect,    base 32,   outer 32   */
   0, 11, 13, 13,  0, 11, 13, 13,  0, 11, 13, 13
};
#endif


/* ======================================================================== */
/* =============================== CALLBACKS ============================== */
/* ======================================================================== */

/* Default callbacks used if the callback hasn't been set yet, or if the
 * callback is set to NULL
 */

/* Interrupt acknowledge */
//static int default_int_ack_callback_data;
static int default_int_ack_callback(M68KCPU &m68ki_cpu, int int_level)
{
  //default_int_ack_callback_data = int_level;
  CPU_INT_LEVEL = 0;
  return M68K_INT_ACK_AUTOVECTOR;
}

/* Breakpoint acknowledge */
//static unsigned int default_bkpt_ack_callback_data;
static void default_bkpt_ack_callback(M68KCPU &m68ki_cpu, unsigned int data)
{
  //default_bkpt_ack_callback_data = data;
}

/* Called when a reset instruction is executed */
static void default_reset_instr_callback(M68KCPU &m68ki_cpu)
{
}

/* Called when a cmpi.l #v, dn instruction is executed */
static void default_cmpild_instr_callback(M68KCPU &m68ki_cpu, unsigned int val, int reg)
{
}

/* Called when a rte instruction is executed */
static void default_rte_instr_callback(M68KCPU &m68ki_cpu)
{
}

/* Called when a tas instruction is executed */
static int default_tas_instr_callback(M68KCPU &m68ki_cpu)
{
  return 1; // allow writeback
}

/* Called when the program counter changed by a large value */
//static unsigned int default_pc_changed_callback_data;
static void default_pc_changed_callback(M68KCPU &m68ki_cpu, unsigned oldPC, unsigned newPC)
{
  //default_pc_changed_callback_data = newPC;
}

/* Called every time there's bus activity (read/write to/from memory */
//static unsigned int default_set_fc_callback_data;
static void default_set_fc_callback(M68KCPU &m68ki_cpu, unsigned int new_fc)
{
  //default_set_fc_callback_data = new_fc;
}

/* Called every instruction cycle prior to execution */
static void default_instr_hook_callback(M68KCPU &m68ki_cpu, unsigned int pc)
{
}


/* ======================================================================== */
/* ================================= API ================================== */
/* ======================================================================== */

/* Access the internals of the CPU */
unsigned int m68k_get_reg(M68KCPU &m68ki_cpu, m68k_register_t regnum)
{
  M68KCPU* cpu = &m68ki_cpu;//context != NULL ?(M68KCPU*)context : &m68ki_cpu;

  switch(regnum)
  {
    case M68K_REG_D0:  return cpu->dar[0];
    case M68K_REG_D1:  return cpu->dar[1];
    case M68K_REG_D2:  return cpu->dar[2];
    case M68K_REG_D3:  return cpu->dar[3];
    case M68K_REG_D4:  return cpu->dar[4];
    case M68K_REG_D5:  return cpu->dar[5];
    case M68K_REG_D6:  return cpu->dar[6];
    case M68K_REG_D7:  return cpu->dar[7];
    case M68K_REG_A0:  return cpu->dar[8];
    case M68K_REG_A1:  return cpu->dar[9];
    case M68K_REG_A2:  return cpu->dar[10];
    case M68K_REG_A3:  return cpu->dar[11];
    case M68K_REG_A4:  return cpu->dar[12];
    case M68K_REG_A5:  return cpu->dar[13];
    case M68K_REG_A6:  return cpu->dar[14];
    case M68K_REG_A7:  return cpu->dar[15];
    case M68K_REG_PC:  return MASK_OUT_ABOVE_32(cpu->pc);
    case M68K_REG_SR:  return  cpu->t1_flag            |
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
                  cpu->t0_flag            |
#endif
                  (cpu->s_flag << 11)          |
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
                  (cpu->m_flag << 11)          |
#endif
                  cpu->int_mask            |
                  ((cpu->x_flag & XFLAG_SET) >> 4)  |
                  ((cpu->n_flag & NFLAG_SET) >> 4)  |
                  ((!cpu->not_z_flag) << 2)      |
                  ((cpu->v_flag & VFLAG_SET) >> 6)  |
                  ((cpu->c_flag & CFLAG_SET) >> 8);
    case M68K_REG_SP:  return cpu->dar[15];
    case M68K_REG_USP:  return cpu->s_flag ? cpu->sp[0] : cpu->dar[15];
    case M68K_REG_ISP:  return cpu->s_flag
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
    		&& !cpu->m_flag
#endif
    		? cpu->dar[15] : cpu->sp[4];
    case M68K_REG_MSP:  return cpu->s_flag
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
    		&& cpu->m_flag
#endif
    		? cpu->dar[15] : cpu->sp[6];
#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
    case M68K_REG_SFC:  return cpu->sfc;
    case M68K_REG_DFC:  return cpu->dfc;
    case M68K_REG_VBR:  return cpu->vbr;
    case M68K_REG_CACR:  return cpu->cacr;
    case M68K_REG_CAAR:  return cpu->caar;
#endif
#if M68K_EMULATE_PREFETCH
    case M68K_REG_PREF_ADDR:  return cpu->pref_addr;
    case M68K_REG_PREF_DATA:  return cpu->pref_data;
#endif
#ifdef M68K_USE_PPC
    case M68K_REG_PPC:  return MASK_OUT_ABOVE_32(cpu->ppc);
#endif
    case M68K_REG_IR:  return cpu->ir;
    case M68K_REG_CPU_TYPE:
      switch(cpu->cpu_type)
      {
        case CPU_TYPE_000:    return (unsigned int)M68K_CPU_TYPE_68000;
        case CPU_TYPE_008:    return (unsigned int)M68K_CPU_TYPE_68008;
        case CPU_TYPE_010:    return (unsigned int)M68K_CPU_TYPE_68010;
        case CPU_TYPE_EC020:  return (unsigned int)M68K_CPU_TYPE_68EC020;
        case CPU_TYPE_020:    return (unsigned int)M68K_CPU_TYPE_68020;
        case CPU_TYPE_040:    return (unsigned int)M68K_CPU_TYPE_68040;
      }
      return M68K_CPU_TYPE_INVALID;
    default:      return 0;
  }
  return 0;
}

void m68k_set_reg(M68KCPU &m68ki_cpu, m68k_register_t regnum, unsigned int value)
{
  switch(regnum)
  {
    case M68K_REG_D0:  REG_D[0] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D1:  REG_D[1] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D2:  REG_D[2] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D3:  REG_D[3] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D4:  REG_D[4] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D5:  REG_D[5] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D6:  REG_D[6] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_D7:  REG_D[7] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A0:  REG_A[0] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A1:  REG_A[1] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A2:  REG_A[2] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A3:  REG_A[3] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A4:  REG_A[4] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A5:  REG_A[5] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A6:  REG_A[6] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_A7:  REG_A[7] = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_PC:  m68ki_jump(m68ki_cpu, MASK_OUT_ABOVE_32(value)); return;
    case M68K_REG_SR:  m68ki_set_sr(m68ki_cpu, value); return;
    case M68K_REG_SP:  REG_SP = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_USP:  if(FLAG_S)
                REG_USP = MASK_OUT_ABOVE_32(value);
              else
                REG_SP = MASK_OUT_ABOVE_32(value);
              return;
    case M68K_REG_ISP:  if(FLAG_S && !FLAG_M)
                REG_SP = MASK_OUT_ABOVE_32(value);
              else
                REG_ISP = MASK_OUT_ABOVE_32(value);
              return;
    case M68K_REG_MSP:  if(FLAG_S && FLAG_M)
                REG_SP = MASK_OUT_ABOVE_32(value);
              else
                REG_MSP = MASK_OUT_ABOVE_32(value);
              return;
#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
    case M68K_REG_VBR:  REG_VBR = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_SFC:  REG_SFC = value & 7; return;
    case M68K_REG_DFC:  REG_DFC = value & 7; return;
    case M68K_REG_CACR:  REG_CACR = MASK_OUT_ABOVE_32(value); return;
    case M68K_REG_CAAR:  REG_CAAR = MASK_OUT_ABOVE_32(value); return;
#endif
#ifdef M68K_USE_PPC
    case M68K_REG_PPC:  REG_PPC = MASK_OUT_ABOVE_32(value); return;
#endif
    case M68K_REG_IR:  REG_IR = MASK_OUT_ABOVE_16(value); return;
#if M68K_EMULATE_PREFETCH
    case M68K_REG_PREF_ADDR:  CPU_PREF_ADDR = MASK_OUT_ABOVE_32(value); return;
#endif
    //case M68K_REG_CPU_TYPE: m68k_set_cpu_type(m68ki_cpu, value); return;
    default:      return;
  }
}

/* Set the callbacks */
void m68k_set_int_ack_callback(M68KCPU &m68ki_cpu, int  (*callback)(M68KCPU &m68ki_cpu, int int_level))
{
#if M68K_EMULATE_INT_ACK == OPT_ON
  CALLBACK_INT_ACK = callback ? callback : default_int_ack_callback;
#endif
}

/*void m68k_set_bkpt_ack_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu, unsigned int data))
{
  CALLBACK_BKPT_ACK = callback ? callback : default_bkpt_ack_callback;
}

void m68k_set_reset_instr_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu))
{
  CALLBACK_RESET_INSTR = callback ? callback : default_reset_instr_callback;
}

void m68k_set_cmpild_instr_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu, unsigned int, int))
{
  CALLBACK_CMPILD_INSTR = callback ? callback : default_cmpild_instr_callback;
}

void m68k_set_rte_instr_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu))
{
  CALLBACK_RTE_INSTR = callback ? callback : default_rte_instr_callback;
}

void m68k_set_tas_instr_callback(M68KCPU &m68ki_cpu, int  (*callback)(M68KCPU &m68ki_cpu))
{
  CALLBACK_TAS_INSTR = callback ? callback : default_tas_instr_callback;
}*/

void m68k_set_pc_changed_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu, unsigned oldPC, unsigned newPC))
{
#if M68K_MONITOR_PC == OPT_ON
  CALLBACK_PC_CHANGED = callback ? callback : default_pc_changed_callback;
#endif
}

/*void m68k_set_fc_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu, unsigned int new_fc))
{
  CALLBACK_SET_FC = callback ? callback : default_set_fc_callback;
}

void m68k_set_instr_hook_callback(M68KCPU &m68ki_cpu, void  (*callback)(M68KCPU &m68ki_cpu, unsigned int pc))
{
  CALLBACK_INSTR_HOOK = callback ? callback : default_instr_hook_callback;
}*/

#if 0
/* Set the CPU type. */
void m68k_set_cpu_type(M68KCPU &m68ki_cpu, unsigned int cpu_type)
{
  switch(cpu_type)
  {
    case M68K_CPU_TYPE_68000:
      /*CPU_TYPE         = CPU_TYPE_000;
      CPU_ADDRESS_MASK = 0x00ffffff;
      CPU_SR_MASK      = 0xa71f; /* T1 -- S  -- -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      /*CYC_INSTRUCTION  = m68ki_cycles[0];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[0];
      CYC_BCC_NOTAKE_B = -2 * M68K_CYCLE_SCALER;
      CYC_BCC_NOTAKE_W = 2 * M68K_CYCLE_SCALER;
      CYC_DBCC_F_NOEXP = -2 * M68K_CYCLE_SCALER;
      CYC_DBCC_F_EXP   = 2 * M68K_CYCLE_SCALER;
      CYC_SCC_R_TRUE   = 2 * M68K_CYCLE_SCALER;
      CYC_MOVEM_W      = 4 * M68K_CYCLE_SCALER;
      CYC_MOVEM_L      = 8 * M68K_CYCLE_SCALER;
      CYC_SHIFT        = 2 * M68K_CYCLE_SCALER;
      CYC_RESET        = 132 * M68K_CYCLE_SCALER;*/
      return;

#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
    case M68K_CPU_TYPE_68008:
      CPU_TYPE         = CPU_TYPE_008;
      CPU_ADDRESS_MASK = 0x003fffff;
      CPU_SR_MASK      = 0xa71f; /* T1 -- S  -- -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      CYC_INSTRUCTION  = m68ki_cycles[0];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[0];
      CYC_BCC_NOTAKE_B = -2;
      CYC_BCC_NOTAKE_W = 2;
      CYC_DBCC_F_NOEXP = -2;
      CYC_DBCC_F_EXP   = 2;
      CYC_SCC_R_TRUE   = 2;
      CYC_MOVEM_W      = 2;
      CYC_MOVEM_L      = 3;
      CYC_SHIFT        = 1;
      CYC_RESET        = 132;
      return;
    case M68K_CPU_TYPE_68010:
      CPU_TYPE         = CPU_TYPE_010;
      CPU_ADDRESS_MASK = 0x00ffffff;
      CPU_SR_MASK      = 0xa71f; /* T1 -- S  -- -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      CYC_INSTRUCTION  = m68ki_cycles[1];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[1];
      CYC_BCC_NOTAKE_B = -4;
      CYC_BCC_NOTAKE_W = 0;
      CYC_DBCC_F_NOEXP = 0;
      CYC_DBCC_F_EXP   = 6;
      CYC_SCC_R_TRUE   = 0;
      CYC_MOVEM_W      = 2;
      CYC_MOVEM_L      = 3;
      CYC_SHIFT        = 1;
      CYC_RESET        = 130;
      return;
    case M68K_CPU_TYPE_68EC020:
      CPU_TYPE         = CPU_TYPE_EC020;
      CPU_ADDRESS_MASK = 0x00ffffff;
      CPU_SR_MASK      = 0xf71f; /* T1 T0 S  M  -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      CYC_INSTRUCTION  = m68ki_cycles[2];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[2];
      CYC_BCC_NOTAKE_B = -2;
      CYC_BCC_NOTAKE_W = 0;
      CYC_DBCC_F_NOEXP = 0;
      CYC_DBCC_F_EXP   = 4;
      CYC_SCC_R_TRUE   = 0;
      CYC_MOVEM_W      = 2;
      CYC_MOVEM_L      = 2;
      CYC_SHIFT        = 0;
      CYC_RESET        = 518;
      return;
    case M68K_CPU_TYPE_68020:
      CPU_TYPE         = CPU_TYPE_020;
      CPU_ADDRESS_MASK = 0xffffffff;
      CPU_SR_MASK      = 0xf71f; /* T1 T0 S  M  -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      CYC_INSTRUCTION  = m68ki_cycles[2];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[2];
      CYC_BCC_NOTAKE_B = -2;
      CYC_BCC_NOTAKE_W = 0;
      CYC_DBCC_F_NOEXP = 0;
      CYC_DBCC_F_EXP   = 4;
      CYC_SCC_R_TRUE   = 0;
      CYC_MOVEM_W      = 2;
      CYC_MOVEM_L      = 2;
      CYC_SHIFT        = 0;
      CYC_RESET        = 518;
      return;
    case M68K_CPU_TYPE_68040:    // TODO: these values are not correct
      CPU_TYPE         = CPU_TYPE_040;
      CPU_ADDRESS_MASK = 0xffffffff;
      CPU_SR_MASK      = 0xf71f; /* T1 T0 S  M  -- I2 I1 I0 -- -- -- X  N  Z  V  C  */
      CYC_INSTRUCTION  = m68ki_cycles[2];
      CYC_EXCEPTION    = m68ki_exception_cycle_table[2];
      CYC_BCC_NOTAKE_B = -2;
      CYC_BCC_NOTAKE_W = 0;
      CYC_DBCC_F_NOEXP = 0;
      CYC_DBCC_F_EXP   = 4;
      CYC_SCC_R_TRUE   = 0;
      CYC_MOVEM_W      = 2;
      CYC_MOVEM_L      = 2;
      CYC_SHIFT        = 0;
      CYC_RESET        = 518;
      return;
#endif
  }
}
#endif

#if 0
/* Execute a single  instruction */
INLINE int m68k_execute(void)
{
    /* Set our pool of clock cycles available */
    SET_CYCLES(0);

    /* Set tracing accodring to T1. (T0 is done inside instruction) */
    m68ki_trace_t1(); /* auto-disable (see m68kcpu.h) */

    /* Set the address space for reads */
    m68ki_use_data_space(); /* auto-disable (see m68kcpu.h) */

    /* Call external hook to peek at CPU */
    m68ki_instr_hook(REG_PC); /* auto-disable (see m68kcpu.h) */

#ifdef M68K_USE_PPC
    /* Record previous program counter */
    REG_PPC = REG_PC;
#endif

    /* Read an instruction and call its handler */
    REG_IR = m68ki_read_imm_16();
    m68ki_instruction_jump_table[REG_IR]();
    USE_CYCLES(CYC_INSTRUCTION[REG_IR]);

    /* Trace m68k_exception, if necessary */
    m68ki_exception_if_trace(); /* auto-disable (see m68kcpu.h) */

    /* return how many clocks we used */
    return GET_CYCLES();
}
#endif

/* ASG: rewrote so that the int_level is a mask of the IPL0/IPL1/IPL2 bits */
/* KS: Modified so that IPL* bits match with mask positions in the SR
 *     and cleaned out remenants of the interrupt controller.
 */
#if 0
void m68k_set_irq(M68KCPU &m68ki_cpu, unsigned int int_level)
{
  unsigned old_level = CPU_INT_LEVEL;
  CPU_INT_LEVEL = int_level << 8;

  /* A transition from < 7 to 7 always interrupts (NMI) */
  /* Note: Level 7 can also level trigger like a normal IRQ */
  if(old_level != 0x0700 && CPU_INT_LEVEL == 0x0700)
    m68ki_exception_interrupt(m68ki_cpu, 7); /* Edge triggered level 7 (NMI) */
  else
    m68ki_check_interrupts(m68ki_cpu); /* Level triggered (IRQ) */
}
#endif

#ifdef LOGVDP
extern uint16_t v_counter;
extern void error(char *format, ...);
#endif



#include "m68kops.c"

void M68KCPU::updateIRQ(unsigned mask)
{
	int_level |= (mask << 8);
	m68ki_check_interrupts(*this);
}

void M68KCPU::setIRQ(unsigned mask)
{
	int_level = mask << 8;
	m68ki_check_interrupts(*this);
}

/* IRQ latency (Fatal Rewind, Sesame's Street Counting Cafe)*/
void M68KCPU::setIRQDelay(unsigned int mask)
{
	M68KCPU &m68ki_cpu = *this;
  /* Prevent reentrance */
  if (!irqLatency)
  {
    /* This is always triggered from MOVE instructions (VDP CTRL port write) */
    /* We just make sure this is not a MOVE.L instruction as we could be in */
    /* the middle of its execution (first memory write).                   */
    if ((ir & 0xF000) != 0x2000)
    {
      /* Finish executing current instruction */
      USE_CYCLES(CYC_INSTRUCTION[ir]);

      /* One instruction delay before interrupt */
      irqLatency = 1;
      m68ki_trace_t1() /* auto-disable (see m68kcpu.h) */
      m68ki_use_data_space() /* auto-disable (see m68kcpu.h) */
      ir = m68ki_read_imm_16(*this);
      m68ki_instruction_jump_table[ir](*this);
      m68ki_exception_if_trace() /* auto-disable (see m68kcpu.h) */
      irqLatency = 0;
    }

    /* Set IRQ level */
    int_level = mask << 8;
  }

#ifdef LOGVDP
  error("[%d(%d)][%d(%d)] IRQ Level = %d(0x%02x) (%x)\n", v_counter, mcycles_68k/3420, mcycles_68k, mcycles_68k%3420,CPU_INT_LEVEL>>8,FLAG_INT_MASK,m68k_get_reg(M68K_REG_PC));
#endif

  /* Check interrupt mask to process IRQ  */
  m68ki_check_interrupts(*this); /* Level triggered (IRQ) */
}

void m68k_run(M68KCPU &m68ki_cpu, int cycles)
{
  /* Make sure we're not stopped */
  if (CPU_STOPPED)
  {
  	m68ki_cpu.cycleCount = cycles;
    return;
  }

  /* Return point for when we have an address error (TODO: use goto) */
  m68ki_set_address_error_trap() /* auto-disable (see m68kcpu.h) */

  /* Save end cycles count for when CPU is stopped */
  m68ki_cpu.endCycles = cycles;

  while (m68ki_cpu.cycleCount < cycles)
  {
    /* Set tracing accodring to T1. */
    m68ki_trace_t1() /* auto-disable (see m68kcpu.h) */

    /* Set the address space for reads */
    m68ki_use_data_space() /* auto-disable (see m68kcpu.h) */

    /* Decode next instruction */
    REG_IR = m68ki_read_imm_16(m68ki_cpu);

    /* Execute instruction */
	  m68ki_instruction_jump_table[REG_IR](m68ki_cpu); /* TODO: use labels table with goto */
    USE_CYCLES(CYC_INSTRUCTION[REG_IR]); /* TODO: move into instruction handlers */

    /* Trace m68k_exception, if necessary */
    m68ki_exception_if_trace(); /* auto-disable (see m68kcpu.h) */
  }
}

#if 0
int m68k_cycles_run(void)
{
  return m68ki_initial_cycles - GET_CYCLES();
}

int m68k_cycles_remaining(void)
{
  return GET_CYCLES();
}

/* Change the timeslice */
void m68k_modify_timeslice(int cycles)
{
  m68ki_initial_cycles += cycles;
  ADD_CYCLES(cycles);
}

void m68k_end_timeslice(void)
{
  m68ki_initial_cycles = GET_CYCLES();
  SET_CYCLES(0);
}
#endif

void m68k_init(M68KCPU &m68ki_cpu)
{
  #ifdef BUILD_OP_TABLE
  static unsigned emulation_initialized = 0;

  /* The first call to this function initializes the opcode handler jump table */
  if(!emulation_initialized)
  {
    m68ki_build_opcode_table();
    emulation_initialized = 1;
  }
  #endif

  m68k_set_int_ack_callback(m68ki_cpu, NULL);
  /*m68k_set_bkpt_ack_callback(m68ki_cpu, NULL);
  m68k_set_reset_instr_callback(m68ki_cpu, NULL);
  m68k_set_cmpild_instr_callback(m68ki_cpu, NULL);
  m68k_set_rte_instr_callback(m68ki_cpu, NULL);
  m68k_set_tas_instr_callback(m68ki_cpu, NULL);*/
  m68k_set_pc_changed_callback(m68ki_cpu, NULL);
  /*m68k_set_fc_callback(m68ki_cpu, NULL);
  m68k_set_instr_hook_callback(m68ki_cpu, NULL);*/
}

/* Pulse the RESET line on the CPU */
void m68k_pulse_reset(M68KCPU &m68ki_cpu)
{
	logMsg("68K pulse reset");
  /* Clear all stop levels and eat up all remaining cycles */
  CPU_STOPPED = 0;
#if 0
  SET_CYCLES(0);
#endif
#if M68K_EMULATE_ADDRESS_ERROR
  CPU_RUN_MODE = RUN_MODE_BERR_AERR_RESET;
#endif

  /* Turn off tracing */
  FLAG_T1 =
#if M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  FLAG_T0 =
#endif
  	0;
  m68ki_clear_trace();
  /* Interrupt mask to level 7 */
  FLAG_INT_MASK = 0x0700;
  CPU_INT_LEVEL = 0;
  m68ki_cpu.irqLatency = 0;
#if M68K_EMULATE_010 || M68K_EMULATE_020 || M68K_EMULATE_EC020 || M68K_EMULATE_040
  /* Reset VBR */
  REG_VBR = 0;
#endif
  /* Go to supervisor mode */
  m68ki_set_sm_flag(m68ki_cpu, SFLAG_SET | MFLAG_CLEAR);

  /* Invalidate the prefetch queue */
#if M68K_EMULATE_PREFETCH
  /* Set to arbitrary number since our first fetch is from 0 */
  CPU_PREF_ADDR = 0x1000;
#endif /* M68K_EMULATE_PREFETCH */

  /* Read the initial stack pointer and program counter */
  m68ki_jump(m68ki_cpu, 0);
  REG_SP = m68ki_read_imm_32(m68ki_cpu);
  REG_PC = m68ki_read_imm_32(m68ki_cpu);
  m68ki_jump(m68ki_cpu, REG_PC);

#if M68K_EMULATE_ADDRESS_ERROR
  CPU_RUN_MODE = RUN_MODE_NORMAL;
#endif

  USE_CYCLES(CYC_EXCEPTION[EXCEPTION_RESET]);
}

/* Pulse the HALT line on the CPU */
void m68k_pulse_halt(M68KCPU &m68ki_cpu)
{
	logMsg("68K pulse halt");
  CPU_STOPPED |= STOP_LEVEL_HALT;
}


/* ======================================================================== */
/* ============================== MAME STUFF ============================== */
/* ======================================================================== */

#if M68K_COMPILE_FOR_MAME == OPT_ON

static struct {
  uint16_t sr;
  uint8_t stopped;
  uint8_t halted;
} m68k_substate;

static void m68k_prepare_substate(void)
{
  m68k_substate.sr = m68ki_get_sr();
  m68k_substate.stopped = (CPU_STOPPED & STOP_LEVEL_STOP) != 0;
  m68k_substate.halted  = (CPU_STOPPED & STOP_LEVEL_HALT) != 0;
}

static void m68k_post_load(void)
{
  m68ki_set_sr_noint_nosp(m68k_substate.sr);
  CPU_STOPPED = m68k_substate.stopped ? STOP_LEVEL_STOP : 0
            | m68k_substate.halted  ? STOP_LEVEL_HALT : 0;
  m68ki_jump(REG_PC);
}

void m68k_state_register(const char *type, int index)
{
  /* Note, D covers A because the dar array is common, REG_A=REG_D+8 */
  state_save_register_item_array(type, index, REG_D);
  state_save_register_item(type, index, REG_PPC);
  state_save_register_item(type, index, REG_PC);
  state_save_register_item(type, index, REG_USP);
  state_save_register_item(type, index, REG_ISP);
  state_save_register_item(type, index, REG_MSP);
  state_save_register_item(type, index, REG_VBR);
  state_save_register_item(type, index, REG_SFC);
  state_save_register_item(type, index, REG_DFC);
  state_save_register_item(type, index, REG_CACR);
  state_save_register_item(type, index, REG_CAAR);
  state_save_register_item(type, index, m68k_substate.sr);
  state_save_register_item(type, index, CPU_INT_LEVEL);
  state_save_register_item(type, index, m68k_substate.stopped);
  state_save_register_item(type, index, m68k_substate.halted);
  state_save_register_item(type, index, CPU_PREF_ADDR);
  state_save_register_item(type, index, CPU_PREF_DATA);
  state_save_register_func_presave(m68k_prepare_substate);
  state_save_register_func_postload(m68k_post_load);
}

#endif /* M68K_COMPILE_FOR_MAME */

/* ======================================================================== */
/* ============================== END OF FILE ============================= */
/* ======================================================================== */
