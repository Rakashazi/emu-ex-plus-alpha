/*
 * traps.c - Allow VICE to replace ROM code with C function calls.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jarkko Sonninen <sonninen@lut.fi>
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
#include <stdlib.h>

#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "machine-bus.h"
#include "maincpu.h"
#include "mem.h"
#include "resources.h"
#include "translate.h"
#include "traps.h"
#include "types.h"
#include "wdc65816.h"

typedef struct traplist_s {
    struct traplist_s *next;
    const trap_t *trap;
} traplist_t;

static traplist_t *traplist = NULL;

static int install_trap(const trap_t *t);
static int remove_trap(const trap_t *t);

static log_t traps_log = LOG_ERR;

/* ------------------------------------------------------------------------- */

/* Trap-related resources.  */

/* Flag: Should we avoid installing traps at all?  */
static int traps_enabled;

static int set_traps_enabled(int val, void *param)
{
    int new_value = val ? 1 : 0;

    if ((!traps_enabled && new_value) || (traps_enabled && !new_value)) {
        if (!new_value) {
            /* Traps have been disabled.  */
            traplist_t *p;

            for (p = traplist; p != NULL; p = p->next) {
                remove_trap(p->trap);
            }
        } else {
            /* Traps have been enabled.  */
            traplist_t *p;

            for (p = traplist; p != NULL; p = p->next) {
                install_trap(p->trap);
            }
        }
    }

    traps_enabled = new_value;

    machine_bus_status_virtualdevices_set((unsigned int)new_value);

    return 0;
}

static const resource_int_t resources_int[] = {
    { "VirtualDevices", 0, RES_EVENT_SAME, NULL,
      &traps_enabled, set_traps_enabled, NULL },
    RESOURCE_INT_LIST_END
};

int traps_resources_init(void)
{
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

/* Trap-related command-line options.  */

static const cmdline_option_t cmdline_options[] = {
    { "-virtualdev", SET_RESOURCE, 0,
      NULL, NULL, "VirtualDevices", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_TRAPS_FAST_EMULATION,
      NULL, NULL },
    { "+virtualdev", SET_RESOURCE, 0,
      NULL, NULL, "VirtualDevices", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_TRAPS_FAST_EMULATION,
      NULL, NULL },
    CMDLINE_LIST_END
};

int traps_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

/* ------------------------------------------------------------------------- */

void traps_init(void)
{
    traps_log = log_open("Traps");
}

void traps_shutdown(void)
{
    traplist_t *list, *list_next;

    list = traplist;

    while (list != NULL) {
        list_next = list->next;
        lib_free(list);
        list = list_next;
    }
}

static int install_trap(const trap_t *t)
{
    int i;

    for (i = 0; i < 3; i++) {
        if ((t->readfunc)((WORD)(t->address + i)) != t->check[i]) {
            log_error(traps_log,
                      "Incorrect checkbyte for trap `%s'.  Not installed.",
                      t->name);
            return -1;
        }
    }

    log_verbose("Trap '%s' installed.", t->name);
    (t->storefunc)(t->address, TRAP_OPCODE);

    return 0;
}

int traps_add(const trap_t *trap)
{
    traplist_t *p;

    p = lib_malloc(sizeof(traplist_t));
    p->next = traplist;
    p->trap = trap;
    traplist = p;

    if (traps_enabled) {
        install_trap(trap);
    } else {
        log_verbose("Traps are disabled, trap '%s' not installed.", trap->name);
    }

    return 0;
}

static int remove_trap(const trap_t *trap)
{
    if ((trap->readfunc)(trap->address) != TRAP_OPCODE) {
        log_error(traps_log, "No trap `%s' installed?", trap->name);
        return -1;
    }
    log_verbose("Trap '%s' disabled.", trap->name);

    (trap->storefunc)(trap->address, trap->check[0]);
    return 0;
}

int traps_remove(const trap_t *trap)
{
    traplist_t *p = traplist, *prev = NULL;

    while (p) {
        if (p->trap->address == trap->address) {
            break;
        }
        prev = p;
        p = p->next;
    }

    if (!p) {
        log_error(traps_log, "Trap `%s' not found.", trap->name);
        return -1;
    }

    if (prev) {
        prev->next = p->next;
    } else {
        traplist = p->next;
    }

    lib_free(p);

    if (traps_enabled) {
        remove_trap(trap);
    }

    return 0;
}

void traps_refresh(void)
{
    if (traps_enabled) {
        traplist_t *p;

        for (p = traplist; p != NULL; p = p->next) {
            remove_trap(p->trap);
            install_trap(p->trap);
        }
    }
    return;
}

DWORD traps_handler(void)
{
    traplist_t *p = traplist;
    unsigned int pc;
    int result;

    pc = maincpu_get_pc();

    while (p) {
        if (p->trap->address == pc) {
            /* This allows the trap function to remove traps.  */
            WORD resume_address = p->trap->resume_address;

            result = (*p->trap->func)();
            if (!result) {
                return (p->trap->check[0] | (p->trap->check[1] << 8) | (p->trap->check[2] << 16));
            }
            /* XXX ALERT!  `p' might not be valid anymore here, because
               `p->trap->func()' might have removed all the traps.  */
            maincpu_set_pc(resume_address);
            return 0;
        }
        p = p->next;
    }

    return (DWORD)-1;
}

int traps_checkaddr(unsigned int addr)
{
    traplist_t *p = traplist;

    while (p) {
        if (p->trap->address == addr) {
            return 1;
        }
        p = p->next;
    }

    return 0;
}
