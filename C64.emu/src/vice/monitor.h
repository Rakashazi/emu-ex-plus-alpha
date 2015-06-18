/*
 * monitor.h - The VICE built-in monitor, external interface.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef VICE_MONITOR_H
#define VICE_MONITOR_H

#include "types.h"
#include "monitor/asm.h"

/** Generic interface.  **/
#define NUM_MEMSPACES e_invalid_space

enum mon_int {
    MI_NONE = 0,
    MI_BREAK = 1 << 0,
    MI_WATCH = 1 << 1,
    MI_STEP = 1 << 2
};

enum t_memspace {
    e_default_space = 0,
    e_comp_space,
    e_disk8_space,
    e_disk9_space,
    e_disk10_space,
    e_disk11_space,
    e_invalid_space
};
typedef enum t_memspace MEMSPACE;

enum CPU_TYPE_s {
    CPU_6502,
    CPU_R65C02,
    CPU_65816,
    CPU_Z80,
    CPU_6502DTV,
    CPU_6809
};
typedef enum CPU_TYPE_s CPU_TYPE_t;

struct interrupt_cpu_status_s;

struct monitor_cpu_type_s {
    CPU_TYPE_t cpu_type;
    unsigned int (*asm_addr_mode_get_size)(unsigned int mode, unsigned int p0,
                                           unsigned int p1, unsigned int p2);
    const struct asm_opcode_info_s *(*asm_opcode_info_get)(unsigned int p0, unsigned int p1,
                                                           unsigned int p2);
    int (*mon_assemble_instr)(const char *opcode_name, asm_mode_addr_info_t operand);
    unsigned int (*mon_register_get_val)(int mem, int reg_id);
    void (*mon_register_set_val)(int mem, int reg_id, WORD val);
    void (*mon_register_print)(int mem);
    const char* (*mon_register_print_ex)(int mem);
    struct mon_reg_list_s *(*mon_register_list_get)(int mem);
};
typedef struct monitor_cpu_type_s monitor_cpu_type_t;

/* This is the standard interface through which the monitor accesses a
   certain CPU.  */
struct monitor_interface_s {
    /* Pointer to the registers of the 6502 CPU.  */
    struct mos6510_regs_s *cpu_regs;

    /* Pointer to the registers of the R65C02 CPU. */
    struct R65C02_regs_s *cpu_R65C02_regs;

    /* Pointer to the registers of the 65816/65802 CPU. */
    struct WDC65816_regs_s *cpu_65816_regs;

    /* Pointer to the registers of the Z80 CPU.  */
    struct z80_regs_s *z80_cpu_regs;

    /* Pointer to the registers of the DTV CPU.  */
    struct mos6510dtv_regs_s *dtv_cpu_regs;

    /* Pointer to the registers of the DTV CPU.  */
    struct h6809_regs_s *h6809_cpu_regs;

    /* Pointer to the alarm/interrupt status.  */
    struct interrupt_cpu_status_s *int_status;

    /* Pointer to the machine's clock counter.  */
    CLOCK *clk;

    int current_bank;
    const char **(*mem_bank_list)(void);
    int (*mem_bank_from_name)(const char *name);
    BYTE (*mem_bank_read)(int bank, WORD addr, void *context);
    BYTE (*mem_bank_peek)(int bank, WORD addr, void *context);
    void (*mem_bank_write)(int bank, WORD addr, BYTE byte, void *context);

    struct mem_ioreg_list_s *(*mem_ioreg_list_get)(void *context);

    /* Pointer to a function to disable/enable watchpoint checking.  */
    /*monitor_toggle_func_t *toggle_watchpoints_func;*/
    void (*toggle_watchpoints_func)(int value, void *context);

    /* Update bank base (used for drives).  */
    void (*set_bank_base)(void *context);

    void (*get_line_cycle)(unsigned int *line, unsigned int *cycle, int *half_cycle);

    void *context;
};
typedef struct monitor_interface_s monitor_interface_t;

/* Externals */
extern unsigned monitor_mask[NUM_MEMSPACES];


/* Prototypes */
extern monitor_cpu_type_t* monitor_find_cpu_type_from_string(const char *cpu_type);

extern void monitor_init(monitor_interface_t * maincpu_interface,
                         monitor_interface_t * drive_interface_init[],
                         struct monitor_cpu_type_s **asmarray);
extern void monitor_shutdown(void);
extern int monitor_cmdline_options_init(void);
extern int monitor_resources_init(void);
void monitor_startup(MEMSPACE mem);
extern void monitor_startup_trap(void);

extern void monitor_abort(void);

extern int monitor_force_import(MEMSPACE mem);
extern void monitor_check_icount(WORD a);
extern void monitor_check_icount_interrupt(void);
extern void monitor_check_watchpoints(unsigned int lastpc, unsigned int pc);

extern void monitor_cpu_type_set(const char *cpu_type);

extern void monitor_watch_push_load_addr(WORD addr, MEMSPACE mem);
extern void monitor_watch_push_store_addr(WORD addr, MEMSPACE mem);

extern monitor_interface_t *monitor_interface_new(void);
extern void monitor_interface_destroy(monitor_interface_t *monitor_interface);

extern int monitor_diskspace_dnr(int mem);
extern int monitor_diskspace_mem(int dnr);

#ifdef __GNUC__
extern int mon_out(const char *format, ...)
    __attribute__((format(printf, 1, 2)));
#else
extern int mon_out(const char *format, ...);
#endif

/** Breakpoint interface.  */

/* Prototypes */
extern int monitor_check_breakpoints(MEMSPACE mem, WORD addr);

/** Disassemble interace */
/* Prototypes */
extern const char *mon_disassemble_to_string(MEMSPACE, unsigned int addr, unsigned int x,
                                             unsigned int p1, unsigned int p2, unsigned int p3,
                                             int hex_mode,
                                             const char *cpu_type);

/** Register interface.  */
extern struct mon_reg_list_s *mon_register_list_get(int mem);
extern void mon_ioreg_add_list(struct mem_ioreg_list_s **list, const char *name,
                               int start, int end, void *dump);

/* Assembler initialization.  */
extern void asm6502_init(struct monitor_cpu_type_s *monitor_cpu_type);
extern void asmR65C02_init(struct monitor_cpu_type_s *monitor_cpu_type);
extern void asm65816_init(struct monitor_cpu_type_s *monitor_cpu_type);
extern void asm6502dtv_init(struct monitor_cpu_type_s *monitor_cpu_type);
extern void asm6809_init(struct monitor_cpu_type_s *monitor_cpu_type);
extern void asmz80_init(struct monitor_cpu_type_s *monitor_cpu_type);

struct monitor_cartridge_commands_s {
    int (*cartridge_attach_image)(int type, const char *filename);
    void (*cartridge_detach_image)(int type);
    void (*cartridge_trigger_freeze)(void);
    void (*cartridge_trigger_freeze_nmi_only)(void);
    void (*export_dump)(void);
};
typedef struct monitor_cartridge_commands_s monitor_cartridge_commands_t;

extern monitor_cartridge_commands_t mon_cart_cmd;

/* CPU history/memmap prototypes */
extern void monitor_cpuhistory_store(unsigned int addr, unsigned int op, unsigned int p1, unsigned int p2,
                                     BYTE reg_a, BYTE reg_x, BYTE reg_y,
                                     BYTE reg_sp, unsigned int reg_st);
extern void monitor_memmap_store(unsigned int addr, unsigned int type);

/* memmap defines */
#define MEMMAP_I_O_R 0x80
#define MEMMAP_I_O_W 0x40
#define MEMMAP_ROM_R 0x20
#define MEMMAP_ROM_W 0x10
#define MEMMAP_ROM_X 0x08
#define MEMMAP_RAM_R 0x04
#define MEMMAP_RAM_W 0x02
#define MEMMAP_RAM_X 0x01

/* HACK to enable fetch/load separation */
extern BYTE memmap_state;
#define MEMMAP_STATE_IGNORE 0x04
#define MEMMAP_STATE_INSTR  0x02
#define MEMMAP_STATE_OPCODE 0x01

/* strtoul replacement for sunos4 */
#if defined(sun) || defined(__sun)
#  if !defined(__SVR4) && !defined(__svr4__)
#    define strtoul(a, b, c) (unsigned long)strtol(a, b, c)
#  endif
#endif

#endif
