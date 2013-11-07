/*
 * rotation.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 * 1541 circuitry simulation code by
 *  Istvan Fabian <if@caps-project.org>
 *  Benjamin 'BeRo' Rosseaux <benjamin@rosseaux.com>
 * GCR Hardware tests by
 *  Peter Rittwage <peter@rittwage.com>
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

#include "drive.h"
#include "drivetypes.h"
#include "rotation.h"
#include "types.h"
#include "p64.h"

#include <stdlib.h>

#define ACCUM_MAX 0x10000

#define ROTATION_TABLE_SIZE 0x1000


struct rotation_s {
    DWORD accum;
    CLOCK rotation_last_clk;

    unsigned int last_read_data;
    BYTE last_write_data;
    int bit_counter;
    int zero_count;

    int frequency; /* 1x/2x speed toggle, index to rot_speed_bps */
    int speed_zone; /* speed zone within rot_speed_bps */

    int ue7_dcba; /* UE7 input BA, counter b1/b0, connected to UCD4 PB6/PB5, DC=0 */
    int ue7_counter; /* UE7 4 bit counter state */
    int uf4_counter; /* UF4 4 bit counter state */
    DWORD fr_randcount; /* counter distance of the last real flux reversal detected from the disk */

    int filter_counter; /* flux filter ignore cycle count */
    int filter_state; /* flux filter current state */
    int filter_last_state; /* flux filter last state */

    int write_flux; /* write flux bit state */

    int so_delay; /* so signal delay */

    DWORD cycle_index; /* cycle_index */

    int ref_advance; /* reference cycles already simulated, e.g. when emulating bus delay */

    DWORD PulseHeadPosition;

    DWORD seed;

    DWORD xorShift32;
};
typedef struct rotation_s rotation_t;


static rotation_t rotation[DRIVE_NUM];

/* Speed (in bps) of the disk in the 4 disk areas.  */
static const int rot_speed_bps[2][4] = { { 250000, 266667, 285714, 307692 },
                                         { 125000, 133333, 142857, 153846 } };


void rotation_init(int freq, unsigned int dnr)
{
    rotation[dnr].frequency = freq;
    rotation[dnr].accum = 0;
    rotation[dnr].ue7_counter = 0;
    rotation[dnr].uf4_counter = 0;
    rotation[dnr].fr_randcount = 0;
    rotation[dnr].xorShift32 = 0x1234abcd;
    rotation[dnr].filter_counter = 0;
    rotation[dnr].filter_state = 0;
    rotation[dnr].filter_last_state = 0;
    rotation[dnr].write_flux = 0;
    rotation[dnr].PulseHeadPosition = 0;
    rotation[dnr].so_delay = 0;
    rotation[dnr].cycle_index = 0;
    rotation[dnr].ref_advance = 0;
}

void rotation_reset(drive_t *drive)
{
    unsigned int dnr;

    dnr = drive->mynumber;

    rotation[dnr].last_read_data = 0;
    rotation[dnr].last_write_data = 0;
    rotation[dnr].bit_counter = 0;
    rotation[dnr].accum = 0;
    rotation[dnr].seed = 0;
    rotation[dnr].xorShift32 = 0x1234abcd;
    rotation[dnr].rotation_last_clk = *(drive->clk);
    rotation[dnr].ue7_counter = 0;
    rotation[dnr].uf4_counter = 0;
    rotation[dnr].fr_randcount = 0;
    rotation[dnr].filter_counter = 0;
    rotation[dnr].filter_state = 0;
    rotation[dnr].filter_last_state = 0;
    rotation[dnr].write_flux = 0;
    rotation[dnr].PulseHeadPosition = 0;
    rotation[dnr].so_delay = 0;
    rotation[dnr].cycle_index = 0;
    rotation[dnr].ref_advance = 0;

    drive->req_ref_cycles = 0;
}

void rotation_speed_zone_set(unsigned int zone, unsigned int dnr)
{
    rotation[dnr].speed_zone = zone;
    rotation[dnr].ue7_dcba = zone & 3;
}

void rotation_table_get(DWORD *rotation_table_ptr)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        rotation_table_ptr[dnr] = rotation[dnr].speed_zone;

        drive->snap_accum = rotation[dnr].accum;
        drive->snap_rotation_last_clk = rotation[dnr].rotation_last_clk;
        drive->snap_last_read_data = rotation[dnr].last_read_data;
        drive->snap_last_write_data = rotation[dnr].last_write_data;
        drive->snap_bit_counter = rotation[dnr].bit_counter;
        drive->snap_zero_count = rotation[dnr].zero_count;
        drive->snap_seed = rotation[dnr].seed;
        drive->snap_speed_zone = rotation[dnr].speed_zone;
        drive->snap_ue7_dcba = rotation[dnr].ue7_dcba;
        drive->snap_ue7_counter = rotation[dnr].ue7_counter;
        drive->snap_uf4_counter = rotation[dnr].uf4_counter;
        drive->snap_fr_randcount = rotation[dnr].fr_randcount;
        drive->snap_filter_counter = rotation[dnr].filter_counter;
        drive->snap_filter_state = rotation[dnr].filter_state;
        drive->snap_filter_last_state = rotation[dnr].filter_last_state;
        drive->snap_write_flux = rotation[dnr].write_flux;
        drive->snap_PulseHeadPosition = rotation[dnr].PulseHeadPosition;
        drive->snap_xorShift32 = rotation[dnr].xorShift32;
        drive->snap_so_delay = rotation[dnr].so_delay;
        drive->snap_cycle_index = rotation[dnr].cycle_index;
        drive->snap_ref_advance = rotation[dnr].ref_advance;
        drive->snap_req_ref_cycles = drive->req_ref_cycles;
    }
}

void rotation_table_set(DWORD *rotation_table_ptr)
{
    unsigned int dnr;
    drive_t *drive;

    for (dnr = 0; dnr < DRIVE_NUM; dnr++) {
        drive = drive_context[dnr]->drive;

        rotation[dnr].speed_zone = rotation_table_ptr[dnr];

        rotation[dnr].accum = drive->snap_accum;
        rotation[dnr].rotation_last_clk = drive->snap_rotation_last_clk;
        rotation[dnr].last_read_data = drive->snap_last_read_data;
        rotation[dnr].last_write_data = drive->snap_last_write_data;
        rotation[dnr].bit_counter = drive->snap_bit_counter;
        rotation[dnr].zero_count = drive->snap_zero_count;
        rotation[dnr].seed = drive->snap_seed;
        rotation[dnr].speed_zone = drive->snap_speed_zone;
        rotation[dnr].ue7_dcba = drive->snap_ue7_dcba;
        rotation[dnr].ue7_counter = drive->snap_ue7_counter;
        rotation[dnr].uf4_counter = drive->snap_uf4_counter;
        rotation[dnr].fr_randcount = drive->snap_fr_randcount;
        rotation[dnr].filter_counter = drive->snap_filter_counter;
        rotation[dnr].filter_state = drive->snap_filter_state;
        rotation[dnr].filter_last_state = drive->snap_filter_state;
        rotation[dnr].write_flux = drive->snap_write_flux;
        rotation[dnr].PulseHeadPosition = drive->snap_PulseHeadPosition;
        rotation[dnr].xorShift32 = drive->snap_xorShift32;
        rotation[dnr].so_delay = drive->snap_so_delay;
        rotation[dnr].cycle_index = drive->snap_cycle_index;
        rotation[dnr].ref_advance = drive->snap_ref_advance;
        drive->req_ref_cycles = drive->snap_req_ref_cycles;
    }
}

void rotation_overflow_callback(CLOCK sub, unsigned int dnr)
{
    rotation[dnr].rotation_last_clk -= sub;
}

inline static void write_next_bit(drive_t *dptr, int value)
{
    int off = dptr->GCR_head_offset;
    int byte_offset = off >> 3;
    int bit = (~off) & 7;

    /* if no image is attached, writes do nothing */
    if (dptr->GCR_image_loaded == 0) {
        return;
    }

    off++;
    if (off >= (int)(dptr->GCR_current_track_size << 3)) {
        off = 0;
    }
    dptr->GCR_head_offset = off;

    /* track does not exists */
    if (dptr->GCR_track_start_ptr == NULL) {
        return;
    }
    dptr->GCR_dirty_track = 1;
    if (value) {
        dptr->GCR_track_start_ptr[byte_offset] |= 1 << bit;
    } else {
        dptr->GCR_track_start_ptr[byte_offset] &= ~(1 << bit);
    }
}

inline static int read_next_bit(drive_t *dptr)
{
    int off = dptr->GCR_head_offset;
    int byte_offset = off >> 3;
    int bit = (~off) & 7;

    /* if no image is attached, read 0 */
    if (dptr->GCR_image_loaded == 0) {
        return 0;
    }

    off++;
    if (off >= (int)(dptr->GCR_current_track_size << 3)) {
        off = 0;
    }
    dptr->GCR_head_offset = off;

    /* track does not exists */
    if (dptr->GCR_track_start_ptr == NULL) {
        return 0;
    }
    return (dptr->GCR_track_start_ptr[byte_offset] >> bit) & 1;
}

inline static SDWORD RANDOM_nextInt(rotation_t *rptr)
{
    DWORD bits = rptr->seed >> 15;
    rptr->seed ^= rptr->accum;
    rptr->seed = rptr->seed << 17 | bits;
    return (SDWORD) rptr->seed;
}

inline static DWORD RANDOM_nextUInt(rotation_t *rptr)
{
    rptr->xorShift32 ^= (rptr->xorShift32 << 13);
    rptr->xorShift32 ^= (rptr->xorShift32 >> 17);
    return rptr->xorShift32 ^= (rptr->xorShift32 << 5);
}

void rotation_begins(drive_t *dptr)
{
    unsigned int dnr = dptr->mynumber;
    rotation[dnr].rotation_last_clk = *(dptr->clk);
    rotation[dnr].cycle_index = 0;
}

/* 1541 circuit simulation for GCR-based images, see 1541 circuit description in
   this file for details */
void rotation_1541_gcr(drive_t *dptr, int ref_cycles)
{
    rotation_t *rptr;
    int clk_ref_per_rev, cyc_act_frv;
    unsigned int todo;
    SDWORD delta;
    DWORD count_new_bitcell, cyc_sum_frv /*, sum_new_bitcell*/;
    unsigned int dnr = dptr->mynumber;

    rptr = &rotation[dptr->mynumber];

    /* drive speed is 300RPM, that is 300/60=5 revolutions per second
     * reference clock is 16MHz, one revolution has 16MHz/5 reference cycles
     */
    clk_ref_per_rev = 16000000 / (300 / 60);

    /* cell cycles for the actual flux reversal period, it is 1 now, but could be different with variable density */
    cyc_act_frv = 1;

    /* the count to reach for a new bitcell */
    count_new_bitcell = cyc_act_frv * clk_ref_per_rev;

    /* the sum of all cell cycles per current revolution, this would be different for variable density */
    cyc_sum_frv = 8 * dptr->GCR_current_track_size;
    cyc_sum_frv = cyc_sum_frv ? cyc_sum_frv : 1;

    if (dptr->read_write_mode) {
        /* emulate the number of reference clocks requested */
        while (ref_cycles > 0) {
            /* calculate how much cycles can we do in one single pass */
            todo = 1;
            delta = count_new_bitcell - rptr->accum;
            if ((delta > 0) && ((cyc_sum_frv << 1) <= (DWORD)delta)) {
                todo = delta / cyc_sum_frv;
                if (ref_cycles < (int)todo) {
                    todo = ref_cycles;
                }
                if ((rptr->ue7_counter < 16) && ((16 - rptr->ue7_counter) < (int)todo)) {
                    todo = 16 - rptr->ue7_counter;
                }
                if ((rptr->filter_counter < 40) && ((40 - rptr->filter_counter) < (int)todo)) {
                    todo = 40 - rptr->filter_counter;
                }
                if ((rptr->fr_randcount > 0) && (rptr->fr_randcount < todo)) {
                    todo = rptr->fr_randcount;
                }
                if ((rptr->so_delay > 0) && (rptr->so_delay < (int)todo)) {
                    todo = rptr->so_delay;
                }
            }

            /* so signal handling */
            if (rptr->so_delay) {
                rptr->so_delay -= todo;
                if (!rptr->so_delay) {
                    dptr->byte_ready_edge = 1;
                    dptr->byte_ready_level = 1;
                }
            }

            /* do 2.5 microsecond flux filter stuff */
            rotation[dnr].filter_counter += todo;
            if ((rotation[dnr].filter_counter >= 40) && (rotation[dnr].filter_last_state != rotation[dnr].filter_state)) {
                /* update the filter last state */
                rotation[dnr].filter_last_state = rotation[dnr].filter_state;

                /* reset the counters at a flux reversal */
                rptr->ue7_counter = rptr->ue7_dcba;
                rptr->uf4_counter = 0;
                rptr->fr_randcount = ((RANDOM_nextUInt(rptr) >> 16) % 31) + 289;
            } else {
                /* no flux reversal detected */
                /* start seeing random flux reversals if 18us passed since the last real flux reversal */
                rptr->fr_randcount -= todo;
                if (!rptr->fr_randcount) {
                    rptr->ue7_counter = rptr->ue7_dcba;
                    rptr->uf4_counter = 0;
                    rptr->fr_randcount = ((RANDOM_nextUInt(rptr) >> 16) % 367) + 33;
                }
            }

            /* divide the reference clock with UE7 */
            rptr->ue7_counter += todo;
            if (rptr->ue7_counter == 16) {
                /* carry asserted; reload the counter */
                rptr->ue7_counter = rptr->ue7_dcba;

                rptr->uf4_counter = (rptr->uf4_counter + 1) & 0xf;

                /* the rising edge of UF4 stage B drives the shifter */
                if ((rptr->uf4_counter & 0x3) == 2) {
                    /* 8+2 bit shifter */

                    /* UE5 NOR gate shifts in a 1 only at C2 when DC is 0 */
                    rptr->last_read_data = ((rptr->last_read_data << 1) & 0x3fe) | (((rptr->uf4_counter + 0x1c) >> 4) & 0x01);

                    rptr->write_flux = rptr->last_write_data & 0x80;
                    rptr->last_write_data <<= 1;

                    /* last 10 bits asserted activates SYNC, reloads UE3, negates BYTE READY */
                    if (rptr->last_read_data == 0x3ff) {
                        rptr->bit_counter = 0;
                        /* FIXME: code should take into account whether BYTE READY has been latched
                         * anywhere in the system or not and negate only the unlatched inputs.
                         * So we just leave it be for now
                         */
                    } else {
                        if (++rptr->bit_counter == 8) {
                            rptr->bit_counter = 0;
                            dptr->GCR_read = (BYTE) rptr->last_read_data;
                            rptr->last_write_data = dptr->GCR_read;

                            /* BYTE READY signal if enabled */
                            if ((dptr->byte_ready_active & 2) != 0) {
                                rptr->so_delay = 16 - ((rptr->cycle_index + (todo - 1)) & 15);
                                if (rptr->so_delay < 10) {
                                    rptr->so_delay += 16;
                                }
                            }
                        }
                    }
                }
            }

            /* advance the count until the next bitcell */
            rptr->accum += cyc_sum_frv * todo;

            /* read the new bitcell */
            if (rptr->accum >= count_new_bitcell) {
                rptr->accum -= count_new_bitcell;
                if (read_next_bit(dptr)) {
                    /* reset 2.5 microsecond flux filter to a fake most ready state, because the
                     * most GCR-based images are almost clean already
                     */
                    rotation[dnr].filter_counter = 39;
                    rotation[dnr].filter_state = rotation[dnr].filter_state ^ 1;
                }
            }

            rptr->cycle_index += todo;
            ref_cycles -= todo;
        }
    } else {
        /* emulate the number of reference clocks requested */
        while (ref_cycles > 0) {
            /* calculate how much cycles can we do in one single pass */
            todo = 1;
            delta = count_new_bitcell - rptr->accum;
            if ((delta > 0) && ((cyc_sum_frv << 1) <= (DWORD)delta)) {
                todo = delta / cyc_sum_frv;
                if (ref_cycles < (int)todo) {
                    todo = ref_cycles;
                }
                if ((rptr->ue7_counter < 16) && ((16 - rptr->ue7_counter) < (int)todo)) {
                    todo = 16 - rptr->ue7_counter;
                }
                if ((rptr->so_delay > 0) && (rptr->so_delay < (int)todo)) {
                    todo = rptr->so_delay;
                }
            }

            /* so signal handling */
            if (rptr->so_delay) {
                rptr->so_delay -= todo;
                if (!rptr->so_delay) {
                    dptr->byte_ready_edge = 1;
                    dptr->byte_ready_level = 1;
                }
            }

            /* advance the count until the next bitcell */
            rptr->accum += cyc_sum_frv * todo;
            if (rptr->accum >= count_new_bitcell) {
                rptr->accum -= count_new_bitcell;
            }

            /* divide the reference clock with UE7 */
            rptr->ue7_counter += todo;
            if (rptr->ue7_counter == 16) {
                /* carry asserted; reload the counter */
                rptr->ue7_counter = rptr->ue7_dcba;

                rptr->uf4_counter = (rptr->uf4_counter + 1) & 0xf;

                /* the rising edge of UF4 stage B drives the shifter */
                if ((rptr->uf4_counter & 0x3) == 2) {
                    /* 8+2 bit shifter */

                    /* UE5 NOR gate shifts in a 1 only at C2 when DC is 0 */
                    rptr->last_read_data = ((rptr->last_read_data << 1) & 0x3fe) | (((rptr->uf4_counter + 0x1c) >> 4) & 0x01);

                    write_next_bit(dptr, rptr->last_write_data & 0x80);

                    rptr->last_write_data <<= 1;

                    rptr->accum = cyc_sum_frv * 2;

                    if (++rptr->bit_counter == 8) {
                        rptr->bit_counter = 0;

                        rptr->last_write_data = dptr->GCR_write_value;

                        /* BYTE READY signal if enabled */
                        if ((dptr->byte_ready_active & 2) != 0) {
                            rptr->so_delay = 16 - ((rptr->cycle_index + (todo - 1)) & 15);
                            if (rptr->so_delay < 10) {
                                rptr->so_delay += 16;
                            }
                        }
                    }
                }
            }

            rptr->cycle_index += todo;
            ref_cycles -= todo;
        }
    }
}

void rotation_1541_gcr_cycle(drive_t *dptr)
{
    rotation_t *rptr = &rotation[dptr->mynumber];
    CLOCK cpu_cycles;
    int ref_cycles, ref_advance_cycles;
    CLOCK one_rotation = rptr->frequency ? 400000 : 200000;

    /* cpu cycles since last call */
    cpu_cycles = *(dptr->clk) - rptr->rotation_last_clk;
    rptr->rotation_last_clk = *(dptr->clk);
    /* modulo, at least one revolution, but not more than two */
    while (cpu_cycles > one_rotation * 2) {
        cpu_cycles -= one_rotation;
    }

    /* Calculate the reference clock cycles from the cpu clock cycles - hw works the other way around...
             The reference clock is actually 16MHz, and the cpu clock is the result of dividing that by 16 */
    ref_cycles = cpu_cycles << (rptr->frequency ? 3 : 4);

    /* add additional R cycles requested; R must be less than a complete C cycle */
    ref_advance_cycles = dptr->req_ref_cycles;
    dptr->req_ref_cycles = 0;
    ref_advance_cycles &= 15;
    ref_cycles += ref_advance_cycles;

    /* run simulation if at least 1 R cycle has elapsed */
    if (ref_cycles > 0) {
        if (ref_cycles > rptr->ref_advance) {
            /* run simulation without the extra reference cycles already simmulated */
            ref_cycles -= rptr->ref_advance;
            rptr->ref_advance = ref_advance_cycles;
            rotation_1541_gcr(dptr, ref_cycles);
        } else {
            /* nothing to do as we are ahead of the cpu simulation; update the delay not catched up by the cpu yet */
            rptr->ref_advance -= ref_cycles;
        }
    }
}

/* 1541 circuit simulation for NZRI transition flux pulse-based images, see 1541 circuit description in this file for details */
void rotation_1541_p64(drive_t *dptr, int ref_cycles)
{
    rotation_t *rptr;
    PP64PulseStream P64PulseStream;
    DWORD DeltaPositionToNextPulse, ToDo;

    rptr = &rotation[dptr->mynumber];

    P64PulseStream = &dptr->p64->PulseStreams[dptr->current_half_track];

    /* Reset if out of head position bounds */
    if ((P64PulseStream->UsedLast >= 0) &&
        (P64PulseStream->Pulses[P64PulseStream->UsedLast].Position <= rptr->PulseHeadPosition)) {
        P64PulseStream->CurrentIndex = -1;
    } else {
        if (P64PulseStream->CurrentIndex < 0) {
            P64PulseStream->CurrentIndex = P64PulseStream->UsedFirst;
        } else {
            while ((P64PulseStream->CurrentIndex >= 0) &&
                   ((P64PulseStream->CurrentIndex != P64PulseStream->UsedFirst) &&
                    ((P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Previous >= 0) &&
                     (P64PulseStream->Pulses[P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Previous].Position > rptr->PulseHeadPosition)))) {
                P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Previous;
            }
        }
        while ((P64PulseStream->CurrentIndex >= 0) &&
               (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position <= rptr->PulseHeadPosition)) {
            P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Next;
        }
    }

    /* Calculate delta to the next NRZI transition flux pulse */
    if (P64PulseStream->CurrentIndex >= 0) {
        DeltaPositionToNextPulse = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position - rptr->PulseHeadPosition;
    } else {
        DeltaPositionToNextPulse = P64PulseSamplesPerRotation - rptr->PulseHeadPosition;
    }

    if (dptr->read_write_mode) {
        while (ref_cycles > 0) {
            /****************************************************************************************************************************************/
            {
                /* How-Much-16MHz-Clock-Cycles-ToDo-Count logic */

                ToDo = DeltaPositionToNextPulse;
                if (ToDo <= 1) {
                    ToDo = 1;
                } else {
                    if (ref_cycles < (int)ToDo) {
                        ToDo = ref_cycles;
                    }
                    if ((rptr->ue7_counter < 16) && ((16 - rptr->ue7_counter) < (int)ToDo)) {
                        ToDo = 16 - rptr->ue7_counter;
                    }
                    if ((rptr->filter_counter < 40) && ((40 - rptr->filter_counter) < (int)ToDo)) {
                        ToDo = 40 - rptr->filter_counter;
                    }
                    if ((rptr->fr_randcount > 0) && (rptr->fr_randcount < ToDo)) {
                        ToDo = rptr->fr_randcount;
                    }
                    if ((rptr->so_delay > 0) && (rptr->so_delay < (int)ToDo)) {
                        ToDo = rptr->so_delay;
                    }
                }
            }
            /****************************************************************************************************************************************/
            {
                /* so signal handling */
                if (rptr->so_delay) {
                    rptr->so_delay -= ToDo;
                    if (!rptr->so_delay) {
                        dptr->byte_ready_edge = 1;
                        dptr->byte_ready_level = 1;
                    }
                }
            }
            /****************************************************************************************************************************************/
            {
                /* Clock logic */

                /* 2.5 microseconds filter */
                rptr->filter_counter += (rptr->filter_counter < 40) ? ToDo : 0;
                if (((rptr->filter_counter >= 40) && (rptr->filter_state != rptr->filter_last_state))) {
                    rptr->filter_last_state = rptr->filter_state;
                    rptr->uf4_counter = 0;
                    rptr->ue7_counter = rptr->ue7_dcba;
                    rptr->fr_randcount = ((RANDOM_nextUInt(rptr) >> 16) % 31) + 289;
                } else {
                    rptr->fr_randcount -= ToDo;
                    if (!rptr->fr_randcount) {
                        rptr->uf4_counter = 0;
                        rptr->ue7_counter = rptr->ue7_dcba;
                        rptr->fr_randcount = ((RANDOM_nextUInt(rptr) >> 16) % 367) + 33;
                    }
                }

                /* Increment the pulse divider clock until the speed zone pulse divider clock threshold value is reached, which is:
                ** 16-(CurrentSpeedZone & 3), and each overflow, increment the pulse counter clock until the 4th pulse is reached
                */
                rptr->ue7_counter += ToDo;
                if (rptr->ue7_counter == 16) {
                    rptr->ue7_counter = rptr->ue7_dcba;

                    rptr->uf4_counter = (rptr->uf4_counter + 1) & 0xf;
                    if ((rptr->uf4_counter & 3) == 2) {
                        /****************************************************************************************************************************************/
                        {
                            /* Decoder logic */

                            rptr->last_read_data = ((rptr->last_read_data << 1) & 0x3fe) | (((rptr->uf4_counter + 0x1c) >> 4) & 1);

                            rptr->last_write_data <<= 1;

                            /* is sync? reset bit counter, don't move data, etc. */
                            if (rptr->last_read_data == 0x3ff) {
                                rptr->bit_counter = 0;
                            } else {
                                if (++rptr->bit_counter == 8) {
                                    rptr->bit_counter = 0;

                                    dptr->GCR_read = (BYTE) rptr->last_read_data;

                                    /* tlr claims that the write register is loaded at every
                                     * byte boundary, and since the bus is shared, it's reasonable
                                     * to guess that it would be loaded with whatever was last read. */
                                    rptr->last_write_data = dptr->GCR_read;

                                    /* BYTE READY signal if enabled */
                                    if ((dptr->byte_ready_active & 2) != 0) {
                                        rptr->so_delay = 16 - ((rptr->cycle_index + (ToDo - 1)) & 15);
                                        if (rptr->so_delay < 10) {
                                            rptr->so_delay += 16;
                                        }
                                    }
                                }
                            }
                        }
                        /****************************************************************************************************************************************/
                    }
                }
            }
            /****************************************************************************************************************************************/
            {
                /* Head logic */

                DeltaPositionToNextPulse -= ToDo;

                /* Track wrap handling */
                rptr->PulseHeadPosition += ToDo;
                if (rptr->PulseHeadPosition >= P64PulseSamplesPerRotation) {
                    rptr->PulseHeadPosition -= P64PulseSamplesPerRotation;

                    P64PulseStream->CurrentIndex = P64PulseStream->UsedFirst;
                    while ((P64PulseStream->CurrentIndex >= 0) &&
                           (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position < rptr->PulseHeadPosition)) {
                        P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Next;
                    }
                    if (P64PulseStream->CurrentIndex >= 0) {
                        DeltaPositionToNextPulse = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position - rptr->PulseHeadPosition;
                    } else {
                        DeltaPositionToNextPulse = P64PulseSamplesPerRotation - rptr->PulseHeadPosition;
                    }
                }

                /* Next NRZI transition flux pulse handling */
                if (!DeltaPositionToNextPulse) {
                    if ((P64PulseStream->CurrentIndex >= 0) &&
                        (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position == rptr->PulseHeadPosition)) {
                        DWORD Strength = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Strength;

                        /* Forward pulse high hit to the decoder logic */
                        if ((Strength == 0xffffffffUL) ||                                   /* Strong pulse */
                            (((DWORD)(RANDOM_nextInt(rptr) ^ 0x80000000UL)) < Strength)) {  /* Weak pulse */
                            rptr->filter_state ^= 1;
                            rptr->filter_counter = 0;
                        }

                        P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Next;
                    }
                    if (P64PulseStream->CurrentIndex >= 0) {
                        DeltaPositionToNextPulse = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position - rptr->PulseHeadPosition;
                    } else {
                        DeltaPositionToNextPulse = P64PulseSamplesPerRotation - rptr->PulseHeadPosition;
                    }
                }
            }
            /****************************************************************************************************************************************/

            rptr->cycle_index += ToDo;
            ref_cycles -= ToDo;
        }
    } else {
        int head_write;

        head_write = 0;

        while (ref_cycles > 0) {
            /****************************************************************************************************************************************/
            {
                /* How-Much-16MHz-Clock-Cycles-ToDo-Count logic */

                ToDo = DeltaPositionToNextPulse;
                if (ToDo <= 1) {
                    ToDo = 1;
                } else {
                    if ((rptr->PulseHeadPosition + ToDo) >= P64PulseSamplesPerRotation) {
                        ToDo = P64PulseSamplesPerRotation - rptr->PulseHeadPosition;
                    }
                    if (ref_cycles < (int)ToDo) {
                        ToDo = ref_cycles;
                    }
                    if ((rptr->ue7_counter < 16) && ((16 - rptr->ue7_counter) < (int)ToDo)) {
                        ToDo = 16 - rptr->ue7_counter;
                    }
                    if ((rptr->so_delay > 0) && (rptr->so_delay < (int)ToDo)) {
                        ToDo = rptr->so_delay;
                    }
                }
            }
            /****************************************************************************************************************************************/
            {
                /* so signal handling */
                if (rptr->so_delay) {
                    rptr->so_delay -= ToDo;
                    if (!rptr->so_delay) {
                        dptr->byte_ready_edge = 1;
                        dptr->byte_ready_level = 1;
                    }
                }
            }
            /****************************************************************************************************************************************/
            {
                /* Clock logic */

                /* Increment the pulse divider clock until the speed zone pulse divider clock threshold value is reached, which is:
                ** 16-(CurrentSpeedZone & 3), and each overflow, increment the pulse counter clock until the 4th pulse is reached
                */
                rptr->ue7_counter += ToDo;
                if (rptr->ue7_counter == 16) {
                    rptr->ue7_counter = rptr->ue7_dcba;

                    rptr->uf4_counter = (rptr->uf4_counter + 1) & 0xf;
                    if ((rptr->uf4_counter & 3) == 2) {
                        /****************************************************************************************************************************************/

                        /* Encoder logic */

                        rptr->last_read_data = ((rptr->last_read_data << 1) & 0x3fe) | (((rptr->uf4_counter + 0x1c) >> 4) & 1);

                        head_write = (rptr->last_write_data & 0x80) >> 7;
                        rptr->last_write_data <<= 1;

                        if (++rptr->bit_counter == 8) {
                            rptr->bit_counter = 0;

                            rptr->last_write_data = dptr->GCR_write_value;

                            /* BYTE READY signal if enabled */
                            if ((dptr->byte_ready_active & 2) != 0) {
                                rptr->so_delay = 16 - ((rptr->cycle_index + (ToDo - 1)) & 15);
                                if (rptr->so_delay < 10) {
                                    rptr->so_delay += 16;
                                }
                            }
                        }

                        /****************************************************************************************************************************************/
                    }
                }
            }

            /****************************************************************************************************************************************/
            {
                /* Head logic */

                /* Track wrap handling */
                rptr->PulseHeadPosition += ToDo;
                if (rptr->PulseHeadPosition >= P64PulseSamplesPerRotation) {
                    rptr->PulseHeadPosition -= P64PulseSamplesPerRotation;
                    P64PulseStream->CurrentIndex = P64PulseStream->UsedFirst;
                    while ((P64PulseStream->CurrentIndex >= 0) &&
                           (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position < rptr->PulseHeadPosition)) {
                        P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Next;
                    }
                }

                /* Write head handling */
                if ((!head_write) &&
                    (P64PulseStream->CurrentIndex >= 0) &&
                    (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position == rptr->PulseHeadPosition)) {
                    /* Remove pulse */
                    P64PulseStreamFreePulse(P64PulseStream, P64PulseStream->CurrentIndex);
                    dptr->P64_dirty = 1;
                } else if (head_write) {
                    /* Add a strong flux pulse */
                    if ((P64PulseStream->CurrentIndex >= 0) &&
                        (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position == rptr->PulseHeadPosition)) {
                        if (P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Strength != 0xffffffffUL) {
                            P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Strength = 0xffffffffUL;
                            dptr->P64_dirty = 1;
                        }
                    } else {
                        P64PulseStreamAddPulse(P64PulseStream, rptr->PulseHeadPosition, 0xffffffffUL);
                        dptr->P64_dirty = 1;
                    }
                    P64PulseStream->CurrentIndex = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Next;
                    head_write = 0;
                }

                /* Calculate new delta */
                if (P64PulseStream->CurrentIndex >= 0) {
                    DeltaPositionToNextPulse = P64PulseStream->Pulses[P64PulseStream->CurrentIndex].Position - rptr->PulseHeadPosition;
                } else {
                    DeltaPositionToNextPulse = P64PulseSamplesPerRotation - rptr->PulseHeadPosition;
                }
            }
            /****************************************************************************************************************************************/

            rptr->cycle_index += ToDo;
            ref_cycles -= ToDo;
        }
    }
}

void rotation_1541_p64_cycle(drive_t *dptr)
{
    rotation_t *rptr = &rotation[dptr->mynumber];
    CLOCK cpu_cycles;
    int ref_cycles, ref_advance_cycles;
    CLOCK one_rotation = rptr->frequency ? 400000 : 200000;

    /* cpu cycles since last call */
    cpu_cycles = *(dptr->clk) - rptr->rotation_last_clk;
    rptr->rotation_last_clk = *(dptr->clk);
    /* modulo, at least one revolution, but not more than two */
    while (cpu_cycles > one_rotation * 2) {
        cpu_cycles -= one_rotation;
    }

    /* Calculate the reference clock cycles from the cpu clock cycles - hw works the other way around...
             The reference clock is actually 16MHz, and the cpu clock is the result of dividing that by 16 */
    ref_cycles = cpu_cycles << (rptr->frequency ? 3 : 4);

    /* add additional R cycles requested; R must be less than a complete C cycle */
    ref_advance_cycles = dptr->req_ref_cycles;
    dptr->req_ref_cycles = 0;
    ref_advance_cycles &= 15;
    ref_cycles += ref_advance_cycles;

    /* run simulation if at least 1 R cycle has elapsed */
    if (ref_cycles > 0) {
        if (ref_cycles > rptr->ref_advance) {
            /* run simulation without the extra reference cycles already simmulated */
            ref_cycles -= rptr->ref_advance;
            rptr->ref_advance = ref_advance_cycles;
            rotation_1541_p64(dptr, ref_cycles);
        } else {
            /* nothing to do as we are ahead of the cpu simulation; update the delay not catched up by the cpu yet */
            rptr->ref_advance -= ref_cycles;
        }
    }
}

/* Rotate the disk according to the current value of `drive_clk[]'.  If
   `mode_change' is non-zero, there has been a Read -> Write mode switch.  */
void rotation_rotate_disk(drive_t *dptr)
{
    rotation_t *rptr;
    CLOCK delta;
    int tdelta;
    int bits_moved = 0;

    if ((dptr->byte_ready_active & 4) == 0) {
        dptr->req_ref_cycles = 0;
        return;
    }

    if (dptr->complicated_image_loaded) { /* stuff that needs complex and slow emulation */
        if (dptr->P64_image_loaded) {
            rotation_1541_p64_cycle(dptr);
        } else {
            rotation_1541_gcr_cycle(dptr);
        }
        return;
    }
    /* very simple and fast emulation for perfect images like those comming from dxx files */

    dptr->req_ref_cycles = 0;

    rptr = &rotation[dptr->mynumber];

    /* Calculate the number of bits that have passed under the R/W head since
       the last time.  */
    delta = *(dptr->clk) - rptr->rotation_last_clk;
    rptr->rotation_last_clk = *(dptr->clk);

    while (delta > 0) {
        tdelta = delta > 1000 ? 1000 : delta;
        delta -= tdelta;

        rptr->accum += rot_speed_bps[rptr->frequency][rptr->speed_zone] * tdelta;
        bits_moved += rptr->accum / 1000000;
        rptr->accum %= 1000000;
    }

    if (dptr->read_write_mode) {
        int off = dptr->GCR_head_offset;
        unsigned int byte, last_read_data = rptr->last_read_data << 7;
        unsigned int bit_counter = rptr->bit_counter;

        /* if no image is attached or track does not exists, read 0 */
        if (dptr->GCR_image_loaded == 0 || dptr->GCR_track_start_ptr == NULL) {
            byte = 0;
        } else {
            byte = dptr->GCR_track_start_ptr[off >> 3] << (off & 7);
        }

        while (bits_moved-- != 0) {
            byte <<= 1; off++;
            if (!(off & 7)) {
                if ((off >> 3) >= (int)dptr->GCR_current_track_size) {
                    off = 0;
                }
                /* if no image is attached or track does not exists, read 0 */
                if (dptr->GCR_image_loaded == 0 || dptr->GCR_track_start_ptr == NULL) {
                    byte = 0;
                } else {
                    byte = dptr->GCR_track_start_ptr[off >> 3];
                }
            }

            last_read_data <<= 1;
            last_read_data |= byte & 0x80;
            rptr->last_write_data <<= 1;

            /* is sync? reset bit counter, don't move data, etc. */
            if (~last_read_data & 0x1ff80) {
                if (++bit_counter == 8) {
                    bit_counter = 0;
                    dptr->GCR_read = (BYTE) (last_read_data >> 7);
                    /* tlr claims that the write register is loaded at every
                     * byte boundary, and since the bus is shared, it's reasonable
                     * to guess that it would be loaded with whatever was last read. */
                    rptr->last_write_data = dptr->GCR_read;
                    if ((dptr->byte_ready_active & 2) != 0) {
                        dptr->byte_ready_edge = 1;
                        dptr->byte_ready_level = 1;
                    }
                }
            } else {
                bit_counter = 0;
            }
        }
        rptr->last_read_data = (last_read_data >> 7) & 0x3ff;
        rptr->bit_counter = bit_counter;
        dptr->GCR_head_offset = off;
        if (!dptr->GCR_read) {    /* can only happen if on a half or unformatted track */
            dptr->GCR_read = 0x11; /* should be good enough, there's no data after all */
        }
    } else {
        /* When writing, the first byte after transition is going to echo the
         * bits from the last read value.
         */
        while (bits_moved-- != 0) {
            rptr->last_read_data = (rptr->last_read_data << 1) & 0x3fe;
            if ((rptr->last_read_data & 0xf) == 0) {
                rptr->last_read_data |= 1;
            }

            write_next_bit(dptr, rptr->last_write_data & 0x80);
            rptr->last_write_data <<= 1;

            if (++rptr->bit_counter == 8) {
                rptr->bit_counter = 0;
                rptr->last_write_data = dptr->GCR_write_value;
                if ((dptr->byte_ready_active & 2) != 0) {
                    dptr->byte_ready_edge = 1;
                    dptr->byte_ready_level = 1;
                }
            }
        }
        /* TODO: only if we introduced more than two 0 bits in a row */
        dptr->complicated_image_loaded = 1;
    }
}

/* Return non-zero if the Sync mark is found.  It is required to
   call rotation_rotate_disk() to update drive[].GCR_head_offset first.
   The return value corresponds to bit#7 of VIA2 PRB. This means 0x0
   is returned when sync is found and 0x80 is returned when no sync
   is found.  */
BYTE rotation_sync_found(drive_t *dptr)
{
    unsigned int dnr = dptr->mynumber;

    if (dptr->read_write_mode == 0 || dptr->attach_clk != (CLOCK)0) {
        return 0x80;
    }

    return rotation[dnr].last_read_data == 0x3ff ? 0 : 0x80;
}

void rotation_byte_read(drive_t *dptr)
{
    if (dptr->attach_clk != (CLOCK)0) {
        if (*(dptr->clk) - dptr->attach_clk < DRIVE_ATTACH_DELAY) {
            dptr->GCR_read = 0;
        } else {
            dptr->attach_clk = (CLOCK)0;
        }
    } else if (dptr->attach_detach_clk != (CLOCK)0) {
        if (*(dptr->clk) - dptr->attach_detach_clk < DRIVE_ATTACH_DETACH_DELAY) {
            dptr->GCR_read = 0;
        } else {
            dptr->attach_detach_clk = (CLOCK)0;
        }
    } else {
        rotation_rotate_disk(dptr);
    }
    dptr->req_ref_cycles = 0;
}


/* IF:
In order to provide maximum compatibility with all protection schemes VICE has to simulate the read circuitry
as well as signal and bus delays affecting the real hardware regarding disk access.
Without the read circuitry simulation many unaltered disk images would fail, for example all RapidLok protected
titles - originally they were patched for emulation.
BetaSkip protection is also known to contain fixed data for emulators, so are later Software Toolworks titles, e.g.
The Fidelity Chessmaster 2100, Beyond the Black Hole.
The HLS protection scheme relies on fully synchronized and cycle exact reads of SYNC and BYTE READY signals for
 several thousand bytes continuously.
The protection check eventually fails if any of the signals goes out of sync, due to accumulated timing errors
 and missing delays.
For this level of correctness, all the signal timing and bus delays have to be very close to the real hardware.

Explanation of the cycle delays used.
Bus timing was verified using original datasheets, SO signal timing verified using modified Visual 6502 simulator.
The BYTE READY signal:
 triggers the PA latch (disk data read/write) on UCD4 VIA
 activates the SO signal on the 6502 cpu
The V flag bit can be asserted (but not negated) when SO changes from 1 to 0.
Setting the V flag is subject to the time of sampling and propagation delay.
The drive is running from a 16MHz reference clock, the cpu gets the reference clock divided by 16.
Since the clock is 16MHz divided by 16, 16 R reference clock cycles correspond to 1 cpu cycle.
The cpu does different things in the two phases of the clock cycle.
The first phase is phi 1 or P1, the second phase is phi2 or P2.
One cpu cycle is comprised of P1 and P2 in this order, and is 1us.
The full cpu cycle is referenced as C, a half cycle (or phase) is H.
P1 and P2 are both 8 reference clock cycles each.

The cpu is clocked as:
H0: C0, P1, R0...7
H1: C0, P2, R8..15
H2: C1, P1, R0...7
H3: C1, P2, R8..15
H4: C2, P1, R0...7
H5: C2, P2, R8..15
H6: C3, P1, R0...7
H7: C3, P2, R8..15
...

SO is sampled at the trailing edge of P1, the cpu V flag is updated at next P1.
SO is actually sampled at 400ns in C (P1) cycle; see timing graphs in mos_6500_mpu_nov_1985.pdf.
During the first C cycle of any instruction following an instruction setting the V flag SO is not taken.
If the V flag is directly cleared using CLV, SO is not taken during the first two C cycles of the next instruction.
The first case is correctly emulated, the 2 C cycle effect is currently not emulated.
When the cpu code checks the V flag using BVC/BVS, the time the flag can be set and have an effect is during the
first 4 H cycles of the instruction.
In practice V gets updated at P1 cycles, therefore whatever the value of V is at the 3rd half-cycle of the
execution of the BVC/BVS branch decides the code path taken.

Bus read, as an example using LDX $1C00 to read the SYNC signal:
The LDX instruction is 4 C cycles (8 H), and the address bus gets set to $1C00 in the last C cycle of LDX.
For memory access description of the cpu; see Synertek hardware manual, 1.3.1 Bus Structure.
In the Synertek manual P1 is called Phase One, P2 is Phase Two.
- P1 is the time for address line change
- P2 is the time for data transfer
- the address bus is stable at most 300ns after after starting P1
- the data must be stable (therefore readable and taken) from the data bus at 100ns before the end of P2
- the memory device has 575ns to put and stabilize the data on the data bus. This is about 1000-300-100ns,
25 ns additionally used for rise and fall times.
In other words the value can change, until that point, and whatever the last state on the data bus is will be the
data read.
Summary:
- executing the LDX is 4 cycles, 1 cycle is 1us; data must be stable at 100ns before the next cycle.
- SYNC at most should be available on the data bus at 3.875us into the execution of LDX of its total execution
time of 4us.

1541 circuit description for reading
C1541 read simulation information based on C1541 schematics.
Component naming follows 1540008-01, the original 'Long Board' schematics.
UE7: 74LS193, 4 bit counter
UF4: 74LS193, 4 bit counter
UE5: 74LS02, NOR gate
UD2: 74LS164, serial in, parallel out shift register
UC3: 74LS245, octal bus transceiver
UE4: 74LS74, positive-edge-triggered D flip-flop
UE3: 74LS191, 4 bit counter
UCD4: 6522, VIA

UE7 4 bit counter, clocked at 16MHz
UE7 counts up from input value 00BA (BA counter bits b1/b0), connected to UCD4 PB6/PB5, DC=0
UE7 carry output at value 16 generates an LD signal reloading the counter value set by BA
    effectively dividing 16MHz by 16, 15, 14, 13 depending on BA input
UF4 4 bit counter, counts up from 0 to 16->0, clocked by UE7 carry signal
UE7 and UF4 counters both get reset at a flux reversal detected, by LD and CLR signals respectively.
The two MSBs (DC) of counter UF4 go through a NOR gate (UE5), meaning whenever b3 and b2 is 0,
the NOR gate outputs 1, any other time the NOR gate is 0. The NOR gate drives the B input of UD2.
The data is clocked in by the B stage of UF4 (bit#1 of the counter) on its rising edge - switching from 0 to 1.

Notes:
- Without a flux reversal, the data window stays at the timing wherever the last 1 bit was seen+N*speed.
- A bit 0 is clocked into the shifter periodically, correlated to bitcell speed set by the user on UE7
- After 1x1+3x0 bits UF4 wraps, UE5 goes high, outputing an 1 bit for the shifter regardless of flux reversals.
However, this would only be clocked in if the B stage of UF4 is set to 1 after two UF4 cycles - e.g a flux
transition would discard the timer wrap generated data bit.
- With a flux reversal an 1 is output, and the B stage of the counter would eventually clock that into the shifter
after 2 UF4 cycles, ie 2 carry signals from UE7
- The data window (UE7) instantly resets at a flux reversal.
- The complete bitcell cycle is 4 (3+1) cycles of UF4.
Presumably the shifter already contains the bit after C#2, C#6 etc - see
below.
C#0: 1 bit output on UE5
C#1: same
C#2: 1 bit from UE5 is clocked into the shifter
C#3: hold clock
C#4: 0 bit output on UE5
C#5: same
C#6: 0 bit from UE5 is clocked into the shifter
C#7: hold clock
C#8: 0 bit output on UE5
C#9: same
C#a: 0 bit from UE5 is clocked into the shifter
C#b: hold clock
C#c: 0 bit output on UE5
C#d: same
C#e: 0 bit from UE5 is clocked into the shifter
C#f: hold clock
Note, how a flux reversal before the clock stages C2, C6, CA, CE cancels shifting in the last data bit.

The actual bitcell size is correlated to the first counter and the second counter.
e.g. at speed 0:
UE7 is dividing 16MHz by 16, resulting in a 1MHz clock rate for UF4.
At every 4 cycles of UF4 (but see above) we have a new bit shifted into UD2.
In practice, the 1MHz clock is divided by 4; the output rate is 250KHz.
250KHz is 4us per each new bitcell shifted into UD2.
Similarly, speed 1 is 3.75us, speed 2 is 3.5us, speed 3 is 3.25us.

In the table below, UF4 cycles xn are the ideal flux reversal intervals, x0 is when the flux reversal is detected.
B is the bit value that gets clocked into the UD2 shifter at UF4 cycles C2, C6, CA, CE.
cnn is the number of cycles elapsed @16MHz when speed is nn since the last flux reversal (1 bit)
tnn is the time elapsed @16MHz when speed is nn since the last flux reversal (1 bit)

Example data and timing written at 3.5us:
11      3.5us
101     7.0us
1001   10.5us
10001  14.0us

e.g. pattern 11 If read at the same speed as written (c02/t02) would produce:
- an 1 bit at 1.75us after the first flux reversal detected
- a 0 bit at 5.25us after the first flux reversal detected
- if the flux reversal period is shorter than 1.75us, the 1 bit is not output, there is no shifter data
- if the period is 5.25us or longer, the shifter gets an extra 0, producing an 10 pattern
In other words: the flux reversal is exactly in the middle of the data window (speed setting)
and tolerance is <data window time>/2 before an error occurs.

Pattern 101 taking 7us, if read at 4us speed (c00/t00) has its data window changed by -1us/+3us

Pattern 1001 taking 10.5us, if read at 4us speed (c00/t00) has its data window changed by -0.5us/+3.5us
0.5us is 500ns which is easily produced by peak shifts and drive speed wobble, producing an 101 instead of 1001.

UF4>UD2
DCBA B C# c00/t00    c01/t01    c02/t02    c03/t03
0000 1 x0   0/0        0/0        0/0        0/0
0001 1
0010 1 C2  32/2       30/1.875   28/1.75    26/1.625
0011 1
0100 0 x4  64/4       60/3.75    56/3.5     52/3.25
0101 0
0110 0 C6  96/6       90/5.625   84/5.25    78/4.875
0111 0
1000 0 x8 128/8      120/7.5    112/7      104/6.5
1001 0
1010 0 CA 160/10     150/9.375  140/8.75   130/8.125
1011 0
1100 0 xC 192/12     180/11.25  168/10.5   156/9.75
1101 0
1110 0 CE 224/14     210/13.125 196/12.25  182/11.375
1111 0

The VIA is connected to the UC3 transceiver whose main job is to isolate the output stages of the UD2 shifter from
the ports in write mode, otherwise it reflects the shifter output with a slight delay.
The shifter is 8 bits. Two additional UE4 flip-flops are buffering the H stage of the shifter.
Those together (plus a few other lines) combined give the seemingly 10 bits long SYNC signal.

The shifter always clocks in the data and unless the VIA is in write mode, those bits always appear on PA as is.
Note that what appears there is the last 8 bits, the SYNC signal is effectively the last 10 bits ANDed;
all bits read should be asserted (plus a few lines).
BYTE READY gets generated each time the 8th bit is clocked into the shifter and it is counted by UE3 and some other
logic. Whenever the SYNC is asserted UE3 gets reloaded, meaning BYTE READY should be negated.
As long as SYNC is asserted BYTE READY should remain negated.
Note, the data is still clocked into the shifter regardless of the state of BYTE READY or the SYNC signals.
*/
