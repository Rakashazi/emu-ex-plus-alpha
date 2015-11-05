/*
 * plus4memhannes256k.c - HANNES 256K EXPANSION emulation.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "mem.h"
#include "plus4mem.h"
#include "plus4memcsory256k.h"
#include "plus4memhannes256k.h"
#include "resources.h"
#include "snapshot.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"


/* HANNES 256K registers */
static BYTE h256k_reg = 0;

static log_t h256k_log = LOG_ERR;

static int h256k_activate(int type);
static int h256k_deactivate(void);

int h256k_enabled = 0;

static int h256k_bank = 3;
static int h256k_bound = 1;

BYTE *h256k_ram = NULL;

int set_h256k_enabled(int val)
{
    switch (val) {
        case H256K_DISABLED:
        case H256K_256K:
        case H256K_1024K:
        case H256K_4096K:
            break;
        default:
            return -1;
    }

    if (!val) {
        if (h256k_enabled) {
            if (h256k_deactivate() < 0) {
                return -1;
            }
        }
        h256k_enabled = 0;
    } else {
        if (!h256k_enabled || h256k_enabled != val) {
            if (h256k_activate(val) < 0) {
                return -1;
            }
        }
        h256k_enabled = 1;
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

void h256k_init(void)
{
    h256k_log = log_open("H256K");
}

void h256k_reset(void)
{
    h256k_reg = 0xff;
    h256k_bank = 3;
    h256k_bound = 1;
}

static int h256k_activate(int type)
{
    if (type == 1) {
        h256k_ram = lib_realloc((void *)h256k_ram, (size_t)0x30000);
        log_message(h256k_log, "HANNES 256K expansion installed.");
    }
    if (type == 2) {
        h256k_ram = lib_realloc((void *)h256k_ram, (size_t)0xf0000);
        log_message(h256k_log, "HANNES 1024K expansion installed.");
    }
    if (type == 3) {
        h256k_ram = lib_realloc((void *)h256k_ram, (size_t)0x3f0000);
        log_message(h256k_log, "HANNES 4096K expansion installed.");
    }
    h256k_reset();
    return 0;
}

static int h256k_deactivate(void)
{
    lib_free(h256k_ram);
    h256k_ram = NULL;
    return 0;
}

void h256k_shutdown(void)
{
    if (h256k_enabled) {
        h256k_deactivate();
    }
}

/* ------------------------------------------------------------------------- */

BYTE h256k_reg_read(WORD addr)
{
    return h256k_reg;
}

void h256k_reg_store(WORD addr, BYTE value)
{
    h256k_bank = value & 3;
    h256k_reg = ((value & 0xbf) | 0x40);
    if (h256k_enabled == 1) {
        h256k_reg = h256k_reg | 0x7c;
    }
    if (h256k_enabled == 2) {
        h256k_bank = h256k_bank + ((3 - ((value & 0xc) >> 2)) << 2);
        h256k_reg = h256k_reg | 0x70;
    }
    if (h256k_enabled == 3) {
        h256k_bank = h256k_bank + ((3 - ((value & 0x30) >> 4)) << 4);
    }
    h256k_bound = (value & 0x80) >> 7;
}

void h256k_store(WORD addr, BYTE value)
{
    int real_bank;

    if (h256k_enabled != 1 && h256k_bank > 3) {
        real_bank = h256k_bank - 1;
    } else {
        real_bank = h256k_bank;
    }

    if (addr < 0x1000 || h256k_bank == 3) {
        mem_ram[addr] = value;
    }

    if (h256k_bound == 0 && addr >= 0x1000 && h256k_bank != 3) {
        h256k_ram[(real_bank * 0x10000) + addr] = value;
    }

    if (h256k_bound == 1 && addr >= 0x1000 && addr < 0x4000) {
        mem_ram[addr] = value;
    }

    if (addr >= 0x4000 && h256k_bank != 3) {
        h256k_ram[(real_bank * 0x10000) + addr] = value;
    }
}

BYTE h256k_read(WORD addr)
{
    int real_bank;

    if (h256k_enabled != 1 && h256k_bank > 3) {
        real_bank = h256k_bank - 1;
    } else {
        real_bank = h256k_bank;
    }

    if (addr < 0x1000 || h256k_bank == 3) {
        return mem_ram[addr];
    }

    if (h256k_bound == 0 && addr >= 0x1000 && h256k_bank != 3) {
        return h256k_ram[(real_bank * 0x10000) + addr];
    }

    if (h256k_bound == 1 && addr >= 0x1000 && addr < 0x4000) {
        return mem_ram[addr];
    }

    if (addr >= 0x4000 && h256k_bank != 3) {
        return h256k_ram[(real_bank * 0x10000) + addr];
    }

    return mem_ram[addr];
}
