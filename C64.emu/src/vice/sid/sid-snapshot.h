/*
 * sid-snapshot.h - SID snapshot.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef VICE_SID_SNAPSHOT_H
#define VICE_SID_SNAPSHOT_H

#include "types.h"

struct snapshot_s;

typedef struct sid_snapshot_state_s {
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
} sid_snapshot_state_t;

typedef struct sid_fastsid_snapshot_state_s {
    DWORD factor;
    BYTE d[32];
    BYTE has3;
    BYTE vol;
    SDWORD adrs[16];
    DWORD sz[16];
    DWORD speed1;
    BYTE update;
    BYTE newsid;
    BYTE laststore;
    BYTE laststorebit;
    DWORD laststoreclk;
    DWORD emulatefilter;
    float filterDy;
    float filterResDy;
    BYTE filterType;
    BYTE filterCurType;
    WORD filterValue;

    DWORD v_nr[3];
    DWORD v_f[3];
    DWORD v_fs[3];
    BYTE v_noise[3];
    DWORD v_adsr[3];
    SDWORD v_adsrs[3];
    DWORD v_adsrz[3];
    BYTE v_sync[3];
    BYTE v_filter[3];
    BYTE v_update[3];
    BYTE v_gateflip[3];
    BYTE v_adsrm[3];
    BYTE v_attack[3];
    BYTE v_decay[3];
    BYTE v_sustain[3];
    BYTE v_release[3];
    DWORD v_rv[3];
    BYTE v_wt[3];
    WORD v_wt_offset[3];
    DWORD v_wtpf[3];
    DWORD v_wtl[3];
    WORD v_wtr[2][3];
    BYTE v_filtIO[3];
    float v_filtLow[3];
    float v_filtRef[3];
} sid_fastsid_snapshot_state_t;

typedef struct sid_cw3_snapshot_state_s {
    BYTE ntsc;
    DWORD cycles_per_second;
    BYTE regs[32];
} sid_cw3_snapshot_state_t;

typedef struct sid_hs_snapshot_state_s {
    BYTE regs[32];
    DWORD hsid_main_clk;
    DWORD hsid_alarm_clk;
    DWORD lastaccess_clk;
    DWORD lastaccess_ms;
    DWORD lastaccess_chipno;
    DWORD chipused;
    DWORD device_map[4];
} sid_hs_snapshot_state_t;

typedef struct sid_parsid_snapshot_state_s {
    BYTE regs[32];
    BYTE parsid_ctrport;
} sid_parsid_snapshot_state_t;

typedef struct sid_ssi2001_snapshot_state_s {
    BYTE regs[32];
} sid_ssi2001_snapshot_state_t;

extern int sid_snapshot_write_module(struct snapshot_s *s);
extern int sid_snapshot_read_module(struct snapshot_s *s);

#endif
