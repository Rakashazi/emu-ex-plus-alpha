/*
 * spi-flash.c
 *
 * Written by
 *  Groepaz <groepaz@gmx.net>
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


/* this implements the EN25QH128A EEPROM as used by the GMod3 cartridge.
   see http://wiki.icomp.de/wiki/GMod3
   
   FIXME: only a couple commands are implemented, barely enough to make the
          examples in the ICOMP repository work, see https://svn.icomp.de/svn/gmod3/
 */

#define SPIFLASHDEBUG

#include "vice.h"

#include <stdio.h>
#include <string.h> /* for memset */

#include "log.h"
#include "spi-flash.h"
#include "resources.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"

#ifdef SPIFLASHDEBUG
#define LOG(_x_) log_debug _x_
#else
#define LOG(_x_)
#endif

#define MAX_ROM_SIZE        (16 * 1024 * 1024)
#define MAX_ROM_64K_PAGES   (MAX_ROM_SIZE / (64 * 1024))

#define SPI_2MB_FLASH_SIZE (2*1024*1024)
#define SPI_4MB_FLASH_SIZE (4*1024*1024)
#define SPI_8MB_FLASH_SIZE (8*1024*1024)
#define SPI_16MB_FLASH_SIZE (16*1024*1024)

static uint8_t *spi_flash_data = NULL;
static uint32_t spi_flash_size = 0;

static int eeprom_cs = 0;
static int eeprom_clock = 0;
static int eeprom_data_in = 0;
static int eeprom_data_out = 0;

static unsigned int input_shiftreg = 0;
static unsigned int input_count = 0;
static unsigned int output_shiftreg = 0;
static unsigned int output_count = 0;

static unsigned int command = 0;
static unsigned int addr = 0;

static int write_enable_status = 0;
static int ready_busy_status = 1;

#define FLASH_CMD_PAGE_PROGRAM  0x02
#define FLASH_CMD_READ_DATA     0x03
#define FLASH_CMD_READ_STATUS   0x05
#define FLASH_CMD_WRITE_ENABLE  0x06
#define FLASH_CMD_BLOCK_ERASE   0xd8    /* 64k erase blocks */
#define FLASH_CMD_REMS          0x9f

#define STATUSREADY 1
#define STATUSBUSY  0

static void reset_input_shiftreg(void)
{
    /* LOG(("reset_input_shiftreg")); */
    /* clear input shift register */
    input_shiftreg = 0;
    input_count = 0;
}

static void reset_output_shiftreg(void)
{
    /* LOG(("reset_output_shiftreg")); */
    /* clear output shift register */
    output_shiftreg = 0;
    output_count = 0;
}

static void shift_input_shiftreg(void)
{
    /* shift internal shift register */
    input_shiftreg <<= 1;
    /* put bit from input to shift register */
    input_shiftreg |= eeprom_data_in;
    input_count++;
    /* LOG(("shift_input_shiftreg %d %02x", input_count, input_shiftreg)); */
}

static void shift_output_shiftreg(void)
{
    if (output_count) {
        /* LOG(("shift_output_shiftreg %d %08x", output_count, output_shiftreg)); */
        /* put bit from shift register to output */
        eeprom_data_out = (output_shiftreg >> 31) & 1;
        /* shift internal shift register */
        output_shiftreg <<= 1;
        output_count--;
    } else {
        eeprom_data_out = 0;
    }
}

uint8_t spi_flash_read_data(void)
{
    if (eeprom_cs == 0) {
        /* LOG(("spi_flash_read_data %d", eeprom_data_out));         */
        return eeprom_data_out;
    }
    return 0;
}

void spi_flash_write_data(uint8_t value)
{
    if (eeprom_cs == 0) {
        /* LOG(("spi_flash_write_data %d", value)); */
        eeprom_data_in = value;
    }
}

void spi_flash_write_select(uint8_t value)
{
    /* LOG(("spi_flash_write_select %d", value)); */
    
    /* Each instruction is preceded by a rising edge on Chip Select Input with Serial Clock being held low. */
    if ((eeprom_cs == 1) && (value == 0) /* && (eeprom_clock == 0) */) {
        LOG(("spi_flash_write_select raising edge (select)"));
        reset_input_shiftreg();
        reset_output_shiftreg();
    } else if ((eeprom_cs == 0) && (value == 1)) {
        LOG(("spi_flash_write_select falling edge (deselect) command %02x", command));
        switch(command) {
            case FLASH_CMD_REMS:
                break;
            case FLASH_CMD_READ_STATUS:
                break;
            case FLASH_CMD_BLOCK_ERASE:
                addr = (input_shiftreg & 0xff0000) & (spi_flash_size - 1);
                LOG(("executing command FLASH_CMD_BLOCK_ERASE %08x (addr:%08x)", 
                     input_shiftreg, addr));
                memset(spi_flash_data + addr, 0xff, 0x10000);
                command = STATUSBUSY;
                break;
            case FLASH_CMD_WRITE_ENABLE:
                LOG(("executing command FLASH_CMD_WRITE_ENABLE"));
                write_enable_status = 1;
                break;
            case FLASH_CMD_PAGE_PROGRAM:
                LOG(("executing command FLASH_CMD_PAGE_PROGRAM"));
                command = STATUSBUSY;
                break;
            case FLASH_CMD_READ_DATA:
                LOG(("closing command FLASH_CMD_READ_DATA"));
                command = STATUSBUSY;
                break;
            default:
                log_error(LOG_DEFAULT, "spi_flash_write_select: unknown flash command: %02x", command);
                break;
        }
    }
    eeprom_cs = value;
}

void spi_flash_write_clock(uint8_t value)
{
    /* LOG(("spi_flash_write_clock %d", value)); */

    /* rising edge of clock will read one bit from data input */
    if ((eeprom_cs == 0) && (value == 1) && (eeprom_clock == 0)) {
        
            shift_input_shiftreg();
            switch (input_count) {
                case 8:
                    /* LOG(("got byte 1: %02x\n", input_shiftreg)); */
                    if (command == FLASH_CMD_PAGE_PROGRAM) {
                        addr &= (spi_flash_size - 1);
                        LOG(("writing byte: %02x->%02x %08x", 
                             spi_flash_data[addr], 
                             spi_flash_data[addr] & input_shiftreg, 
                             addr));
                        spi_flash_data[addr] &= input_shiftreg;
                        addr++;
                        reset_input_shiftreg();
                    } else if (command == FLASH_CMD_READ_DATA) {
                        addr &= (spi_flash_size - 1);
                        output_shiftreg = spi_flash_data[addr] << 24;
                        output_count = 8;
                        LOG(("reading byte: %02x %08x", output_shiftreg, addr));
                        addr++;
                        reset_input_shiftreg();
                    } else {
                        switch(input_shiftreg) {
                            case FLASH_CMD_REMS:
                                LOG(("got cmd FLASH_CMD_REMS"));
                                command = FLASH_CMD_REMS;
                                break;
                            case FLASH_CMD_READ_STATUS:
                                LOG(("got cmd FLASH_CMD_READ_STATUS"));
                                command = FLASH_CMD_READ_STATUS;
                                /* reading stats will work without deselecting first */
                                output_shiftreg = 0x01000000;
                                output_count = (1 * 8);
                                break;
                            case FLASH_CMD_BLOCK_ERASE:
                                LOG(("got cmd FLASH_CMD_BLOCK_ERASE"));
                                command = FLASH_CMD_BLOCK_ERASE;
                                break;
                            case FLASH_CMD_WRITE_ENABLE:
                                LOG(("got cmd FLASH_CMD_WRITE_ENABLE"));
                                command = FLASH_CMD_WRITE_ENABLE;
                                break;
                            case FLASH_CMD_PAGE_PROGRAM:
                                LOG(("got cmd FLASH_CMD_PAGE_PROGRAM"));
                                command = FLASH_CMD_PAGE_PROGRAM;
                                break;
                            case FLASH_CMD_READ_DATA:
                                LOG(("got cmd FLASH_CMD_READ_DATA"));
                                command = FLASH_CMD_READ_DATA;
                                break;
                            default:
                                log_error(LOG_DEFAULT, "spi_flash_write_clock: unknown flash command: %02x\n", input_shiftreg);
                                reset_input_shiftreg();
                                break;
                        }
                    }
                    break;
                case 16:
                    /* LOG(("got byte 2: %04x\n", input_shiftreg)); */
                    break;
                case 24:
                    /* LOG(("got byte 3: %06x\n", input_shiftreg)); */
                    break;
                case 32:
                    /* LOG(("got byte 4: %08x\n", input_shiftreg)); */
                    
                    switch(command) {
                        /* reading id will work without deselecting first */
                        case FLASH_CMD_REMS:
                            /*
                                0: manufacturer ID ($1c)
                                1: device ID ($70)
                                2: capacity ($18/24 - 2^24, 16MB)
                            */
                            switch (spi_flash_size) {
                                case SPI_2MB_FLASH_SIZE:
                                    output_shiftreg = 0x1c700300;
                                    break;
                                case SPI_4MB_FLASH_SIZE:
                                    output_shiftreg = 0x1c700600;
                                    break;
                                case SPI_8MB_FLASH_SIZE:
                                    output_shiftreg = 0x1c700c00;
                                    break;
                                case SPI_16MB_FLASH_SIZE:
                                    output_shiftreg = 0x1c701800;
                                    break;
                                default:
                                    LOG(("unsupported flash size: %08x", spi_flash_size));
                                    output_shiftreg = 0x1c701800;   /* FIXME */
                                    break;
                            }
                            output_count = (3 * 8);
                            command = STATUSBUSY;
                            LOG(("executing command FLASH_CMD_REMS"));
                            break;
                        case FLASH_CMD_BLOCK_ERASE:
                            LOG(("got addr command FLASH_CMD_BLOCK_ERASE %08x", input_shiftreg));
                            break;
                        case FLASH_CMD_PAGE_PROGRAM:
                            LOG(("got addr command FLASH_CMD_PAGE_PROGRAM %08x", input_shiftreg));
                            addr = input_shiftreg & (spi_flash_size - 1);
                            reset_input_shiftreg();
                            break;
                        case FLASH_CMD_READ_DATA:
                            LOG(("got addr command FLASH_CMD_READ_DATA %08x", input_shiftreg));
                            addr = input_shiftreg & (spi_flash_size - 1);
                            output_shiftreg = spi_flash_data[addr] << 24;
                            output_count = 8;
                            LOG(("reading byte: %02x %08x", output_shiftreg, addr));
                            addr++;
                            reset_input_shiftreg();
                            break;
                        default:
                            log_error(LOG_DEFAULT, "spi_flash_write_clock: unknown flash command: %02x\n", input_shiftreg);
                            reset_input_shiftreg();
                            break;
                    }
                    
                    break;
            }
            shift_output_shiftreg();
    }

    eeprom_clock = value;
}

void spi_flash_set_image(uint8_t *img, uint32_t size)
{
    spi_flash_data = img;
    spi_flash_size = size;
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

#define CART_DUMP_VER_MAJOR   0
#define CART_DUMP_VER_MINOR   1
#define SNAP_MODULE_NAME  "EN25QH128A"

int spi_flash_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;

    m = snapshot_module_create(s, SNAP_MODULE_NAME,
                               CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR);
    if (m == NULL) {
        return -1;
    }

    if (0
        || SMW_B(m, eeprom_cs) < 0
        || SMW_B(m, eeprom_clock) < 0
        || SMW_B(m, eeprom_data_in) < 0
        || SMW_B(m, eeprom_data_out) < 0
        || SMW_B(m, input_shiftreg) < 0
        || SMW_B(m, input_count) < 0
        || SMW_B(m, output_shiftreg) < 0
        || SMW_B(m, output_count) < 0
        || SMW_B(m, command) < 0
        || SMW_B(m, addr) < 0
        || SMW_B(m, write_enable_status) < 0
        || SMW_B(m, ready_busy_status) < 0
        || SMW_BA(m, spi_flash_data, MAX_ROM_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);
    return 0;
}

int spi_flash_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;

    m = snapshot_module_open(s, SNAP_MODULE_NAME, &vmajor, &vminor);
    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, CART_DUMP_VER_MAJOR, CART_DUMP_VER_MINOR)) {
        snapshot_module_close(m);
        return -1;
    }

    if (0
        || SMR_B_INT(m, &eeprom_cs) < 0
        || SMR_B_INT(m, &eeprom_clock) < 0
        || SMR_B_INT(m, &eeprom_data_in) < 0
        || SMR_B_INT(m, &eeprom_data_out) < 0
        || SMR_B_INT(m, (int*)&input_shiftreg) < 0
        || SMR_B_INT(m, (int*)&input_count) < 0
        || SMR_B_INT(m, (int*)&output_shiftreg) < 0
        || SMR_B_INT(m, (int*)&output_count) < 0
        || SMR_B_INT(m, (int*)&command) < 0
        || SMR_B_INT(m, (int*)&addr) < 0
        || SMR_B_INT(m, &write_enable_status) < 0
        || SMR_B_INT(m, &ready_busy_status) < 0
        || SMR_BA(m, spi_flash_data, MAX_ROM_SIZE) < 0) {
        snapshot_module_close(m);
        return -1;
    }

    snapshot_module_close(m);

    return 0;
}
