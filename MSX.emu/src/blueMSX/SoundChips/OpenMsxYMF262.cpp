// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.

// $Id: OpenMsxYMF262.cpp,v 1.8 2009-07-18 15:08:35 dvik Exp $

/*
 *
 * File: ymf262.c - software implementation of YMF262
 *                  FM sound generator type OPL3
 *
 * Copyright (C) 2003 Jarek Burczynski
 *
 * Version 0.2
 *
 *
 * Revision History:
 *
 * 03-03-2003: initial release
 *  - thanks to Olivier Galibert and Chris Hardy for YMF262 and YAC512 chips
 *  - thanks to Stiletto for the datasheets
 *
 *
 *
 * differences between OPL2 and OPL3 not documented in Yamaha datahasheets:
 * - sinus table is a little different: the negative part is off by one...
 *
 * - in order to enable selection of four different waveforms on OPL2
 *   one must set bit 5 in register 0x01(test).
 *   on OPL3 this bit is ignored and 4-waveform select works *always*.
 *   (Don't confuse this with OPL3's 8-waveform select.)
 *
 * - Envelope Generator: all 15 x rates take zero time on OPL3
 *   (on OPL2 15 0 and 15 1 rates take some time while 15 2 and 15 3 rates
 *   take zero time)
 *
 * - channel calculations: output of operator 1 is in perfect sync with
 *   output of operator 2 on OPL3; on OPL and OPL2 output of operator 1
 *   is always delayed by one sample compared to output of operator 2
 *
 *
 * differences between OPL2 and OPL3 shown in datasheets:
 * - YMF262 does not support CSM mode
 */

#include "OpenMsxYMF262.h"
#include <cmath>
#include <cstring>


extern "C" {
#include "SaveState.h"
}

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

const DoubleT PI = 3.14159265358979323846;

const int FREQ_SH   = 16;  // 16.16 fixed point (frequency calculations)
const int EG_SH     = 16;  // 16.16 fixed point (EG timing)
const int LFO_SH    = 24;  //  8.24 fixed point (LFO calculations)
const int TIMER_SH  = 16;  // 16.16 fixed point (timers calculations)
const int FREQ_MASK = (1 << FREQ_SH) - 1;
const unsigned int EG_TIMER_OVERFLOW = 1 << EG_SH;

// envelope output entries
const int ENV_BITS    = 10;
const int ENV_LEN     = 1 << ENV_BITS;
const DoubleT ENV_STEP = 128.0 / ENV_LEN;

const int MAX_ATT_INDEX = (1 << (ENV_BITS - 1)) - 1; //511
const int MIN_ATT_INDEX = 0;

// sinwave entries
const int SIN_BITS = 10;
const int SIN_LEN  = 1 << SIN_BITS;
const int SIN_MASK = SIN_LEN - 1;

const int TL_RES_LEN = 256;	// 8 bits addressing (real chip)

// register number to channel number , slot offset
const byte SLOT1 = 0;
const byte SLOT2 = 1;

// Envelope Generator phases
const int EG_ATT = 4;
const int EG_DEC = 3;
const int EG_SUS = 2;
const int EG_REL = 1;
const int EG_OFF = 0;


// mapping of register number (offset) to slot number used by the emulator
const int slot_array[32] =
{
	 0,  2,  4,  1,  3,  5, -1, -1,
	 6,  8, 10,  7,  9, 11, -1, -1,
	12, 14, 16, 13, 15, 17, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1
};

// key scale level
// table is 3dB/octave , DV converts this into 6dB/octave
// 0.1875 is bit 0 weight of the envelope counter (volume) expressed in the 'decibel' scale
#define DV(x) (unsigned)(x / (0.1875/2.0))
const unsigned ksl_tab[8 * 16] =
{
	// OCT 0
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	// OCT 1
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 0.750), DV( 1.125), DV( 1.500),
	DV( 1.875), DV( 2.250), DV( 2.625), DV( 3.000),
	// OCT 2
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 0.000),
	DV( 0.000), DV( 1.125), DV( 1.875), DV( 2.625),
	DV( 3.000), DV( 3.750), DV( 4.125), DV( 4.500),
	DV( 4.875), DV( 5.250), DV( 5.625), DV( 6.000),
	// OCT 3
	DV( 0.000), DV( 0.000), DV( 0.000), DV( 1.875),
	DV( 3.000), DV( 4.125), DV( 4.875), DV( 5.625),
	DV( 6.000), DV( 6.750), DV( 7.125), DV( 7.500),
	DV( 7.875), DV( 8.250), DV( 8.625), DV( 9.000),
	// OCT 4 
	DV( 0.000), DV( 0.000), DV( 3.000), DV( 4.875),
	DV( 6.000), DV( 7.125), DV( 7.875), DV( 8.625),
	DV( 9.000), DV( 9.750), DV(10.125), DV(10.500),
	DV(10.875), DV(11.250), DV(11.625), DV(12.000),
	// OCT 5 
	DV( 0.000), DV( 3.000), DV( 6.000), DV( 7.875),
	DV( 9.000), DV(10.125), DV(10.875), DV(11.625),
	DV(12.000), DV(12.750), DV(13.125), DV(13.500),
	DV(13.875), DV(14.250), DV(14.625), DV(15.000),
	// OCT 6 
	DV( 0.000), DV( 6.000), DV( 9.000), DV(10.875),
	DV(12.000), DV(13.125), DV(13.875), DV(14.625),
	DV(15.000), DV(15.750), DV(16.125), DV(16.500),
	DV(16.875), DV(17.250), DV(17.625), DV(18.000),
	// OCT 7 
	DV( 0.000), DV( 9.000), DV(12.000), DV(13.875),
	DV(15.000), DV(16.125), DV(16.875), DV(17.625),
	DV(18.000), DV(18.750), DV(19.125), DV(19.500),
	DV(19.875), DV(20.250), DV(20.625), DV(21.000)
};
#undef DV

// sustain level table (3dB per step) 
// 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)
#define SC(db) (unsigned) (db * (2.0/ENV_STEP))
const unsigned sl_tab[16] = {
 SC( 0), SC( 1), SC( 2), SC(3 ), SC(4 ), SC(5 ), SC(6 ), SC( 7),
 SC( 8), SC( 9), SC(10), SC(11), SC(12), SC(13), SC(14), SC(31)
};
#undef SC


const byte RATE_STEPS = 8;
const byte eg_inc[15 * RATE_STEPS] =
{
//cycle:0 1  2 3  4 5  6 7
		0,1, 0,1, 0,1, 0,1, //  0  rates 00..12 0 (increment by 0 or 1)
		0,1, 0,1, 1,1, 0,1, //  1  rates 00..12 1
		0,1, 1,1, 0,1, 1,1, //  2  rates 00..12 2
		0,1, 1,1, 1,1, 1,1, //  3  rates 00..12 3

		1,1, 1,1, 1,1, 1,1, //  4  rate 13 0 (increment by 1)
		1,1, 1,2, 1,1, 1,2, //  5  rate 13 1
		1,2, 1,2, 1,2, 1,2, //  6  rate 13 2
		1,2, 2,2, 1,2, 2,2, //  7  rate 13 3

		2,2, 2,2, 2,2, 2,2, //  8  rate 14 0 (increment by 2)
		2,2, 2,4, 2,2, 2,4, //  9  rate 14 1
		2,4, 2,4, 2,4, 2,4, // 10  rate 14 2
		2,4, 4,4, 2,4, 4,4, // 11  rate 14 3

		4,4, 4,4, 4,4, 4,4, // 12  rates 15 0, 15 1, 15 2, 15 3 for decay
		8,8, 8,8, 8,8, 8,8, // 13  rates 15 0, 15 1, 15 2, 15 3 for attack (zero time)
		0,0, 0,0, 0,0, 0,0, // 14  infinity rates for attack and decay(s)
};


#define O(a) (a*RATE_STEPS)
// note that there is no O(13) in this table - it's directly in the code
const byte eg_rate_select[16 + 64 + 16] =
{
	// Envelope Generator rates (16 + 64 rates + 16 RKS)
	// 16 infinite time rates
	O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),
	O(14),O(14),O(14),O(14),O(14),O(14),O(14),O(14),

	// rates 00-12
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),
	O( 0),O( 1),O( 2),O( 3),

	// rate 13 
	O( 4),O( 5),O( 6),O( 7),

	// rate 14 
	O( 8),O( 9),O(10),O(11),

	// rate 15 
	O(12),O(12),O(12),O(12),

	// 16 dummy rates (same as 15 3) 
	O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
	O(12),O(12),O(12),O(12),O(12),O(12),O(12),O(12),
};
#undef O

//rate  0,    1,    2,    3,   4,   5,   6,  7,  8,  9,  10, 11, 12, 13, 14, 15 
//shift 12,   11,   10,   9,   8,   7,   6,  5,  4,  3,  2,  1,  0,  0,  0,  0  
//mask  4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3,  1,  0,  0,  0,  0  
#define O(a) (a*1)
const byte eg_rate_shift[16 + 64 + 16] =
{
	// Envelope Generator counter shifts (16 + 64 rates + 16 RKS) 
	// 16 infinite time rates 
	O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
	O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

	// rates 00-15 
	O(12),O(12),O(12),O(12),
	O(11),O(11),O(11),O(11),
	O(10),O(10),O(10),O(10),
	O( 9),O( 9),O( 9),O( 9),
	O( 8),O( 8),O( 8),O( 8),
	O( 7),O( 7),O( 7),O( 7),
	O( 6),O( 6),O( 6),O( 6),
	O( 5),O( 5),O( 5),O( 5),
	O( 4),O( 4),O( 4),O( 4),
	O( 3),O( 3),O( 3),O( 3),
	O( 2),O( 2),O( 2),O( 2),
	O( 1),O( 1),O( 1),O( 1),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),

	// 16 dummy rates (same as 15 3)
	O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
};
#undef O


// multiple table
#define ML(x) (byte)(2 * x)
const byte mul_tab[16] =
{
	// 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,10,12,12,15,15
	ML( 0.5),ML( 1.0),ML( 2.0),ML( 3.0),ML( 4.0),ML( 5.0),ML( 6.0),ML( 7.0),
	ML( 8.0),ML( 9.0),ML(10.0),ML(10.0),ML(12.0),ML(12.0),ML(15.0),ML(15.0)
};
#undef ML

// TL_TAB_LEN is calculated as:
//  (12+1)=13 - sinus amplitude bits     (Y axis)
//  additional 1: to compensate for calculations of negative part of waveform
//  (if we don't add it then the greatest possible _negative_ value would be -2
//  and we really need -1 for waveform #7)
//  2  - sinus sign bit           (Y axis)
//  TL_RES_LEN - sinus resolution (X axis)

const int TL_TAB_LEN = 13 * 2 * TL_RES_LEN;
static int tl_tab[TL_TAB_LEN];
const int ENV_QUIET = TL_TAB_LEN >> 4;

// sin waveform table in 'decibel' scale
// there are eight waveforms on OPL3 chips
static unsigned int sin_tab[SIN_LEN * 8];


// LFO Amplitude Modulation table (verified on real YM3812)
//  27 output levels (triangle waveform); 1 level takes one of: 192, 256 or 448 samples
//
// Length: 210 elements
//
// Each of the elements has to be repeated
// exactly 64 times (on 64 consecutive samples).
// The whole table takes: 64 * 210 = 13440 samples.
//
// When AM = 1 data is used directly
// When AM = 0 data is divided by 4 before being used (loosing precision is important)

const unsigned int LFO_AM_TAB_ELEMENTS = 210;
const byte lfo_am_table[LFO_AM_TAB_ELEMENTS] =
{
	0,0,0,0,0,0,0,
	1,1,1,1,
	2,2,2,2,
	3,3,3,3,
	4,4,4,4,
	5,5,5,5,
	6,6,6,6,
	7,7,7,7,
	8,8,8,8,
	9,9,9,9,
	10,10,10,10,
	11,11,11,11,
	12,12,12,12,
	13,13,13,13,
	14,14,14,14,
	15,15,15,15,
	16,16,16,16,
	17,17,17,17,
	18,18,18,18,
	19,19,19,19,
	20,20,20,20,
	21,21,21,21,
	22,22,22,22,
	23,23,23,23,
	24,24,24,24,
	25,25,25,25,
	26,26,26,
	25,25,25,25,
	24,24,24,24,
	23,23,23,23,
	22,22,22,22,
	21,21,21,21,
	20,20,20,20,
	19,19,19,19,
	18,18,18,18,
	17,17,17,17,
	16,16,16,16,
	15,15,15,15,
	14,14,14,14,
	13,13,13,13,
	12,12,12,12,
	11,11,11,11,
	10,10,10,10,
	9,9,9,9,
	8,8,8,8,
	7,7,7,7,
	6,6,6,6,
	5,5,5,5,
	4,4,4,4,
	3,3,3,3,
	2,2,2,2,
	1,1,1,1
};

// LFO Phase Modulation table (verified on real YM3812) 
const int lfo_pm_table[8 * 8 * 2] =
{
	// FNUM2/FNUM = 00 0xxxxxxx (0x0000)
	0, 0, 0, 0, 0, 0, 0, 0,	//LFO PM depth = 0
	0, 0, 0, 0, 0, 0, 0, 0,	//LFO PM depth = 1

	// FNUM2/FNUM = 00 1xxxxxxx (0x0080)
	0, 0, 0, 0, 0, 0, 0, 0,	//LFO PM depth = 0
	1, 0, 0, 0,-1, 0, 0, 0,	//LFO PM depth = 1

	// FNUM2/FNUM = 01 0xxxxxxx (0x0100)
	1, 0, 0, 0,-1, 0, 0, 0,	//LFO PM depth = 0
	2, 1, 0,-1,-2,-1, 0, 1,	//LFO PM depth = 1

	// FNUM2/FNUM = 01 1xxxxxxx (0x0180)
	1, 0, 0, 0,-1, 0, 0, 0,	//LFO PM depth = 0
	3, 1, 0,-1,-3,-1, 0, 1,	//LFO PM depth = 1

	// FNUM2/FNUM = 10 0xxxxxxx (0x0200)
	2, 1, 0,-1,-2,-1, 0, 1,	//LFO PM depth = 0
	4, 2, 0,-2,-4,-2, 0, 2,	//LFO PM depth = 1

	// FNUM2/FNUM = 10 1xxxxxxx (0x0280)
	2, 1, 0,-1,-2,-1, 0, 1,	//LFO PM depth = 0
	5, 2, 0,-2,-5,-2, 0, 2,	//LFO PM depth = 1

	// FNUM2/FNUM = 11 0xxxxxxx (0x0300)
	3, 1, 0,-1,-3,-1, 0, 1,	//LFO PM depth = 0
	6, 3, 0,-3,-6,-3, 0, 3,	//LFO PM depth = 1

	// FNUM2/FNUM = 11 1xxxxxxx (0x0380)
	3, 1, 0,-1,-3,-1, 0, 1,	//LFO PM depth = 0
	7, 3, 0,-3,-7,-3, 0, 3	//LFO PM depth = 1
};


static int* chanOut;
#define PHASE_MOD1 18
#define PHASE_MOD2 19



YMF262Slot::YMF262Slot()
{
	ar = dr = rr = KSR = ksl = ksr = mul = 0;
	Cnt = Incr = FB = op1_out[0] = op1_out[1] = CON = 0;
	connect = 0;
	eg_type = state = TL = TLL = volume = sl = 0;
	eg_m_ar = eg_sh_ar = eg_sel_ar = eg_m_dr = eg_sh_dr = 0;
	eg_sel_dr = eg_m_rr = eg_sh_rr = eg_sel_rr = 0;
	key = AMmask = vib = waveform_number = wavetable = 0;
}

YMF262Channel::YMF262Channel()
{
	block_fnum = fc = ksl_base = kcode = extended = 0;
}


void YMF262::callback(byte flag)
{
	setStatus(flag);
}

// status set and IRQ handling
void YMF262::setStatus(byte flag)
{
	// set status flag masking out disabled IRQs 
	status |= flag;
	if (status & statusMask) {
		status |= 0x80;
		irq.set();
	}
}

// status reset and IRQ handling 
void YMF262::resetStatus(byte flag)
{
	// reset status flag 
	status &= ~flag;
	if (!(status & statusMask)) {
		status &= 0x7F;
		irq.reset();
	}
}

// IRQ mask set
void YMF262::changeStatusMask(byte flag)
{
	statusMask = flag;
	status &= statusMask;
	if (status) {
		status |= 0x80;
		irq.set();
	} else {
		status &= 0x7F;
		irq.reset();
	}
}


// advance LFO to next sample
void YMF262::advance_lfo()
{
	// LFO 
	lfo_am_cnt += lfo_am_inc;
	if (lfo_am_cnt >= (LFO_AM_TAB_ELEMENTS << LFO_SH)) {
		// lfo_am_table is 210 elements long 
		lfo_am_cnt -= (LFO_AM_TAB_ELEMENTS << LFO_SH);
	}

	byte tmp = lfo_am_table[lfo_am_cnt >> LFO_SH];
	if (lfo_am_depth) {
		LFO_AM = tmp;
	} else {
		LFO_AM = tmp >> 2;
	}
	lfo_pm_cnt += lfo_pm_inc;
	LFO_PM = ((lfo_pm_cnt >> LFO_SH) & 7) | lfo_pm_depth_range;
}

// advance to next sample 
void YMF262::advance()
{
	eg_timer += eg_timer_add;

    if (eg_timer > 4 * EG_TIMER_OVERFLOW) {
        eg_timer = EG_TIMER_OVERFLOW;
    }
	while (eg_timer >= EG_TIMER_OVERFLOW) {
		eg_timer -= EG_TIMER_OVERFLOW;
		eg_cnt++;

		for (int i = 0; i < 18 * 2; i++) {
			YMF262Channel &ch = channels[i / 2];
			YMF262Slot &op = ch.slots[i & 1];
			// Envelope Generator 
			switch(op.state) {
			case EG_ATT:	// attack phase 
				if (!(eg_cnt & op.eg_m_ar)) {
					op.volume += (~op.volume * eg_inc[op.eg_sel_ar + ((eg_cnt >> op.eg_sh_ar) & 7)]) >> 3;
					if (op.volume <= MIN_ATT_INDEX) {
						op.volume = MIN_ATT_INDEX;
						op.state = EG_DEC;
					}
				}
				break;

			case EG_DEC:	// decay phase 
				if (!(eg_cnt & op.eg_m_dr)) {
					op.volume += eg_inc[op.eg_sel_dr + ((eg_cnt >> op.eg_sh_dr) & 7)];
					if (op.volume >= op.sl) {
						op.state = EG_SUS;
					}
				}
				break;

			case EG_SUS:	// sustain phase 
				// this is important behaviour:
				// one can change percusive/non-percussive
				// modes on the fly and the chip will remain
				// in sustain phase - verified on real YM3812 
				if (op.eg_type) {
					// non-percussive mode 
					// do nothing 
				} else {
					// percussive mode 
					// during sustain phase chip adds Release Rate (in percussive mode) 
					if (!(eg_cnt & op.eg_m_rr)) {
						op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt>>op.eg_sh_rr) & 7)];
						if (op.volume >= MAX_ATT_INDEX) {
							op.volume = MAX_ATT_INDEX;
						}
					} else {
						// do nothing in sustain phase
					}
				}
				break;

			case EG_REL:	// release phase 
				if (!(eg_cnt & op.eg_m_rr)) {
					op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt>>op.eg_sh_rr) & 7)];
					if (op.volume >= MAX_ATT_INDEX) {
						op.volume = MAX_ATT_INDEX;
						op.state = EG_OFF;
					}
				}
			break;

			default:
				break;
			}
		}
	}

    int i;
	for (i = 0; i < 18 * 2; i++) {
		YMF262Channel &ch = channels[i / 2];
		YMF262Slot &op = ch.slots[i & 1];

		// Phase Generator 
		if (op.vib) {
			byte block;
			unsigned int block_fnum = ch.block_fnum;
			unsigned int fnum_lfo   = (block_fnum & 0x0380) >> 7;
			signed int lfo_fn_table_index_offset = lfo_pm_table[LFO_PM + 16 * fnum_lfo];

			if (lfo_fn_table_index_offset) {
				// LFO phase modulation active 
				block_fnum += lfo_fn_table_index_offset;
				block = (block_fnum & 0x1c00) >> 10;
				op.Cnt += (fn_tab[block_fnum & 0x03ff] >> (7 - block)) * op.mul;
			} else {
				// LFO phase modulation  = zero 
				op.Cnt += op.Incr;
			}
		} else {
			// LFO phase modulation disabled for this operator 
			op.Cnt += op.Incr;
		}
	}

	// The Noise Generator of the YM3812 is 23-bit shift register.
	// Period is equal to 2^23-2 samples.
	// Register works at sampling frequency of the chip, so output
	// can change on every sample.
	//
	// Output of the register and input to the bit 22 is:
	// bit0 XOR bit14 XOR bit15 XOR bit22
	//
	// Simply use bit 22 as the noise output.
	noise_p += noise_f;
    i = (noise_p >> FREQ_SH) & 0x1f;		// number of events (shifts of the shift register) 
	noise_p &= FREQ_MASK;
	while (i--) {
		// unsigned j = ( (noise_rng) ^ (noise_rng>>14) ^ (noise_rng>>15) ^ (noise_rng>>22) ) & 1;
		// noise_rng = (j<<22) | (noise_rng>>1);
		//
		// Instead of doing all the logic operations above, we
		// use a trick here (and use bit 0 as the noise output).
		// The difference is only that the noise bit changes one
		// step ahead. This doesn't matter since we don't know
		// what is real state of the noise_rng after the reset.

		if (noise_rng & 1) {
			noise_rng ^= 0x800302;
		}
		noise_rng >>= 1;
	}
}


signed int op_calc(unsigned phase, unsigned env, signed int pm, unsigned int wave_tab)
{
	int i = (phase & ~FREQ_MASK) + (pm << 16);
	int p = (env << 4) + sin_tab[wave_tab + ((i >> FREQ_SH ) & SIN_MASK)];
	if (p >= TL_TAB_LEN) {
		return 0;
	}
	return tl_tab[p];
}

signed int op_calc1(unsigned phase, unsigned int env, signed int pm, unsigned int wave_tab)
{
	int i = (phase & ~FREQ_MASK) + pm;
	int p = (env << 4) + sin_tab[wave_tab + ((i >> FREQ_SH) & SIN_MASK)];
	if (p >= TL_TAB_LEN) {
		return 0;
	}
	return tl_tab[p];
}

inline int YMF262Slot::volume_calc(byte LFO_AM)
{
	return TLL + volume + (LFO_AM & AMmask);
}

// calculate output of a standard 2 operator channel
// (or 1st part of a 4-op channel) 
void YMF262Channel::chan_calc(byte LFO_AM)
{
    chanOut[PHASE_MOD1] = 0;
    chanOut[PHASE_MOD2] = 0;
	chanOut[PHASE_MOD1]  = 0;
	chanOut[PHASE_MOD2] = 0;

	// SLOT 1 
	int env = slots[SLOT1].volume_calc(LFO_AM);
	int out = slots[SLOT1].op1_out[0] + slots[SLOT1].op1_out[1];
	slots[SLOT1].op1_out[0] = slots[SLOT1].op1_out[1];
	slots[SLOT1].op1_out[1] = 0;
	if (env < ENV_QUIET) {
		if (!slots[SLOT1].FB) {
			out = 0;
		}
		slots[SLOT1].op1_out[1] = op_calc1(slots[SLOT1].Cnt, env, (out<<slots[SLOT1].FB), slots[SLOT1].wavetable);
	}
	chanOut[slots[SLOT1].connect] += slots[SLOT1].op1_out[1];

	// SLOT 2 
	env = slots[SLOT2].volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		chanOut[slots[SLOT2].connect] += op_calc(slots[SLOT2].Cnt, env, chanOut[PHASE_MOD1], slots[SLOT2].wavetable);
	}
}

// calculate output of a 2nd part of 4-op channel 
void YMF262Channel::chan_calc_ext(byte LFO_AM)
{
	chanOut[PHASE_MOD1] = 0;

	// SLOT 1
	int env  = slots[SLOT1].volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		chanOut[slots[SLOT1].connect] += op_calc(slots[SLOT1].Cnt, env, chanOut[PHASE_MOD2], slots[SLOT1].wavetable );
	}

	// SLOT 2
	env = slots[SLOT2].volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		chanOut[slots[SLOT2].connect] += op_calc(slots[SLOT2].Cnt, env, chanOut[PHASE_MOD1], slots[SLOT2].wavetable);
	}
}

// operators used in the rhythm sounds generation process:
//
// Envelope Generator:
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// / slot   number    TL ARDR SLRR Wave Drum  Hat   Drum  Tom  Cymbal
//  6 / 0   12        50  70   90   f0  +
//  6 / 1   15        53  73   93   f3  +
//  7 / 0   13        51  71   91   f1        +
//  7 / 1   16        54  74   94   f4              +
//  8 / 0   14        52  72   92   f2                    +
//  8 / 1   17        55  75   95   f5                          +
//
// Phase Generator:
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// / slot   number    MULTIPLE          Drum  Hat   Drum  Tom  Cymbal
//  6 / 0   12        30                +
//  6 / 1   15        33                +
//  7 / 0   13        31                      +     +           +
//  7 / 1   16        34                -----  n o t  u s e d -----
//  8 / 0   14        32                                  +
//  8 / 1   17        35                      +                 +
//
// channel  operator  register number   Bass  High  Snare Tom  Top
// number   number    BLK/FNUM2 FNUM    Drum  Hat   Drum  Tom  Cymbal
//    6     12,15     B6        A6      +
//
//    7     13,16     B7        A7            +     +           +
//
//    8     14,17     B8        A8            +           +     +

// calculate rhythm 
void YMF262::chan_calc_rhythm(bool noise)
{
	YMF262Slot& SLOT6_1 = channels[6].slots[SLOT1];
	YMF262Slot& SLOT6_2 = channels[6].slots[SLOT2];
	YMF262Slot& SLOT7_1 = channels[7].slots[SLOT1];
	YMF262Slot& SLOT7_2 = channels[7].slots[SLOT2];
	YMF262Slot& SLOT8_1 = channels[8].slots[SLOT1];
	YMF262Slot& SLOT8_2 = channels[8].slots[SLOT2];

	// Bass Drum (verified on real YM3812):
	//  - depends on the channel 6 'connect' register:
	//      when connect = 0 it works the same as in normal (non-rhythm) mode (op1->op2->out)
	//      when connect = 1 _only_ operator 2 is present on output (op2->out), operator 1 is ignored
	//  - output sample always is multiplied by 2

	chanOut[PHASE_MOD1] = 0;

	// SLOT 1 
	int env = SLOT6_1.volume_calc(LFO_AM);
	int out = SLOT6_1.op1_out[0] + SLOT6_1.op1_out[1];
	SLOT6_1.op1_out[0] = SLOT6_1.op1_out[1];

	if (!SLOT6_1.CON) {
		chanOut[PHASE_MOD1] = SLOT6_1.op1_out[0];
	} else {
		// ignore output of operator 1
	}

	SLOT6_1.op1_out[1] = 0;
	if (env < ENV_QUIET) {
		if (!SLOT6_1.FB) {
			out = 0;
		}
		SLOT6_1.op1_out[1] = op_calc1(SLOT6_1.Cnt, env, (out << SLOT6_1.FB), SLOT6_1.wavetable);
	}

	// SLOT 2 
	env = SLOT6_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		chanout[6] += op_calc(SLOT6_2.Cnt, env, chanOut[PHASE_MOD1], SLOT6_2.wavetable) * 2;
	}

	// Phase generation is based on: 
	// HH  (13) channel 7->slot 1 combined with channel 8->slot 2 (same combination as TOP CYMBAL but different output phases)
	// SD  (16) channel 7->slot 1
	// TOM (14) channel 8->slot 1
	// TOP (17) channel 7->slot 1 combined with channel 8->slot 2 (same combination as HIGH HAT but different output phases)

	// Envelope generation based on: 
	// HH  channel 7->slot1
	// SD  channel 7->slot2
	// TOM channel 8->slot1
	// TOP channel 8->slot2

	// The following formulas can be well optimized.
	// I leave them in direct form for now (in case I've missed something).

	// High Hat (verified on real YM3812) 
	env = SLOT7_1.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// high hat phase generation:
		// phase = d0 or 234 (based on frequency only)
		// phase = 34 or 2d0 (based on noise)

		// base frequency derived from operator 1 in channel 7 
		bool bit7 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x80) != 0;
		bool bit3 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x08) != 0;
		bool bit2 = ((SLOT7_1.Cnt >> FREQ_SH) & 0x04) != 0;
		bool res1 = ((bit2 ^ bit7) | bit3) != 0;
		// when res1 = 0 phase = 0x000 | 0xd0; 
		// when res1 = 1 phase = 0x200 | (0xd0>>2); 
		unsigned phase = res1 ? (0x200|(0xd0>>2)) : 0xd0;

		// enable gate based on frequency of operator 2 in channel 8 
		bool bit5e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x20) != 0;
		bool bit3e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x08) != 0;
		bool res2 = (bit3e ^ bit5e) != 0;
		// when res2 = 0 pass the phase from calculation above (res1); 
		// when res2 = 1 phase = 0x200 | (0xd0>>2); 
		if (res2) {
			phase = (0x200|(0xd0>>2));
		}

		// when phase & 0x200 is set and noise=1 then phase = 0x200|0xd0 
		// when phase & 0x200 is set and noise=0 then phase = 0x200|(0xd0>>2), ie no change 
		if (phase&0x200) {
			if (noise) {
				phase = 0x200|0xd0;
			}
		} else {
		// when phase & 0x200 is clear and noise=1 then phase = 0xd0>>2 
		// when phase & 0x200 is clear and noise=0 then phase = 0xd0, ie no change 
			if (noise) {
				phase = 0xd0>>2;
			}
		}
		chanout[7] += op_calc(phase<<FREQ_SH, env, 0, SLOT7_1.wavetable) * 2;
	}

	// Snare Drum (verified on real YM3812) 
	env = SLOT7_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// base frequency derived from operator 1 in channel 7 
		bool bit8 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x100) != 0;
		// when bit8 = 0 phase = 0x100; 
		// when bit8 = 1 phase = 0x200; 
		unsigned phase = bit8 ? 0x200 : 0x100;

		// Noise bit XOR'es phase by 0x100 
		// when noisebit = 0 pass the phase from calculation above 
		// when noisebit = 1 phase ^= 0x100;
		// in other words: phase ^= (noisebit<<8); 
		if (noise) {
			phase ^= 0x100;
		}
		chanout[7] += op_calc(phase<<FREQ_SH, env, 0, SLOT7_2.wavetable) * 2;
	}

	// Tom Tom (verified on real YM3812) 
	env = SLOT8_1.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		chanout[8] += op_calc(SLOT8_1.Cnt, env, 0, SLOT8_1.wavetable) * 2;
	}

	// Top Cymbal (verified on real YM3812) 
	env = SLOT8_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// base frequency derived from operator 1 in channel 7 
		bool bit7 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x80) != 0;
		bool bit3 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x08) != 0;
		bool bit2 = ((SLOT7_1.Cnt>>FREQ_SH) & 0x04) != 0;
		bool res1 = ((bit2 ^ bit7) | bit3) != 0;
		// when res1 = 0 phase = 0x000 | 0x100; 
		// when res1 = 1 phase = 0x200 | 0x100; 
		unsigned phase = res1 ? 0x300 : 0x100;

		// enable gate based on frequency of operator 2 in channel 8 
		bool bit5e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x20) != 0;
		bool bit3e= ((SLOT8_2.Cnt>>FREQ_SH) & 0x08) != 0;
		bool res2 = (bit3e ^ bit5e) != 0;
		// when res2 = 0 pass the phase from calculation above (res1);
		// when res2 = 1 phase = 0x200 | 0x100; 
		if (res2) {
			phase = 0x300;
		}
		chanout[8] += op_calc(phase<<FREQ_SH, env, 0, SLOT8_2.wavetable) * 2;
	}
}


// generic table initialize 
void YMF262::init_tables(void)
{
    int i;
	static bool alreadyInit = false;
	if (alreadyInit) {
		return;
	}
	alreadyInit = true;

	for (int x = 0; x < TL_RES_LEN; x++) {
		DoubleT m = (1 << 16) / pow((DoubleT)2, DoubleT((x + 1) * (ENV_STEP / 4.0) / 8.0));
		m = floor(m);

		// we never reach (1<<16) here due to the (x+1) 
		// result fits within 16 bits at maximum 
		int n = (int)m;		// 16 bits here 
		n >>= 4;		// 12 bits here 
		if (n & 1) {		// round to nearest 
			n = (n >> 1) + 1;
		} else {
			n = n >> 1;
		}
		// 11 bits here (rounded) 
		n <<= 1;		// 12 bits here (as in real chip) 
		tl_tab[x * 2 + 0] = n;
		tl_tab[x * 2 + 1] = ~tl_tab[x * 2 + 0]; // this _is_ different from OPL2 (verified on real YMF262)

		for (i = 1; i < 13; i++) {
			tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN] =  tl_tab[x * 2 + 0] >> i;
			tl_tab[x * 2 + 1 + i * 2 * TL_RES_LEN] = ~tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN];  // this _is_ different from OPL2 (verified on real YMF262) 
		}
	}

	const DoubleT LOG2 = ::log((DoubleT)2);
	for (i = 0; i < SIN_LEN; i++) {
		// non-standard sinus
		DoubleT m = sin(((i * 2) + 1) * PI / SIN_LEN); // checked against the real chip
		// we never reach zero here due to ((i * 2) + 1) 
		DoubleT o = (m > 0.0) ?
			8 * ::log( 1.0 / m) / LOG2:	// convert to 'decibels' 
			8 * ::log(-1.0 / m) / LOG2;	// convert to 'decibels'
		o = o / (ENV_STEP / 4);

		int n = (int)(2 * o);
		if (n & 1) {// round to nearest 
			n = (n>>1)+1;
		} else {
			n = n>>1;
		}
		sin_tab[i] = n * 2 + (m >=0.0 ? 0 : 1);
	}

	for (i = 0; i < SIN_LEN; i++) {
		// these 'pictures' represent _two_ cycles 
		// waveform 1:  __      __     
		//             /  \____/  \____
		// output only first half of the sinus waveform (positive one) 
		if (i & (1 << (SIN_BITS - 1))) {
			sin_tab[1*SIN_LEN+i] = TL_TAB_LEN;
		} else {
			sin_tab[1*SIN_LEN+i] = sin_tab[i];
		}
		
		// waveform 2:  __  __  __  __ 
		//             /  \/  \/  \/  \.
		// abs(sin) 
		sin_tab[2 * SIN_LEN + i] = sin_tab[i & (SIN_MASK >> 1)];

		// waveform 3:  _   _   _   _  
		//             / |_/ |_/ |_/ |_
		// abs(output only first quarter of the sinus waveform) 
		if (i & (1<<(SIN_BITS-2))) {
			sin_tab[3*SIN_LEN+i] = TL_TAB_LEN;
		} else {
			sin_tab[3*SIN_LEN+i] = sin_tab[i & (SIN_MASK>>2)];
		}

		// waveform 4:                 
		//             /\  ____/\  ____
		//               \/      \/    
		// output whole sinus waveform in half the cycle(step=2) and output 0 on the other half of cycle
		if (i & (1 << (SIN_BITS-1))) {
			sin_tab[4*SIN_LEN+i] = TL_TAB_LEN;
		} else {
			sin_tab[4*SIN_LEN+i] = sin_tab[i*2];
		}

		// waveform 5:                 
		//             /\/\____/\/\____
		//                             
		// output abs(whole sinus) waveform in half the cycle(step=2) and output 0 on the other half of cycle 
		if (i & (1 << (SIN_BITS-1))) {
			sin_tab[5*SIN_LEN+i] = TL_TAB_LEN;
		} else {
			sin_tab[5*SIN_LEN+i] = sin_tab[(i*2) & (SIN_MASK>>1)];
		}

		// waveform 6: ____    ____    
		//                             
		//                 ____    ____
		// output maximum in half the cycle and output minimum on the other half of cycle 
		if (i & (1 << (SIN_BITS - 1))) {
			sin_tab[6*SIN_LEN+i] = 1;	// negative 
		} else {
			sin_tab[6*SIN_LEN+i] = 0;	// positive
		}

		// waveform 7:                 
		//             |\____  |\____  
		//                   \|      \|
		// output sawtooth waveform    
		int x = (i & (1 << (SIN_BITS - 1))) ?
			((SIN_LEN - 1) - i) * 16 + 1 : // negative: from 8177 to 1 
			i * 16;                        //positive: from 0 to 8176 
		if (x > TL_TAB_LEN) {
			x = TL_TAB_LEN;	// clip to the allowed range 
		}
		sin_tab[7 * SIN_LEN+i] = x;
	}
}


void YMF262::setSampleRate(int sampleRate, int Oversampling)
{
    oplOversampling = Oversampling;
	const int CLCK_FREQ = 14318180;
	DoubleT freqbase  = ((DoubleT)CLCK_FREQ / (8.0 * 36)) / (DoubleT)(sampleRate * oplOversampling);

	// make fnumber -> increment counter table 
	for (int i = 0; i < 1024; i++) {
		// opn phase increment counter = 20bit 
		// -10 because chip works with 10.10 fixed point, while we use 16.16 
		fn_tab[i] = (unsigned)( (DoubleT)i * 64 * freqbase * (1<<(FREQ_SH - 10)));
	}

	// Amplitude modulation: 27 output levels (triangle waveform);
	// 1 level takes one of: 192, 256 or 448 samples 
	// One entry from LFO_AM_TABLE lasts for 64 samples 
	lfo_am_inc = (unsigned)((1 << LFO_SH) * freqbase / 64.0);

	// Vibrato: 8 output levels (triangle waveform); 1 level takes 1024 samples
	lfo_pm_inc = (unsigned)((1 << LFO_SH) * freqbase / 1024.0);

	// Noise generator: a step takes 1 sample 
	noise_f = (unsigned)((1 << FREQ_SH) * freqbase);

	eg_timer_add  = (unsigned)((1 << EG_SH) * freqbase);
}

void YMF262Slot::FM_KEYON(byte key_set)
{
	if (!key) {
		// restart Phase Generator 
		Cnt = 0;
		// phase -> Attack 
		state = EG_ATT;
	}
	key |= key_set;
}

void YMF262Slot::FM_KEYOFF(byte key_clr)
{
	if (key) {
		key &= key_clr;
		if (!key) {
			// phase -> Release 
			if (state > EG_REL) {
				state = EG_REL;
			}
		}
	}
}

// update phase increment counter of operator (also update the EG rates if necessary) 
void YMF262Channel::CALC_FCSLOT(YMF262Slot &slot)
{
	// (frequency) phase increment counter 
	slot.Incr = fc * slot.mul;
	int ksr = kcode >> slot.KSR;

	if (slot.ksr != ksr) {
		slot.ksr = ksr;

		// calculate envelope generator rates 
		if ((slot.ar + slot.ksr) < 16+60) {
			slot.eg_sh_ar  = eg_rate_shift [slot.ar + slot.ksr ];
			slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
			slot.eg_sel_ar = eg_rate_select[slot.ar + slot.ksr ];
		} else {
			slot.eg_sh_ar  = 0;
			slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
			slot.eg_sel_ar = 13 * RATE_STEPS;
		}
		slot.eg_sh_dr  = eg_rate_shift [slot.dr + slot.ksr ];
		slot.eg_m_dr   = (1 << slot.eg_sh_dr) - 1;
		slot.eg_sel_dr = eg_rate_select[slot.dr + slot.ksr ];
		slot.eg_sh_rr  = eg_rate_shift [slot.rr + slot.ksr ];
		slot.eg_m_rr   = (1 << slot.eg_sh_rr) - 1;
		slot.eg_sel_rr = eg_rate_select[slot.rr + slot.ksr ];
	}
}

// set multi,am,vib,EG-TYP,KSR,mul 
void YMF262::set_mul(byte sl, byte v)
{
	int chan_no = sl / 2;
	YMF262Channel &ch  = channels[chan_no];
	YMF262Slot &slot = ch.slots[sl & 1];

	slot.mul     = mul_tab[v & 0x0f];
	slot.KSR     = (v & 0x10) ? 0 : 2;
	slot.eg_type = (v & 0x20);
	slot.vib     = (v & 0x40);
	slot.AMmask  = (v & 0x80) ? ~0 : 0;

	if (OPL3_mode) {
		// in OPL3 mode
		// DO THIS:
		//  if this is one of the slots of 1st channel forming up a 4-op channel
		//  do normal operation
		//  else normal 2 operator function
		// OR THIS:
		//  if this is one of the slots of 2nd channel forming up a 4-op channel
		//  update it using channel data of 1st channel of a pair
		//  else normal 2 operator function
		switch(chan_no) {
		case 0: case 1: case 2:
		case 9: case 10: case 11:
			if (ch.extended) {
				// normal
				ch.CALC_FCSLOT(slot);
			} else {
				// normal 
				ch.CALC_FCSLOT(slot);
			}
			break;
		case 3: case 4: case 5:
		case 12: case 13: case 14: {
			YMF262Channel &ch3 = channels[chan_no - 3];
			if (ch3.extended) {
				// update this slot using frequency data for 1st channel of a pair 
				ch3.CALC_FCSLOT(slot);
			} else {
				// normal 
				ch.CALC_FCSLOT(slot);
			}
			break;
		}
		default:
			// normal 
			ch.CALC_FCSLOT(slot);
			break;
		}
	} else {
		// in OPL2 mode 
		ch.CALC_FCSLOT(slot);
	}
}

// set ksl & tl 
void YMF262::set_ksl_tl(byte sl, byte v)
{
	int chan_no = sl/2;
	YMF262Channel &ch = channels[chan_no];
	YMF262Slot &slot = ch.slots[sl & 1];

	int ksl = v >> 6; // 0 / 1.5 / 3.0 / 6.0 dB/OCT 

	slot.ksl = ksl ? 3 - ksl : 31;
	slot.TL  = (v & 0x3F) << (ENV_BITS - 1 - 7); // 7 bits TL (bit 6 = always 0) 

	if (OPL3_mode) {

		// in OPL3 mode 
		//DO THIS:
		//if this is one of the slots of 1st channel forming up a 4-op channel
		//do normal operation
		//else normal 2 operator function
		//OR THIS:
		//if this is one of the slots of 2nd channel forming up a 4-op channel
		//update it using channel data of 1st channel of a pair
		//else normal 2 operator function
		switch(chan_no) {
		case 0: case 1: case 2:
		case 9: case 10: case 11:
			if (ch.extended) {
				// normal 
				slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
			} else {
				// normal 
				slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
			}
			break;
		case 3: case 4: case 5:
		case 12: case 13: case 14: {
			YMF262Channel &ch3 = channels[chan_no - 3];
			if (ch3.extended) {
				// update this slot using frequency data for 1st channel of a pair 
				slot.TLL = slot.TL + (ch3.ksl_base >> slot.ksl);
			} else {
				// normal 
				slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
			}
			break;
		}
		default:
			// normal
			slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
			break;
		}
	} else {
		// in OPL2 mode 
		slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
	}
}

// set attack rate & decay rate  
void YMF262::set_ar_dr(byte sl, byte v)
{
	YMF262Channel &ch = channels[sl / 2];
	YMF262Slot &slot = ch.slots[sl & 1];

	slot.ar = (v >> 4) ? 16 + ((v >> 4) << 2) : 0;

	if ((slot.ar + slot.ksr) < 16 + 60) {
		// verified on real YMF262 - all 15 x rates take "zero" time 
		slot.eg_sh_ar  = eg_rate_shift [slot.ar + slot.ksr];
		slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
		slot.eg_sel_ar = eg_rate_select[slot.ar + slot.ksr];
	} else {
		slot.eg_sh_ar  = 0;
		slot.eg_m_ar   = (1 << slot.eg_sh_ar) - 1;
		slot.eg_sel_ar = 13 * RATE_STEPS;
	}

	slot.dr    = (v & 0x0F) ? 16 + ((v & 0x0F) << 2) : 0;
	slot.eg_sh_dr  = eg_rate_shift [slot.dr + slot.ksr];
	slot.eg_m_dr   = (1 << slot.eg_sh_dr) - 1;
	slot.eg_sel_dr = eg_rate_select[slot.dr + slot.ksr];
}

// set sustain level & release rate 
void YMF262::set_sl_rr(byte sl, byte v)
{
	YMF262Channel &ch = channels[sl / 2];
	YMF262Slot &slot = ch.slots[sl & 1];

	slot.sl  = sl_tab[v >> 4];
	slot.rr  = (v & 0x0F) ? 16 + ((v & 0x0F) << 2) : 0;
	slot.eg_sh_rr  = eg_rate_shift [slot.rr + slot.ksr];
	slot.eg_m_rr   = (1 << slot.eg_sh_rr) - 1;
	slot.eg_sel_rr = eg_rate_select[slot.rr + slot.ksr];
}

void YMF262::update_channels(YMF262Channel &ch)
{
	// update channel passed as a parameter and a channel at CH+=3; 
	if (ch.extended) {
		// we've just switched to combined 4 operator mode 
	} else {
		// we've just switched to normal 2 operator mode 
	}
}

byte YMF262::peekReg(int r)
{
	return reg[r];
}

byte YMF262::readReg(int r)
{
	return reg[r];
}

void YMF262::writeReg(int r, byte v, const EmuTime &time)
{
	if (!OPL3_mode && (r != 0x105)) {
		// in OPL2 mode the only accessible in set #2 is register 0x05 
		r &= ~0x100;
	}
	writeRegForce(r, v, time);
	checkMute();
}
void YMF262::writeRegForce(int r, byte v, const EmuTime &time)
{
	reg[r] = v;

	byte ch_offset = 0;
	if (r & 0x100) {
		switch(r) {
		case 0x101:	// test register
			return;
		
		case 0x104: { // 6 channels enable 
			YMF262Channel &ch0 = channels[0];
			byte prev = ch0.extended;
			ch0.extended = (v >> 0) & 1;
			if (prev != ch0.extended) {
				update_channels(ch0);
			}
			YMF262Channel &ch1 = channels[1];
			prev = ch1.extended;
			ch1.extended = (v >> 1) & 1;
			if (prev != ch1.extended) {
				update_channels(ch1);
			}
			YMF262Channel &ch2 = channels[2];
			prev = ch2.extended;
			ch2.extended = (v >> 2) & 1;
			if (prev != ch2.extended) {
				update_channels(ch2);
			}
			YMF262Channel &ch9 = channels[9];
			prev = ch9.extended;
			ch9.extended = (v >> 3) & 1;
			if (prev != ch9.extended) {
				update_channels(ch9);
			}
			YMF262Channel &ch10 = channels[10];
			prev = ch10.extended;
			ch10.extended = (v >> 4) & 1;
			if (prev != ch10.extended) {
				update_channels(ch10);
			}
			YMF262Channel &ch11 = channels[11];
			prev = ch11.extended;
			ch11.extended = (v >> 5) & 1;
			if (prev != ch11.extended) {
				update_channels(ch11);
			}
			return;
		}
		case 0x105:	// OPL3 extensions enable register 
			// OPL3 mode when bit0=1 otherwise it is OPL2 mode 
			OPL3_mode = v & 0x01;
			if (OPL3_mode) {
				status2 = 0x02;
			}
			
			// following behaviour was tested on real YMF262,
			// switching OPL3/OPL2 modes on the fly:
			//  - does not change the waveform previously selected
			//    (unless when ....)
			//  - does not update CH.A, CH.B, CH.C and CH.D output
			//    selectors (registers c0-c8) (unless when ....)
			//  - does not disable channels 9-17 on OPL3->OPL2 switch
			//  - does not switch 4 operator channels back to 2
			//    operator channels
			return;

		default:
			break;
		}
		ch_offset = 9;	// register page #2 starts from channel 9
	}

	r &= 0xFF;
	switch(r & 0xE0) {
	case 0x00: // 00-1F:control 
		switch(r & 0x1F) {
		case 0x01: // test register
			break;
			
		case 0x02: // Timer 1 
			timer1.setValue(v);
			break;

		case 0x03: // Timer 2 
			timer2.setValue(v);
			break;

		case 0x04: // IRQ clear / mask and Timer enable 
			if (v & 0x80) {
				// IRQ flags clear 
				resetStatus(0x60);
			} else {
				changeStatusMask((~v) & 0x60);
				timer1.setStart((v & R04_ST1) != 0, time);
				timer2.setStart((v & R04_ST2) != 0, time);
			}
			break;
			
		case 0x08: // x,NTS,x,x, x,x,x,x
			nts = v;
			break;
			
		default:
			break;
		}
		break;
	
	case 0x20: { // am ON, vib ON, ksr, eg_type, mul 
		int slot = slot_array[r & 0x1F];
		if (slot < 0) return;
		set_mul(slot + ch_offset * 2, v);
		break;
	}
	case 0x40: {
		int slot = slot_array[r & 0x1F];
		if (slot < 0) return;
		set_ksl_tl(slot + ch_offset * 2, v);
		break;
	}
	case 0x60: {
		int slot = slot_array[r & 0x1F];
		if (slot < 0) return;
		set_ar_dr(slot + ch_offset * 2, v);
		break;
	}
	case 0x80: {
		int slot = slot_array[r & 0x1F];
		if (slot < 0) return;
		set_sl_rr(slot + ch_offset * 2, v);
		break;
	}
	case 0xA0: {
		if (r == 0xBD) {
			// am depth, vibrato depth, r,bd,sd,tom,tc,hh 
			if (ch_offset != 0) {
				// 0xbd register is present in set #1 only 
				return;
			}
			lfo_am_depth = v & 0x80;
			lfo_pm_depth_range = (v & 0x40) ? 8 : 0;
			rhythm = v & 0x3F;

			if (rhythm & 0x20) {
				// BD key on/off 
				if (v & 0x10) {
					channels[6].slots[SLOT1].FM_KEYON ( 2);
					channels[6].slots[SLOT2].FM_KEYON ( 2);
				} else {
					channels[6].slots[SLOT1].FM_KEYOFF(~2);
					channels[6].slots[SLOT2].FM_KEYOFF(~2);
				}
				// HH key on/off 
				if (v & 0x01) {
					channels[7].slots[SLOT1].FM_KEYON ( 2);
				} else {
					channels[7].slots[SLOT1].FM_KEYOFF(~2);
				}
				// SD key on/off 
				if (v & 0x08) {
					channels[7].slots[SLOT2].FM_KEYON ( 2);
				} else {
					channels[7].slots[SLOT2].FM_KEYOFF(~2);
				}
				// TOM key on/off 
				if (v & 0x04) {
					channels[8].slots[SLOT1].FM_KEYON ( 2);
				} else {
					channels[8].slots[SLOT1].FM_KEYOFF(~2);
				}
				// TOP-CY key on/off 
				if (v & 0x02) {
					channels[8].slots[SLOT2].FM_KEYON ( 2);
				} else {
					channels[8].slots[SLOT2].FM_KEYOFF(~2);
				}
			} else {
				// BD key off 
				channels[6].slots[SLOT1].FM_KEYOFF(~2);
				channels[6].slots[SLOT2].FM_KEYOFF(~2);
				// HH key off 
				channels[7].slots[SLOT1].FM_KEYOFF(~2);
				// SD key off 
				channels[7].slots[SLOT2].FM_KEYOFF(~2);
				// TOM key off 
				channels[8].slots[SLOT1].FM_KEYOFF(~2);
				// TOP-CY off 
				channels[8].slots[SLOT2].FM_KEYOFF(~2);
			}
			return;
		}

		// keyon,block,fnum 
		if ((r & 0x0F) > 8) {
			return;
		}
		int chan_no = (r & 0x0F) + ch_offset;
		YMF262Channel &ch  = channels[chan_no];
		YMF262Channel &ch3 = channels[chan_no + 3];
		int block_fnum;
		if (!(r & 0x10)) {
			// a0-a8 
			block_fnum  = (ch.block_fnum&0x1F00) | v;
		} else {
			// b0-b8 
			block_fnum = ((v & 0x1F) << 8) | (ch.block_fnum & 0xFF);
			if (OPL3_mode) {
				// in OPL3 mode 
				// DO THIS:
				// if this is 1st channel forming up a 4-op channel
				// ALSO keyon/off slots of 2nd channel forming up 4-op channel
				// else normal 2 operator function keyon/off
				// OR THIS:
				// if this is 2nd channel forming up 4-op channel just do nothing
				// else normal 2 operator function keyon/off
				switch(chan_no) {
				case 0: case 1: case 2:
				case 9: case 10: case 11:
					if (ch.extended) {
						//if this is 1st channel forming up a 4-op channel
						//ALSO keyon/off slots of 2nd channel forming up 4-op channel
						if (v & 0x20) {
							ch.slots[SLOT1].FM_KEYON ( 1);
							ch.slots[SLOT2].FM_KEYON ( 1);
							ch3.slots[SLOT1].FM_KEYON( 1);
							ch3.slots[SLOT2].FM_KEYON( 1);
						} else {
							ch.slots[SLOT1].FM_KEYOFF (~1);
							ch.slots[SLOT2].FM_KEYOFF (~1);
							ch3.slots[SLOT1].FM_KEYOFF(~1);
							ch3.slots[SLOT2].FM_KEYOFF(~1);
						}
					} else {
						//else normal 2 operator function keyon/off
						if (v & 0x20) {
							ch.slots[SLOT1].FM_KEYON ( 1);
							ch.slots[SLOT2].FM_KEYON ( 1);
						} else {
							ch.slots[SLOT1].FM_KEYOFF(~1);
							ch.slots[SLOT2].FM_KEYOFF(~1);
						}
					}
					break;

				case 3: case 4: case 5:
				case 12: case 13: case 14: {
					YMF262Channel &ch_3 = channels[chan_no - 3];
					if (ch_3.extended) {
						//if this is 2nd channel forming up 4-op channel just do nothing
					} else {
						//else normal 2 operator function keyon/off
						if (v & 0x20) {
							ch.slots[SLOT1].FM_KEYON ( 1);
							ch.slots[SLOT2].FM_KEYON ( 1);
						} else {
							ch.slots[SLOT1].FM_KEYOFF(~1);
							ch.slots[SLOT2].FM_KEYOFF(~1);
						}
					}
					break;
				}
				default:
					if (v & 0x20) {
						ch.slots[SLOT1].FM_KEYON ( 1);
						ch.slots[SLOT2].FM_KEYON ( 1);
					} else {
						ch.slots[SLOT1].FM_KEYOFF(~1);
						ch.slots[SLOT2].FM_KEYOFF(~1);
					}
					break;
				}
			} else {
				if (v & 0x20) {
					ch.slots[SLOT1].FM_KEYON ( 1);
					ch.slots[SLOT2].FM_KEYON ( 1);
				} else {
					ch.slots[SLOT1].FM_KEYOFF(~1);
					ch.slots[SLOT2].FM_KEYOFF(~1);
				}
			}
		}
		// update
		if (ch.block_fnum != block_fnum) {
			byte block  = block_fnum >> 10;
			ch.block_fnum = block_fnum;
			ch.ksl_base = ksl_tab[block_fnum >> 6];
			ch.fc       = fn_tab[block_fnum & 0x03FF] >> (7 - block);

			// BLK 2,1,0 bits -> bits 3,2,1 of kcode 
			ch.kcode = (ch.block_fnum & 0x1C00) >> 9;

			// the info below is actually opposite to what is stated
			// in the Manuals (verifed on real YMF262)
			// if notesel == 0 -> lsb of kcode is bit 10 (MSB) of fnum  
			// if notesel == 1 -> lsb of kcode is bit 9 (MSB-1) of fnum 
			if (nts & 0x40) {
				ch.kcode |= (ch.block_fnum & 0x100) >> 8;	// notesel == 1 
			} else {
				ch.kcode |= (ch.block_fnum & 0x200) >> 9;	// notesel == 0 
			}
			if (OPL3_mode) {
				int chan_no = (r & 0x0F) + ch_offset;
				// in OPL3 mode 
				//DO THIS:
				//if this is 1st channel forming up a 4-op channel
				//ALSO update slots of 2nd channel forming up 4-op channel
				//else normal 2 operator function keyon/off
				//OR THIS:
				//if this is 2nd channel forming up 4-op channel just do nothing
				//else normal 2 operator function keyon/off
				switch(chan_no) {
				case 0: case 1: case 2:
				case 9: case 10: case 11:
					if (ch.extended) {
						//if this is 1st channel forming up a 4-op channel
						//ALSO update slots of 2nd channel forming up 4-op channel

						// refresh Total Level in FOUR SLOTs of this channel and channel+3 using data from THIS channel 
						ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
						ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);
						ch3.slots[SLOT1].TLL = ch3.slots[SLOT1].TL + (ch.ksl_base >> ch3.slots[SLOT1].ksl);
						ch3.slots[SLOT2].TLL = ch3.slots[SLOT2].TL + (ch.ksl_base >> ch3.slots[SLOT2].ksl);

						// refresh frequency counter in FOUR SLOTs of this channel and channel+3 using data from THIS channel 
						ch.CALC_FCSLOT(ch.slots[SLOT1]);
						ch.CALC_FCSLOT(ch.slots[SLOT2]);
						ch.CALC_FCSLOT(ch3.slots[SLOT1]);
						ch.CALC_FCSLOT(ch3.slots[SLOT2]);
					} else {
						//else normal 2 operator function
						// refresh Total Level in both SLOTs of this channel 
						ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
						ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

						// refresh frequency counter in both SLOTs of this channel 
						ch.CALC_FCSLOT(ch.slots[SLOT1]);
						ch.CALC_FCSLOT(ch.slots[SLOT2]);
					}
					break;

				case 3: case 4: case 5:
				case 12: case 13: case 14: {
					YMF262Channel &ch_3 = channels[chan_no - 3];
					if (ch_3.extended) {
						//if this is 2nd channel forming up 4-op channel just do nothing
					} else {
						//else normal 2 operator function
						// refresh Total Level in both SLOTs of this channel 
						ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
						ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

						// refresh frequency counter in both SLOTs of this channel 
						ch.CALC_FCSLOT(ch.slots[SLOT1]);
						ch.CALC_FCSLOT(ch.slots[SLOT2]);
					}
					break;
				}
				default:
					// refresh Total Level in both SLOTs of this channel 
					ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
					ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

					// refresh frequency counter in both SLOTs of this channel 
					ch.CALC_FCSLOT(ch.slots[SLOT1]);
					ch.CALC_FCSLOT(ch.slots[SLOT2]);
					break;
				}
			} else {
				// in OPL2 mode 
				// refresh Total Level in both SLOTs of this channel 
				ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
				ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

				// refresh frequency counter in both SLOTs of this channel 
				ch.CALC_FCSLOT(ch.slots[SLOT1]);
				ch.CALC_FCSLOT(ch.slots[SLOT2]);
			}
		}
		break;
	}
	case 0xC0: {
		// CH.D, CH.C, CH.B, CH.A, FB(3bits), C 
		if ((r & 0xF) > 8) {
			return;
		}
		int chan_no = (r & 0x0F) + ch_offset;
		YMF262Channel &ch = channels[chan_no];

		int base = chan_no * 4;
		if (OPL3_mode) {
			// OPL3 mode 
			pan[base + 0] = (v & 0x10) ? ~0 : 0;	// ch.A 
			pan[base + 1] = (v & 0x20) ? ~0 : 0;	// ch.B 
			pan[base + 2] = (v & 0x40) ? ~0 : 0;	// ch.C 
			pan[base + 3] = (v & 0x80) ? ~0 : 0;	// ch.D
		} else {
			// OPL2 mode - always enabled 
			pan[base + 0] = ~0;	// ch.A 
			pan[base + 1] = ~0;	// ch.B 
			pan[base + 2] = ~0;	// ch.C 
			pan[base + 3] = ~0;	// ch.D 
		}

		ch.slots[SLOT1].FB  = (v >> 1) & 7 ? ((v >> 1) & 7) + 7 : 0;
		ch.slots[SLOT1].CON = v & 1;

		if (OPL3_mode) {
			switch(chan_no) {
			case 0: case 1: case 2:
			case 9: case 10: case 11:
				if (ch.extended) {
					YMF262Channel &ch3 = channels[chan_no + 3];
					switch((ch.slots[SLOT1].CON << 1) | ch3.slots[SLOT1].CON) {
					case 0:
						// 1 -> 2 -> 3 -> 4 - out 
						ch.slots[SLOT1].connect = PHASE_MOD1;
						ch.slots[SLOT2].connect = PHASE_MOD2;
						ch3.slots[SLOT1].connect = PHASE_MOD1;
						ch3.slots[SLOT2].connect = chan_no + 3;
						break;
						
					case 1:
						// 1 -> 2 -\.
						// 3 -> 4 -+- out 
						ch.slots[SLOT1].connect = PHASE_MOD1;
						ch.slots[SLOT2].connect = chan_no;
						ch3.slots[SLOT1].connect = PHASE_MOD1;
						ch3.slots[SLOT2].connect = chan_no + 3;
						break;
						
					case 2:
						// 1 -----------\.
						// 2 -> 3 -> 4 -+- out 
						ch.slots[SLOT1].connect = chan_no;
						ch.slots[SLOT2].connect = PHASE_MOD2;
						ch3.slots[SLOT1].connect = PHASE_MOD1;
						ch3.slots[SLOT2].connect = chan_no + 3;
						break;

					case 3:
						// 1 ------\.
						// 2 -> 3 -+- out
						// 4 ------/     
						ch.slots[SLOT1].connect = chan_no;
						ch.slots[SLOT2].connect = PHASE_MOD2;
						ch3.slots[SLOT1].connect = chan_no + 3;
						ch3.slots[SLOT2].connect = chan_no + 3;
						break;
					}
				} else {
					// 2 operators mode 
					ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
					ch.slots[SLOT2].connect = chan_no;
				}
				break;

			case 3: case 4: case 5:
			case 12: case 13: case 14: {
				YMF262Channel &ch3 = channels[chan_no - 3];
				if (ch3.extended) {
					switch((ch3.slots[SLOT1].CON << 1) | ch.slots[SLOT1].CON) {
					case 0:
						// 1 -> 2 -> 3 -> 4 - out 
						ch3.slots[SLOT1].connect = PHASE_MOD1;
						ch3.slots[SLOT2].connect = PHASE_MOD2;
						ch.slots[SLOT1].connect = PHASE_MOD1;
						ch.slots[SLOT2].connect = chan_no;
						break;

					case 1:
						// 1 -> 2 -\.
						// 3 -> 4 -+- out 
						ch3.slots[SLOT1].connect = PHASE_MOD1;
						ch3.slots[SLOT2].connect = chan_no - 3;
						ch.slots[SLOT1].connect = PHASE_MOD1;
						ch.slots[SLOT2].connect = chan_no;
						break;
						
					case 2:
						// 1 -----------\.
						// 2 -> 3 -> 4 -+- out 
						ch3.slots[SLOT1].connect = chan_no - 3;
						ch3.slots[SLOT2].connect = PHASE_MOD2;
						ch.slots[SLOT1].connect = PHASE_MOD1;
						ch.slots[SLOT2].connect = chan_no;
						break;
						
					case 3:
						// 1 ------\.
						// 2 -> 3 -+- out
						// 4 ------/     
						ch3.slots[SLOT1].connect = chan_no - 3;
						ch3.slots[SLOT2].connect = PHASE_MOD2;
						ch.slots[SLOT1].connect = chan_no;
						ch.slots[SLOT2].connect = chan_no;
						break;
					}
				} else {
					// 2 operators mode 
					ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
					ch.slots[SLOT2].connect = chan_no;
				}
				break;
			}
			default:
				// 2 operators mode 
				ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
				ch.slots[SLOT2].connect = chan_no;
				break;
			}
		} else {
			// OPL2 mode - always 2 operators mode
			ch.slots[SLOT1].connect = ch.slots[SLOT1].CON ? chan_no : PHASE_MOD1;
			ch.slots[SLOT2].connect = chan_no;
		}
		break;
	}
	case 0xE0: {
		// waveform select 
		int slot = slot_array[r & 0x1f];
		if (slot < 0) return;
		slot += ch_offset * 2;
		YMF262Channel &ch = channels[slot / 2];

		// store 3-bit value written regardless of current OPL2 or OPL3
		// mode... (verified on real YMF262) 
		v &= 7;
		ch.slots[slot & 1].waveform_number = v;
		// ... but select only waveforms 0-3 in OPL2 mode 
		if (!OPL3_mode) {
			v &= 3;
		}
		ch.slots[slot & 1].wavetable = v * SIN_LEN;
		break;
	}
	}
}


void YMF262::reset(const EmuTime &time)
{
	eg_timer = 0;
	eg_cnt   = 0;

	noise_rng = 1;	// noise shift register
	nts       = 0;	// note split
	resetStatus(0x60);

	// reset with register write
	writeRegForce(0x01, 0, time); // test register
	writeRegForce(0x02, 0, time); // Timer1
	writeRegForce(0x03, 0, time); // Timer2
	writeRegForce(0x04, 0, time); // IRQ mask clear

	//FIX IT  registers 101, 104 and 105
	//FIX IT (dont change CH.D, CH.C, CH.B and CH.A in C0-C8 registers)
    int c;
	for (c = 0xFF; c >= 0x20; c--) {
		writeRegForce(c, 0, time);
	}
	//FIX IT (dont change CH.D, CH.C, CH.B and CH.A in C0-C8 registers)
	for (c = 0x1FF; c >= 0x120; c--) {
		writeRegForce(c, 0, time);
	}

	// reset operator parameters 
	for (c = 0; c < 9 * 2; c++) {
		YMF262Channel &ch = channels[c];
		for (int s = 0; s < 2; s++) {
			ch.slots[s].state  = EG_OFF;
			ch.slots[s].volume = MAX_ATT_INDEX;
		}
	}
	setInternalMute(true);
}

YMF262::YMF262(short volume, const EmuTime &time, void* ref)
	: timer1(this, ref), timer2(this, ref)
{
    chanOut = chanout;

	LFO_AM = LFO_PM = 0;
	lfo_am_depth = lfo_pm_depth_range = lfo_am_cnt = lfo_pm_cnt = 0;
	noise_rng = noise_p = 0;
	rhythm = nts = 0;
	OPL3_mode = false;
	status = status2 = statusMask = 0;
	
    oplOversampling = 1;

	init_tables();

	reset(time);
}

YMF262::~YMF262()
{
}

byte YMF262::peekStatus()
{
	return status | status2;
}

byte YMF262::readStatus()
{
	byte result = status | status2;
	status2 = 0;
	return result;
}

void YMF262::checkMute()
{
	bool mute = checkMuteHelper();
	//PRT_DEBUG("YMF262: muted " << mute);
	setInternalMute(mute);
}
bool YMF262::checkMuteHelper()
{
	// TODO this doesn't always mute when possible
	for (int i = 0; i < 18; i++) {
		for (int j = 0; j < 2; j++) {
			YMF262Slot &sl = channels[i].slots[j];
			if (!((sl.state == EG_OFF) ||
			      ((sl.state == EG_REL) &&
			       ((sl.TLL + sl.volume) >= ENV_QUIET)))) {
				return false;
			}
		}
	}
	return true;
}

int* YMF262::updateBuffer(int length)
{
	if (isInternalMuted()) {
		return NULL;
	}
	
	bool rhythmEnabled = (rhythm & 0x20) != 0;

	int* buf = buffer;
	while (length--) {
		int a = 0;
		int b = 0;
		int c = 0;
		int d = 0;
        int count = oplOversampling;
        while (count--) {
		    advance_lfo();

		    // clear channel outputs 
		    memset(chanout, 0, sizeof(int) * 18);

		    // register set #1 
		    // extended 4op ch#0 part 1 or 2op ch#0 
		    channels[0].chan_calc(LFO_AM);
		    if (channels[0].extended) {
			    // extended 4op ch#0 part 2 
			    channels[3].chan_calc_ext(LFO_AM);
		    } else {
			    // standard 2op ch#3 
			    channels[3].chan_calc(LFO_AM);
		    }

		    // extended 4op ch#1 part 1 or 2op ch#1 
		    channels[1].chan_calc(LFO_AM);
		    if (channels[1].extended) {
			    // extended 4op ch#1 part 2 
			    channels[4].chan_calc_ext(LFO_AM);
		    } else {
			    // standard 2op ch#4 
			    channels[4].chan_calc(LFO_AM);
		    }

		    // extended 4op ch#2 part 1 or 2op ch#2 
		    channels[2].chan_calc(LFO_AM);
		    if (channels[2].extended) {
			    // extended 4op ch#2 part 2 
			    channels[5].chan_calc_ext(LFO_AM);
		    } else {
			    // standard 2op ch#5 
			    channels[5].chan_calc(LFO_AM);
		    }

		    if (!rhythmEnabled) {
			    channels[6].chan_calc(LFO_AM);
			    channels[7].chan_calc(LFO_AM);
			    channels[8].chan_calc(LFO_AM);
		    } else {
			    // Rhythm part 
			    chan_calc_rhythm(noise_rng & 1);
		    }

		    // register set #2 
		    channels[9].chan_calc(LFO_AM);
		    if (channels[9].extended) {
			    channels[12].chan_calc_ext(LFO_AM);
		    } else {
			    channels[12].chan_calc(LFO_AM);
		    }

		    channels[10].chan_calc(LFO_AM);
		    if (channels[10].extended) {
			    channels[13].chan_calc_ext(LFO_AM);
		    } else {
			    channels[13].chan_calc(LFO_AM);
		    }

		    channels[11].chan_calc(LFO_AM);
		    if (channels[11].extended) {
			    channels[14].chan_calc_ext(LFO_AM);
		    } else {
			    channels[14].chan_calc(LFO_AM);
		    }

		    // channels 15,16,17 are fixed 2-operator channels only 
		    channels[15].chan_calc(LFO_AM);
		    channels[16].chan_calc(LFO_AM);
		    channels[17].chan_calc(LFO_AM);

		    for (int i = 0; i < 18; i++) {
			    a += chanout[i] & pan[4 * i + 0];
			    b += chanout[i] & pan[4 * i + 1];
			    c += chanout[i] & pan[4 * i + 2];
			    d += chanout[i] & pan[4 * i + 3];
		    }

		    advance();
        }
		*(buf++) = (a << 3) / oplOversampling;
		*(buf++) = (b << 3) / oplOversampling;
	}

	checkMute();
	return buffer;
}

void YMF262::setInternalVolume(short newVolume)
{
	maxVolume = newVolume;
}

void YMF262::loadState()
{
    SaveState* state = saveStateOpenForRead("ymf262");
    char tag[32];
    int i;
    
    saveStateGetBuffer(state, "reg", reg, sizeof(reg));

    for (i = 0; i < sizeof(fn_tab) / sizeof(fn_tab[0]); i++) {
        sprintf(tag, "fn_tab%.4d", i);
        fn_tab[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(pan) / sizeof(pan[0]); i++) {
        sprintf(tag, "pan%.4d", i);
        pan[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(chanout) / sizeof(chanout[0]); i++) {
        sprintf(tag, "chanout%.4d", i);
        chanout[i] = saveStateGet(state, tag, 0);
    }

    
    eg_cnt             = saveStateGet(state, "eg_cnt",             0);
    eg_timer           = saveStateGet(state, "eg_timer",           0);
    eg_timer_add       = saveStateGet(state, "eg_timer_add",       0);

    LFO_AM             = (byte)saveStateGet(state, "LFO_AM",             0);
    LFO_PM             = (byte)saveStateGet(state, "LFO_PM",             0);

    lfo_am_depth       = (byte)saveStateGet(state, "lfo_am_depth",       0);
    lfo_pm_depth_range = (byte)saveStateGet(state, "lfo_pm_depth_range", 0);
    lfo_am_cnt         = saveStateGet(state, "lfo_am_cnt",         0);
    lfo_am_inc         = saveStateGet(state, "lfo_am_inc",         0);
    lfo_pm_cnt         = saveStateGet(state, "lfo_pm_cnt",         0);
    lfo_pm_inc         = saveStateGet(state, "lfo_pm_inc",         0);
    
    noise_rng          = saveStateGet(state, "noise_rng",          0);
    noise_p            = saveStateGet(state, "noise_p",            0);
    noise_f            = saveStateGet(state, "noise_f",            0);
    
    OPL3_mode          = saveStateGet(state, "OPL3_mode",          0) != 0;
    rhythm             = (byte)saveStateGet(state, "rhythm",             0);
    nts                = (byte)saveStateGet(state, "nts",                0);
    
    status             = (byte)saveStateGet(state, "status",             0);
    status2            = (byte)saveStateGet(state, "status2",            0);
    statusMask         = (byte)saveStateGet(state, "statusMask",         0);
    maxVolume          = (short)saveStateGet(state, "maxVolume",          0);

    for (int i = 0; i < 18; i++) {
        sprintf(tag, "block_fnum%d", i);
        channels[i].block_fnum = saveStateGet(state, tag, 0);
        
        sprintf(tag, "fc%d", i);
        channels[i].fc = saveStateGet(state, tag, 0);
        
        sprintf(tag, "ksl_base%d", i);
        channels[i].ksl_base = saveStateGet(state, tag, 0);
        
        sprintf(tag, "kcode%d", i);
        channels[i].kcode = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "extended%d", i);
        channels[i].extended = (byte)saveStateGet(state, tag, 0);
        
        for (int j = 0; j < 2; j++) {
            sprintf(tag, "ar%d_%d", i, j);
            channels[i].slots[j].ar = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "dr%d_%d", i, j);
            channels[i].slots[j].dr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "rr%d_%d", i, j);
            channels[i].slots[j].rr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "KSR%d_%d", i, j);
            channels[i].slots[j].KSR = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "ksl%d_%d", i, j);
            channels[i].slots[j].ksl = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "ksr%d_%d", i, j);
            channels[i].slots[j].ksr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "mul%d_%d", i, j);
            channels[i].slots[j].mul = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "Cnt%d_%d", i, j);
            channels[i].slots[j].Cnt = saveStateGet(state, tag, 0);
            
            sprintf(tag, "Incr%d_%d", i, j);
            channels[i].slots[j].Incr = saveStateGet(state, tag, 0);
            
            sprintf(tag, "FB%d_%d", i, j);
            channels[i].slots[j].FB = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "op1_out%d_%d_0", i, j);
            channels[i].slots[j].op1_out[0] = saveStateGet(state, tag, 0);
            
            sprintf(tag, "op1_out%d_%d_1", i, j);
            channels[i].slots[j].op1_out[1] = saveStateGet(state, tag, 0);
            
            sprintf(tag, "CON%d_%d", i, j);
            channels[i].slots[j].CON = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_type%d_%d", i, j);
            channels[i].slots[j].eg_type = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "state%d_%d", i, j);
            channels[i].slots[j].state = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "TL%d_%d", i, j);
            channels[i].slots[j].TL = saveStateGet(state, tag, 0);
            
            sprintf(tag, "TLL%d_%d", i, j);
            channels[i].slots[j].TLL = saveStateGet(state, tag, 0);
            
            sprintf(tag, "volume%d_%d", i, j);
            channels[i].slots[j].volume = saveStateGet(state, tag, 0);
            
            sprintf(tag, "sl%d_%d", i, j);
            channels[i].slots[j].sl = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_m_ar%d_%d", i, j);
            channels[i].slots[j].eg_m_ar = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_ar%d_%d", i, j);
            channels[i].slots[j].eg_sh_ar = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_ar%d_%d", i, j);
            channels[i].slots[j].eg_sel_ar = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_m_dr%d_%d", i, j);
            channels[i].slots[j].eg_m_dr = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_dr%d_%d", i, j);
            channels[i].slots[j].eg_sh_dr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_dr%d_%d", i, j);
            channels[i].slots[j].eg_sel_dr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_m_rr%d_%d", i, j);
            channels[i].slots[j].eg_m_rr = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_rr%d_%d", i, j);
            channels[i].slots[j].eg_sh_rr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_rr%d_%d", i, j);
            channels[i].slots[j].eg_sel_rr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "key%d_%d", i, j);
            channels[i].slots[j].key = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "AMmask%d_%d", i, j);
            channels[i].slots[j].AMmask = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "vib%d_%d", i, j);
            channels[i].slots[j].vib = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "waveform_number%d_%d", i, j);
            channels[i].slots[j].waveform_number = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "wavetable%d_%d", i, j);
            channels[i].slots[j].wavetable = saveStateGet(state, tag, 0);
            
            sprintf(tag, "connect%d_%d", i, j);
            channels[i].slots[j].connect = saveStateGet(state, tag, 0);
        }
    }

    saveStateClose(state);
}

void YMF262::saveState()
{
    SaveState* state = saveStateOpenForWrite("ymf262");
    char tag[32];
    int i;
    
    saveStateSetBuffer(state, "reg", reg, sizeof(reg));

    for (i = 0; i < sizeof(fn_tab) / sizeof(fn_tab[0]); i++) {
        sprintf(tag, "fn_tab%.4d", i);
        saveStateSet(state, tag, fn_tab[i]);
    }

    for (i = 0; i < sizeof(pan) / sizeof(pan[0]); i++) {
        sprintf(tag, "pan%.4d", i);
        saveStateSet(state, tag, pan[i]);
    }

    for (i = 0; i < sizeof(chanout) / sizeof(chanout[0]); i++) {
        sprintf(tag, "chanout%.4d", i);
        saveStateSet(state, tag, chanout[i]);
    }

    
    saveStateSet(state, "eg_cnt",             eg_cnt);
    saveStateSet(state, "eg_timer",           eg_timer);
    saveStateSet(state, "eg_timer_add",       eg_timer_add);

    saveStateSet(state, "LFO_AM",             LFO_AM);
    saveStateSet(state, "LFO_PM",             LFO_PM);

    saveStateSet(state, "lfo_am_depth",       lfo_am_depth);
    saveStateSet(state, "lfo_pm_depth_range", lfo_pm_depth_range);
    saveStateSet(state, "lfo_am_cnt",         lfo_am_cnt);
    saveStateSet(state, "lfo_am_inc",         lfo_am_inc);
    saveStateSet(state, "lfo_pm_cnt",         lfo_pm_cnt);
    saveStateSet(state, "lfo_pm_inc",         lfo_pm_inc);
    
    saveStateSet(state, "noise_rng",          noise_rng);
    saveStateSet(state, "noise_p",            noise_p);
    saveStateSet(state, "noise_f",            noise_f);
    
    saveStateSet(state, "OPL3_mode",          OPL3_mode);
    saveStateSet(state, "rhythm",             rhythm);
    saveStateSet(state, "nts",                nts);
    
    saveStateSet(state, "status",             status);
    saveStateSet(state, "status2",            status2);
    saveStateSet(state, "statusMask",         statusMask);
    saveStateSet(state, "maxVolume",          maxVolume);

    for (int i = 0; i < 18; i++) {
        sprintf(tag, "block_fnum%d", i);
        saveStateSet(state, tag, channels[i].block_fnum);
        
        sprintf(tag, "fc%d", i);
        saveStateSet(state, tag, channels[i].fc);
        
        sprintf(tag, "ksl_base%d", i);
        saveStateSet(state, tag, channels[i].ksl_base);
        
        sprintf(tag, "kcode%d", i);
        saveStateSet(state, tag, channels[i].kcode);
        
        sprintf(tag, "extended%d", i);
        saveStateSet(state, tag, channels[i].extended);
        
        for (int j = 0; j < 2; j++) {
            sprintf(tag, "ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].ar);
            
            sprintf(tag, "dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].dr);
            
            sprintf(tag, "rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].rr);
            
            sprintf(tag, "KSR%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].KSR);
            
            sprintf(tag, "ksl%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].ksl);
            
            sprintf(tag, "ksr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].ksr);
            
            sprintf(tag, "mul%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].mul);
            
            sprintf(tag, "Cnt%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].Cnt);
            
            sprintf(tag, "Incr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].Incr);
            
            sprintf(tag, "FB%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].FB);
            
            sprintf(tag, "op1_out%d_%d_0", i, j);
            saveStateSet(state, tag, channels[i].slots[j].op1_out[0]);
            
            sprintf(tag, "op1_out%d_%d_1", i, j);
            saveStateSet(state, tag, channels[i].slots[j].op1_out[1]);
            
            sprintf(tag, "CON%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].CON);
            
            sprintf(tag, "eg_type%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_type);
            
            sprintf(tag, "state%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].state);
            
            sprintf(tag, "TL%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].TL);
            
            sprintf(tag, "TLL%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].TLL);
            
            sprintf(tag, "volume%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].volume);
            
            sprintf(tag, "sl%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].sl);
            
            sprintf(tag, "eg_m_ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_m_ar);
            
            sprintf(tag, "eg_sh_ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_ar);
            
            sprintf(tag, "eg_sel_ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_ar);
            
            sprintf(tag, "eg_m_dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_m_dr);
            
            sprintf(tag, "eg_sh_dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_dr);
            
            sprintf(tag, "eg_sel_dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_dr);
            
            sprintf(tag, "eg_m_rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_m_rr);
            
            sprintf(tag, "eg_sh_rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_rr);
            
            sprintf(tag, "eg_sel_rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_rr);
            
            sprintf(tag, "key%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].key);
            
            sprintf(tag, "AMmask%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].AMmask);
            
            sprintf(tag, "vib%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].vib);
            
            sprintf(tag, "waveform_number%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].waveform_number);
            
            sprintf(tag, "wavetable%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].wavetable);
            
            sprintf(tag, "connect%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].connect);
        }
    }

    saveStateClose(state);
}
