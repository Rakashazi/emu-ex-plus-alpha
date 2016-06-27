/*
 * vicii-clock-stretch.c - 8502 clock cycle strechting routines.
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

#include <string.h>

#include "alarm.h"
#include "maincpu.h"
#include "vicii.h"
#include "viciitypes.h"

CLOCK vicii_clock_add(CLOCK clock, int amount)
{
    CLOCK tmp_clock = clock;

    if (vicii.fastmode != 0) {
        if (amount > 0) {
            tmp_clock += (amount >> 1);
            vicii.half_cycles += amount & 1;
            if (vicii.half_cycles > 1) {
                tmp_clock++;
                vicii.half_cycles = 0;
            }
        } else {
            tmp_clock -= ((-amount) >> 1);
            vicii.half_cycles -= (-amount) & 1;
            if (vicii.half_cycles < 0) {
                tmp_clock--;
                vicii.half_cycles = 1;
            }
        }
    } else {
        tmp_clock += amount;
    }
    return tmp_clock;
}

/* if half cycle is 0, add extra half cycle to stretch */
void vicii_clock_read_stretch(void)
{
    int current;

    /* call alarm handler to handle any memory refresh stretching */
    vicii_memory_refresh_alarm_handler();

    if (vicii.fastmode != 0) {
        if (vicii.half_cycles == 0) {
            vicii.half_cycles++;
            maincpu_stretch = 1;
        }
    }

    if (maincpu_clk == c128cpu_memory_refresh_clk) {
        current = c128cpu_memory_refresh_clk % vicii.cycles_per_line;
        if (current + 1 > 15) {
            /* push alarm to the next line */
            c128cpu_memory_refresh_clk += vicii.cycles_per_line - current + 11;
        } else {
            /* push alarm to the next cycle */
            c128cpu_memory_refresh_clk++;
        }
    }
}

/* add 1 full cycle for 2 cycle stretch if rmw,
   otherwise add half cycle for 1 cycle stretch */
void vicii_clock_write_stretch(void)
{
    int current;

    if (maincpu_rmw_flag == 0) {
        /* subtract a (half) cycle and use the read stretch to handle it */
        maincpu_clk = vicii_clock_add(maincpu_clk, -1);
        vicii_clock_read_stretch();

        /* add previously subtracted (half) cycle */
        maincpu_clk = vicii_clock_add(maincpu_clk, 1);
    } else {
        /* the 'read' will already have taken care of any previous memory
           refresh stretches */
        current = c128cpu_memory_refresh_clk % vicii.cycles_per_line;
        if (maincpu_clk - 1 == c128cpu_memory_refresh_clk) {
            if (current + 2 > 15) {
                /* push alarm to the next line */
                c128cpu_memory_refresh_clk += vicii.cycles_per_line - current + 11;
            } else {
                /* push alarm forward 2 cycles */
                c128cpu_memory_refresh_clk += 2;
            }
        }
        if (maincpu_clk == c128cpu_memory_refresh_clk) {
            if (current + 1 > 15) {
                /* push alarm to the next line */
                c128cpu_memory_refresh_clk += vicii.cycles_per_line - current + 11;
            } else {
                /* push alarm forward 1 cycle */
                c128cpu_memory_refresh_clk++;
            }
        }

        if (vicii.fastmode != 0) {
            /* handle I/O access stretch for rmw */
            maincpu_clk++;
        }
    }
}

int vicii_get_half_cycle(void)
{
    if (vicii.fastmode != 0) {
        return vicii.half_cycles;
    }
    return -1;
}

void vicii_memory_refresh_alarm_handler(void)
{
    int offset;
    int amount;
    int current_refresh;
    int current_cycle;

    offset = maincpu_clk - c128cpu_memory_refresh_clk;

    if (offset >= 0) {
        current_refresh = c128cpu_memory_refresh_clk % vicii.cycles_per_line;
        current_cycle = maincpu_clk % vicii.cycles_per_line;

        if (vicii.fastmode == 0) {
            if (current_cycle > 15) {
                /* push alarm to the next line */
                c128cpu_memory_refresh_clk += vicii.cycles_per_line - current_refresh + 11;
            } else {
                /* push alarm to the next cycle to check */
                c128cpu_memory_refresh_clk += offset + 1;
            }
        } else {
            amount = (offset * 2) + vicii.half_cycles;
            if (amount > 0) {
                if (current_refresh + amount > 15) {
                    /* stretch max memory refresh cycles */
                    maincpu_clk = vicii_clock_add(maincpu_clk, 15 - current_refresh + 1);

                    /* push alarm to the next line */
                    c128cpu_memory_refresh_clk += vicii.cycles_per_line - current_refresh + 11;
                } else {
                    /* stretch the amount of half cycles */
                    maincpu_clk = vicii_clock_add(maincpu_clk, amount);

                    /* push alarm to the cycle after the next cycle to check */
                    c128cpu_memory_refresh_clk += amount;
                }
            }
        }
    }
}

int vicii_check_memory_refresh(CLOCK clock)
{
    int current;

    if (vicii.fastmode == 0) {
        return 0;
    }

    if (clock == c128cpu_memory_refresh_clk) {
        vicii_clock_add(maincpu_clk, 1);
        current = c128cpu_memory_refresh_clk % vicii.cycles_per_line;
        if (current + 1 > 15) {
            /* push alarm to the next line */
            c128cpu_memory_refresh_clk += vicii.cycles_per_line - current + 11;
        } else {
            /* push alarm to the next cycle */
            c128cpu_memory_refresh_clk++;
        }
        return 1;
    }
    return 0;
}
