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

/* #define DEBUG_TRAPS */

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
#include "traps.h"
#include "types.h"
#include "wdc65816.h"

#ifdef DEBUG_TRAPS
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

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
static int traps_enabled = 0;

#define MAX_DEVICES 15  /* FIXME: is there another constant we can use instead? */
int traps_enabled_device[MAX_DEVICES];

static void set_traps_status(int enabled)
{
    int new_value = enabled ? 1 : 0;

    DBG(("set_traps_status(%d)", enabled));

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
}

static int set_traps_enabled(int val, void *param)
{
    unsigned int enabled = 0;
    unsigned int unit = vice_ptr_to_int(param);
    unsigned int i;

    DBG(("set_traps_enabled %d device: %u", val, unit));
    traps_enabled_device[unit] = val ? 1 : 0;

    /* check all devices, enable traps if any of them is enabled */
    for (i = 1; i < MAX_DEVICES; i++) {
        enabled |= traps_enabled_device[i];
    }
    set_traps_status(enabled);

    machine_bus_status_virtualdevices_set(unit, enabled);
    return 0;
}

static resource_int_t resources_int[] = {
    /* tape */
    { "VirtualDevice1", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[1], set_traps_enabled, (void*)1 },
    { "VirtualDevice2", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[2], set_traps_enabled, (void*)2 },
    /* printers */
    { "VirtualDevice4", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[4], set_traps_enabled, (void*)4 },
    { "VirtualDevice5", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[5], set_traps_enabled, (void*)5 },
    { "VirtualDevice6", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[6], set_traps_enabled, (void*)6 },
    { "VirtualDevice7", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[7], set_traps_enabled, (void*)7 },
    /* disk drives */
    { "VirtualDevice8", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[8], set_traps_enabled, (void*)8 },
    { "VirtualDevice9", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[9], set_traps_enabled, (void*)9 },
    { "VirtualDevice10", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[10], set_traps_enabled, (void*)10 },
    { "VirtualDevice11", 0, RES_EVENT_SAME, NULL,
      &traps_enabled_device[11], set_traps_enabled, (void*)11 },
    RESOURCE_INT_LIST_END
};

#if 0
static resource_int_t resources_int_ieee[] = {
    /* tape */
    { "VirtualDevice1", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[1], set_traps_enabled, (void*)1 },
    { "VirtualDevice2", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[2], set_traps_enabled, (void*)2 },
    /* printers */
    { "VirtualDevice4", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[4], set_traps_enabled, (void*)4 },
    { "VirtualDevice5", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[5], set_traps_enabled, (void*)5 },
    { "VirtualDevice6", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[6], set_traps_enabled, (void*)6 },
    { "VirtualDevice7", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[7], set_traps_enabled, (void*)7 },
    /* disk drives */
    { "VirtualDevice8", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[8], set_traps_enabled, (void*)8 },
    { "VirtualDevice9", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[9], set_traps_enabled, (void*)9 },
    { "VirtualDevice10", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[10], set_traps_enabled, (void*)10 },
    { "VirtualDevice11", 1, RES_EVENT_SAME, NULL,
      &traps_enabled_device[11], set_traps_enabled, (void*)11 },
    RESOURCE_INT_LIST_END
};
#endif

int traps_resources_init(void)
{
#if 0
    /* the IEEE488 based machines do not use "device traps" (ROM patches),
       instead the virtual devices are actually interfaced to the IEEE bus.
       this makes them much more reliably, which is why we can enable them
       by default here. */
    if ((machine_class == VICE_MACHINE_PET) ||
        (machine_class == VICE_MACHINE_CBM5x0) ||
        (machine_class == VICE_MACHINE_CBM6x0)) {
        return resources_register_int(resources_int_ieee);
    }
#endif
    return resources_register_int(resources_int);
}

/* ------------------------------------------------------------------------- */

/* Trap-related command-line options.  */

static const cmdline_option_t cmdline_options[] =
{
    /* tape */
    { "-virtualdev1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice1", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice1", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice2", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice2", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    /* printers */
    { "-virtualdev4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice4", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice4", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev5", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice5", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev5", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice5", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice6", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev6", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice6", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev7", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice7", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev7", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice7", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    /* disk drives */
    { "-virtualdev8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice8", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev8", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice8", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice9", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice9", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev10", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice10", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev10", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice10", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
    { "-virtualdev11", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice11", (resource_value_t)1,
      NULL, "Enable general mechanisms for fast disk/tape emulation" },
    { "+virtualdev11", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "VirtualDevice11", (resource_value_t)0,
      NULL, "Disable general mechanisms for fast disk/tape emulation" },
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
        if ((t->readfunc)((uint16_t)(t->address + i)) != t->check[i]) {
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
        log_verbose("Trap '%s' added.", trap->name);
        install_trap(trap);
    } else {
        log_verbose("Traps are disabled, trap '%s' not installed.", trap->name);
    }

    return 0;
}

static int remove_trap(const trap_t *trap)
{
    if ((trap->readfunc)(trap->address) != TRAP_OPCODE) {
        log_error(traps_log, "remove_trap($%04x): No trap `%s' installed?",
                  trap->address, trap->name);
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

uint32_t traps_handler(void)
{
    traplist_t *p = traplist;
    unsigned int pc;
    int result;

    pc = maincpu_get_pc();

    while (p) {
        if (p->trap->address == pc) {
            /* This allows the trap function to remove traps.  */
            uint16_t resume_address = p->trap->resume_address;

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

    return (uint32_t)-1;
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
