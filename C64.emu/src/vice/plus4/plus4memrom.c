/*
 * plus4memrom.c -- Plus4 ROM access.
 *
 * Written by
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

#include <string.h>

#include "log.h"
#include "machine.h"
#include "plus4cart.h"
#include "plus4mem.h"
#include "plus4memrom.h"
#include "sysfile.h"
#include "types.h"

uint8_t plus4memrom_basic_rom[PLUS4_BASIC_ROM_SIZE];
uint8_t plus4memrom_kernal_rom[PLUS4_KERNAL_ROM_SIZE];

uint8_t extromlo1[PLUS4_C0LO_ROM_SIZE];
uint8_t extromhi1[PLUS4_C0HI_ROM_SIZE];

/* FIXME: c2 can also be external cartridge */
uint8_t extromlo3[PLUS4_C2LO_ROM_SIZE];
uint8_t extromhi3[PLUS4_C2HI_ROM_SIZE];

uint8_t plus4memrom_kernal_trap_rom[PLUS4_KERNAL_ROM_SIZE];

uint8_t plus4memrom_kernal_read(uint16_t addr)
{
    return plus4memrom_kernal_rom[addr & 0x3fff];
}

#if 0
static void plus4memrom_kernal_store(uint16_t addr, uint8_t value)
{
    plus4memrom_kernal_rom[addr & 0x3fff] = value;
}
#endif

uint8_t plus4memrom_basic_read(uint16_t addr)
{
    return plus4memrom_basic_rom[addr & 0x3fff];
}

#if 0
static void plus4memrom_basic_store(uint16_t addr, uint8_t value)
{
    plus4memrom_basic_rom[addr & 0x3fff] = value;
}
#endif


uint8_t plus4memrom_trap_read(uint16_t addr)
{
    switch (addr & 0xc000) {
        case 0xc000:
            return plus4memrom_kernal_trap_rom[addr & 0x3fff];
    }

    return 0;
}

void plus4memrom_trap_store(uint16_t addr, uint8_t value)
{
    switch (addr & 0xc000) {
        case 0xc000:
            plus4memrom_kernal_trap_rom[addr & 0x3fff] = value;
            break;
    }
}

/* c0lo internal "function rom" (plus4) */
uint8_t plus4memrom_extromlo1_read(uint16_t addr)
{
    return extromlo1[addr & 0x3fff];
}

/* c0hi internal "function rom" (plus4) */
uint8_t plus4memrom_extromhi1_read(uint16_t addr)
{
    return extromhi1[addr & 0x3fff];
}

/* c2lo can be internal or external cartridge, used by v364 speech rom */
uint8_t plus4memrom_extromlo3_read(uint16_t addr)
{
    return extromlo3[addr & 0x3fff];
}

/* c2hi can be internal or external cartridge, used by v364 speech rom */
uint8_t plus4memrom_extromhi3_read(uint16_t addr)
{
    return extromhi3[addr & 0x3fff];
}

/* ---------------------------------------------------------------------*/

/* FIXME: get rid of this ugly hack */
extern int plus4_rom_loaded;

int plus4cart_load_func_lo(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load 3plus1 low ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, machine_name, extromlo1, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load 3plus1 low ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromlo1, 0xff, PLUS4_CART16K_SIZE);
    }
    return 0;
}

int plus4cart_load_func_hi(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load 3plus1 high ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, machine_name, extromhi1, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load 3plus1 high ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        memset(extromhi1, 0xff, PLUS4_CART16K_SIZE);
    }
    return 0;
}

/* FIXME: c2lo/hi can be external or internal */
int plus4cart_load_c2lo(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c2 low ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, machine_name, extromlo3, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 2 low ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        /* memset(extromlo3, 0xff, PLUS4_CART16K_SIZE); */
    }
    return 0;
}

/* FIXME: c2lo/hi can be external or internal */
int plus4cart_load_c2hi(const char *rom_name)
{
    if (!plus4_rom_loaded) {
        return 0;
    }

    /* Load c2 high ROM.  */
    if (*rom_name != 0) {
        if (sysfile_load(rom_name, machine_name, extromhi3, PLUS4_CART16K_SIZE, PLUS4_CART16K_SIZE) < 0) {
            log_error(LOG_ERR,
                      "Couldn't load cartridge 2 high ROM `%s'.",
                      rom_name);
            return -1;
        }
    } else {
        /* memset(extromhi3, 0xff, PLUS4_CART16K_SIZE); */
    }
    return 0;
}

uint8_t plus4memrom_rom_read(uint16_t addr)
{
    switch (addr & 0xc000) {
        case 0x8000:
            switch ((mem_config >> 1) & 3) {
                case 0:
                    return plus4memrom_basic_read(addr);
                case 1: /* c0lo */
                    return plus4memrom_extromlo1_read(addr);
                case 2: /* c1lo */
                    return plus4cart_c1lo_read(addr);
                case 3: /* c2lo */
                    return plus4memrom_extromlo3_read(addr);
            }
            /* Unreachable */
            break;
        case 0xc000:
            if ((addr & 0xff00) == 0xfc00) {
                return plus4memrom_kernal_read(addr);
            } else {
                switch ((mem_config >> 3) & 3) {
                    case 0:
                        return plus4memrom_kernal_read(addr);
                    case 1: /* c0hi */
                        return plus4memrom_extromhi1_read(addr);
                    case 2: /* c1hi */
                        return plus4cart_c1hi_read(addr);
                    case 3: /* c2hi */
                        return plus4memrom_extromhi3_read(addr);
                }
            }
            /* Unreachable */
            break;
    }

    return 0;
}

void plus4memrom_rom_store(uint16_t addr, uint8_t value)
{
    switch (addr & 0xc000) {
        case 0x8000:
            plus4memrom_basic_rom[addr & 0x3fff] = value;
            break;
        case 0xc000:
            plus4memrom_kernal_rom[addr & 0x3fff] = value;
            break;
    }
}
