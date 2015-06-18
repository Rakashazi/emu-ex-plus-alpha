/*
 * sid-snapshot.h - SID snapshot.
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

#ifndef VICE_SID_SNAPSHOT_H
#define VICE_SID_SNAPSHOT_H

#include "types.h"

struct snapshot_s;

struct sid_snapshot_state_s {
    BYTE sid_register[0x20];
    BYTE bus_value;
    DWORD bus_value_ttl;
    DWORD accumulator[3];
    DWORD shift_register[3];
    WORD rate_counter[3];
    WORD rate_counter_period[3];
    WORD exponential_counter[3];
    WORD exponential_counter_period[3];
    BYTE envelope_counter[3];
    BYTE envelope_state[3];
    BYTE hold_zero[3];
    BYTE envelope_pipeline[3];
    BYTE shift_pipeline[3];
    DWORD shift_register_reset[3];
    DWORD floating_output_ttl[3];
    WORD pulse_output[3];
    BYTE write_pipeline;
    BYTE write_address;
    BYTE voice_mask;
};
typedef struct sid_snapshot_state_s sid_snapshot_state_t;

extern int sid_snapshot_write_module(struct snapshot_s *s);
extern int sid_snapshot_read_module(struct snapshot_s *s);

#endif
