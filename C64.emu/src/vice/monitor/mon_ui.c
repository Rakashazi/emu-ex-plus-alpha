/*
 * mon_ui.c - Monitor user interface functions.
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

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lib.h"
#include "mon_breakpoint.h"
#include "mon_memory.h"
#include "mon_register.h"
#include "mon_ui.h"
#include "mon_util.h"
#include "monitor.h"
#include "montypes.h"
#include "resources.h"


static void mon_navigate_init(mon_navigate_private_t * mnp)
{
    mnp->memspace = e_comp_space;
    mnp->StartAddress = 0;
    mnp->EndAddress = 0;
    mnp->CurrentAddress = 0;
    mnp->have_label = 0;
}

void mon_disassembly_init(mon_disassembly_private_t * pmdp)
{
    mon_navigate_init(&pmdp->navigate);

    mon_navigate_goto_pc(&pmdp->navigate);
}

static void mon_navigate_check_if_in_range(mon_navigate_private_t *mnp)
{
    if ((mnp->CurrentAddress < mnp->StartAddress)
        || (mnp->CurrentAddress > mnp->EndAddress)) {
        mnp->StartAddress = mnp->CurrentAddress;
        mnp->EndAddress = 0;
    }
}

void mon_disassembly_update(mon_disassembly_private_t *pmdp)
{
    mon_navigate_goto_pc(&pmdp->navigate);
}

void mon_navigate_set_memspace(mon_navigate_private_t *mnp, MEMSPACE memspace)
{
    mnp->memspace = memspace;
}

MEMSPACE mon_navigate_get_memspace(mon_navigate_private_t *mnp)
{
    return mnp->memspace;
}

WORD mon_navigate_get_startaddress(mon_navigate_private_t * mnp)
{
    return mnp->StartAddress;
}

void mon_navigate_set_startaddress(mon_navigate_private_t * mnp, WORD StartAddress)
{
    mnp->StartAddress = StartAddress;
}

static WORD mon_navigate_get_currentaddress(mon_navigate_private_t * mnp)
{
    return mnp->CurrentAddress;
}

static unsigned int mon_navigate_get_have_label(mon_navigate_private_t * mnp)
{
    return mnp->have_label;
}

static void mon_navigate_set_lines(mon_navigate_private_t * mnp, int Lines)
{
    mnp->Lines = Lines;
}

static void mon_navigate_set_endaddress(mon_navigate_private_t * mnp, WORD loc)
{
    mnp->EndAddress = loc;
}

mon_disassembly_t * mon_disassembly_get_lines(mon_disassembly_private_t *pmdp, int lines_visible, int lines_full_visible)
{
    WORD loc;
    unsigned int size;
    int i;
    unsigned int have_label = mon_navigate_get_have_label(&pmdp->navigate);
    mon_disassembly_t *contents = NULL;
    mon_disassembly_t *ret;

    loc = mon_navigate_get_startaddress(&pmdp->navigate);
    ret = NULL;

    mon_navigate_set_lines(&pmdp->navigate, lines_full_visible);

    for (i = 0; i < lines_visible; i++) {
        mon_disassembly_t *newcont;
        mon_breakpoint_type_t bptype;

        newcont = lib_malloc(sizeof *newcont);

        if (ret == NULL) {
            ret = contents = newcont;
        } else {
            contents = contents->next = newcont;
        }

        contents->next = NULL;
        contents->flags.active_line = loc == mon_navigate_get_currentaddress(&pmdp->navigate) ? 1 : 0;

        /* determine type of breakpoint */
        bptype = mon_breakpoint_is(new_addr(mon_navigate_get_memspace(&pmdp->navigate), loc));

        contents->flags.is_breakpoint = bptype != BP_NONE;
        contents->flags.breakpoint_active = bptype == BP_ACTIVE;

        contents->content =
            mon_disassemble_with_label(mon_navigate_get_memspace(&pmdp->navigate), loc, 1,
                                       &size, &have_label);

        contents->length = strlen(contents->content);

        mon_navigate_set_endaddress(&pmdp->navigate, loc);

        loc += size;
    }

    return ret;
}


mon_disassembly_t *mon_dump_get_lines(mon_memory_private_t *pmdp, int lines_visible, int lines_full_visible)
{
    WORD loc;
    int i;
    unsigned int have_label = mon_navigate_get_have_label(&pmdp->navigate);
    mon_disassembly_t *contents = NULL;
    mon_disassembly_t *ret;

    loc = mon_navigate_get_startaddress(&pmdp->navigate);
    ret = NULL;

    mon_navigate_set_lines(&pmdp->navigate, lines_full_visible);

    for (i = 0; i < lines_visible; i++) {
        mon_disassembly_t *newcont;
        mon_breakpoint_type_t bptype;

        newcont = lib_malloc(sizeof *newcont);

        if (ret == NULL) {
            ret = contents = newcont;
        } else {
            contents = contents->next = newcont;
        }

        contents->next = NULL;
        contents->flags.active_line = loc == mon_navigate_get_currentaddress(&pmdp->navigate) ? 1 : 0;

        /* determine type of breakpoint */
        bptype = mon_breakpoint_is(new_addr(mon_navigate_get_memspace(&pmdp->navigate), loc));

        contents->flags.is_breakpoint = bptype != BP_NONE;
        contents->flags.breakpoint_active = bptype == BP_ACTIVE;

        contents->content =
            mon_dump_with_label(mon_navigate_get_memspace(&pmdp->navigate), loc, 1, &have_label);

        contents->length = strlen(contents->content);

        mon_navigate_set_endaddress(&pmdp->navigate, loc);

        /* MPi: Could have labels with two bytes (lo/hi pairs for example) and display these as 16 bit quantities */
        if (!have_label) {
            loc++;
        }
    }

    return ret;
}


static WORD determine_address_of_line(mon_navigate_private_t *mnp, WORD loc, int line )
{
    unsigned int size;
    int i;
    unsigned int have_label = mnp->have_label;

    /* it's one less than visible, so there will be one line visible left! */
    for (i = 0; i < line; i++) {
        char *content;

        content = mon_disassemble_with_label(mon_navigate_get_memspace(mnp),
                                             loc, 1, &size,
                                             &have_label);

        lib_free(content);

        loc += size;
    }

    return loc;
}

static WORD scroll_down(mon_navigate_private_t *mnp, WORD loc)
{
    return determine_address_of_line(mnp, loc, 1);
}

static WORD scroll_down_page(mon_navigate_private_t *mnp, WORD loc)
{
    /* the count is one less than visible,
       so there will be one visible line left on the screen! */
    return determine_address_of_line( mnp, loc, mnp->Lines - 1 );
}

static WORD scroll_up_count(mon_navigate_private_t *mnp, WORD loc, unsigned int count)
{
    unsigned int size;
    /* this has to be initialized with zero for correct processing */
    unsigned int have_label = 0;

    WORD testloc = loc - 3 * count - 3;

    unsigned int *disp = lib_malloc(sizeof(unsigned int) * count);
    unsigned int storepos = 0;

    while (testloc < loc) {
        char *content;

        disp[storepos++] = loc - testloc;
        if (storepos == count) {
            storepos = 0;
        }

        content = mon_disassemble_with_label(mnp->memspace, testloc, 1, &size, &have_label );

        lib_free(content);
        testloc += size;
    }

    loc -= disp[storepos];

    lib_free(disp);

    return loc;
}

static WORD scroll_up(mon_navigate_private_t *mnp, WORD loc)
{
    return scroll_up_count( mnp, loc, 1 );
}

static WORD scroll_up_page(mon_navigate_private_t *mnp, WORD loc)
{
    /* the count is one less than visible,
       so there will be one visible line left on the screen! */
    return scroll_up_count(mnp, loc, mnp->Lines - 1);
}

WORD mon_navigate_scroll(mon_navigate_private_t *mnp, MON_SCROLL_TYPE ScrollType)
{
    switch (ScrollType) {
        case MON_SCROLL_NOTHING:
            break;

        case MON_SCROLL_DOWN:
            mnp->StartAddress = scroll_down(mnp, mnp->StartAddress);
            break;

        case MON_SCROLL_UP:
            mnp->StartAddress = scroll_up(mnp, mnp->StartAddress);
            break;

        case MON_SCROLL_PAGE_DOWN:
            mnp->StartAddress = scroll_down_page(mnp, mnp->StartAddress);
            break;

        case MON_SCROLL_PAGE_UP:
            mnp->StartAddress = scroll_up_page(mnp, mnp->StartAddress);
            break;
    }
    return mnp->StartAddress;
}

WORD mon_navigate_scroll_to(mon_navigate_private_t *mnp, WORD addr)
{
    mnp->StartAddress = addr;
    return mnp->StartAddress;
}

void mon_disassembly_set_breakpoint(mon_disassembly_private_t *pmdp)
{
    mon_breakpoint_set(pmdp->AddrClicked);
}

void mon_disassembly_unset_breakpoint(mon_disassembly_private_t *pmdp)
{
    mon_breakpoint_unset(pmdp->AddrClicked);
}

void mon_disassembly_enable_breakpoint(mon_disassembly_private_t *pmdp)
{
    mon_breakpoint_enable(pmdp->AddrClicked);
}

void mon_disassembly_disable_breakpoint(mon_disassembly_private_t *pmdp)
{
    mon_breakpoint_disable(pmdp->AddrClicked);
}

void mon_navigate_goto_address(mon_navigate_private_t *mnp, WORD addr)
{
    mnp->CurrentAddress = addr;
    mon_navigate_check_if_in_range(mnp);
}

void mon_navigate_goto_pc(mon_navigate_private_t *mnp)
{
    mon_navigate_goto_address(mnp, (WORD)(monitor_cpu_for_memspace[mnp->memspace]->mon_register_get_val(mnp->memspace, e_PC)));
}

void mon_disassembly_set_next_instruction(mon_disassembly_private_t *pmdp)
{
    monitor_cpu_for_memspace[mon_navigate_get_memspace(&pmdp->navigate)]->mon_register_set_val(mon_navigate_get_memspace(&pmdp->navigate), e_PC, (WORD) addr_location(pmdp->AddrClicked));
}

void mon_navigate_goto_string(mon_navigate_private_t * mnp, char *addr)
{
    unsigned long address;
    char * remain;

    address = strtoul(addr, &remain, 16);

    if (*remain == 0) {
        mon_navigate_goto_address(mnp, (WORD) address);
    }
}


void mon_disassembly_determine_popup_commands(mon_disassembly_private_t *pmdp, int xPos, int yPos, WORD *ulMask, WORD *ulDefault)
{
    MON_ADDR CurrentAddress;
    mon_breakpoint_type_t mbt;

    int drive_true_emulation;

    resources_get_int("DriveTrueEmulation", &drive_true_emulation);

    CurrentAddress = new_addr(mon_navigate_get_memspace(&pmdp->navigate),
                              determine_address_of_line(&pmdp->navigate,
                                                        mon_navigate_get_startaddress(&pmdp->navigate), yPos));
    mbt = mon_breakpoint_is(CurrentAddress);

    /* remember values to be re-used when command is executed */
    pmdp->AddrClicked = CurrentAddress;

    switch (mbt) {
        case BP_ACTIVE:
            *ulMask = MDDPC_UNSET_BREAKPOINT | MDDPC_DISABLE_BREAKPOINT;
            *ulDefault = MDDPC_UNSET_BREAKPOINT;
            break;

        case BP_INACTIVE:
            *ulMask = MDDPC_SET_BREAKPOINT | MDDPC_UNSET_BREAKPOINT
                      | MDDPC_ENABLE_BREAKPOINT;
            *ulDefault = MDDPC_SET_BREAKPOINT;
            break;

        case BP_NONE:
            *ulMask = MDDPC_SET_BREAKPOINT;
            *ulDefault = MDDPC_SET_BREAKPOINT;
            break;
    }

    if (drive_true_emulation) {
        *ulMask |= MDDPC_SET_COMPUTER | MDDPC_SET_DRIVE8 | MDDPC_SET_DRIVE9 | MDDPC_SET_DRIVE10 | MDDPC_SET_DRIVE11;

        switch (mon_navigate_get_memspace(&pmdp->navigate)) {
            case e_comp_space:
                *ulMask &= ~MDDPC_SET_COMPUTER;
                break;
            case e_disk8_space:
                *ulMask &= ~MDDPC_SET_DRIVE8;
                break;
            case e_disk9_space:
                *ulMask &= ~MDDPC_SET_DRIVE9;
                break;
            case e_disk10_space:
                *ulMask &= ~MDDPC_SET_DRIVE10;
                break;
            case e_disk11_space:
                *ulMask &= ~MDDPC_SET_DRIVE11;
                break;
            case e_default_space:
            case e_invalid_space:
                break;
        }
    }
}

void mon_ui_init(void)
{
}

void mon_memory_init(mon_memory_private_t * pmmp)
{
    mon_navigate_init(&pmmp->navigate);
}

void mon_memory_deinit(mon_memory_private_t * pmmp)
{
}

void mon_memory_update(mon_memory_private_t * pmmp)
{
}

mon_memory_t *mon_memory_get_lines(mon_memory_private_t * pmmp, int lines_visible, int lines_full_visible)
{
    WORD loc;
    unsigned int size = 0;
    int i;
    mon_memory_t *contents = NULL;
    mon_memory_t *ret;

    loc = mon_navigate_get_startaddress(&pmmp->navigate);
    ret = NULL;

    mon_navigate_set_lines(&pmmp->navigate, lines_full_visible);

    for (i = 0; i < lines_visible; i++) {
        mon_memory_t *newcont;

        newcont = lib_malloc(sizeof *newcont);

        if (ret == NULL) {
            ret = contents = newcont;
        } else {
            contents = contents->next = newcont;
        }

        contents->next = NULL;
        contents->flags.active_line = 0;
        contents->flags.is_breakpoint = 0;
        contents->flags.breakpoint_active = 0;

        contents->content = lib_stralloc(">C:a0e0  54 4f d0 4f  ce 57 41 49   TO.O.WAI");
        size += 8;

        contents->length = strlen(contents->content);

        mon_navigate_set_endaddress(&pmmp->navigate, loc);

        loc += size;
    }

    return ret;
}
