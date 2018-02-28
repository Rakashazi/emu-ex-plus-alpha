/*
 * m93c86.c
 *
 * Written by
 *  Groepaz/Hitmen <groepaz@gmx.net>
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


/* this implements the M93C86 EEPROM in 16 bit mode as used by the GMod2 cartridge.
   other types (M93C86-x M93C76-x M93C66-x M93C56-x M93C46-x) can probably be 
   supported with some reworking too (if we ever need it) */

/* #define M93C86DEBUG */

#include "vice.h"

#include <stdio.h>
#include <string.h> /* for memset */

#include "log.h"
#include "m93c86.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

#ifdef M93C86DEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

/* FIXME get rid of this */
#define M93C86_SIZE (2048)

/* Image file */
static FILE *m93c86_image_file = NULL;

static BYTE m93c86_data[M93C86_SIZE];

static int eeprom_cs = 0;
static int eeprom_clock = 0;
static int eeprom_data_in = 0;
static int eeprom_data_out = 0;

static unsigned int input_shiftreg = 0;
static unsigned int input_count = 0;
static unsigned int output_shiftreg = 0;
static unsigned int output_count = 0;

static int command = 0;
static int addr = 0;
static int data0 = 0;
static int data1 = 0;

static int write_enable_status = 0;
static int ready_busy_status = 1;

#define CMD00 1
#define CMDWRITE 2
#define CMDREAD 3
#define CMDERASE 4
#define CMDWEN 5
#define CMDWDS 6
#define CMDERAL 7
#define CMDWRAL 8
#define CMDREADDUMMY 9
#define CMDREADDATA 10
#define CMDISBUSY 11
#define CMDISREADY 12

#define STATUSREADY 1
#define STATUSBUSY 0

static void reset_input_shiftreg(void)
{
    /* clear input shift register */
    input_shiftreg = 0;
    input_count = 0;
}

BYTE m93c86_read_data(void)
{
    if (eeprom_cs == 1) {
        switch (command) {
            case CMDISBUSY:
                /* the software will see one busy state for one read, this is not
                   quite what really happens */
                    LOG(("busy status is 1"));
                    command = CMDISREADY;
                    return STATUSBUSY;
                break;
            case CMDISREADY:
                    LOG(("busy status is 0, end of command"));
                    ready_busy_status = STATUSREADY;
                    command = 0;
                    return STATUSREADY;
                break;
            default:
                return eeprom_data_out;
                break;
        }
    } else {
        return 0;
    }
}

void m93c86_write_data(BYTE value)
{
    if (eeprom_cs == 1) {
        eeprom_data_in = value;
    }
}

void m93c86_write_select(BYTE value)
{
    /* Each instruction is preceded by a rising edge on Chip Select Input with Serial Clock being held low. */
    if ((eeprom_cs == 0) && (value == 1) && (eeprom_clock == 0)) {
        reset_input_shiftreg();
    } else if ((eeprom_cs == 1) && (value == 0)) {
        /* a write or erase command kicks off on falling edge on CS and then signals busy state */
        switch (command) {
            case CMDWRITE:
            case CMDWRAL:
            case CMDERAL:
                command = CMDISBUSY;
                break;
        }
    }
    eeprom_cs = value;
    if (eeprom_cs == 0) {
        /* read command is aborted when CS goes low */
        if ((command == CMDREAD) || (command == CMDREADDUMMY) || (command == CMDREADDATA)) {
            command = 0;
        }
    }
}

void m93c86_write_clock(BYTE value)
{
    /* rising edge of clock will read one bit from data input */
    if ((eeprom_cs == 1) && (value == 1) && (eeprom_clock == 0)) {
        if (command == CMDREADDUMMY) {
            /* FIXME: this is kinda hackery, but works. *shrug* */
            output_shiftreg = m93c86_data[(addr << 1)];

            eeprom_data_out = 0;
            output_count = 0;

            eeprom_data_out = (output_shiftreg >> 7) & 1;
            LOG(("output %d pos %d addr %04x", eeprom_data_out, output_count, addr));
            output_shiftreg <<= 1;
            output_count++;

            command = CMDREADDATA;
            LOG(("load output from %04x with %02x", addr, output_shiftreg));
        } else if (command == CMDREADDATA) {
            eeprom_data_out = (output_shiftreg >> 7) & 1;
            output_shiftreg <<= 1;
            output_count++;
            switch(output_count) {
                case 8:
                    output_shiftreg = m93c86_data[(addr << 1) + 1];
                    LOG(("reload output from %04x with %02x", addr, output_shiftreg));
                    break;
                case 16:
                    addr = (addr + 1) & ((M93C86_SIZE / 2) - 1);
                    output_shiftreg = m93c86_data[(addr << 1)];
                    output_count = 0;
                    LOG(("reload output from %04x with %02x", addr, output_shiftreg));
                    break;
            }
        } else {
            /* shift internal shift register */
            input_shiftreg <<= 1;
            /* put bit from input to shift register */
            input_shiftreg |= eeprom_data_in;
            input_count++;
            switch(input_count) {
                case 1: /* start bit */
                    if (eeprom_data_in == 0) {
                        reset_input_shiftreg();
                    }
                    break;
                case 3: /* 2 command bits recieved */
                    switch (input_shiftreg) {
                        case 0x04: /* 100 */
                            command = CMD00;
                            break;
                        case 0x05: /* 101 */
                            command = CMDWRITE;
                            break;
                        case 0x06: /* 110 */
                            command = CMDREAD;
                            break;
                        case 0x07: /* 111 */
                            command = CMDERASE;
                            break;
                    }
                    LOG(("first three command bits are: %x", input_shiftreg));
                    break;
                case 5: /* 5 command bits recieved */
                    if (command == CMD00) {
                        switch (input_shiftreg) {
                            case 0x10: /* 10000 */
                                command = CMDWDS;
                                break;
                            case 0x11: /* 10001 */
                                command = CMDWRAL;
                                break;
                            case 0x12: /* 10010 */
                                command = CMDERAL;
                                break;
                            case 0x13: /* 10011 */
                                command = CMDWEN;
                                write_enable_status = 1;
                                break;
                        }
                        LOG(("first five command bits are: %x", input_shiftreg));
                    }
                    break;
                case 13:
                    switch (command) {
                        case CMDREAD:
                            command = CMDREADDUMMY;
                            addr = ((input_shiftreg >> 0) & 0x3ff);
                            reset_input_shiftreg();
                            LOG(("CMD: read addr %04x", addr));
                            break;
                        case CMDWDS:
                            write_enable_status = 0;
                            reset_input_shiftreg();
                            command = 0;
                            LOG(("CMD: write disable"));
                            break;
                        case CMDWEN:
                            write_enable_status = 1;
                            reset_input_shiftreg();
                            command = 0;
                            LOG(("CMD: write enable"));
                            break;
                        case CMDERAL:
                            if (write_enable_status == 0) {
                                log_error(LOG_DEFAULT, "EEPROM: write not permitted for CMD 'erase all'");
                                reset_input_shiftreg();
                                command = 0;
                            } else {
                                ready_busy_status = STATUSBUSY;
                                reset_input_shiftreg();
                                memset(m93c86_data, 0xff, M93C86_SIZE);
                                LOG(("CMD: erase all"));
                            }
                            break;
                    }
                    break;
                case 29:
                    switch (command) {
                        case CMDWRITE:
                            if (write_enable_status == 0) {
                                log_error(LOG_DEFAULT, "EEPROM: write not permitted for CMD 'write'");
                                reset_input_shiftreg();
                                command = 0;
                            } else {
                                addr = ((input_shiftreg >> 16) & 0x3ff);
                                data0 = ((input_shiftreg >> 8) & 0xff);
                                data1 = ((input_shiftreg >> 0) & 0xff);
                                ready_busy_status = STATUSBUSY;
                                reset_input_shiftreg();
                                m93c86_data[(addr << 1)] = data0;
                                m93c86_data[(addr << 1) + 1] = data1;
                                LOG(("CMD: write addr %04x %02x %02x", addr, data0, data1));
                            }
                            break;
                        case CMDWRAL:
                            if (write_enable_status == 0) {
                                log_error(LOG_DEFAULT, "EEPROM: write not permitted for CMD 'write all'");
                                reset_input_shiftreg();
                                command = 0;
                            } else {
                                data0 = ((input_shiftreg >> 8) & 0xff);
                                data1 = ((input_shiftreg >> 0) & 0xff);
                                ready_busy_status = STATUSBUSY;
                                reset_input_shiftreg();
                                for (addr = 0; addr < (M93C86_SIZE / 2); addr++) {
                                    m93c86_data[(addr << 1)] = data0;
                                    m93c86_data[(addr << 1) + 1] = data1;
                                }
                                LOG(("CMD: write all %02x %02x", data0, data1));
                            }
                            break;
                    }
                    break;
            }
        }
    }
    eeprom_clock = value;
}

int m93c86_open_image(char *name, int rw)
{
    char *m93c86_image_filename = name;

    if (m93c86_image_filename != NULL) {
        /* FIXME */
    } else {
        /* FIXME */
        log_debug("eeprom card image name not set");
        return 0;
    }

    if (m93c86_image_file != NULL) {
        m93c86_close_image(rw);
    }

    if (rw) {
        m93c86_image_file = fopen(m93c86_image_filename, "rb+");
    }

    if (m93c86_image_file == NULL) {
        m93c86_image_file = fopen(m93c86_image_filename, "rb");

        if (m93c86_image_file == NULL) {
            log_debug("could not open eeprom card image: %s", m93c86_image_filename);
            return -1;
        } else {
            if (fread(m93c86_data, 1, M93C86_SIZE, m93c86_image_file) == 0) {
                log_debug("could not read eeprom card image: %s", m93c86_image_filename);
            }
            fseek(m93c86_image_file, 0, SEEK_SET);
            log_debug("opened eeprom card image (ro): %s", m93c86_image_filename);
        }
    } else {
        if (fread(m93c86_data, 1, M93C86_SIZE, m93c86_image_file) == 0) {
            log_debug("could not read eeprom card image: %s", m93c86_image_filename);
        }
        fseek(m93c86_image_file, 0, SEEK_SET);
        log_debug("opened eeprom card image (rw): %s", m93c86_image_filename);
    }
    return 0;
}

void m93c86_close_image(int rw)
{
    /* unmount EEPROM image */
    if (m93c86_image_file != NULL) {
        if (rw) {
            fseek(m93c86_image_file, 0, SEEK_SET);
            if (fwrite(m93c86_data, 1, M93C86_SIZE, m93c86_image_file) == 0) {
                log_debug("could not write eeprom card image");
            }
        }
        fclose(m93c86_image_file);
        m93c86_image_file = NULL;
    }
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   0
#define SNAP_MODULE_NAME  "M93C86"

/* FIXME: implement snapshot support */
int m93c86_snapshot_write_module(snapshot_t *s)
{
    return -1;
#if 0
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}

int m93c86_snapshot_read_module(snapshot_t *s)
{
    return -1;
#if 0
    BYTE vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    if ((vmajor != CART_DUMP_VER_MAJOR) || (vminor != CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
#endif
}
