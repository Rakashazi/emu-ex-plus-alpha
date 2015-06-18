%{
/* -*- C -*-
 *
 * mon_parse.y - Parser for the VICE built-in monitor.
 *
 * Written by
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Andreas Boose <viceteam@t-online.de>
 *  Thomas Giesel <skoe@directbox.com>
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

#include "vice.h"

#if !defined(__minix_vmd) && !defined(MACOS_COMPILE)
#ifdef __GNUC__
#undef alloca
#ifndef ANDROID_COMPILE
#define        alloca(n)       __builtin_alloca (n)
#endif
#else
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#else  /* Not HAVE_ALLOCA_H.  */
#if !defined(_AIX) && !defined(WINCE)
#ifndef _MSC_VER
extern char *alloca();
#else
#define alloca(n)   _alloca(n)
#endif  /* MSVC */
#endif /* Not AIX and not WINCE.  */
#endif /* HAVE_ALLOCA_H.  */
#endif /* GCC.  */
#endif /* MINIXVMD */

/* SunOS 4.x specific stuff */
#if defined(sun) || defined(__sun)
#  if !defined(__SVR4) && !defined(__svr4__)
#    ifdef __sparc__
#      define YYFREE
#    endif
#  endif
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "asm.h"
#include "console.h"
#include "lib.h"
#include "machine.h"
#include "mon_breakpoint.h"
#include "mon_command.h"
#include "mon_disassemble.h"
#include "mon_drive.h"
#include "mon_file.h"
#include "mon_memory.h"
#include "mon_register.h"
#include "mon_util.h"
#include "montypes.h"
#include "resources.h"
#include "types.h"
#include "uimon.h"


#define join_ints(x,y) (LO16_TO_HI16(x)|y)
#define separate_int1(x) (HI16_TO_LO16(x))
#define separate_int2(x) (LO16(x))

static int yyerror(char *s);
static int temp;
static int resolve_datatype(unsigned guess_type, const char *num);
static int resolve_range(enum t_memspace memspace, MON_ADDR range[2],
                         const char *num);

#ifdef __IBMC__
static void __yy_memcpy (char *to, char *from, int count);
#endif

/* Defined in the lexer */
extern int new_cmd, opt_asm;
extern void free_buffer(void);
extern void make_buffer(char *str);
extern int yylex(void);
extern int cur_len, last_len;

#define ERR_ILLEGAL_INPUT 1     /* Generic error as returned by yacc.  */
#define ERR_RANGE_BAD_START 2
#define ERR_RANGE_BAD_END 3
#define ERR_BAD_CMD 4
#define ERR_EXPECT_CHECKNUM 5
#define ERR_EXPECT_END_CMD 6
#define ERR_MISSING_CLOSE_PAREN 7
#define ERR_INCOMPLETE_COMPARE_OP 8
#define ERR_EXPECT_FILENAME 9
#define ERR_ADDR_TOO_BIG 10
#define ERR_IMM_TOO_BIG 11
#define ERR_EXPECT_STRING 12
#define ERR_UNDEFINED_LABEL 13
#define ERR_EXPECT_DEVICE_NUM 14
#define ERR_EXPECT_ADDRESS 15
#define ERR_INVALID_REGISTER 16

#define BAD_ADDR (new_addr(e_invalid_space, 0))
#define CHECK_ADDR(x) ((x) == addr_mask(x))

#define YYDEBUG 1

%}

%union {
    MON_ADDR a;
    MON_ADDR range[2];
    int i;
    REG_ID reg;
    CONDITIONAL cond_op;
    cond_node_t *cond_node;
    RADIXTYPE rt;
    ACTION action;
    char *str;
    asm_mode_addr_info_t mode;
}

%token<i> H_NUMBER D_NUMBER O_NUMBER B_NUMBER CONVERT_OP B_DATA
%token<str> H_RANGE_GUESS D_NUMBER_GUESS O_NUMBER_GUESS B_NUMBER_GUESS
%token<i> BAD_CMD MEM_OP IF MEM_COMP MEM_DISK8 MEM_DISK9 MEM_DISK10 MEM_DISK11 EQUALS
%token TRAIL CMD_SEP LABEL_ASGN_COMMENT
%token CMD_SIDEFX CMD_RETURN CMD_BLOCK_READ CMD_BLOCK_WRITE CMD_UP CMD_DOWN
%token CMD_LOAD CMD_SAVE CMD_VERIFY CMD_IGNORE CMD_HUNT CMD_FILL CMD_MOVE
%token CMD_GOTO CMD_REGISTERS CMD_READSPACE CMD_WRITESPACE CMD_RADIX
%token CMD_MEM_DISPLAY CMD_BREAK CMD_TRACE CMD_IO CMD_BRMON CMD_COMPARE
%token CMD_DUMP CMD_UNDUMP CMD_EXIT CMD_DELETE CMD_CONDITION CMD_COMMAND
%token CMD_ASSEMBLE CMD_DISASSEMBLE CMD_NEXT CMD_STEP CMD_PRINT CMD_DEVICE
%token CMD_HELP CMD_WATCH CMD_DISK CMD_QUIT CMD_CHDIR CMD_BANK
%token CMD_LOAD_LABELS CMD_SAVE_LABELS CMD_ADD_LABEL CMD_DEL_LABEL CMD_SHOW_LABELS
%token CMD_RECORD CMD_MON_STOP CMD_PLAYBACK CMD_CHAR_DISPLAY CMD_SPRITE_DISPLAY
%token CMD_TEXT_DISPLAY CMD_SCREENCODE_DISPLAY CMD_ENTER_DATA CMD_ENTER_BIN_DATA CMD_KEYBUF
%token CMD_BLOAD CMD_BSAVE CMD_SCREEN CMD_UNTIL CMD_CPU CMD_YYDEBUG
%token CMD_BACKTRACE CMD_SCREENSHOT CMD_PWD CMD_DIR
%token CMD_RESOURCE_GET CMD_RESOURCE_SET CMD_LOAD_RESOURCES CMD_SAVE_RESOURCES
%token CMD_ATTACH CMD_DETACH CMD_MON_RESET CMD_TAPECTRL CMD_CARTFREEZE
%token CMD_CPUHISTORY CMD_MEMMAPZAP CMD_MEMMAPSHOW CMD_MEMMAPSAVE
%token CMD_COMMENT CMD_LIST CMD_STOPWATCH RESET
%token CMD_EXPORT CMD_AUTOSTART CMD_AUTOLOAD
%token<str> CMD_LABEL_ASGN
%token<i> L_PAREN R_PAREN ARG_IMMEDIATE REG_A REG_X REG_Y COMMA INST_SEP
%token<i> L_BRACKET R_BRACKET LESS_THAN REG_U REG_S REG_PC REG_PCR
%token<i> REG_B REG_C REG_D REG_E REG_H REG_L
%token<i> REG_AF REG_BC REG_DE REG_HL REG_IX REG_IY REG_SP
%token<i> REG_IXH REG_IXL REG_IYH REG_IYL
%token<i> PLUS MINUS
%token<str> STRING FILENAME R_O_L OPCODE LABEL BANKNAME CPUTYPE
%token<reg> MON_REGISTER
%left<cond_op> COMPARE_OP
%token<rt> RADIX_TYPE INPUT_SPEC
%token<action> CMD_CHECKPT_ON CMD_CHECKPT_OFF TOGGLE
%type<range> address_range address_opt_range
%type<a>  address opt_address
%type<cond_node> opt_if_cond_expr cond_expr compare_operand
%type<i> number expression d_number guess_default device_num
%type<i> memspace memloc memaddr checkpt_num mem_op opt_mem_op
%type<i> top_level value
%type<i> assembly_instruction register
%type<str> rest_of_line opt_rest_of_line data_list data_element filename
%token<i> MASK
%type<str> hunt_list hunt_element
%type<mode> asm_operand_mode
%type<i> index_reg index_ureg

%left '+' '-'
%left '*' '/'

%%

top_level: command_list { $$ = 0; }
         | assembly_instruction TRAIL { $$ = 0; }
         | TRAIL { new_cmd = 1; asm_mode = 0;  $$ = 0; }
         ;

command_list: command
            | command_list command
            ;

end_cmd: CMD_SEP
       | TRAIL
       | error { return ERR_EXPECT_END_CMD; }
       ;

command: machine_state_rules
       | symbol_table_rules
       | memory_rules
       | asm_rules
       | checkpoint_rules
       | checkpoint_control_rules
       | monitor_state_rules
       | monitor_misc_rules
       | disk_rules
       | cmd_file_rules
       | data_entry_rules
       | monitor_debug_rules
       | BAD_CMD { return ERR_BAD_CMD; }
       ;

machine_state_rules: CMD_BANK end_cmd
                     { mon_bank(e_default_space, NULL); }
                   | CMD_BANK memspace end_cmd
                     { mon_bank($2, NULL); }
                   | CMD_BANK BANKNAME end_cmd
                     { mon_bank(e_default_space, $2); }
                   | CMD_BANK memspace opt_sep BANKNAME end_cmd
                     { mon_bank($2, $4); }
                   | CMD_GOTO address end_cmd
                     { mon_jump($2); }
                   | CMD_GOTO end_cmd
                     { mon_go(); }
                   | CMD_IO end_cmd
                     { mon_display_io_regs(0); }
                   | CMD_IO address end_cmd
                     { mon_display_io_regs($2); }
                   | CMD_CPU end_cmd
                     { monitor_cpu_type_set(""); }
                   | CMD_CPU CPUTYPE end_cmd
                     { monitor_cpu_type_set($2); }
                   | CMD_CPUHISTORY end_cmd
                     { mon_cpuhistory(-1); }
                   | CMD_CPUHISTORY opt_sep expression end_cmd
                     { mon_cpuhistory($3); }
                   | CMD_RETURN end_cmd
                     { mon_instruction_return(); }
                   | CMD_DUMP filename end_cmd
                     { machine_write_snapshot($2,0,0,0); /* FIXME */ }
                   | CMD_UNDUMP filename end_cmd
                     { machine_read_snapshot($2, 0); }
                   | CMD_STEP end_cmd
                     { mon_instructions_step(-1); }
                   | CMD_STEP opt_sep expression end_cmd
                     { mon_instructions_step($3); }
                   | CMD_NEXT end_cmd
                     { mon_instructions_next(-1); }
                   | CMD_NEXT opt_sep expression end_cmd
                     { mon_instructions_next($3); }
                   | CMD_UP end_cmd
                     { mon_stack_up(-1); }
                   | CMD_UP opt_sep expression end_cmd
                     { mon_stack_up($3); }
                   | CMD_DOWN end_cmd
                     { mon_stack_down(-1); }
                   | CMD_DOWN opt_sep expression end_cmd
                     { mon_stack_down($3); }
                   | CMD_SCREEN end_cmd
                     { mon_display_screen(); }
                   | register_mod
                   ;

register_mod: CMD_REGISTERS end_cmd
              { (monitor_cpu_for_memspace[default_memspace]->mon_register_print)(default_memspace); }
            | CMD_REGISTERS memspace end_cmd
              { (monitor_cpu_for_memspace[$2]->mon_register_print)($2); }
            | CMD_REGISTERS reg_list end_cmd
            ;

symbol_table_rules: CMD_LOAD_LABELS memspace opt_sep filename end_cmd
                    {
                        /* What about the memspace? */
                        mon_playback_init($4);
                    }
                  | CMD_LOAD_LABELS filename end_cmd
                    {
                        /* What about the memspace? */
                        mon_playback_init($2);
                    }
                  | CMD_SAVE_LABELS memspace opt_sep filename end_cmd
                    { mon_save_symbols($2, $4); }
                  | CMD_SAVE_LABELS filename end_cmd
                    { mon_save_symbols(e_default_space, $2); }
                  | CMD_ADD_LABEL address opt_sep LABEL end_cmd
                    { mon_add_name_to_symbol_table($2, $4); }
                  | CMD_DEL_LABEL LABEL end_cmd
                    { mon_remove_name_from_symbol_table(e_default_space, $2); }
                  | CMD_DEL_LABEL memspace opt_sep LABEL end_cmd
                    { mon_remove_name_from_symbol_table($2, $4); }
                  | CMD_SHOW_LABELS memspace end_cmd
                    { mon_print_symbol_table($2); }
                  | CMD_SHOW_LABELS end_cmd
                    { mon_print_symbol_table(e_default_space); }
                  | CMD_LABEL_ASGN EQUALS address end_cmd
                    {
                        mon_add_name_to_symbol_table($3, mon_prepend_dot_to_name($1));
                    }
                  | CMD_LABEL_ASGN EQUALS address LABEL_ASGN_COMMENT end_cmd
                    {
                        mon_add_name_to_symbol_table($3, mon_prepend_dot_to_name($1));
                    }
                  ;

asm_rules: CMD_ASSEMBLE address
           { mon_start_assemble_mode($2, NULL); } post_assemble end_cmd
           { }
         | CMD_ASSEMBLE address end_cmd
           { mon_start_assemble_mode($2, NULL); }
         | CMD_DISASSEMBLE address_opt_range end_cmd
           { mon_disassemble_lines($2[0], $2[1]); }
         | CMD_DISASSEMBLE end_cmd
           { mon_disassemble_lines(BAD_ADDR, BAD_ADDR); }
         ;

memory_rules: CMD_MOVE address_range opt_sep address end_cmd
              { mon_memory_move($2[0], $2[1], $4); }
            | CMD_COMPARE address_range opt_sep address end_cmd
              { mon_memory_compare($2[0], $2[1], $4); }
            | CMD_FILL address_range opt_sep data_list end_cmd
              { mon_memory_fill($2[0], $2[1],(unsigned char *)$4); }
            | CMD_HUNT address_range opt_sep hunt_list end_cmd
              { mon_memory_hunt($2[0], $2[1],(unsigned char *)$4); }
            | CMD_MEM_DISPLAY RADIX_TYPE opt_sep address_opt_range end_cmd
              { mon_memory_display($2, $4[0], $4[1], DF_PETSCII); }
            | CMD_MEM_DISPLAY address_opt_range end_cmd
              { mon_memory_display(default_radix, $2[0], $2[1], DF_PETSCII); }
            | CMD_MEM_DISPLAY end_cmd
              { mon_memory_display(default_radix, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
            | CMD_CHAR_DISPLAY address_opt_range end_cmd
              { mon_memory_display_data($2[0], $2[1], 8, 8); }
            | CMD_CHAR_DISPLAY end_cmd
              { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 8, 8); }
            | CMD_SPRITE_DISPLAY address_opt_range end_cmd
              { mon_memory_display_data($2[0], $2[1], 24, 21); }
            | CMD_SPRITE_DISPLAY end_cmd
              { mon_memory_display_data(BAD_ADDR, BAD_ADDR, 24, 21); }
            | CMD_TEXT_DISPLAY address_opt_range end_cmd
              { mon_memory_display(0, $2[0], $2[1], DF_PETSCII); }
            | CMD_TEXT_DISPLAY end_cmd
              { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_PETSCII); }
            | CMD_SCREENCODE_DISPLAY address_opt_range end_cmd
              { mon_memory_display(0, $2[0], $2[1], DF_SCREEN_CODE); }
            | CMD_SCREENCODE_DISPLAY end_cmd
              { mon_memory_display(0, BAD_ADDR, BAD_ADDR, DF_SCREEN_CODE); }
            | CMD_MEMMAPZAP end_cmd
              { mon_memmap_zap(); }
            | CMD_MEMMAPSHOW end_cmd
              { mon_memmap_show(-1,BAD_ADDR,BAD_ADDR); }
            | CMD_MEMMAPSHOW opt_sep expression end_cmd
              { mon_memmap_show($3,BAD_ADDR,BAD_ADDR); }
            | CMD_MEMMAPSHOW opt_sep expression address_opt_range end_cmd
              { mon_memmap_show($3,$4[0],$4[1]); }
            | CMD_MEMMAPSAVE filename opt_sep expression end_cmd
              { mon_memmap_save($2,$4); }
            ;

checkpoint_rules: CMD_BREAK opt_mem_op address_opt_range opt_if_cond_expr end_cmd
                  {
                      if ($2) {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], TRUE, $2, FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], TRUE, e_exec, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, $4);
                  }
                | CMD_BREAK end_cmd
                  { mon_breakpoint_print_checkpoints(); }

                | CMD_UNTIL address_opt_range end_cmd
                  {
                      mon_breakpoint_add_checkpoint($2[0], $2[1], TRUE, e_exec, TRUE);
                  }
                | CMD_UNTIL end_cmd
                  { mon_breakpoint_print_checkpoints(); }

                | CMD_WATCH opt_mem_op address_opt_range opt_if_cond_expr end_cmd
                  {
                      if ($2) {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], TRUE, $2, FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], TRUE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, $4);
                  }
                | CMD_WATCH end_cmd
                  { mon_breakpoint_print_checkpoints(); }

                | CMD_TRACE opt_mem_op address_opt_range opt_if_cond_expr end_cmd
                  {
                      if ($2) {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], FALSE, $2, FALSE);
                      } else {
                          temp = mon_breakpoint_add_checkpoint($3[0], $3[1], FALSE, e_load | e_store, FALSE);
                      }
                      mon_breakpoint_set_checkpoint_condition(temp, $4);
                  }
                | CMD_TRACE end_cmd
                  { mon_breakpoint_print_checkpoints(); }
                ;


checkpoint_control_rules: CMD_CHECKPT_ON checkpt_num end_cmd
                          { mon_breakpoint_switch_checkpoint(e_ON, $2); }
                        | CMD_CHECKPT_ON end_cmd
                          { mon_breakpoint_switch_checkpoint(e_ON, -1); }
                        | CMD_CHECKPT_OFF checkpt_num end_cmd
                          { mon_breakpoint_switch_checkpoint(e_OFF, $2); }
                        | CMD_CHECKPT_OFF end_cmd
                          { mon_breakpoint_switch_checkpoint(e_OFF, -1); }
                        | CMD_IGNORE checkpt_num end_cmd
                          { mon_breakpoint_set_ignore_count($2, -1); }
                        | CMD_IGNORE checkpt_num opt_sep expression end_cmd
                          { mon_breakpoint_set_ignore_count($2, $4); }
                        | CMD_DELETE checkpt_num end_cmd
                          { mon_breakpoint_delete_checkpoint($2); }
                        | CMD_DELETE end_cmd
                          { mon_breakpoint_delete_checkpoint(-1); }
                        | CMD_CONDITION checkpt_num IF cond_expr end_cmd
                          { mon_breakpoint_set_checkpoint_condition($2, $4); }
                        | CMD_COMMAND checkpt_num opt_sep STRING end_cmd
                          { mon_breakpoint_set_checkpoint_command($2, $4); }
                        | CMD_COMMAND checkpt_num error end_cmd
                          { return ERR_EXPECT_STRING; }
                        ;

monitor_state_rules: CMD_SIDEFX TOGGLE end_cmd
                     { sidefx = (($2 == e_TOGGLE) ? (sidefx ^ 1) : $2); }
                   | CMD_SIDEFX end_cmd
                     {
                         mon_out("I/O side effects are %s\n",
                                   sidefx ? "enabled" : "disabled");
                     }
                   | CMD_RADIX RADIX_TYPE end_cmd
                     { default_radix = $2; }
                   | CMD_RADIX end_cmd
                     {
                         const char *p;

                         if (default_radix == e_hexadecimal)
                             p = "Hexadecimal";
                         else if (default_radix == e_decimal)
                             p = "Decimal";
                         else if (default_radix == e_octal)
                             p = "Octal";
                         else if (default_radix == e_binary)
                             p = "Binary";
                         else
                             p = "Unknown";

                         mon_out("Default radix is %s\n", p);
                     }

                   | CMD_DEVICE memspace end_cmd
                     { monitor_change_device($2); }
                   | CMD_EXPORT end_cmd
                     { mon_export(); }
                   | CMD_QUIT end_cmd
                     { mon_quit(); YYACCEPT; }
                   | CMD_EXIT end_cmd
                     { mon_exit(); YYACCEPT; }
                   ;

monitor_misc_rules: CMD_DISK rest_of_line end_cmd
                    { mon_drive_execute_disk_cmd($2); }
                  | CMD_PRINT expression end_cmd
                    { mon_out("\t%d\n",$2); }
                  | CMD_HELP end_cmd
                    { mon_command_print_help(NULL); }
                  | CMD_HELP rest_of_line end_cmd
                    { mon_command_print_help($2); }
                  | CONVERT_OP expression end_cmd
                    { mon_print_convert($2); }
                  | CMD_CHDIR rest_of_line end_cmd
                    { mon_change_dir($2); }
                  | CMD_KEYBUF rest_of_line end_cmd /* STRING */
                    { mon_keyboard_feed($2); }
                  | CMD_BACKTRACE end_cmd
                    { mon_backtrace(); }
                  | CMD_DIR opt_rest_of_line end_cmd
                     { mon_show_dir($2); }
                  | CMD_PWD end_cmd
                     { mon_show_pwd(); }
                  | CMD_SCREENSHOT filename end_cmd
                    { mon_screenshot_save($2,-1); }
                  | CMD_SCREENSHOT filename opt_sep expression end_cmd
                    { mon_screenshot_save($2,$4); }
                  | CMD_RESOURCE_GET STRING end_cmd
                    { mon_resource_get($2); }
                  | CMD_RESOURCE_SET STRING STRING end_cmd
                    { mon_resource_set($2,$3); }
                  | CMD_LOAD_RESOURCES filename end_cmd
                    { resources_load($2); }
                  | CMD_SAVE_RESOURCES filename end_cmd
                    { resources_save($2); }
                  | CMD_MON_RESET end_cmd
                    { mon_reset_machine(-1); }
                  | CMD_MON_RESET opt_sep expression end_cmd
                    { mon_reset_machine($3); }
                  | CMD_TAPECTRL opt_sep expression end_cmd
                    { mon_tape_ctrl($3); }
                  | CMD_CARTFREEZE end_cmd
                    { mon_cart_freeze(); }
                  | CMD_COMMENT opt_rest_of_line end_cmd
                     { }
                  | CMD_STOPWATCH RESET end_cmd
                     { mon_stopwatch_reset(); }
                  | CMD_STOPWATCH end_cmd
                     { mon_stopwatch_show("Stopwatch: ", "\n"); }
                  ;

disk_rules: CMD_LOAD filename device_num opt_address end_cmd
            { mon_file_load($2, $3, $4, FALSE); }
          | CMD_BLOAD filename device_num opt_address end_cmd
            { mon_file_load($2, $3, $4, TRUE); }
          | CMD_SAVE filename device_num address_range end_cmd
            { mon_file_save($2, $3, $4[0], $4[1], FALSE); }
          | CMD_SAVE filename error
            { return ERR_EXPECT_DEVICE_NUM; }
          | CMD_SAVE filename device_num error
            { return ERR_EXPECT_ADDRESS; }
          | CMD_BSAVE filename device_num address_range end_cmd
            { mon_file_save($2, $3, $4[0], $4[1], TRUE); }
          | CMD_BSAVE filename device_num error
            { return ERR_EXPECT_ADDRESS; }
          | CMD_VERIFY filename device_num address end_cmd
            { mon_file_verify($2,$3,$4); }
          | CMD_VERIFY filename device_num error
            { return ERR_EXPECT_ADDRESS; }
          | CMD_BLOCK_READ expression expression opt_address end_cmd
            { mon_drive_block_cmd(0,$2,$3,$4); }
          | CMD_BLOCK_WRITE expression expression address end_cmd
            { mon_drive_block_cmd(1,$2,$3,$4); }
          | CMD_LIST end_cmd
            { mon_drive_list(-1); }
          | CMD_LIST device_num end_cmd
            { mon_drive_list($2); }
          | CMD_ATTACH filename expression end_cmd
            { mon_attach($2,$3); }
          | CMD_DETACH expression end_cmd
            { mon_detach($2); }
          | CMD_AUTOSTART filename end_cmd
            { mon_autostart($2,0,1); }
          | CMD_AUTOSTART filename opt_sep number end_cmd
            { mon_autostart($2,$4,1); }
          | CMD_AUTOLOAD filename end_cmd
            { mon_autostart($2,0,0); }
          | CMD_AUTOLOAD filename opt_sep number end_cmd
            { mon_autostart($2,$4,0); }
          ;

cmd_file_rules: CMD_RECORD filename end_cmd
                { mon_record_commands($2); }
              | CMD_MON_STOP end_cmd
                { mon_end_recording(); }
              | CMD_PLAYBACK filename end_cmd
                { mon_playback_init($2); }
              ;

data_entry_rules: CMD_ENTER_DATA address data_list end_cmd
                  { mon_memory_fill($2, BAD_ADDR, (unsigned char *)$3); }
                | CMD_ENTER_BIN_DATA end_cmd
                  { printf("Not yet.\n"); }
                ;

monitor_debug_rules: CMD_YYDEBUG end_cmd
                     { yydebug = 1; }
                   ;

rest_of_line: R_O_L { $$ = $1; }
            ;

opt_rest_of_line: R_O_L { $$ = $1; }
                  | { $$ = NULL; }
                  ;

filename: FILENAME
        | error { return ERR_EXPECT_FILENAME; }
        ;

device_num: expression
      | error { return ERR_EXPECT_DEVICE_NUM; }
      ;

mem_op: mem_op MEM_OP { $$ = $1 | $2; }
      | MEM_OP { $$ = $1; }
      ;

opt_mem_op: mem_op { $$ = $1; }
          | { $$ = 0; }
          ;

register: MON_REGISTER          {
                                    if (!mon_register_valid(default_memspace, $1)) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    $$ = new_reg(default_memspace, $1);
                                }
        | memspace MON_REGISTER {
                                    if (!mon_register_valid($1, $2)) {
                                        return ERR_INVALID_REGISTER;
                                    }
                                    $$ = new_reg($1, $2);
                                }
        ;

reg_list: reg_list COMMA reg_asgn
        | reg_asgn
        ;

reg_asgn: register EQUALS number
          { (monitor_cpu_for_memspace[reg_memspace($1)]->mon_register_set_val)(reg_memspace($1), reg_regid($1), (WORD) $3); }
        ;

checkpt_num: d_number { $$ = $1; }
           | error { return ERR_EXPECT_CHECKNUM; }
           ;

address_opt_range: address_range
                 | address { $$[0] = $1; $$[1] = BAD_ADDR; }
                 ;

address_range: address opt_sep address { $$[0] = $1; $$[1] = $3; }
             | H_RANGE_GUESS
               { if (resolve_range(e_default_space, $$, $1)) return ERR_ADDR_TOO_BIG; }
             | memspace opt_sep H_RANGE_GUESS
               { if (resolve_range($1, $$, $3)) return ERR_ADDR_TOO_BIG; }
             ;

opt_address: opt_sep address { $$ = $2; }
           | { $$ = BAD_ADDR; }
           ;

address: memloc
         {
             $$ = new_addr(e_default_space,$1);
             if (opt_asm) new_cmd = asm_mode = 1;
         }
       | memspace opt_sep memloc
         {
             $$ = new_addr($1, $3);
             if (opt_asm) new_cmd = asm_mode = 1;
         }
       | LABEL
         {
             temp = mon_symbol_table_lookup_addr(e_default_space, $1);
             if (temp >= 0)
                 $$ = new_addr(e_default_space, temp);
             else
                 return ERR_UNDEFINED_LABEL;
         }
       ;

opt_sep: COMMA
       |
       ;

memspace: MEM_COMP { $$ = e_comp_space; }
        | MEM_DISK8 { $$ = e_disk8_space; }
        | MEM_DISK9 { $$ = e_disk9_space; }
        | MEM_DISK10 { $$ = e_disk10_space; }
        | MEM_DISK11 { $$ = e_disk11_space; }
        ;

memloc: memaddr { $$ = $1; if (!CHECK_ADDR($1)) return ERR_ADDR_TOO_BIG; };

memaddr: number { $$ = $1; };

expression: expression '+' expression { $$ = $1 + $3; }
          | expression '-' expression { $$ = $1 - $3; }
          | expression '*' expression { $$ = $1 * $3; }
          | expression '/' expression { $$ = ($3) ? ($1 / $3) : 1; }
          | '(' expression ')' { $$ = $2; }
          | '(' expression error { return ERR_MISSING_CLOSE_PAREN; }
          | value { $$ = $1; }
          ;

opt_if_cond_expr: IF cond_expr { $$ = $2; }
                | { $$ = 0; }

cond_expr: cond_expr COMPARE_OP cond_expr
           {
               $$ = new_cond; $$->is_parenthized = FALSE;
               $$->child1 = $1; $$->child2 = $3; $$->operation = $2;
           }
         | cond_expr COMPARE_OP error
           { return ERR_INCOMPLETE_COMPARE_OP; }
         | L_PAREN cond_expr R_PAREN
           { $$ = $2; $$->is_parenthized = TRUE; }
         | L_PAREN cond_expr error
           { return ERR_MISSING_CLOSE_PAREN; }
         | compare_operand
           { $$ = $1; }
         ;

compare_operand: register { $$ = new_cond;
                            $$->operation = e_INV;
                            $$->is_parenthized = FALSE;
                            $$->reg_num = $1; $$->is_reg = TRUE;
                            $$->child1 = NULL; $$->child2 = NULL;
                          }
               | number   { $$ = new_cond;
                            $$->operation = e_INV;
                            $$->is_parenthized = FALSE;
                            $$->value = $1; $$->is_reg = FALSE;
                            $$->child1 = NULL; $$->child2 = NULL;
                          }
               ;

data_list: data_list opt_sep data_element
         | data_element
         ;

data_element: number { mon_add_number_to_buffer($1); }
            | STRING { mon_add_string_to_buffer($1); }
            ;

hunt_list: hunt_list hunt_element
         | hunt_element
         ;

hunt_element: number { mon_add_number_to_buffer($1); }
            | MASK   { mon_add_number_masked_to_buffer($1, 0x00); }
            | STRING { mon_add_string_to_buffer($1); }
            ;

value: number { $$ = $1; }
     | register { $$ = (monitor_cpu_for_memspace[reg_memspace($1)]->mon_register_get_val)(reg_memspace($1), reg_regid($1)); }
     ;

d_number: D_NUMBER { $$ = $1; }
        | B_NUMBER_GUESS { $$ = strtol($1, NULL, 10); }
        | O_NUMBER_GUESS { $$ = strtol($1, NULL, 10); }
        | D_NUMBER_GUESS { $$ = strtol($1, NULL, 10); }
        ;

guess_default: B_NUMBER_GUESS { $$ = resolve_datatype(B_NUMBER,$1); }
             | O_NUMBER_GUESS { $$ = resolve_datatype(O_NUMBER,$1); }
             | D_NUMBER_GUESS { $$ = resolve_datatype(D_NUMBER,$1); }
             ;

number: H_NUMBER { $$ = $1; }
      | D_NUMBER { $$ = $1; }
      | O_NUMBER { $$ = $1; }
      | B_NUMBER { $$ = $1; }
      | guess_default { $$ = $1; }
      ;

assembly_instr_list: assembly_instr_list INST_SEP assembly_instruction
                   | assembly_instruction INST_SEP assembly_instruction
                   | assembly_instruction INST_SEP
                   ;

assembly_instruction: OPCODE asm_operand_mode { $$ = 0;
                                                if ($1) {
                                                    (monitor_cpu_for_memspace[default_memspace]->mon_assemble_instr)($1, $2);
                                                } else {
                                                    new_cmd = 1;
                                                    asm_mode = 0;
                                                }
                                                opt_asm = 0;
                                              };

post_assemble: assembly_instruction
             | assembly_instr_list { asm_mode = 0; }
             ;

asm_operand_mode: ARG_IMMEDIATE number { if ($2 > 0xff) {
                          $$.addr_mode = ASM_ADDR_MODE_IMMEDIATE_16;
                          $$.param = $2;
                        } else {
                          $$.addr_mode = ASM_ADDR_MODE_IMMEDIATE;
                          $$.param = $2;
                        } }
  | number { if ($1 >= 0x10000) {
               $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG;
               $$.param = $1;
             } else if ($1 < 0x100) {
               $$.addr_mode = ASM_ADDR_MODE_ZERO_PAGE;
               $$.param = $1;
             } else {
               $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE;
               $$.param = $1;
             }
           }
  | number COMMA REG_X  { if ($1 >= 0x10000) {
                            $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_LONG_X;
                            $$.param = $1;
                          } else if ($1 < 0x100) { 
                            $$.addr_mode = ASM_ADDR_MODE_ZERO_PAGE_X;
                            $$.param = $1;
                          } else {
                            $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_X;
                            $$.param = $1;
                          }
                        }
  | number COMMA REG_Y  { if ($1 < 0x100) {
                            $$.addr_mode = ASM_ADDR_MODE_ZERO_PAGE_Y;
                            $$.param = $1;
                          } else {
                            $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_Y;
                            $$.param = $1;
                          }
                        }
  | number COMMA REG_S  { if ($1 < 0x100) {
                            $$.addr_mode = ASM_ADDR_MODE_STACK_RELATIVE;
                            $$.param = $1;
                          } else { /* 6809 */
                            $$.addr_mode = ASM_ADDR_MODE_INDEXED;
                            if ($1 >= -16 && $1 < 16) {
                                $$.addr_submode = $3 | ($1 & 0x1F);
                            } else if ($1 >= -128 && $1 < 128) {
                                $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFF8;
                                $$.param = $1;
                            } else if ($1 >= -32768 && $1 < 32768) {
                                $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFF16;
                                $$.param = $1;
                            } else {
                                $$.addr_mode = ASM_ADDR_MODE_ILLEGAL;
                                mon_out("offset too large even for 16 bits (signed)\n");
                            }
                          }
                        }
  | number COMMA number { if ($1 < 0x100) {
                            $$.addr_mode = ASM_ADDR_MODE_DOUBLE;
                            $$.param = $3;
                            $$.addr_submode = $1;
                          }
                        }
  | L_PAREN number R_PAREN { if ($2 < 0x100) {
                               $$.addr_mode = ASM_ADDR_MODE_INDIRECT;
                               $$.param = $2;
                             } else {
                               $$.addr_mode = ASM_ADDR_MODE_ABS_INDIRECT;
                               $$.param = $2;
                             }
                           }
  | L_PAREN number COMMA REG_X R_PAREN { if ($2 < 0x100) {
                                           $$.addr_mode = ASM_ADDR_MODE_INDIRECT_X;
                                           $$.param = $2;
                                         } else {
                                           $$.addr_mode = ASM_ADDR_MODE_ABS_INDIRECT_X;
                                           $$.param = $2;
                                         }
                                       }
  | L_PAREN number COMMA REG_S R_PAREN COMMA REG_Y
    { $$.addr_mode = ASM_ADDR_MODE_STACK_RELATIVE_Y; $$.param = $2; }
  | L_PAREN number R_PAREN COMMA REG_Y
    { $$.addr_mode = ASM_ADDR_MODE_INDIRECT_Y; $$.param = $2; }
  | L_PAREN REG_BC R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_BC; }
  | L_PAREN REG_DE R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_DE; }
  | L_PAREN REG_HL R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_HL; }
  | L_PAREN REG_IX R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_IX; }
  | L_PAREN REG_IY R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_IY; }
  | L_PAREN REG_SP R_PAREN { $$.addr_mode = ASM_ADDR_MODE_REG_IND_SP; }
  | L_PAREN number R_PAREN COMMA REG_A
    { $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_A; $$.param = $2; }
  | L_PAREN number R_PAREN COMMA REG_HL
    { $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_HL; $$.param = $2; }
  | L_PAREN number R_PAREN COMMA REG_IX
    { $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_IX; $$.param = $2; }
  | L_PAREN number R_PAREN COMMA REG_IY
    { $$.addr_mode = ASM_ADDR_MODE_ABSOLUTE_IY; $$.param = $2; }
  | { $$.addr_mode = ASM_ADDR_MODE_IMPLIED; }
  | REG_A { $$.addr_mode = ASM_ADDR_MODE_ACCUMULATOR; }
  | REG_B { $$.addr_mode = ASM_ADDR_MODE_REG_B; }
  | REG_C { $$.addr_mode = ASM_ADDR_MODE_REG_C; }
  | REG_D { $$.addr_mode = ASM_ADDR_MODE_REG_D; }
  | REG_E { $$.addr_mode = ASM_ADDR_MODE_REG_E; }
  | REG_H { $$.addr_mode = ASM_ADDR_MODE_REG_H; }
  | REG_IXH { $$.addr_mode = ASM_ADDR_MODE_REG_IXH; }
  | REG_IYH { $$.addr_mode = ASM_ADDR_MODE_REG_IYH; }
  | REG_L { $$.addr_mode = ASM_ADDR_MODE_REG_L; }
  | REG_IXL { $$.addr_mode = ASM_ADDR_MODE_REG_IXL; }
  | REG_IYL { $$.addr_mode = ASM_ADDR_MODE_REG_IYL; }
  | REG_AF { $$.addr_mode = ASM_ADDR_MODE_REG_AF; }
  | REG_BC { $$.addr_mode = ASM_ADDR_MODE_REG_BC; }
  | REG_DE { $$.addr_mode = ASM_ADDR_MODE_REG_DE; }
  | REG_HL { $$.addr_mode = ASM_ADDR_MODE_REG_HL; }
  | REG_IX { $$.addr_mode = ASM_ADDR_MODE_REG_IX; }
  | REG_IY { $$.addr_mode = ASM_ADDR_MODE_REG_IY; }
  | REG_SP { $$.addr_mode = ASM_ADDR_MODE_REG_SP; }
    /* 6809 modes */
  | LESS_THAN number { $$.addr_mode = ASM_ADDR_MODE_DIRECT; $$.param = $2; }
  | number COMMA index_ureg {    /* Clash with addr,x addr,y addr,s modes! */
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        if ($1 >= -16 && $1 < 16) {
            $$.addr_submode = (3 << 5) | ($1 & 0x1F);
        } else if ($1 >= -128 && $1 < 128) {
            $$.addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF8;
            $$.param = $1;
        } else if ($1 >= -32768 && $1 < 32768) {
            $$.addr_submode = 0x80 | (3 << 5) | ASM_ADDR_MODE_INDEXED_OFF16;
            $$.param = $1;
        } else {
            $$.addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
  | COMMA index_reg PLUS {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_INC1;
        }
  | COMMA index_reg PLUS PLUS {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_INC2;
        }
  | COMMA MINUS index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_DEC1;
        }
  | COMMA MINUS MINUS index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $4 | ASM_ADDR_MODE_INDEXED_DEC2;
        }
  | COMMA index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_OFF0;
        }
  | REG_B COMMA index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_OFFB;
        }
  | REG_A COMMA index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_OFFA;
        }
  | REG_D COMMA index_reg {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $2 | ASM_ADDR_MODE_INDEXED_OFFD;
        }
  | number COMMA REG_PC {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.param = $1;
        if ($1 >= -128 && $1 < 128) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8;
        } else if ($1 >= -32768 && $1 < 32768) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16;
        } else {
            $$.addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
  | L_BRACKET number COMMA index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        if ($2 >= -16 && $2 < 16) {
            $$.addr_submode = $2 & 0x1F;
        } else if ($1 >= -128 && $1 < 128) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFF8;
            $$.param = $2;
        } else if ($2 >= -32768 && $2 < 32768) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFF16;
            $$.param = $2;
        } else {
            $$.addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
  | L_BRACKET COMMA index_reg PLUS R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_INC1;
        }
  | L_BRACKET COMMA index_reg PLUS PLUS R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_INC2;
        }
  | L_BRACKET COMMA MINUS index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $4 | ASM_ADDR_MODE_INDEXED_DEC1;
        }
  | L_BRACKET COMMA MINUS MINUS index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $5 | ASM_ADDR_MODE_INDEXED_DEC2;
        }
  | L_BRACKET COMMA index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFF0;
        }
  | L_BRACKET REG_B COMMA index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFFB;
        }
  | L_BRACKET REG_A COMMA index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFFA;
        }
  | L_BRACKET REG_D COMMA index_reg R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | $3 | ASM_ADDR_MODE_INDEXED_OFFD;
        }
  | L_BRACKET number COMMA REG_PC R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.param = $2;
        if ($2 >= -128 && $2 < 128) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC8_IND;
        } else if ($2 >= -32768 && $2 < 32768) {
            $$.addr_submode = ASM_ADDR_MODE_INDEXED_OFFPC16_IND;
        } else {
            $$.addr_mode = ASM_ADDR_MODE_ILLEGAL;
            mon_out("offset too large even for 16 bits (signed)\n");
        }
    }
  | L_BRACKET number R_BRACKET {
        $$.addr_mode = ASM_ADDR_MODE_INDEXED;
        $$.addr_submode = 0x80 | ASM_ADDR_MODE_EXTENDED_INDIRECT;
        $$.param = $2;
        }
  | L_BRACKET number R_BRACKET COMMA REG_Y {
        $$.addr_mode = ASM_ADDR_MODE_INDIRECT_LONG_Y;
        $$.param = $2;
        }
  /* TODO: register lists for PSHx PULx, and register pairs for TFR and EXG */
  ;

index_reg:
    REG_X { $$ = (0 << 5); printf("reg_x\n"); }
  | REG_Y { $$ = (1 << 5); printf("reg_y\n"); }
  | index_ureg { $$ = $1; }
  | REG_S { $$ = (3 << 5); printf("reg_s\n"); }
  ;

index_ureg:
    REG_U { $$ = (2 << 5); printf("reg_u\n"); }
  ;


%%

void parse_and_execute_line(char *input)
{
   char *temp_buf;
   int i, rc;

   temp_buf = lib_malloc(strlen(input) + 3);
   strcpy(temp_buf,input);
   i = (int)strlen(input);
   temp_buf[i++] = '\n';
   temp_buf[i++] = '\0';
   temp_buf[i++] = '\0';

   make_buffer(temp_buf);
   if ( (rc =yyparse()) != 0) {
       mon_out("ERROR -- ");
       switch(rc) {
         case ERR_BAD_CMD:
           mon_out("Bad command:\n");
           break;
         case ERR_RANGE_BAD_START:
           mon_out("Bad first address in range:\n");
           break;
         case ERR_RANGE_BAD_END:
           mon_out("Bad second address in range:\n");
           break;
         case ERR_EXPECT_CHECKNUM:
           mon_out("Checkpoint number expected:\n");
           break;
         case ERR_EXPECT_END_CMD:
           mon_out("Unexpected token:\n");
           break;
         case ERR_MISSING_CLOSE_PAREN:
           mon_out("')' expected:\n");
           break;
         case ERR_INCOMPLETE_COMPARE_OP:
           mon_out("Compare operation missing an operand:\n");
           break;
         case ERR_EXPECT_FILENAME:
           mon_out("Expecting a filename:\n");
           break;
         case ERR_ADDR_TOO_BIG:
           mon_out("Address too large:\n");
           break;
         case ERR_IMM_TOO_BIG:
           mon_out("Immediate argument too large:\n");
           break;
         case ERR_EXPECT_STRING:
           mon_out("Expecting a string.\n");
           break;
         case ERR_UNDEFINED_LABEL:
           mon_out("Found an undefined label.\n");
           break;
         case ERR_EXPECT_DEVICE_NUM:
           mon_out("Expecting a device number.\n");
           break;
         case ERR_EXPECT_ADDRESS:
           mon_out("Expecting an address.\n");
           break;
         case ERR_INVALID_REGISTER:
           mon_out("Invalid register.\n");
           break;
         case ERR_ILLEGAL_INPUT:
         default:
           mon_out("Wrong syntax:\n");
       }
       mon_out("  %s\n", input);
       for (i = 0; i < last_len; i++)
           mon_out(" ");
       mon_out("  ^\n");
       asm_mode = 0;
       new_cmd = 1;
   }
   lib_free(temp_buf);
   free_buffer();
}

static int yyerror(char *s)
{
   fprintf(stderr, "ERR:%s\n", s);
   return 0;
}

static int resolve_datatype(unsigned guess_type, const char *num)
{
   /* FIXME: Handle cases when default type is non-numerical */
   if (default_radix == e_hexadecimal) {
       return strtol(num, NULL, 16);
   }

   if ((guess_type == D_NUMBER) || (default_radix == e_decimal)) {
       return strtol(num, NULL, 10);
   }

   if ((guess_type == O_NUMBER) || (default_radix == e_octal)) {
       return strtol(num, NULL, 8);
   }

   return strtol(num, NULL, 2);
}

/*
 * Resolve a character sequence containing 8 hex digits like "08001000".
 * This could be a lazy version of "0800 1000". If the default radix is not
 * hexadecimal, we handle it like a ordinary number, in the latter case there
 * is only one number in the range.
 */
static int resolve_range(enum t_memspace memspace, MON_ADDR range[2],
                         const char *num)
{
    char start[5];
    char end[5];
    long sa;

    range[1] = BAD_ADDR;

    switch (default_radix)
    {
    case e_hexadecimal:
        /* checked twice, but as the code must have exactly 8 digits: */
        if (strlen(num) == 8) {
            memcpy(start, num, 4);
            start[4] = '\0';
            memcpy(end, num + 4, 4);
            end[4] = '\0';
            sa = strtol(start, NULL, 16);
            range[1] = new_addr(memspace, strtol(end, NULL, 16));
        }
        else
            sa = strtol(num, NULL, 16);
        break;

    case e_decimal:
       sa = strtol(num, NULL, 10);
       break;

    case e_octal:
       sa = strtol(num, NULL, 8);
       break;

    default:
       sa = strtol(num, NULL, 2);
    }

    if (!CHECK_ADDR(sa))
        return ERR_ADDR_TOO_BIG;

    range[0] = new_addr(memspace, sa);
    return 0;
}


