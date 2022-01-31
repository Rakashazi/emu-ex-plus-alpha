/*
 * vic20cartmem.c -- VIC20 Cartridge memory handling.
 *
 * Written by
 *  Daniel Kahlin <daniel@kahlin.net>
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

#define DEBUGCART

#include "vice.h"

#include <stdio.h>

#include "behrbonz.h"
#include "c64acia.h"
#include "cartridge.h"
#include "digimax.h"
#include "ds12c887rtc.h"
#include "finalexpansion.h"
#include "georam.h"
#include "ioramcart.h"
#include "megacart.h"
#include "machine.h"
#include "mem.h"
#include "resources.h"
#include "sfx_soundexpander.h"
#include "sfx_soundsampler.h"
#include "sidcart.h"
#ifdef HAVE_RAWNET
#define CARTRIDGE_INCLUDE_PRIVATE_API
#define CARTRIDGE_INCLUDE_PUBLIC_API
#include "ethernetcart.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API
#undef CARTRIDGE_INCLUDE_PUBLIC_API
#endif
#include "types.h"
#include "ultimem.h"
#include "vic20mem.h"
#include "vic20cart.h"
#include "vic20cartmem.h"
#include "vic20-generic.h"
#include "vic20-ieee488.h"
#include "vic20-midi.h"
#include "vic-fp.h"

#ifdef DEBUGCART
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/* ------------------------------------------------------------------------- */

int mem_cartridge_type = CARTRIDGE_NONE;
int mem_cart_blocks = 0;

/* ------------------------------------------------------------------------- */

uint8_t cartridge_read_ram123(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_ram123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_ram123_read(addr);
            break;
        default:
            vic20_cpu_last_data = vic20_v_bus_last_data;
            break;
    }
    vic20_mem_v_bus_read(addr);
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_ram123(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            return generic_ram123_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_ram123_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_ram123_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_ram123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_ram123_read(addr);
        default:
            break;
    }
    return 0;
}

void cartridge_store_ram123(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_ram123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_ram123_store(addr, value);
            break;
    }
    vic20_mem_v_bus_store(addr);
}

uint8_t cartridge_read_blk1(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk13_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk1_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk1_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk1(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk13_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk1_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk1_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk1_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk1_read(addr);
    }
    return 0;
}

void cartridge_store_blk1(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk1_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk1_store(addr, value);
            break;
    }
}

uint8_t cartridge_read_blk2(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk25_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk2_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk2_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk2(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk25_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk2_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk23_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk23_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk2_read(addr);
    }
    return 0;
}

void cartridge_store_blk2(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk2_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk2_store(addr, value);
            break;
    }
}

uint8_t cartridge_read_blk3(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk13_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk3_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk23_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk123_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk3_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk3(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk13_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk3_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk23_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk23_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk123_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk3_read(addr);
    }
    return 0;
}

void cartridge_store_blk3(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk3_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk23_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk123_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk3_store(addr, value);
            break;
    }
}

uint8_t cartridge_read_blk5(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            vic20_cpu_last_data = behrbonz_blk25_read(addr);
            break;
        case CARTRIDGE_VIC20_GENERIC:
            vic20_cpu_last_data = generic_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_UM:
            vic20_cpu_last_data = vic_um_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_FP:
            vic20_cpu_last_data = vic_fp_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            vic20_cpu_last_data = megacart_blk5_read(addr);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            vic20_cpu_last_data = finalexpansion_blk5_read(addr);
            break;
    }
    return vic20_cpu_last_data;
}

uint8_t cartridge_peek_blk5(uint16_t addr)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            return behrbonz_blk25_read(addr);
        case CARTRIDGE_VIC20_GENERIC:
            return generic_blk5_read(addr);
        case CARTRIDGE_VIC20_UM:
            return vic_um_blk5_read(addr);
        case CARTRIDGE_VIC20_FP:
            return vic_fp_blk5_read(addr);
        case CARTRIDGE_VIC20_MEGACART:
            return megacart_blk5_read(addr);
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            return finalexpansion_blk5_read(addr);
    }
    return 0;
}

void cartridge_store_blk5(uint16_t addr, uint8_t value)
{
    vic20_cpu_last_data = value;
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_blk5_store(addr, value);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_blk5_store(addr, value);
            break;
    }
}

/* ------------------------------------------------------------------------- */

void cartridge_init(void)
{
    behrbonz_init();
    generic_init();
    megacart_init();
    finalexpansion_init();
    vic_fp_init();
#ifdef HAVE_RAWNET
    ethernetcart_init();
#endif
    aciacart_init();
    georam_init();
}

void cartridge_reset(void)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_BEHRBONZ:
            behrbonz_reset();
            break;
        case CARTRIDGE_VIC20_GENERIC:
            generic_reset();
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_reset();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_reset();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_reset();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_reset();
            break;
    }
#ifdef HAVE_RAWNET
    if (ethernetcart_cart_enabled()) {
        ethernetcart_reset();
    }
#endif
    if (aciacart_cart_enabled()) {
        aciacart_reset();
    }
    if (digimax_cart_enabled()) {
        digimax_reset();
    }
    if (ds12c887rtc_cart_enabled()) {
        ds12c887rtc_reset();
    }
    if (sfx_soundexpander_cart_enabled()) {
        sfx_soundexpander_reset();
    }
    if (sfx_soundsampler_cart_enabled()) {
        sfx_soundsampler_reset();
    }
    if (georam_cart_enabled()) {
        georam_reset();
    }
}

void cartridge_powerup(void)
{
    switch (mem_cartridge_type) {
        case CARTRIDGE_VIC20_UM:
            vic_um_powerup();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_powerup();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_powerup();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_powerup();
            break;
    }
}

void cartridge_attach(int type, uint8_t *rawcart)
{
    int cartridge_reset;

    mem_cartridge_type = type;
    
    DBG(("cartridge_attach type: %d\n", type));
#if 0
    switch (type) {
        case CARTRIDGE_VIC20_GENERIC:
            generic_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_config_setup(rawcart);
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_config_setup(rawcart);
            break;
        default:
            mem_cartridge_type = CARTRIDGE_NONE;
    }
#endif

    resources_get_int("CartridgeReset", &cartridge_reset);

    if (cartridge_reset != 0) {
        /* "Turn off machine before inserting cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
}

static void cart_detach_all(void)
{
    /* vic20 carts */
    behrbonz_detach();
    generic_detach();
    finalexpansion_detach();
    ioramcart_io2_detach();
    ioramcart_io3_detach();
    megacart_detach();
    vic_um_detach();
    vic20_ieee488_detach();
#ifdef HAVE_MIDI
    vic20_midi_detach();
#endif
    sidcart_detach();
    vic_fp_detach();

    /* c64 through mascuerade carts */
    aciacart_detach();
    digimax_detach();
    ds12c887rtc_detach();
    georam_detach();
    sfx_soundexpander_detach();
    sfx_soundsampler_detach();
#ifdef HAVE_RAWNET
    ethernetcart_detach();
#endif
}

void cartridge_detach(int type)
{
    int cartridge_reset;

    switch (type) {
        case -1:
            cart_detach_all();
            break;
        case CARTRIDGE_VIC20_BEHRBONZ:
            behrbonz_detach();
            break;
        case CARTRIDGE_VIC20_GENERIC:
            generic_detach();
            break;
        case CARTRIDGE_VIC20_UM:
            vic_um_detach();
            break;
        case CARTRIDGE_VIC20_FP:
            vic_fp_detach();
            break;
        case CARTRIDGE_VIC20_MEGACART:
            megacart_detach();
            break;
        case CARTRIDGE_VIC20_FINAL_EXPANSION:
            finalexpansion_detach();
            break;
    }
    mem_cartridge_type = CARTRIDGE_NONE;
    /* this is probably redundant as it is also performed by the
       local detach functions. */
    mem_cart_blocks = 0;
    mem_initialize_memory();

    resources_get_int("CartridgeReset", &cartridge_reset);

    if (cartridge_reset != 0) {
        /* "Turn off machine before removing cartridge" */
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
}

/* ------------------------------------------------------------------------- */

void cartridge_sound_chip_init(void)
{
    digimax_sound_chip_init();
    sfx_soundexpander_sound_chip_init();
    sfx_soundsampler_sound_chip_init();
}
