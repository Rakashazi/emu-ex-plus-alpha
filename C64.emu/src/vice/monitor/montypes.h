/*
 * montypes.h - The VICE built-in monitor, internal interface.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_MONTYPES_H
#define VICE_MONTYPES_H

#include <stdbool.h>

#include "monitor.h"
#include "types.h"

#if 0
/* set this to enable experimental 24-bit address space support */
#define HAVE_MEMSPACE24
#endif

/* Types */

/* 65xx: 6502/6509/6510/7501/8500/8501/8502/65C02/65SC02
   658xx: 65802/65816
   6x09: 6809/6309

   These values are used in the binary monitor API, so it
   is important that they remain consistent.
 */
enum t_reg_id       {
    e_A            = 0x00, /* 65xx/c64dtv/658xx/6x09/z80 */
    e_X            = 0x01, /* 65xx/c64dtv/658xx/6x09     */
    e_Y            = 0x02, /* 65xx/c64dtv/658xx/6x09     */
    e_PC           = 0x03, /* 65xx/c64dtv/658xx/6x09/z80 */
    e_SP           = 0x04, /* 65xx/c64dtv/658xx/6x09/z80 */
    e_FLAGS        = 0x05, /* 65xx/c64dtv/658xx/6x09     */
    e_AF           = 0x06, /* z80                        */
    e_BC           = 0x07, /* z80                        */
    e_DE           = 0x08, /* z80                        */
    e_HL           = 0x09, /* z80                        */
    e_IX           = 0x0a, /* z80                        */
    e_IY           = 0x0b, /* z80                        */
    e_I            = 0x0c, /* z80                        */
    e_R            = 0x0d, /* z80                        */
    e_AF2          = 0x0e, /* z80                        */
    e_BC2          = 0x0f, /* z80                        */
    e_DE2          = 0x10, /* z80                        */
    e_HL2          = 0x11, /* z80                        */
    e_R3           = 0x12, /* c64dtv                     */
    e_R4           = 0x13, /* c64dtv                     */
    e_R5           = 0x14, /* c64dtv                     */
    e_R6           = 0x15, /* c64dtv                     */
    e_R7           = 0x16, /* c64dtv                     */
    e_R8           = 0x17, /* c64dtv                     */
    e_R9           = 0x18, /* c64dtv                     */
    e_R10          = 0x19, /* c64dtv                     */
    e_R11          = 0x1a, /* c64dtv                     */
    e_R12          = 0x1b, /* c64dtv                     */
    e_R13          = 0x1c, /* c64dtv                     */
    e_R14          = 0x1d, /* c64dtv                     */
    e_R15          = 0x1e, /* c64dtv                     */
    e_ACM          = 0x1f, /* c64dtv                     */
    e_YXM          = 0x20, /* c64dtv                     */
    e_B            = 0x21, /* 658xx/6x09/z80             */
    e_C            = 0x22, /* 658xx/z80                  */
    e_DPR          = 0x23, /* 658xx                      */
    e_PBR          = 0x24, /* 658xx                      */
    e_DBR          = 0x25, /* 658xx                      */
    e_D            = 0x26, /* 6x09/z80                   */
    e_U            = 0x27, /* 6x09                       */
    e_DP           = 0x28, /* 6x09                       */
    e_E            = 0x29, /* 658xx/6309/z80             */
    e_F            = 0x2a, /* 6309                       */
    e_W            = 0x2b, /* 6309                       */
    e_Q            = 0x2c, /* 6309                       */
    e_V            = 0x2d, /* 6309                       */
    e_MD           = 0x2e, /* 6309                       */
    e_H            = 0x2f, /* z80                        */
    e_L            = 0x30, /* z80                        */
    e_IXL          = 0x31, /* z80                        */
    e_IXH          = 0x32, /* z80                        */
    e_IYL          = 0x33, /* z80                        */
    e_IYH          = 0x34, /* z80                        */
    e_Rasterline   = 0x35, /* Rasterline                 */
    e_Cycle        = 0x36, /* Cycle                      */
    e_Zero         = 0x37, /* 6510/c64dtv                */
    e_One          = 0x38  /* 6510/c64dtv                */
};
typedef enum t_reg_id REG_ID;

enum t_memory_op {
    e_load  = 0x01,
    e_store = 0x02,
    e_exec  = 0x04
};
typedef enum t_memory_op MEMORY_OP;

typedef unsigned int MON_ADDR;
typedef unsigned int MON_REG;

enum t_conditional {
    e_INV,
    e_EQU,
    e_NEQ,
    e_GT,
    e_LT,
    e_GTE,
    e_LTE,
    e_LOGICAL_AND,
    e_LOGICAL_OR,
    e_ADD,
    e_SUB,
    e_MUL,
    e_DIV,
    e_BINARY_AND,
    e_BINARY_OR
};
typedef enum t_conditional CONDITIONAL;

enum t_radixtype {
    e_default_radix,
    e_hexadecimal,
    e_decimal,
    e_octal,
    e_binary
};
typedef enum t_radixtype RADIXTYPE;

enum t_action {
    e_OFF = 0,
    e_ON = 1,
    e_TOGGLE = 2
};
typedef enum t_action ACTION;

enum t_io_sim_result {
    e_IO_SIM_RESULT_OK = 0,
    e_IO_SIM_RESULT_GENERAL_FAILURE = -1,
    e_IO_SIM_RESULT_ILLEGAL_VALUE = -2,
    e_IO_SIM_RESULT_ILLEGAL_PORT = -3,
};
typedef enum t_io_sim_result IO_SIM_RESULT;

struct cond_node_s {
    int operation;
    int value;
    int banknum;
    MON_REG reg_num;
    bool is_reg;
    bool is_parenthized;
    struct cond_node_s *child1;
    struct cond_node_s *child2;
};
typedef struct cond_node_s cond_node_t;

typedef void monitor_toggle_func_t(int value);

/* Defines */
#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define HI16(x) ((x) & 0xffff0000)
#define LO16(x) ((x) & 0xffff)
#define LO16_TO_HI16(x) (((x) & 0xffff) << 16)
#define HI16_TO_LO16(x) (((x) >> 16) & 0xffff)

#ifdef HAVE_MEMSPACE24
#define HI8(x) ((x) & 0xff000000)
#define LO24(x) ((x) & 0xffffff)
#define LO8_TO_HI8(x) (((x) & 0xff) << 24)
#define HI8_TO_LO8(x) (((x) >> 24) & 0xff)
#endif

#define STATE_INITIAL  0
#define STATE_FNAME    1
#define STATE_REG_ASGN 2
#define STATE_ROL      3
#define STATE_BNAME    4
#define STATE_CTYPE    5

#define DEFAULT_DISASSEMBLY_SIZE 40

#define any_watchpoints_load(mem) (watchpoints_load[(mem)] != NULL)
#define any_watchpoints_store(mem) (watchpoints_store[(mem)] != NULL)

#define new_cond ((cond_node_t *)(lib_malloc(sizeof(cond_node_t))))
#ifndef HAVE_MEMSPACE24
#define addr_memspace(ma) (HI16_TO_LO16(ma))
#define addr_location(ma) (LO16(ma))
#define addr_mask(l) (LO16(l))
#define new_addr(m, l) (LO16_TO_HI16(m) | (l))
#else
#define addr_memspace(ma) (HI8_TO_LO8(ma))
#define addr_location(ma) (LO24(ma))
#define addr_mask(l) (LO24(l))
#define new_addr(m, l) (LO8_TO_HI8(m) | (l))
#endif
#define new_reg(m, r) (LO16_TO_HI16(m) | (r))
#define reg_memspace(mr) (HI16_TO_LO16(mr))
#define reg_regid(mr) (LO16(mr))

/* Global variables */

extern const char * const _mon_space_strings[];

struct console_s;
struct monitor_cpu_type_s;

extern struct console_s *console_log;
extern int sidefx;
extern int break_on_dummy_access;
extern int exit_mon;

extern RADIXTYPE default_radix;
extern MEMSPACE default_memspace;
extern bool asm_mode;
extern MON_ADDR asm_mode_addr;
extern struct monitor_cpu_type_s *monitor_cpu_for_memspace[NUM_MEMSPACES];
extern MON_ADDR dot_addr[NUM_MEMSPACES];
extern const char * const mon_memspace_string[];
extern int mon_stop_output;
extern monitor_interface_t *mon_interfaces[NUM_MEMSPACES];
extern bool force_array[NUM_MEMSPACES];
extern unsigned char data_buf[256];
extern unsigned char data_mask_buf[256];
extern unsigned int data_buf_len;

/* Function declarations */
void mon_clear_buffer(void);
void mon_add_number_to_buffer(int number);
void mon_add_number_masked_to_buffer(int number, int mask);
void mon_add_string_to_buffer(char *str);
void mon_backtrace(void);
void mon_cart_freeze(void);
IO_SIM_RESULT mon_userport_set_output(int value);
IO_SIM_RESULT mon_joyport_set_output(int port, int value);
void mon_reset_machine(int type);
void mon_resource_get(const char *name);
void mon_resource_set(const char *name, const char* value);
void mon_screenshot_save(const char* filename, int format);
void mon_show_dir(const char *path);
void mon_show_pwd(void);
void mon_make_dir(const char *path);
void mon_remove_dir(const char *path);
void mon_tape_ctrl(int port, int command);
void mon_display_screen(long addr);
void mon_instructions_step(int count);
void mon_instructions_next(int count);
void mon_instruction_return(void);
void mon_stack_up(int count);
void mon_stack_down(int count);
void mon_print_convert(int val);
void mon_change_dir(const char *path);
void mon_bank(MEMSPACE mem, const char *bank);
const char *mon_get_current_bank_name(MEMSPACE mem);
const char *mon_get_bank_name_for_bank(MEMSPACE mem, int banknum);
int mon_get_bank_index_for_bank(MEMSPACE mem, int banknum);
int mon_get_bank_flags_for_bank(MEMSPACE mem, int banknum);
const int mon_banknum_validate(MEMSPACE mem, int banknum);
int mon_banknum_from_bank(MEMSPACE mem, const char *bankname);

void mon_display_io_regs(MON_ADDR addr);
void mon_evaluate_default_addr(MON_ADDR *a);
void mon_set_mem_val(MEMSPACE mem, uint16_t mem_addr, uint8_t val);
void mon_set_mem_val_ex(MEMSPACE mem, int bank, uint16_t mem_addr, uint8_t val);
bool mon_inc_addr_location(MON_ADDR *a, unsigned inc);
void mon_start_assemble_mode(MON_ADDR addr, char *asm_line);
long mon_evaluate_address_range(MON_ADDR *start_addr, MON_ADDR *end_addr, bool must_be_range, uint16_t default_len);

bool check_drive_emu_level_ok(int drive_num);
void mon_print_conditional(cond_node_t *cnode);
void mon_delete_conditional(cond_node_t *cnode);
int mon_evaluate_conditional(cond_node_t *cnode);
int mon_write_snapshot(const char* name, int save_roms, int save_disks, int even_mode);
int mon_read_snapshot(const char* name, int even_mode);
bool mon_is_valid_addr(MON_ADDR a);
bool mon_is_in_range(MON_ADDR start_addr, MON_ADDR end_addr, unsigned loc);
void mon_print_bin(int val, char on, char off);
uint8_t mon_get_mem_val(MEMSPACE mem, uint16_t mem_addr);
uint8_t mon_get_mem_val_ex(MEMSPACE mem, int bank, uint16_t mem_addr);

/* the _nosfx variants must be used when the monitor must absolutely not cause
   any sideeffect, be it emulated I/O or (re)triggering checkpoints */
uint8_t mon_get_mem_val_ex_nosfx(MEMSPACE mem, int bank, uint16_t mem_addr);
uint8_t mon_get_mem_val_nosfx(MEMSPACE mem, uint16_t mem_config, uint16_t mem_addr);
void mon_get_mem_block(MEMSPACE mem, uint16_t mem_start, uint16_t mem_end, uint8_t *data);
void mon_get_mem_block_ex(MEMSPACE mem, int bank, uint16_t mem_start, uint16_t mem_end, uint8_t *data);
void mon_jump(MON_ADDR addr);
void mon_go(void);
void mon_exit(void);
void mon_quit(void);
void mon_keyboard_feed(const char *string);
char *mon_symbol_table_lookup_name(MEMSPACE mem, uint16_t addr);
int mon_symbol_table_lookup_addr(MEMSPACE mem, char *name);
char* mon_prepend_dot_to_name(char *name);
void mon_add_name_to_symbol_table(MON_ADDR addr, char *name);
void mon_remove_name_from_symbol_table(MEMSPACE mem, char *name);
void mon_print_symbol_table(MEMSPACE mem);
void mon_clear_symbol_table(MEMSPACE mem);
void mon_load_symbols(MEMSPACE mem, const char *filename);
void mon_save_symbols(MEMSPACE mem, const char *filename);

void mon_record_commands(char *filename);
void mon_end_recording(void);
int mon_playback_commands(const char* filename, bool interrupt_current_playback);
void monitor_change_device(MEMSPACE mem);

void mon_export(void);

void mon_stopwatch_show(const char* prefix, const char* suffix);
void mon_stopwatch_reset(void);
void mon_maincpu_trace(void);
void mon_maincpu_toggle_trace(int state);

void mon_breakpoint_set_dummy_state(MEMSPACE mem, int state);

void mon_update_all_checkpoint_state(void);

#endif
