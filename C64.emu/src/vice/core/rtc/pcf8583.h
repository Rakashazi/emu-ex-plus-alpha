/*
 * pcf8583.h - PCF8583 RTC emulation.
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

#ifndef VICE_PCF8583_H
#define VICE_PCF8583_H

#include <time.h>

#include "snapshot.h"
#include "types.h"

enum {
    PCF8583_IDLE = 0,
    PCF8583_GET_ADDRESS,
    PCF8583_GET_REG_NR,
    PCF8583_READ_REGS,
    PCF8583_WRITE_REGS,
    PCF8583_ADDRESS_READ_ACK,
    PCF8583_ADDRESS_WRITE_ACK,
    PCF8583_REG_NR_ACK,
    PCF8583_WRITE_ACK,
    PCF8583_READ_ACK,
    PCF8583_READ_REGS_TRAIN
};

#define PCF8583_RAM_SIZE  240
#define PCF8583_REG_SIZE   16

typedef struct rtc_pcf8583_s {
    int clock_halt;
    time_t clock_halt_latch;
    int am_pm;
    int read_bit_shift;
    time_t latch;
    time_t offset;
    time_t old_offset;
    uint8_t *clock_regs;
    uint8_t old_clock_regs[PCF8583_REG_SIZE];
    uint8_t clock_regs_for_read[PCF8583_REG_SIZE];
    uint8_t *ram;
    uint8_t old_ram[PCF8583_RAM_SIZE];
    uint8_t state;
    uint8_t reg;
    uint8_t reg_ptr;
    uint8_t bit;
    uint8_t io_byte;
    uint8_t sclk_line;
    uint8_t data_line;
    uint8_t clock_register;
    char *device;
} rtc_pcf8583_t;

rtc_pcf8583_t *pcf8583_init(char *device, int read_bit_shift);
void pcf8583_destroy(rtc_pcf8583_t *context, int save);

void pcf8583_set_clk_line(rtc_pcf8583_t *context, uint8_t data);
void pcf8583_set_data_line(rtc_pcf8583_t *context, uint8_t data);

uint8_t pcf8583_read_data_line(rtc_pcf8583_t *context);

int pcf8583_write_snapshot(rtc_pcf8583_t *context, snapshot_t *s);
int pcf8583_read_snapshot(rtc_pcf8583_t *context, snapshot_t *s);

#endif
