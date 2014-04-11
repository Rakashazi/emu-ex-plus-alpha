#ifndef Z80_H
#define Z80_H

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#ifndef WORDS_BIGENDIAN
#define LSB_FIRST
#endif

//#include "SDL_types.h"
#include <imagine/util/ansiTypes.h>
#define UINT8     uint8
#define UINT16    uint16
#define UINT32    uint32
#define INT8      sint8
#define INT16     sint16
#define INT32     sint32
#ifndef INLINE
#define INLINE static __inline__
#endif

#define CALL_MAME_DEBUG

/* ----- Extracted from MAME cpuintrf --------- */

typedef union {
#ifdef LSB_FIRST
        struct { UINT8 l,h,h2,h3; } b;
        struct { UINT16 l,h; } w;
#else
        struct { UINT8 h3,h2,h,l; } b;
        struct { UINT16 h,l; } w;
#endif
        UINT32 d;
}       PAIR;

typedef struct
{
        void (*reset)(int);                     /* reset callback         */
        int  (*interrupt_entry)(int);   /* entry callback         */
        void (*interrupt_reti)(int);    /* reti callback          */
        int irq_param;                                  /* callback paramater */
} Z80_DaisyChain;

#define Z80_MAXDAISY    1               /* maximum of daisy chan device */

#define Z80_INT_REQ     0x01    /* interrupt request mask               */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))
enum
{
  /* line states */
  CLEAR_LINE = 0,  /* clear (a fired, held or pulsed) line */
  ASSERT_LINE,     /* assert an interrupt immediately */
  HOLD_LINE,       /* hold interrupt line until acknowledged */
  PULSE_LINE,      /* pulse interrupt line -for one instruction */

  /* internal flags (not for use by drivers!) */
  INTERNAL_CLEAR_LINE = 100 + CLEAR_LINE,
  INTERNAL_ASSERT_LINE = 100 + ASSERT_LINE,

  /* interrupt parameters */
  MAX_IRQ_LINES = 16,                     /* maximum number of IRQ lines per CPU */
  IRQ_LINE_NMI = 127                      /* IRQ line for NMIs */
};

enum
{
        MAX_REGS = 128,                         /* maximum number of register of any CPU */

        /* This value is passed to activecpu_get_reg to retrieve the previous
         * program counter value, ie. before a CPU emulation started
         * to fetch opcodes and arguments for the current instrution. */
        REG_PREVIOUSPC = -1,

        /* This value is passed to activecpu_get_reg to retrieve the current
         * program counter value. */
        REG_PC = -2,

        /* This value is passed to activecpu_get_reg to retrieve the current
         * stack pointer value. */
        REG_SP = -3,

        /* This value is passed to activecpu_get_reg/activecpu_set_reg, instead of one of
         * the names from the enum a CPU core defines for it's registers,
         * to get or set the contents of the memory pointed to by a stack pointer.
         * You can specify the n'th element on the stack by (REG_SP_CONTENTS-n),
         * ie. lower negative values. The actual element size (UINT16 or UINT32)
         * depends on the CPU core. */
        REG_SP_CONTENTS = -4
};


#define change_pc16(pc) Z80.PC.w.l=pc;

/* ---- END of mame extract ----- */

enum
{
        CPU_INFO_REG,
        CPU_INFO_FLAGS = MAX_REGS,
        CPU_INFO_NAME,
        CPU_INFO_FAMILY,
        CPU_INFO_VERSION,
        CPU_INFO_FILE,
        CPU_INFO_CREDITS,
        CPU_INFO_REG_LAYOUT,
        CPU_INFO_WIN_LAYOUT
};

enum {
	Z80_PC=1, Z80_SP, Z80_AF, Z80_BC, Z80_DE, Z80_HL,
	Z80_IX, Z80_IY,	Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
	Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
	Z80_NMI_STATE, Z80_IRQ_STATE, Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3
};

enum {
	Z80_TABLE_op,
	Z80_TABLE_cb,
	Z80_TABLE_ed,
	Z80_TABLE_xy,
	Z80_TABLE_xycb,
	Z80_TABLE_ex	/* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

extern int z80_ICount;              /* T-state count                        */

extern void z80_init(void);
extern void z80_reset (void *param);
extern void z80_exit (void);
extern int z80_execute(int cycles);
extern void z80_burn(int cycles);
extern unsigned z80_get_context (void *dst);
extern void z80_set_context (void *src);
extern const void *z80_get_cycle_table (int which);
extern void z80_set_cycle_table (int which, void *new_tbl);
extern unsigned z80_get_reg (int regnum);
extern void z80_set_reg (int regnum, unsigned val);
extern void z80_set_irq_line(int irqline, int state);
extern void z80_set_irq_callback(int (*irq_callback)(int));
extern const char *z80_info(void *context, int regnum);
extern unsigned z80_dasm(char *buffer, unsigned pc);


/* interface */
extern void mame_z80_writemem16(UINT16 addr,UINT8 val);
extern UINT8 mame_z80_readmem16(UINT16 addr);
extern UINT8 mame_z80_readop(UINT16 addr);
extern UINT8 mame_z80_readop_arg(UINT16 addr);
extern UINT8 mame_z80_readport16(UINT16 port);
extern void mame_z80_writeport16(UINT16 port,UINT8 value);

#ifdef MAME_DEBUG
extern unsigned DasmZ80(char *buffer, unsigned pc);
#endif

extern int z80_stateDataSize;
UINT8 *z80_stateData();

#endif

