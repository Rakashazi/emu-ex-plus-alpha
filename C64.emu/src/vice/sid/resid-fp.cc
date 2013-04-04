/*
 * resid-fp.cc - reSID-fp interface code.
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Dag Lem <resid@nimrod.no>
 *  Andreas Boose <viceteam@t-online.de>
 *  Antti S. Lankila <alankila@bel.fi>
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

#ifdef WATCOM_COMPILE
#define _STDIO_H_INCLUDED
#include <cstdio>
using std::FILE;
using std::sprintf;
#endif

/* resid itself is always compiled with C64DTV support */
#include "resid-fp/sid.h"

extern "C" {

/* QNX has problems with const and inline definitions
   in its string.h file when using g++ */

#ifndef __QNX__
#include <string.h>
#else
extern char *strcpy(char *s1, char *s2);
#endif

#include "sid/sid.h" /* sid_engine_t */
#include "lib.h"
#include "log.h"
#include "resid-fp.h"
#include "resources.h"
#include "sid-snapshot.h"
#include "types.h"

struct sound_s
{
    /* resid sid implementation */
    SIDFP *sid;
};

typedef struct sound_s sound_t;

static sound_t *residfp_open(BYTE *sidstate)
{
    sound_t *psid;
    int i;

    psid = new sound_t;
    psid->sid = new SIDFP;

    for (i = 0x00; i <= 0x18; i++) {
        psid->sid->write(i, sidstate[i]);
    }

    return psid;
}

static int residfp_init(sound_t *psid, int speed, int cycles_per_sec)
{
    sampling_method method;
    char model_text[100];
    char method_text[100];
    float  passband;
    int filters_enabled, model, sampling, passband_percentage;

    if (resources_get_int("SidFilters", &filters_enabled) < 0)
        return 0;

    if (resources_get_int("SidModel", &model) < 0)
        return 0;

    if (resources_get_int("SidResidSampling", &sampling) < 0)
        return 0;

    if (resources_get_int("SidResidPassband", &passband_percentage) < 0)
        return 0;

    passband = speed * passband_percentage / 200.f;
 
    /* Some mostly-common settings for all modes abstracted here. */
    psid->sid->input(0);

    /* Model numbers 8-15 are reserved for distorted 6581s. */
    if (model < 8 || model > 15) {
      psid->sid->set_chip_model(MOS8580FP);
      psid->sid->set_voice_nonlinearity(1.0f);
      psid->sid->get_filter().set_distortion_properties(0.5f, 0.f);
    } else {
      psid->sid->set_chip_model(MOS6581FP);
      psid->sid->set_voice_nonlinearity(0.96f);
      psid->sid->get_filter().set_distortion_properties(0.50f, 3.3e6f);
    }

    switch (model) {

    case SID_MODEL_8580R5_3691:
      psid->sid->get_filter().set_type4_properties(6.55f, 20.0f);
      strcpy(model_text, "8580R5 3691");
      break;
    case SID_MODEL_8580R5_3691D:
      psid->sid->get_filter().set_type4_properties(6.55f, 20.0f);
      psid->sid->input(-32768);
      strcpy(model_text, "8580R5 3691 + digi boost");
      break;

    case SID_MODEL_8580R5_1489:
      psid->sid->get_filter().set_type4_properties(5.7f, 20.0f);
      strcpy(model_text, "8580R5 1489");
      break;
    case SID_MODEL_8580R5_1489D:
      psid->sid->get_filter().set_type4_properties(5.7f, 20.0f);
      psid->sid->input(-32768);
      strcpy(model_text, "8580R5 1489 + digi boost");
      break;

    case SID_MODEL_6581R3_4885:
      psid->sid->get_filter().set_type3_properties(840577.4520801408f, 1909158.8633669745f, 1.0068865662510837f, 14858.140079688419f);
      strcpy(model_text, "6581R3 4885");
      break;
    case SID_MODEL_6581R3_0486S:
      psid->sid->get_filter().set_type3_properties(1164920.4999651583f, 12915042.165290257f, 1.0058853753357735f, 12914.5661141159f);
      strcpy(model_text, "6581R3 0486S");
      break;
    case SID_MODEL_6581R3_3984:
      psid->sid->get_filter().set_type3_properties(1522171.922983084f, 21729926.667291082f, 1.004994802537475f, 14299.149638099827f);
      strcpy(model_text, "6581R3 3984");
      break;
    default:
    case SID_MODEL_6581R4AR_3789:
      psid->sid->get_filter().set_type3_properties(1141069.9277645703f, 276016753.85303545f, 1.0066634233403395f, 16402.86712485317f);
      strcpy(model_text, "6581R4AR 3789");
      break;
    case SID_MODEL_6581R3_4485:
      psid->sid->get_filter().set_type3_properties(1399768.3253307983f, 553018906.8926692f, 1.0051493199361266f, 11961.908870403166f);
      strcpy(model_text, "6581R3 4485");
      break;
    case SID_MODEL_6581R4_1986S:
      psid->sid->get_filter().set_type3_properties(1250736.2235895505f, 1521187976.8735676f, 1.005543646897986f, 8581.78418415723f);
      strcpy(model_text, "6581R4 1986S");
      break;
    }

    psid->sid->enable_filter(filters_enabled ? true : false);

    switch (sampling) {
      default:
      case 1:
        method = SAMPLE_INTERPOLATE;
        strcpy(method_text, "interpolation");
        break;
      case 2:
      case 3:
        method = SAMPLE_RESAMPLE_INTERPOLATE;
        sprintf(method_text, "%sresampling, cutoff %d Hz",
                             (psid->sid->sse_enabled() ? "SSE " : ""),
                             (int) (passband > 20000.f ? 20000.f : passband));
        break;
    }

    //! \todo FIXME: These casts have to go away
    if (!psid->sid->set_sampling_parameters((float)cycles_per_sec, method,
                                            (float)speed, passband)) {
        log_warning(LOG_DEFAULT,
                    "ReSID-FP: unable to set sampling mode; try increasing sampling frequency to 44.1-48 kHz and keep passband around 80-90 %%.");
        return 0;
    }

    log_message(LOG_DEFAULT,
                "ReSID-FP: %s, filter %s, sampling rate %d Hz with %s",
                model_text,
                filters_enabled ? "on" : "off",
                speed,
                method_text);
    return 1;
}

static void residfp_close(sound_t *psid)
{
    delete psid->sid;
    delete psid;
}

static BYTE residfp_read(sound_t *psid, WORD addr)
{
    return psid->sid->read(addr & 0xffu);
}

static void residfp_store(sound_t *psid, WORD addr, BYTE byte)
{
    psid->sid->write(addr & 0xffu, byte);
}

static void residfp_reset(sound_t *psid, CLOCK cpu_clk)
{
    psid->sid->reset();
}

static int residfp_calculate_samples(sound_t *psid, SWORD *pbuf, int nr,
                                   int interleave, int *delta_t)
{
    return psid->sid->clock(*delta_t, pbuf, nr, interleave);
}

static void residfp_prevent_clk_overflow(sound_t *psid, CLOCK sub)
{
}

static char *residfp_dump_state(sound_t *psid)
{
    return lib_stralloc("");
}

static void residfp_state_read(sound_t *psid, sid_snapshot_state_t *sid_state)
{
    SIDFP::State state;
    unsigned int i;

    state = psid->sid->read_state();

    for (i = 0; i < 0x20; i++) {
        sid_state->sid_register[i] = (BYTE)state.sid_register[i];
    }

    sid_state->bus_value = (BYTE)state.bus_value;
    sid_state->bus_value_ttl = (DWORD)state.bus_value_ttl;
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

static void residfp_state_write(sound_t *psid, sid_snapshot_state_t *sid_state)
{
    SIDFP::State state;
    unsigned int i;

    for (i = 0; i < 0x20; i++) {
        state.sid_register[i] = (char)sid_state->sid_register[i];
    }

    state.bus_value = (reg8)sid_state->bus_value;
    state.bus_value_ttl = (cycle_count)sid_state->bus_value_ttl;
    for (i = 0; i < 3; i++) {
        state.accumulator[i] = (reg24)sid_state->accumulator[i];
        state.shift_register[i] = (reg24)sid_state->shift_register[i];
        state.rate_counter[i] = (reg16)sid_state->rate_counter[i];
        if (sid_state->rate_counter_period[i])
            state.rate_counter_period[i] = (reg16)sid_state->rate_counter_period[i];
        state.exponential_counter[i] = (reg16)sid_state->exponential_counter[i];
        if (sid_state->exponential_counter_period[i])
            state.exponential_counter_period[i] = (reg16)sid_state->exponential_counter_period[i];
        state.envelope_counter[i] = (reg8)sid_state->envelope_counter[i];
        state.envelope_state[i] = (EnvelopeGeneratorFP::State)sid_state->envelope_state[i];
        state.hold_zero[i] = (sid_state->hold_zero[i] != 0);
    }

    psid->sid->write_state((const SIDFP::State)state);
}

sid_engine_t residfp_hooks =
{
    residfp_open,
    residfp_init,
    residfp_close,
    residfp_read,
    residfp_store,
    residfp_reset,
    residfp_calculate_samples,
    residfp_prevent_clk_overflow,
    residfp_dump_state,
    residfp_state_read,
    residfp_state_write
};

} // extern "C"
