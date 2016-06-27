/*
 * mon_command.c - The VICE built-in monitor command table.
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "lib.h"
#include "mon_command.h"
#include "asm.h"
#include "montypes.h"
#include "mon_parse.h" /* FIXME ! */
#include "mon_util.h"
#include "translate.h"
#include "uimon.h"
#include "util.h"

#define GET_DESCRIPTION(c) ((c->use_description_id == USE_DESCRIPTION_ID) ? translate_text(c->description_id) : _(c->description))

typedef struct mon_cmds_s {
    const char *str;
    const char *abbrev;
    int use_param_names_id;
    int use_description_id;
    char *braces;
    int param_amount;
    int param_ids[4];
    int description_id;
    const char *param_names;
    const char *description;
} mon_cmds_t;


static const mon_cmds_t mon_cmd_array[] = {
    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_AVAILABLE_COMMANDS_ARE,
      "", NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MACHINE_STATE_COMMANDS,
      "", NULL },

    { "bank", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] [%s]", 2,
      { IDGS_MEMSPACE, IDGS_BANKNAME, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_BANK_DESCRIPTION,
      NULL, NULL },

    { "backtrace", "bt",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_BACKTRACE_DESCRIPTION,
      NULL, NULL },

    { "cpu", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_TYPE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CPU_DESCRIPTION,
      NULL, NULL },

    { "cpuhistory", "chis",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_COUNT, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CPUHISTORY_DESCRIPTION,
      NULL, NULL },

    { "dump", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DUMP_DESCRIPTION,
      NULL, NULL },

    { "export", "exp",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_EXPORT_DESCRIPTION,
      NULL, NULL },

    { "goto", "g",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_GOTO_DESCRIPTION,
      NULL, NULL },

    { "io", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_IO_DESCRIPTION,
      NULL, NULL },

    { "next", "n",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_NEXT_DESCRIPTION,
      NULL, NULL },

    { "registers", "r",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s> = <%s> [, <%s> = <%s>]*]", 4,
      { IDGS_REG_NAME, IDGS_NUMBER, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_REGISTERS_DESCRIPTION,
      NULL, NULL },

    { "reset", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_TYPE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RESET_DESCRIPTION,
      NULL, NULL },

    { "return", "ret",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RETURN_DESCRIPTION,
      NULL, NULL },

    { "screen", "sc",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SCREEN_DESCRIPTION,
      NULL, NULL },

    { "step", "z",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_COUNT, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_STEP_DESCRIPTION,
      NULL, NULL },

    { "stopwatch", "sw",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[reset]", 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_STOPWATCH_DESCRIPTION,
      NULL, NULL },

    { "undump", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_UNDUMP_DESCRIPTION,
      NULL, NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_SYMBOL_TABLE_COMMANDS,
      "", NULL },

    { "add_label", "al",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] <%s> <%s>", 3,
      { IDGS_MEMSPACE, IDGS_ADDRESS, IDGS_LABEL, IDGS_UNUSED },
      IDGS_MON_ADD_LABEL_DESCRIPTION,
      NULL, NULL },

    { "delete_label", "dl",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] <%s>", 2,
      { IDGS_MEMSPACE, IDGS_LABEL, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DELETE_LABEL_DESCRIPTION,
      NULL, NULL },

    { "load_labels", "ll",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] \"<%s>\"", 2,
      { IDGS_MEMSPACE, IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_LOAD_LABELS_DESCRIPTION,
      NULL, NULL },

    { "save_labels", "sl",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] \"<%s>\"", 2,
      { IDGS_MEMSPACE, IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SAVE_LABELS_DESCRIPTION,
      NULL, NULL },

    { "show_labels", "shl",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_MEMSPACE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SHOW_LABELS_DESCRIPTION,
      NULL, NULL },

    { "clear_labels", "cl",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_MEMSPACE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CLEAR_LABELS_DESCRIPTION,
      NULL, NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_ASSEMBLER_AND_MEMORY_COMMANDS,
      "", NULL },

    { ">", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] <%s>", 2,
      { IDGS_ADDRESS, IDGS_DATA_LIST, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_WRITE_DESCRIPTION,
      NULL, NULL },

    { "a", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> [ <%s> [: <%s>]* ]", 3,
      { IDGS_ADDRESS, IDGS_INSTRUCTION, IDGS_INSTRUCTION, IDGS_UNUSED },
      IDGS_MON_ASSEMBLE_DESCRIPTION,
      NULL, NULL },

    { "compare", "c",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s>", 2,
      { IDGS_ADDRESS_RANGE, IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_COMPARE_DESCRIPTION,
      NULL, NULL },

    { "disass", "d",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s> [<%s>]]", 2,
      { IDGS_ADDRESS, IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DISASS_DESCRIPTION,
      NULL, NULL },

    { "fill", "f",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s>", 2,
      { IDGS_ADDRESS_RANGE, IDGS_DATA_LIST, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_FILL_DESCRIPTION,
      NULL, NULL },

    { "hunt", "h",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s>", 2,
      { IDGS_ADDRESS_RANGE, IDGS_DATA_LIST, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_HUNT_DESCRIPTION,
      NULL, NULL },

    { "i", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_I_DESCRIPTION,
      NULL, NULL },

    { "ii", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_II_DESCRIPTION,
      NULL, NULL },

    { "mem", "m",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] [<%s>]", 2,
      { IDGS_DATA_TYPE, IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEM_DESCRIPTION,
      NULL, NULL },

    { "memchar", "mc",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] [<%s>]", 2,
      { IDGS_DATA_TYPE, IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEMCHAR_DESCRIPTION,
      NULL, NULL },

    { "memmapsave", "mmsave",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" <%s>", 2,
      { IDGS_FILENAME, IDGS_FORMAT, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEMMAPSAVE_DESCRIPTION,
      NULL, NULL },

    { "memmapshow", "mmsh",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] [<%s>]", 2,
      { IDGS_MASK, IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEMMAPSHOW_DESCRIPTION,
      NULL, NULL },

    { "memmapzap", "mmzap",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEMMAPZAP_DESCRIPTION,
      NULL, NULL },

    { "memsprite", "ms",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>] [<%s>]", 2,
      { IDGS_DATA_TYPE, IDGS_ADDRESS_OPT_RANGE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MEMSPRITE_DESCRIPTION,
      NULL, NULL },

    { "move", "t",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s>", 2,
      { IDGS_ADDRESS_RANGE, IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_MOVE_DESCRIPTION,
      NULL, NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_CHECKPOINT_COMMANDS,
      "", NULL },

    { "break", "bk",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[load|store|exec] [%s [%s] [if <%s>]]", 3,
      { IDGS_ADDRESS, IDGS_ADDRESS, IDGS_COND_EXPR, IDGS_UNUSED },
      IDGS_MON_BREAK_DESCRIPTION,
      NULL, NULL },

    { "command", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> \"<%s>\"", 2,
      { IDGS_CHECKNUM, IDGS_COMMAND, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_COMMAND_DESCRIPTION,
      NULL, NULL },

    { "condition", "cond",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> if <%s>", 2,
      { IDGS_CHECKNUM, IDGS_COND_EXPR, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CONDITION_DESCRIPTION,
      NULL, NULL },

    { "delete", "del",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_CHECKNUM, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DELETE_DESCRIPTION,
      NULL, NULL },

    { "disable", "dis",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_CHECKNUM, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DISABLE_DESCRIPTION,
      NULL, NULL },

    { "enable", "en",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_CHECKNUM, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_ENABLE_DESCRIPTION,
      NULL, NULL },

    { "ignore", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> [<%s>]", 2,
      { IDGS_CHECKNUM, IDGS_COUNT, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_IGNORE_DESCRIPTION,
      NULL, NULL },

    { "until", "un",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_ADDRESS, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_UNTIL_DESCRIPTION,
      NULL, NULL },

    { "watch", "w",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[load|store|exec] [%s [%s] [if <%s>]]", 3,
      { IDGS_ADDRESS, IDGS_ADDRESS, IDGS_COND_EXPR, IDGS_UNUSED },
      IDGS_MON_WATCH_DESCRIPTION,
      NULL, NULL },

    { "trace", "tr",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[load|store|exec] [%s [%s] [if <%s>]]", 3,
      { IDGS_ADDRESS, IDGS_ADDRESS, IDGS_COND_EXPR, IDGS_UNUSED },
      IDGS_MON_TRACE_DESCRIPTION,
      NULL, NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MONITOR_STATE_COMMANDS,
      "", NULL },

    { "device", "dev",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DEVICE_DESCRIPTION,
      "[c:|8:|9:|10:|11:]", NULL },

    { "exit", "x",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_EXIT_DESCRIPTION,
      NULL, NULL },

    { "quit", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
#ifdef __OS2__
      IDGS_MON_EXIT_DESCRIPTION,
#else
      IDGS_MON_QUIT_DESCRIPTION,
#endif
      NULL, NULL },

    { "radix", "rad",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RADIX_DESCRIPTION,
      "[H|D|O|B]", NULL },

    { "sidefx", "sfx",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SIDEFX_DESCRIPTION,
      "[on|off|toggle]", NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_DISK_COMMANDS,
      "", NULL },

    { "@", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_DISK_COMMAND, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_AT_DESCRIPTION,
      NULL, NULL },

    { "attach", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s>", 2,
      { IDGS_FILENAME, IDGS_DEVICE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_ATTACH_DESCRIPTION,
      NULL, NULL },

    { "autostart", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> [%s]", 2,
      { IDGS_FILENAME, IDGS_FILE_INDEX, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_AUTOSTART_DESCRIPTION,
      NULL, NULL },

    { "autoload", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> [%s]", 2,
      { IDGS_FILENAME, IDGS_FILE_INDEX, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_AUTOLOAD_DESCRIPTION,
      NULL, "autoload given disk/tape image or program" },

    { "bload", "bl",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" <%s> <%s>", 3,
      { IDGS_FILENAME, IDGS_DEVICE, IDGS_ADDRESS, IDGS_UNUSED },
      IDGS_MON_BLOAD_DESCRIPTION,
      NULL, NULL },

    { "block_read", "br",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s> [<%s>]", 3,
      { IDGS_TRACK, IDGS_SECTOR, IDGS_ADDRESS, IDGS_UNUSED },
      IDGS_MON_BLOCK_READ_DESCRIPTION,
      NULL, NULL },

    { "bsave", "bs",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" <%s> <%s1> <%s2>", 4,
      { IDGS_FILENAME, IDGS_DEVICE, IDGS_ADDRESS, IDGS_ADDRESS },
      IDGS_MON_BSAVE_DESCRIPTION,
      NULL, NULL },

    { "block_write", "bw",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s> <%s> <%s>", 3,
      { IDGS_TRACK, IDGS_SECTOR, IDGS_ADDRESS, IDGS_UNUSED },
      IDGS_MON_BLOCK_WRITE_DESCRIPTION,
      NULL, NULL },

    { "cd", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_DIRECTORY, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CD_DESCRIPTION,
      NULL, NULL },

    { "detach", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_DEVICE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DETACH_DESCRIPTION,
      NULL, NULL },

    { "dir", "ls",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_DIRECTORY, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DIR_DESCRIPTION,
      NULL, NULL },

    { "list", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_DIRECTORY, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_LIST_DESCRIPTION,
      NULL, NULL },

    { "load", "l",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" <%s> [<%s>]", 3,
      { IDGS_FILENAME, IDGS_DEVICE, IDGS_ADDRESS, IDGS_UNUSED },
      IDGS_MON_LOAD_DESCRIPTION,
      NULL, NULL },

    { "pwd", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_PWD_DESCRIPTION,
      NULL, NULL },

    { "save", "s",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" <%s> <%s1> <%s2>", 4,
      { IDGS_FILENAME, IDGS_DEVICE, IDGS_ADDRESS, IDGS_ADDRESS },
      IDGS_MON_SAVE_DESCRIPTION,
      NULL, NULL },

    { "", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_OTHER_COMMANDS,
      "", NULL },

    { "~", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_NUMBER, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_DISPLAY_NUMBER_DESCRIPTION,
      NULL, NULL },

    { "cartfreeze", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_CARTFREEZE_DESCRIPTION,
      NULL, NULL },

    { "help", "?",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "[<%s>]", 1,
      { IDGS_COMMAND, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_HELP_DESCRIPTION,
      NULL, NULL },

    { "keybuf", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_STRING, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_KEYBUF_DESCRIPTION,
      NULL, NULL },

    { "playback", "pb",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_PLAYBACK_DESCRIPTION,
      NULL, NULL },

    { "print", "p",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_EXPRESSION, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_PRINT_DESCRIPTION,
      NULL, NULL },

    { "record", "rec",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RECORD_DESCRIPTION,
      NULL, NULL },

    { "resourceget", "resget",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_RESOURCE, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RESOURCEGET_DESCRIPTION,
      NULL, NULL },

    { "resourceset", "resset",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" \"<%s>\"", 2,
      { IDGS_RESOURCE, IDGS_VALUE, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_RESOURCESET_DESCRIPTION,
      NULL, NULL },

    { "load_resources", "resload",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_LOAD_RESOURCES_DESCRIPTION,

      NULL, NULL },
    { "save_resources", "ressave",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\"", 1,
      { IDGS_FILENAME, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SAVE_RESOURCES_DESCRIPTION,
      NULL, NULL },

    { "stop", "",
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      NULL, 0,
      { IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_STOP_DESCRIPTION,
      NULL, NULL },

    { "screenshot", "scrsh",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "\"<%s>\" [<%s>]", 2,
      { IDGS_FILENAME, IDGS_FORMAT, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_SCREENSHOT_DESCRIPTION,
      NULL, NULL },

    { "tapectrl", "",
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      "<%s>", 1,
      { IDGS_COMMAND, IDGS_UNUSED, IDGS_UNUSED, IDGS_UNUSED },
      IDGS_MON_TAPECTRL_DESCRIPTION,
      NULL, NULL },

    { NULL }
};

int mon_get_nth_command(int index, const char** full_name, const char **short_name, int *takes_filename_as_arg)
{
    if (index < 0 || index >= sizeof(mon_cmd_array) / sizeof(*mon_cmd_array) - 1) {
        return 0;
    }
    *full_name = mon_cmd_array[index].str;
    *short_name = mon_cmd_array[index].abbrev;
    *takes_filename_as_arg = mon_cmd_array[index].param_ids[0] == IDGS_FILENAME;
    return 1;
}

static int mon_command_lookup_index(const char *str)
{
    int num = 0;

    if (str == NULL) {
        return -1;
    }

    do {
        if ((strcasecmp(str, mon_cmd_array[num].str) == 0) ||
            (strcasecmp(str, mon_cmd_array[num].abbrev) == 0)) {
            return num;
        }
        num++;
    } while (mon_cmd_array[num].str != NULL);

    return -1;
}

void mon_command_print_help(const char *cmd)
{
    const mon_cmds_t *c;
    int column;
    int len;
    int longest;
    int max_col;
    char *parameters;
    char *braces;
    int param_amount;

    if (cmd == NULL) {
        longest = 0;
        for (c = mon_cmd_array; c->str != NULL; c++) {
            len = (int)strlen(c->str);
            if (!util_check_null_string(c->abbrev)) {
                len += 3 + (int)strlen(c->abbrev); /* 3 => " ()" */
            }
            if (len > longest) {
                longest = len;
            }
        }
        longest += 2; /* some space */
        max_col = 80 / longest - 1;

        column = 0;
        for (c = mon_cmd_array; c->str != NULL; c++) {
            int tot = (int)strlen(c->str);

            /* "Empty" command, that's a head line  */
            if (tot == 0) {
                if (column != 0) {
                    mon_out("\n");
                    column = 0;
                }
                mon_out("\n%s\n", GET_DESCRIPTION(c));
                continue;
            }

            mon_out("%s", c->str);

            if (!util_check_null_string(c->abbrev)) {
                mon_out(" (%s)", c->abbrev);
                tot += 3 + (int)strlen(c->abbrev);
            }

            if (column >= max_col) {
                mon_out("\n");
                column = 0;
            } else {
                for (; tot < longest; tot++) {
                    mon_out(" ");
                }
                column++;
            }
            if (mon_stop_output != 0) {
                break;
            }
        }
        mon_out("\n\n");
    } else {
        int cmd_num;

        cmd_num = mon_command_lookup_index(cmd);

        if (cmd_num == -1) {
            mon_out(translate_text(IDGS_COMMAND_S_UNKNOWN), cmd);
        } else if (mon_cmd_array[cmd_num].description == NULL && mon_cmd_array[cmd_num].description_id == IDGS_UNUSED) {
            mon_out(translate_text(IDGS_NO_HELP_AVAILABLE_FOR_S), cmd);
        } else {
            const mon_cmds_t *c;

            c = &mon_cmd_array[cmd_num];

            if (c->use_param_names_id == USE_PARAM_ID) {
                braces = c->braces;
                param_amount = c->param_amount;
                switch (param_amount) {
                    default:
                    case 1:
                        parameters = lib_msprintf(braces, translate_text(c->param_ids[0]));
                        break;
                    case 2:
                        parameters = lib_msprintf(braces, translate_text(c->param_ids[0]),
                                                  translate_text(c->param_ids[1]));
                        break;
                    case 3:
                        parameters = lib_msprintf(braces, translate_text(c->param_ids[0]),
                                                  translate_text(c->param_ids[1]),
                                                  translate_text(c->param_ids[2]));
                        break;
                    case 4:
                        parameters = lib_msprintf(braces, translate_text(c->param_ids[0]),
                                                  translate_text(c->param_ids[1]),
                                                  translate_text(c->param_ids[2]),
                                                  translate_text(c->param_ids[3]));
                        break;
                }
            } else {
                if (c->param_names == NULL) {
                    parameters = NULL;
                } else {
                    parameters = lib_stralloc(_(c->param_names));
                }
            }

            mon_out(translate_text(IDGS_SYNTAX_S_S),
                    c->str,
                    parameters != NULL ? parameters : "");
            lib_free(parameters);
            if (!util_check_null_string(c->abbrev)) {
                mon_out(translate_text(IDGS_ABBREVIATION_S), c->abbrev);
            }
            mon_out("\n%s\n\n", GET_DESCRIPTION(c));
        }
    }
}
