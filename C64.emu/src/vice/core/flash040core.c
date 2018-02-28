/*
 * flash040core.c - (AM)29F0[14]0(B) Flash emulation.
 *
 * Written by
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 * Extended by
 *  Marko Makela <marko.makela@iki.fi>
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
 *  Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "alarm.h"
#include "flash040.h"
#include "lib.h"
#include "log.h"
#include "maincpu.h"
#include "snapshot.h"
#include "types.h"

/* -------------------------------------------------------------------------- */

/* #define FLASH_DEBUG_ENABLED */

#ifdef FLASH_DEBUG_ENABLED
#define FLASH_DEBUG(x) log_debug x
#else
#define FLASH_DEBUG(x)
#endif

/* Timeout after sector erase command (datasheet states 50us) */
#define ERASE_SECTOR_TIMEOUT_CYCLES 50
/* Time taken by sector & chip erase (FIXME: numbers pulled from a hat) */
#define ERASE_SECTOR_CYCLES 1012
#define ERASE_CHIP_CYCLES 8192

struct flash_types_s {
    BYTE manufacturer_ID;
    BYTE device_ID;
    BYTE device_ID_addr;
    unsigned int size;
    unsigned int sector_mask;
    unsigned int sector_size;
    unsigned int sector_shift;
    unsigned int magic_1_addr;
    unsigned int magic_2_addr;
    unsigned int magic_1_mask;
    unsigned int magic_2_mask;
    BYTE status_toggle_bits;
};
typedef struct flash_types_s flash_types_t;

static const flash_types_t flash_types[FLASH040_TYPE_NUM] = {
    /* 29F040 */
    { 0x01, 0xa4, 1,
      0x80000,
      0x70000, 0x10000, 16,
      0x5555, 0x2aaa, 0x7fff, 0x7fff,
      0x40 },
    /* 29F040B */
    { 0x01, 0xa4, 1,
      0x80000,
      0x70000, 0x10000, 16,
      0x555, 0x2aa, 0x7ff, 0x7ff,
      0x40 },
    /* 29F010 */
    { 0x01, 0x20, 1,
      0x20000,
      0x1c000, 0x04000, 14,
      0x5555, 0x2aaa, 0x7fff, 0x7fff,
      0x40 },
    /* 29F032B with A0/1 swap */
    { 0x01, 0x41, 1,
      0x400000,
      0x3f0000, 0x10000, 16,
      0x556, 0x2a9, 0x7ff, 0x7ff,
      0x44 },
    /* Spansion S29GL064N */
    { 0x01, 0x7e, 2,
      0x800000,
      /* FIXME: some models support non-uniform sector layout */
      0x7f0000, 0x10000, 16,
      0xaaa, 0x555, 0xfff, 0xfff,
      0x40 },
};

/* -------------------------------------------------------------------------- */

inline static int flash_magic_1(flash040_context_t *flash040_context, unsigned int addr)
{
    return ((addr & flash_types[flash040_context->flash_type].magic_1_mask) == flash_types[flash040_context->flash_type].magic_1_addr);
}

inline static int flash_magic_2(flash040_context_t *flash040_context, unsigned int addr)
{
    return ((addr & flash_types[flash040_context->flash_type].magic_2_mask) == flash_types[flash040_context->flash_type].magic_2_addr);
}

inline static void flash_clear_erase_mask(flash040_context_t *flash040_context)
{
    int i;

    for (i = 0; i < FLASH040_ERASE_MASK_SIZE; ++i) {
        flash040_context->erase_mask[i] = 0;
    }
}

inline static unsigned int flash_sector_to_addr(flash040_context_t *flash040_context, unsigned int sector)
{
    unsigned int sector_size = flash_types[flash040_context->flash_type].sector_size;

    return sector * sector_size;
}

inline static unsigned int flash_addr_to_sector_number(flash040_context_t *flash040_context, unsigned int addr)
{
    unsigned int sector_addr = flash_types[flash040_context->flash_type].sector_mask & addr;
    unsigned int sector_shift = flash_types[flash040_context->flash_type].sector_shift;

    return sector_addr >> sector_shift;
}

inline static void flash_add_sector_to_erase_mask(flash040_context_t *flash040_context, unsigned int addr)
{
    unsigned int sector_num = flash_addr_to_sector_number(flash040_context, addr);

    flash040_context->erase_mask[sector_num >> 3] |= (BYTE)(1 << (sector_num & 0x7));
}

inline static void flash_erase_sector(flash040_context_t *flash040_context, unsigned int sector)
{
    unsigned int sector_size = flash_types[flash040_context->flash_type].sector_size;
    unsigned int sector_addr;

    sector_addr = flash_sector_to_addr(flash040_context, sector);

    FLASH_DEBUG(("Erasing 0x%x - 0x%x", sector_addr, sector_addr + sector_size - 1));
    memset(&(flash040_context->flash_data[sector_addr]), 0xff, sector_size);
    flash040_context->flash_dirty = 1;
}

inline static void flash_erase_chip(flash040_context_t *flash040_context)
{
    FLASH_DEBUG(("Erasing chip"));
    memset(flash040_context->flash_data, 0xff, flash_types[flash040_context->flash_type].size);
    flash040_context->flash_dirty = 1;
}

inline static int flash_program_byte(flash040_context_t *flash040_context, unsigned int addr, BYTE byte)
{
    BYTE old_data = flash040_context->flash_data[addr];
    BYTE new_data = old_data & byte;

    FLASH_DEBUG(("Programming 0x%05x with 0x%02x (%02x->%02x)", addr, byte, old_data, old_data & byte));
    flash040_context->program_byte = byte;
    flash040_context->flash_data[addr] = new_data;
    flash040_context->flash_dirty = 1;

    return (new_data == byte) ? 1 : 0;
}

inline static int flash_write_operation_status(flash040_context_t *flash040_context)
{
    return ((flash040_context->program_byte ^ 0x80) & 0x80)   /* DQ7 = inverse of programmed data */
           | ((maincpu_clk & 2) << 5)                         /* DQ6 = toggle bit (2 us) */
           | (1 << 5)                                         /* DQ5 = timeout */
    ;
}

inline static int flash_erase_operation_status(flash040_context_t *flash040_context)
{
    int v;

    /* DQ6 = toggle bit */
    v = flash040_context->program_byte;

    /* toggle the toggle bit(s) */
    /* FIXME better toggle bit II emulation */
    flash040_context->program_byte ^= flash_types[flash040_context->flash_type].status_toggle_bits;

    /* DQ3 = sector erase timer */
    if (flash040_context->flash_state != FLASH040_STATE_SECTOR_ERASE_TIMEOUT) {
        v |= 0x08;
    }

    return v;
}

/* -------------------------------------------------------------------------- */

static void erase_alarm_handler(CLOCK offset, void *data)
{
    unsigned int i, j;
    BYTE m;
    flash040_context_t *flash040_context = (flash040_context_t *)data;

    alarm_unset(flash040_context->erase_alarm);

    FLASH_DEBUG(("Erase alarm, state %i", (int)flash040_context->flash_state));

    switch (flash040_context->flash_state) {
        case FLASH040_STATE_SECTOR_ERASE_TIMEOUT:
        case FLASH040_STATE_SECTOR_ERASE:
            for (i = 0; i < (8 * FLASH040_ERASE_MASK_SIZE); ++i) {
                j = i >> 3;
                m = (BYTE)(1 << (i & 0x7));
                if (flash040_context->erase_mask[j] & m) {
                    flash_erase_sector(flash040_context, i);
                    flash040_context->erase_mask[j] &= (BYTE) ~m;
                    break;
                }
            }

            for (i = 0, m = 0; i < FLASH040_ERASE_MASK_SIZE; ++i) {
                m |= flash040_context->erase_mask[i];
            }

            if (m != 0) {
                alarm_set(flash040_context->erase_alarm, maincpu_clk + ERASE_SECTOR_CYCLES);
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_CHIP_ERASE:
            flash_erase_chip(flash040_context);
            flash040_context->flash_state = flash040_context->flash_base_state;
            break;

        default:
            FLASH_DEBUG(("Erase alarm - error, state %i unhandled!", (int)flash040_context->flash_state));
            break;
    }
}

/* -------------------------------------------------------------------------- */

static void flash040core_store_internal(flash040_context_t *flash040_context,
                                        unsigned int addr, BYTE byte)
{
#ifdef FLASH_DEBUG_ENABLED
    flash040_state_t old_state = flash040_context->flash_state;
    flash040_state_t old_base_state = flash040_context->flash_base_state;
#endif

    switch (flash040_context->flash_state) {
        case FLASH040_STATE_READ:
            if (flash_magic_1(flash040_context, addr) && (byte == 0xaa)) {
                flash040_context->flash_state = FLASH040_STATE_MAGIC_1;
            }
            break;

        case FLASH040_STATE_MAGIC_1:
            if (flash_magic_2(flash040_context, addr) && (byte == 0x55)) {
                flash040_context->flash_state = FLASH040_STATE_MAGIC_2;
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_MAGIC_2:
            if (flash_magic_1(flash040_context, addr)) {
                switch (byte) {
                    case 0x90:
                        flash040_context->flash_state = FLASH040_STATE_AUTOSELECT;
                        flash040_context->flash_base_state = FLASH040_STATE_AUTOSELECT;
                        break;
                    case 0xf0:
                        flash040_context->flash_state = FLASH040_STATE_READ;
                        flash040_context->flash_base_state = FLASH040_STATE_READ;
                        break;
                    case 0xa0:
                        flash040_context->flash_state = FLASH040_STATE_BYTE_PROGRAM;
                        break;
                    case 0x80:
                        flash040_context->flash_state = FLASH040_STATE_ERASE_MAGIC_1;
                        break;
                    default:
                        flash040_context->flash_state = flash040_context->flash_base_state;
                        break;
                }
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_BYTE_PROGRAM:
            if (flash_program_byte(flash040_context, addr, byte)) {
                /* The byte program time is short enough to ignore */
                flash040_context->flash_state = flash040_context->flash_base_state;
            } else {
                flash040_context->flash_state = FLASH040_STATE_BYTE_PROGRAM_ERROR;
            }
            break;

        case FLASH040_STATE_ERASE_MAGIC_1:
            if (flash_magic_1(flash040_context, addr) && (byte == 0xaa)) {
                flash040_context->flash_state = FLASH040_STATE_ERASE_MAGIC_2;
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_ERASE_MAGIC_2:
            if (flash_magic_2(flash040_context, addr) && (byte == 0x55)) {
                flash040_context->flash_state = FLASH040_STATE_ERASE_SELECT;
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_ERASE_SELECT:
            if (flash_magic_1(flash040_context, addr) && (byte == 0x10)) {
                flash040_context->flash_state = FLASH040_STATE_CHIP_ERASE;
                flash040_context->program_byte = 0;
                alarm_set(flash040_context->erase_alarm, maincpu_clk + ERASE_CHIP_CYCLES);
            } else if (byte == 0x30) {
                flash_add_sector_to_erase_mask(flash040_context, addr);
                flash040_context->program_byte = 0;
                flash040_context->flash_state = FLASH040_STATE_SECTOR_ERASE_TIMEOUT;
                alarm_set(flash040_context->erase_alarm, maincpu_clk + ERASE_SECTOR_TIMEOUT_CYCLES);
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
            }
            break;

        case FLASH040_STATE_SECTOR_ERASE_TIMEOUT:
            if (byte == 0x30) {
                flash_add_sector_to_erase_mask(flash040_context, addr);
            } else {
                flash040_context->flash_state = flash040_context->flash_base_state;
                flash_clear_erase_mask(flash040_context);
                alarm_unset(flash040_context->erase_alarm);
            }
            break;

        case FLASH040_STATE_SECTOR_ERASE:
            /* TODO not all models support suspending */
            if (byte == 0xb0) {
                flash040_context->flash_state = FLASH040_STATE_SECTOR_ERASE_SUSPEND;
                alarm_unset(flash040_context->erase_alarm);
            }
            break;

        case FLASH040_STATE_SECTOR_ERASE_SUSPEND:
            if (byte == 0x30) {
                flash040_context->flash_state = FLASH040_STATE_SECTOR_ERASE;
                alarm_set(flash040_context->erase_alarm, maincpu_clk + ERASE_SECTOR_CYCLES);
            }
            break;

        case FLASH040_STATE_BYTE_PROGRAM_ERROR:
        case FLASH040_STATE_AUTOSELECT:
            if (flash_magic_1(flash040_context, addr) && (byte == 0xaa)) {
                flash040_context->flash_state = FLASH040_STATE_MAGIC_1;
            }
            if (byte == 0xf0) {
                flash040_context->flash_state = FLASH040_STATE_READ;
                flash040_context->flash_base_state = FLASH040_STATE_READ;
            }
            break;

        case FLASH040_STATE_CHIP_ERASE:
        default:
            break;
    }

    FLASH_DEBUG(("Write %02x to %05x, state %i->%i (base state %i->%i)", byte, addr, (int)old_state, (int)flash040_context->flash_state, (int)old_base_state, (int)flash040_context->flash_base_state));
}

/* -------------------------------------------------------------------------- */

void flash040core_store(flash040_context_t *flash040_context, unsigned int addr, BYTE byte)
{
    if (maincpu_rmw_flag) {
        maincpu_clk--;
        flash040core_store_internal(flash040_context, addr, flash040_context->last_read);
        maincpu_clk++;
    }

    flash040core_store_internal(flash040_context, addr, byte);
}


BYTE flash040core_read(flash040_context_t *flash040_context, unsigned int addr)
{
    BYTE value;
#ifdef FLASH_DEBUG_ENABLED
    flash040_state_t old_state = flash040_context->flash_state;
#endif

    switch (flash040_context->flash_state) {
        case FLASH040_STATE_AUTOSELECT:
            if (flash040_context->flash_type == FLASH040_TYPE_032B_A0_1_SWAP) {
                if ((addr & 0xff) < 4) {
                    addr = "\0\2\1\3"[addr & 0x3];
                }
            }

            if ((addr & 0xff) == 0) {
                value = flash_types[flash040_context->flash_type].manufacturer_ID;
            } else if ((addr & 0xff) == flash_types[flash040_context->flash_type].device_ID_addr) {
                value = flash_types[flash040_context->flash_type].device_ID;
            } else if ((addr & 0xff) == 2) {
                value = 0;
            } else {
                value = flash040_context->flash_data[addr];
            }
            break;

        case FLASH040_STATE_BYTE_PROGRAM_ERROR:
            value = flash_write_operation_status(flash040_context);
            break;

        case FLASH040_STATE_SECTOR_ERASE_SUSPEND:
        case FLASH040_STATE_CHIP_ERASE:
        case FLASH040_STATE_SECTOR_ERASE:
        case FLASH040_STATE_SECTOR_ERASE_TIMEOUT:
            value = flash_erase_operation_status(flash040_context);
            break;

        default:
        /* The state doesn't reset if a read occurs during a command sequence */
        /* fall through */
        case FLASH040_STATE_READ:
            value = flash040_context->flash_data[addr];
            break;
    }

#ifdef FLASH_DEBUG_ENABLED
    if (old_state != FLASH040_STATE_READ) {
        FLASH_DEBUG(("Read %02x from %05x, state %i->%i", value, addr, (int)old_state, (int)flash040_context->flash_state));
    }
#endif

    flash040_context->last_read = value;
    return value;
}

BYTE flash040core_peek(flash040_context_t *flash040_context, unsigned int addr)
{
    return flash040_context->flash_data[addr];
}

void flash040core_reset(flash040_context_t *flash040_context)
{
    FLASH_DEBUG(("Reset"));
    flash040_context->flash_state = FLASH040_STATE_READ;
    flash040_context->flash_base_state = FLASH040_STATE_READ;
    flash040_context->program_byte = 0;
    flash_clear_erase_mask(flash040_context);
    alarm_unset(flash040_context->erase_alarm);
}

void flash040core_init(struct flash040_context_s *flash040_context,
                       struct alarm_context_s *alarm_context,
                       flash040_type_t type, BYTE *data)
{
    FLASH_DEBUG(("Init"));
    flash040_context->flash_data = data;
    flash040_context->flash_type = type;
    flash040_context->flash_state = FLASH040_STATE_READ;
    flash040_context->flash_base_state = FLASH040_STATE_READ;
    flash040_context->program_byte = 0;
    flash_clear_erase_mask(flash040_context);
    flash040_context->flash_dirty = 0;
    flash040_context->erase_alarm = alarm_new(alarm_context, "Flash040Alarm", erase_alarm_handler, flash040_context);
}

void flash040core_shutdown(flash040_context_t *flash040_context)
{
    FLASH_DEBUG(("Shutdown"));
}

/* -------------------------------------------------------------------------- */

#define FLASH040_DUMP_VER_MAJOR   2
#define FLASH040_DUMP_VER_MINOR   0

int flash040core_snapshot_write_module(snapshot_t *s, flash040_context_t *flash040_context, const char *name)
{
    snapshot_module_t *m;
    BYTE state, base_state;

    m = snapshot_module_create(s, name, FLASH040_DUMP_VER_MAJOR, FLASH040_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    state = (BYTE)(flash040_context->flash_state);
    base_state = (BYTE)(flash040_context->flash_base_state);

    if (0
        || (SMW_B(m, state) < 0)
        || (SMW_B(m, base_state) < 0)
        || (SMW_B(m, flash040_context->program_byte) < 0)
        || (SMW_BA(m, flash040_context->erase_mask, FLASH040_ERASE_MASK_SIZE) < 0)
        || (SMW_B(m, flash040_context->last_read) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int flash040core_snapshot_read_module(snapshot_t *s, flash040_context_t *flash040_context, const char *name)
{
    BYTE vmajor, vminor, state, base_state;
    snapshot_module_t *m;

    m = snapshot_module_open(s, name, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if (vmajor != FLASH040_DUMP_VER_MAJOR) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || (SMR_B(m, &state) < 0)
        || (SMR_B(m, &base_state) < 0)
        || (SMR_B(m, &(flash040_context->program_byte)) < 0)
        || (SMR_BA(m, flash040_context->erase_mask, FLASH040_ERASE_MASK_SIZE) < 0)
        || (SMR_B(m, &(flash040_context->last_read)) < 0)) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    flash040_context->flash_state = (flash040_state_t)state;
    flash040_context->flash_base_state = (flash040_state_t)base_state;

    /* Restore alarm if needed */
    switch (flash040_context->flash_state) {
        case FLASH040_STATE_SECTOR_ERASE_TIMEOUT:
        case FLASH040_STATE_SECTOR_ERASE:
        case FLASH040_STATE_CHIP_ERASE:
            /* the alarm timing is not saved, just use some value for now */
            alarm_set(flash040_context->erase_alarm, maincpu_clk + ERASE_SECTOR_CYCLES);
            break;

        default:
            break;
    }

    return 0;
}
