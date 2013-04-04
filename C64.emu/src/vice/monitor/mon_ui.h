/*
 * mon_ui.h - The VICE built-in monitor, external interface for the UI.
 *
 * Written by
 *  Spiro Trikaliotis <Spiro.Trikaliotis@gmx.de>
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

#ifndef VICE_MON_UI_H
#define VICE_MON_UI_H

#include <stdio.h>

#include "monitor.h"
#include "montypes.h"

typedef struct mon_navigate_private_s {
    MEMSPACE memspace;
    WORD StartAddress;
    WORD EndAddress;
    WORD CurrentAddress;
    int have_label;
    int Lines;
} mon_navigate_private_t;

typedef struct mon_disassembly_private_s {
    mon_navigate_private_t navigate;
    MON_ADDR AddrClicked;
} mon_disassembly_private_t;

typedef struct mon_disassembly_flags_s {
    int active_line       : 1;
    int is_breakpoint     : 1;
    int breakpoint_active : 1;
} mon_disassembly_flags_t;

typedef struct mon_disassembly_s mon_disassembly_t;

struct mon_disassembly_s {
    mon_disassembly_t *next;
    mon_disassembly_flags_t flags;
    size_t length;
    char * content;
};

typedef struct mon_memory_private_s {
    mon_navigate_private_t navigate;
    MON_ADDR AddrClicked;
} mon_memory_private_t;

typedef mon_disassembly_t mon_memory_t;
typedef mon_disassembly_flags_t mon_memory_flags_t;

typedef enum MON_SCROLL_TYPE_TAG {
    MON_SCROLL_NOTHING,
    MON_SCROLL_UP,
    MON_SCROLL_DOWN,
    MON_SCROLL_PAGE_UP,
    MON_SCROLL_PAGE_DOWN
} MON_SCROLL_TYPE;

extern void mon_disassembly_init(mon_disassembly_private_t *);
extern void mon_disassembly_update(mon_disassembly_private_t *);
extern mon_disassembly_t *mon_disassembly_get_lines(mon_disassembly_private_t *, int lines_visible, int lines_full_visible);
extern mon_disassembly_t *mon_dump_get_lines(mon_memory_private_t *pmmp, int lines_visible, int lines_full_visible);
extern void mon_navigate_set_memspace(mon_navigate_private_t *, MEMSPACE);
extern MEMSPACE mon_navigate_get_memspace(mon_navigate_private_t *);
extern WORD mon_navigate_scroll(mon_navigate_private_t *,
                                MON_SCROLL_TYPE );
extern WORD mon_navigate_scroll_to(mon_navigate_private_t *, WORD);

#define MDDPC_SET_BREAKPOINT      (1 << 0)
#define MDDPC_UNSET_BREAKPOINT    (1 << 1)
#define MDDPC_ENABLE_BREAKPOINT   (1 << 2)
#define MDDPC_DISABLE_BREAKPOINT  (1 << 3)

#define MDDPC_SET_COMPUTER        (1 << 4)
#define MDDPC_SET_DRIVE8          (1 << 5)
#define MDDPC_SET_DRIVE9          (1 << 6)
#define MDDPC_SET_DRIVE10         (1 << 7)
#define MDDPC_SET_DRIVE11         (1 << 8)

extern void mon_disassembly_determine_popup_commands(mon_disassembly_private_t *, int xPos, int yPos, WORD *ulMask, WORD *ulDefault);

extern void mon_disassembly_set_breakpoint(mon_disassembly_private_t *);
extern void mon_disassembly_unset_breakpoint(mon_disassembly_private_t *);
extern void mon_disassembly_enable_breakpoint(mon_disassembly_private_t *);
extern void mon_disassembly_disable_breakpoint(mon_disassembly_private_t *);

extern void mon_navigate_goto_address(mon_navigate_private_t *, WORD addr);
extern void mon_navigate_goto_pc(mon_navigate_private_t *);
extern WORD mon_navigate_get_startaddress(mon_navigate_private_t *mnp);
extern void mon_navigate_set_startaddress(mon_navigate_private_t *mnp, WORD StartAddress);
/* MPi: TODO. This would lookup a label or a hex address and then call mon_navigate_goto_address() */
extern void mon_navigate_goto_string(mon_navigate_private_t *, char *addr);

extern void mon_disassembly_set_next_instruction(mon_disassembly_private_t *pmdp);



extern void mon_memory_init(mon_memory_private_t *);
extern void mon_memory_update(mon_memory_private_t *);
extern mon_memory_t *mon_memory_get_lines(mon_memory_private_t *, int lines_visible, int lines_full_visible);



extern void mon_ui_init(void);

#endif
