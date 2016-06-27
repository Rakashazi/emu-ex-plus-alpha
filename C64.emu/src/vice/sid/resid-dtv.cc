/*
 * resid-dtv.cc - reSID-DTV interface code.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Dag Lem <resid@nimrod.no>
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

#ifdef _M_ARM
#undef _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE
#define _ARM_WINAPI_PARTITION_DESKTOP_SDK_AVAILABLE 1
#endif

#include "vice.h"

#ifdef WATCOM_COMPILE
#define _STDIO_H_INCLUDED
#include <cstdio>
using std::FILE;
using std::sprintf;
#endif

extern "C" {

/* QNX has problems with const and inline definitions
   in its string.h file when using g++ */

#ifndef __QNX__
#include <string.h>
#endif

#include "sid/sid.h" /* sid_engine_t */
#include "lib.h"
#include "log.h"
#include "resid.h"
#include "resources.h"
#include "sid-snapshot.h"
#include "types.h"

} // extern "C"

#include "resid-dtv/sid.h"

using namespace reSID;

extern "C" {

struct sound_s
{
    /* speed factor */
    int factor;

    /* resid sid implementation */
    reSID::SID *sid;
};

typedef struct sound_s sound_t;

static sound_t *resid_open(BYTE *sidstate)
{
    sound_t *psid;
    int i;

    psid = new sound_t;
    psid->sid = new reSID::SID;

    for (i = 0x00; i <= 0x18; i++) {
        psid->sid->write(i, sidstate[i]);
    }

    return psid;
}

static int resid_init(sound_t *psid, int speed, int cycles_per_sec, int factor)
{
    sampling_method method;
    char model_text[100];
    char method_text[100];
    double passband, gain;
    int filters_enabled, model, sampling, passband_percentage, gain_percentage, filter_bias_mV;

    if (resources_get_int("SidFilters", &filters_enabled) < 0) {
        return 0;
    }

    if (resources_get_int("SidModel", &model) < 0) {
        return 0;
    }

    if (resources_get_int("SidResidSampling", &sampling) < 0) {
        return 0;
    }

    if (resources_get_int("SidResidPassband", &passband_percentage) < 0) {
        return 0;
    }

    if (resources_get_int("SidResidGain", &gain_percentage) < 0) {
        return 0;
    }
    
    if (resources_get_int("SidResidFilterBias", &filter_bias_mV) < 0) {
        return 0;
    }

    passband = speed * passband_percentage / 200.0;
    gain = gain_percentage / 100.0;

    psid->factor = factor;

    switch (model) {
      default:
      case 0:
        psid->sid->set_chip_model(MOS6581);
        psid->sid->set_voice_mask(0x07);
        psid->sid->input(0);
        strcpy(model_text, "MOS6581");
        break;
      case 1:
        psid->sid->set_chip_model(MOS8580);
        psid->sid->set_voice_mask(0x07);
        psid->sid->input(0);
        strcpy(model_text, "MOS8580");
        break;
      case 2:
        psid->sid->set_chip_model(MOS8580);
        psid->sid->set_voice_mask(0x0f);
        psid->sid->input(-32768);
        strcpy(model_text, "MOS8580 + digi boost");
        break;
#if 0
      case 3: /* not yet */
        psid->sid->set_chip_model(MOS6581R4);
        psid->sid->set_voice_mask(0x07);
        psid->sid->input(0);
        strcpy(model_text, "MOS6581R4");
        break;
#endif
      case 4:
        /* resid-dtv has only the DTVSID model and no ext input*/
        strcpy(model_text, "DTVSID");
        break;
    }
    psid->sid->enable_filter(filters_enabled ? true : false);
    psid->sid->adjust_filter_bias(filter_bias_mV / 1000.0);
    psid->sid->enable_external_filter(filters_enabled ? true : false);

    switch (sampling) {
      default:
      case 0:
        method = SAMPLE_FAST;
        strcpy(method_text, "fast");
        break;
      case 1:
        method = SAMPLE_INTERPOLATE;
        strcpy(method_text, "interpolating");
        break;
      case 2:
        method = SAMPLE_RESAMPLE;
        sprintf(method_text, "resampling, pass to %dHz", (int)passband);
        break;
      case 3:
        method = SAMPLE_RESAMPLE_FASTMEM;
        sprintf(method_text, "resampling, pass to %dHz", (int)passband);
        break;
    }

    if (!psid->sid->set_sampling_parameters(cycles_per_sec, method,
                                            speed, passband, gain)) {
        log_warning(LOG_DEFAULT,
                    "reSID: Out of spec, increase sampling rate or decrease maximum speed");
        return 0;
    }

    log_message(LOG_DEFAULT, "reSID: %s, filter %s, sampling rate %dHz - %s",
                model_text,
                filters_enabled ? "on" : "off",
                speed, method_text);

    return 1;
}

static void resid_close(sound_t *psid)
{
    delete psid->sid;
    delete psid;
}

static BYTE resid_read(sound_t *psid, WORD addr)
{
    return psid->sid->read(addr);
}

static void resid_store(sound_t *psid, WORD addr, BYTE byte)
{
    psid->sid->write(addr, byte);
}

static void resid_reset(sound_t *psid, CLOCK cpu_clk)
{
    psid->sid->reset();
}

static int resid_calculate_samples(sound_t *psid, SWORD *pbuf, int nr,
                                   int interleave, int *delta_t)
{
    return psid->sid->clock(*delta_t, pbuf, nr, interleave);
}

static void resid_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
}

static char *resid_dump_state(sound_t *psid)
{
    return lib_stralloc("");
}

static void resid_state_read(sound_t *psid, sid_snapshot_state_t *sid_state)
{
    reSID::SID::State state;
    unsigned int i;

    state = psid->sid->read_state();

    for (i = 0; i < 0x20; i++) {
        sid_state->sid_register[i] = (BYTE)state.sid_register[i];
    }

    sid_state->bus_value = (BYTE)state.bus_value;
    for (i = 0; i < 3; i++) {
        sid_state->accumulator[i] = (DWORD)state.accumulator[i];
        sid_state->shift_register[i] = (DWORD)state.shift_register[i];
        sid_state->rate_counter[i] = (WORD)state.rate_counter[i];
        sid_state->rate_counter_period[i] = (WORD)state.rate_counter_period[i];
        sid_state->exponential_counter[i] = (WORD)state.exponential_counter[i];
        sid_state->exponential_counter_period[i] = (WORD)state.exponential_counter_period[i];
        sid_state->envelope_counter[i] = (BYTE)state.envelope_counter[i];
        sid_state->envelope_state[i] = (BYTE)state.envelope_state[i];
        sid_state->hold_zero[i] = (BYTE)state.hold_zero[i];
    }
}

static void resid_state_write(sound_t *psid, sid_snapshot_state_t *sid_state)
{
    reSID::SID::State state;
    unsigned int i;

    for (i = 0; i < 0x20; i++) {
        state.sid_register[i] = (char)sid_state->sid_register[i];
    }

    state.bus_value = (reg8)sid_state->bus_value;
    for (i = 0; i < 3; i++) {
        state.accumulator[i] = (reg24)sid_state->accumulator[i];
        state.shift_register[i] = (reg24)sid_state->shift_register[i];
        state.rate_counter[i] = (reg16)sid_state->rate_counter[i];
        if (sid_state->rate_counter_period[i]) {
            state.rate_counter_period[i] = (reg16)sid_state->rate_counter_period[i];
        }
        state.exponential_counter[i] = (reg16)sid_state->exponential_counter[i];
        if (sid_state->exponential_counter_period[i]) {
            state.exponential_counter_period[i] = (reg16)sid_state->exponential_counter_period[i];
        }
        state.envelope_counter[i] = (reg8)sid_state->envelope_counter[i];
        state.envelope_state[i] = (EnvelopeGenerator::State)sid_state->envelope_state[i];
        state.hold_zero[i] = (sid_state->hold_zero[i] != 0);
    }

    psid->sid->write_state((const reSID::SID::State)state);
}

sid_engine_t resid_hooks =
{
    resid_open,
    resid_init,
    resid_close,
    resid_read,
    resid_store,
    resid_reset,
    resid_calculate_samples,
    resid_prevent_clk_overflow,
    resid_dump_state,
    resid_state_read,
    resid_state_write
};

} // extern "C"
