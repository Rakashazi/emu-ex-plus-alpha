//  ---------------------------------------------------------------------------
//  This file is part of reSID, a MOS6581 SID emulator engine.
//  Copyright (C) 2004  Dag Lem <resid@nimrod.no>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  ---------------------------------------------------------------------------

#include "wave.h"
#include "sid.h"

typedef struct {
    float bias;
    float pulsestrength;
    float topbit;
    float distance;
    float stmix;
} waveformconfig_t;

const float sharpness = 512.f;
const waveformconfig_t wfconfig[2][5] = {
{
    /* kevtris chip G (6581) */
    { 0.880815f, 0.f, 0.f, 0.3279614f, 0.5999545f }, // error 1795
    { 0.8924618f, 2.014781f, 1.003332f, 0.02992322f, 0.0f }, // error 11610
    { 0.8646501f, 1.712586f, 1.137704f, 0.02845423f, 0.f }, // error 21307
    { 0.9527834f, 1.794777f, 0.f, 0.09806272f, 0.7752482f }, // error 196
    { 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, },
}, {
    /* kevtris chip V (8580) */
    { 0.9781665f, 0.f, 0.9899469f, 8.087667f, 0.8226412f }, // error 5546
    { 0.9097769f, 2.039997f, 0.9584096f, 0.1765447f, 0.f }, // error 18763
    { 0.9231212f, 2.084788f, 0.9493895f, 0.1712518f, 0.f }, // error 17103
    { 0.9845552f, 1.415612f, 0.9703883f, 3.68829f, 0.8265008f }, // error 3319
    { 0.5f, 0.0f, 1.0f, 0.0f, 0.0f },
}
};

/* render output from bitstate */
static float make_sample(float *o) {
  float out = 0;
  for (int i = 0; i < 12; i ++) {
    out += o[i] * dac[i];
  }
  return out;
}

/* generate tables for waveforms 1 .. 7 */
void WaveformGeneratorFP::rebuild_wftable() {
    float o[12];
    reg8 oldwf = waveform;
    reg24 oldacc = accumulator;
    reg12 oldpw = pw;

    for (waveform = 1; waveform < 8; waveform ++) {
        for (accumulator = 0; accumulator < (1<<24); accumulator += (1<<12)) {
            /* generate pulse-low variants. Also,
             * when waveform < 4, pw doesn't matter. */
            pw = 0x1000; /* pulse always low */
            calculate_waveform_sample(o);
            wftable[waveform - 1][accumulator >> 12] = make_sample(o) + wave_zero;
            /* Add pulse-high variants after pulse-low state variants */
            if (waveform >= 4) {
                pw = 0x000; /* pulse always high */
                calculate_waveform_sample(o);
                wftable[waveform + 3][accumulator >> 12] = make_sample(o) + wave_zero;
            }
        }
    }

    waveform = oldwf;
    accumulator = oldacc;
    pw = oldpw;
}

/* explode reg12 to a floating point bit array */
static void populate(reg12 v, float o[12])
{
    int j = 1;
    for (int i = 0; i < 12; i ++) {
        o[i] = v & j ? 1.f : 0.f;
        j <<= 1;
    }
}

/* waveform values valid are 1 .. 7 */
void WaveformGeneratorFP::calculate_waveform_sample(float o[12])
{
  int i;

  /* P */
  if (waveform == 4) {
    populate((accumulator >> 12) >= pw ? 0xfff : 0x000, o);
    return;
  }

  const waveformconfig_t config = wfconfig[
    model == MOS6581FP ? 0 : 1
  ][
    waveform == 3 ? 0 :
    waveform == 5 ? 1 :
    waveform == 6 ? 2 :
    waveform == 7 ? 3 :
                    4
  ];

  /* S with strong top bit for 6581 */
  populate(accumulator >> 12, o);

  /* convert to T */
  if ((waveform & 3) == 1) {
    bool top = (accumulator & 0x800000) != 0;
    for (i = 11; i > 0; i --) {
      if (top) {
        o[i] = 1.0f - o[i-1];
      } else {
        o[i] = o[i-1];
      }
    }
    o[0] = 0;
  }

  /* convert to ST */
  if ((waveform & 3) == 3) {
    /* bottom bit is grounded via T waveform selector */
    o[0] *= config.stmix;
    for (i = 1; i < 12; i ++) {
      o[i] = o[i - 1] * (1.f - config.stmix) + o[i] * config.stmix;
    }
  }

  o[11] *= config.topbit;

  /* ST, P* waveform? */
  if (waveform == 3 || waveform > 4) {
    float distancetable[12 * 2 + 1];
    for (i = 0; i <= 12; i ++) {
        distancetable[12+i] = distancetable[12-i] = 1.f / (1.f + i * i * config.distance);
    }

    float pulse = (accumulator >> 12) >= pw ? 1.f : -1.f;
    pulse *= config.pulsestrength;

    float tmp[12];
    for (i = 0; i < 12; i ++) {
        float avg = 0;
        float n = 0;
        for (int j = 0; j < 12; j ++) {
            float weight = distancetable[i - j + 12];
            avg += o[j] * weight;
            n += weight;
        }
        /* pulse control bit */
        if (waveform > 4) {
            float weight = distancetable[i - 12 + 12];
            avg += pulse * weight;
            n += weight;
        }

        tmp[i] = (o[i] + avg / n) * 0.5f;
    }

    for (i = 0; i < 12; i ++) {
        o[i] = tmp[i];
    }
  }

  /* use the environment around bias value to set/clear dac bit. The
   * relationship is nonlinear because that seems to sound a bit better. */
  for (i = 0; i < 12; i ++) {
    o[i] = (o[i] - config.bias) * sharpness;

    o[i] += 0.5f;
    if (o[i] > 1.f) {
       o[i] = 1.f;
    }
    if (o[i] < 0.f) {
        o[i] = 0.;
    }
  }
}

void WaveformGeneratorFP::set_nonlinearity(float nl)
{
  for (int i = 0; i < 12; i ++) {
    dac[i] = SIDFP::kinked_dac((1 << i), nl, 12);
  }
}

// ----------------------------------------------------------------------------
// Constructor.
// ----------------------------------------------------------------------------
WaveformGeneratorFP::WaveformGeneratorFP()
{
  set_chip_model(MOS6581FP);
  reset();
}

// ----------------------------------------------------------------------------
// Set chip model.
// ----------------------------------------------------------------------------
void WaveformGeneratorFP::set_chip_model(chip_model model)
{
  this->model = model;
  wave_zero = static_cast<float>(model == MOS6581FP ? -0x380 : -0x800);
}


// ----------------------------------------------------------------------------
// Register functions.
// ----------------------------------------------------------------------------
void WaveformGeneratorFP::writeFREQ_LO(reg8 freq_lo)
{
  freq = (freq & 0xff00) | (freq_lo & 0xff);
}

void WaveformGeneratorFP::writeFREQ_HI(reg8 freq_hi)
{
  freq = ((freq_hi << 8) & 0xff00) | (freq & 0xff);
}

void WaveformGeneratorFP::writePW_LO(reg8 pw_lo)
{
  pw = (pw & 0xf00) | (pw_lo & 0x0ff);
  update_pw();
}

void WaveformGeneratorFP::writePW_HI(reg8 pw_hi)
{
  pw = ((pw_hi << 8) & 0xf00) | (pw & 0x0ff);
  update_pw();
}

void WaveformGeneratorFP::writeCONTROL_REG(WaveformGeneratorFP& source, reg8 control)
{
  /* when selecting the 0 waveform, the previous output is held for
   * a time in the DAC MOSFET gates. We keep on holding forever, though... */
  reg4 waveform_next = (control >> 4) & 0x0f;
  if (waveform_next == 0 && waveform >= 1 && waveform <= 7) {
    /* NB: "6581" version is the undelayed variant. We want that. */
    previous = readOSC6581(source);
    previous_dac = output(source);
  }

  waveform = waveform_next;
  ring = (control & 0x04) != 0 && (waveform & 0x3) == 1 ? 0x800000 : 0;
  sync = (control & 0x02) != 0;
  bool test_next = (control & 0x08) != 0;

  // Test bit rising? Invert bit 19 and write it to bit 1.
  if (test_next && !test) {
    accumulator = 0;
    noise_overwrite_delay = 200000; /* 200 ms, probably too generous? */
  } else {
    // Test bit falling? clock noise once,
    // otherwise just emulate noise's combined waveforms.
    if (! test_next) {
      clock_noise(test);
    }
  }
 
  test = test_next;
  update_pw();
}

reg8 WaveformGeneratorFP::readOSC6581(WaveformGeneratorFP& source) {
    return readOSC(source.accumulator, accumulator);
}

reg8 WaveformGeneratorFP::readOSC8580(WaveformGeneratorFP& source) {
    return readOSC(source.accumulator_prev, accumulator_prev);
}

reg8 WaveformGeneratorFP::readOSC(reg24 ring_accumulator, reg24 my_accumulator)
{
  float o[12];

  if (waveform == 0 || waveform > 7) {
    return previous;
  }

  /* Include effects of the test bit & ring mod */
  reg12 oldpw = pw;
  if (test) {
    pw = 0;
  }
  reg24 oldaccumulator = accumulator;
  accumulator = my_accumulator ^ (ring_accumulator & ring);
  calculate_waveform_sample(o);
  pw = oldpw;
  accumulator = oldaccumulator;

  reg8 out = 0;
  reg8 bit = 1;
  for (int i = 4; i < 12; i ++) {
    if (o[i] > 0.5f) {
        out |= bit;
    }
    bit <<= 1;
  }
  return out;
}

void WaveformGeneratorFP::clock_noise(const bool clock)
{
  if (clock) {
    reg24 bit0 = (((shift_register >> 22) | (test ? 1 : 0)) ^ (shift_register >> 17)) & 0x1;
    shift_register <<= 1;
    shift_register |= bit0;
  }

  // clear output bits of shift register if noise and other waveforms
  // are selected simultaneously
  if (waveform > 8) {
    shift_register &= 0x7fffff^(1<<20)^(1<<18)^(1<<14)^(1<<11)^(1<<9)^(1<<5)^(1<<2)^(1<<0);
  }

  if (waveform >= 8) {
    previous = outputN___();
    previous_dac = wave_zero;
    for (int i = 0; i < 8; i ++) {
      if (previous & (1 << i)) {
        previous_dac += dac[i+4];
      }
    }
  }
}

reg8 WaveformGeneratorFP::outputN___()
{
  return
    ((shift_register & (1 << 20)) >> 13) |
    ((shift_register & (1 << 18)) >> 12) |
    ((shift_register & (1 << 14)) >> 9) |
    ((shift_register & (1 << 11)) >> 7) |
    ((shift_register & (1 <<  9)) >> 6) |
    ((shift_register & (1 <<  5)) >> 3) |
    ((shift_register & (1 <<  2)) >> 1) |
    ((shift_register & (1 <<  0)));
}

// ----------------------------------------------------------------------------
// SID reset.
// ----------------------------------------------------------------------------
void WaveformGeneratorFP::reset()
{
  accumulator_prev = accumulator = 0;
  previous = 0;
  previous_dac = 0;
  shift_register = 0x7fffff;
  freq = 0;
  pw = 0;
  test = 0;
  waveform = 0;
  writeCONTROL_REG(*this, 0);
}
