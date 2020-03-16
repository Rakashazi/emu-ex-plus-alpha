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
    uint8_t sid_register[0x20];
    uint8_t bus_value;
    uint32_t bus_value_ttl;
    uint32_t accumulator[3];
    uint32_t shift_register[3];
    uint16_t rate_counter[3];
    uint16_t rate_counter_period[3];
    uint16_t exponential_counter[3];
    uint16_t exponential_counter_period[3];
    uint8_t envelope_counter[3];
    uint8_t envelope_state[3];
    uint8_t hold_zero[3];
    uint8_t envelope_pipeline[3];
    uint8_t shift_pipeline[3];
    uint32_t shift_register_reset[3];
    uint32_t floating_output_ttl[3];
    uint16_t pulse_output[3];
    uint8_t write_pipeline;
    uint8_t write_address;
    uint8_t voice_mask;
} sid_snapshot_state_t;

typedef struct sid_fastsid_snapshot_state_s {
    uint32_t factor;
    uint8_t d[32];
    uint8_t has3;
    uint8_t vol;
    int32_t adrs[16];
    uint32_t sz[16];
    uint32_t speed1;
    uint8_t update;
    uint8_t newsid;
    uint8_t laststore;
    uint8_t laststorebit;
    uint32_t laststoreclk;
    uint32_t emulatefilter;
    float filterDy;
    float filterResDy;
    uint8_t filterType;
    uint8_t filterCurType;
    uint16_t filterValue;

    uint32_t v_nr[3];
    uint32_t v_f[3];
    uint32_t v_fs[3];
    uint8_t v_noise[3];
    uint32_t v_adsr[3];
    int32_t v_adsrs[3];
    uint32_t v_adsrz[3];
    uint8_t v_sync[3];
    uint8_t v_filter[3];
    uint8_t v_update[3];
    uint8_t v_gateflip[3];
    uint8_t v_adsrm[3];
    uint8_t v_attack[3];
    uint8_t v_decay[3];
    uint8_t v_sustain[3];
    uint8_t v_release[3];
    uint32_t v_rv[3];
    uint8_t v_wt[3];
    uint16_t v_wt_offset[3];
    uint32_t v_wtpf[3];
    uint32_t v_wtl[3];
    uint16_t v_wtr[2][3];
    uint8_t v_filtIO[3];
    float v_filtLow[3];
    float v_filtRef[3];
} sid_fastsid_snapshot_state_t;

typedef struct sid_cw3_snapshot_state_s {
    uint8_t ntsc;
    uint32_t cycles_per_second;
    uint8_t regs[32];
} sid_cw3_snapshot_state_t;

typedef struct sid_hs_snapshot_state_s {
    uint8_t regs[32];
    uint32_t hsid_main_clk;
    uint32_t hsid_alarm_clk;
    uint32_t lastaccess_clk;
    uint32_t lastaccess_ms;
    uint32_t lastaccess_chipno;
    uint32_t chipused;
    uint32_t device_map[4];
} sid_hs_snapshot_state_t;

typedef struct sid_parsid_snapshot_state_s {
    uint8_t regs[32];
    uint8_t parsid_ctrport;
} sid_parsid_snapshot_state_t;

typedef struct sid_ssi2001_snapshot_state_s {
    uint8_t regs[32];
} sid_ssi2001_snapshot_state_t;

extern int sid_snapshot_write_module(struct snapshot_s *s);
extern int sid_snapshot_read_module(struct snapshot_s *s);

#endif
