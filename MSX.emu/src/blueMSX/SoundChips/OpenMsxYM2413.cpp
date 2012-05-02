// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.
//
// The file was originally written by Mitsutaka Okazaki and by Jarek Burczynski.
//
#include "OpenMsxYM2413.h"

extern "C" {
#include "SaveState.h"
}

#include <stdio.h>
#include <cmath>
#include <cstring>

const DoubleT PI = 3.14159265358979323846;
 
const int FREQ_SH = 16;	// 16.16 fixed point (frequency calculations)
const int EG_SH   = 16;	// 16.16 fixed point (EG timing)
const int LFO_SH  = 24;	//  8.24 fixed point (LFO calculations)
const int FREQ_MASK = (1 << FREQ_SH) - 1;
const unsigned EG_TIMER_OVERFLOW = (1 << EG_SH);

// envelope output entries
const int ENV_BITS = 10;
const int ENV_LEN  = 1 << ENV_BITS;
const DoubleT ENV_STEP = 128.0 / ENV_LEN;

const int MAX_ATT_INDEX = (1 << (ENV_BITS - 2)) - 1;	// 255
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
const byte EG_DMP = 5;
const byte EG_ATT = 4;
const byte EG_DEC = 3;
const byte EG_SUS = 2;
const byte EG_REL = 1;
const byte EG_OFF = 0;

// key scale level
// table is 3dB/octave, DV converts this into 6dB/octave
// 0.1875 is bit 0 weight of the envelope counter (volume) expressed
// in the 'decibel' scale
#define DV(x) (int)(x / 0.1875)
static const int ksl_tab[8 * 16] =
{
	// OCT 0
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	// OCT 1
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 0.750),DV( 1.125),DV( 1.500),
	DV( 1.875),DV( 2.250),DV( 2.625),DV( 3.000),
	// OCT 2
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 0.000),
	DV( 0.000),DV( 1.125),DV( 1.875),DV( 2.625),
	DV( 3.000),DV( 3.750),DV( 4.125),DV( 4.500),
	DV( 4.875),DV( 5.250),DV( 5.625),DV( 6.000),
	// OCT 3
	DV( 0.000),DV( 0.000),DV( 0.000),DV( 1.875),
	DV( 3.000),DV( 4.125),DV( 4.875),DV( 5.625),
	DV( 6.000),DV( 6.750),DV( 7.125),DV( 7.500),
	DV( 7.875),DV( 8.250),DV( 8.625),DV( 9.000),
	// OCT 4
	DV( 0.000),DV( 0.000),DV( 3.000),DV( 4.875),
	DV( 6.000),DV( 7.125),DV( 7.875),DV( 8.625),
	DV( 9.000),DV( 9.750),DV(10.125),DV(10.500),
	DV(10.875),DV(11.250),DV(11.625),DV(12.000),
	// OCT 5
	DV( 0.000),DV( 3.000),DV( 6.000),DV( 7.875),
	DV( 9.000),DV(10.125),DV(10.875),DV(11.625),
	DV(12.000),DV(12.750),DV(13.125),DV(13.500),
	DV(13.875),DV(14.250),DV(14.625),DV(15.000),
	// OCT 6
	DV( 0.000),DV( 6.000),DV( 9.000),DV(10.875),
	DV(12.000),DV(13.125),DV(13.875),DV(14.625),
	DV(15.000),DV(15.750),DV(16.125),DV(16.500),
	DV(16.875),DV(17.250),DV(17.625),DV(18.000),
	// OCT 7
	DV( 0.000),DV( 9.000),DV(12.000),DV(13.875),
	DV(15.000),DV(16.125),DV(16.875),DV(17.625),
	DV(18.000),DV(18.750),DV(19.125),DV(19.500),
	DV(19.875),DV(20.250),DV(20.625),DV(21.000)
};

// sustain level table (3dB per step)
// 0 - 15: 0, 3, 6, 9,12,15,18,21,24,27,30,33,36,39,42,45 (dB)
#define SC(db) (int)(((DoubleT)db) / ENV_STEP)
static const int sl_tab[16] = {
	SC( 0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5 ),SC(6 ),SC( 7),
	SC( 8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(15)
};

const int RATE_STEPS = 8;
static const byte eg_inc[15 * RATE_STEPS] =
{
	//cycle:0 1  2 3  4 5  6 7

	/* 0 */ 0,1, 0,1, 0,1, 0,1, // rates 00..12 0 (increment by 0 or 1)
	/* 1 */ 0,1, 0,1, 1,1, 0,1, // rates 00..12 1
	/* 2 */ 0,1, 1,1, 0,1, 1,1, // rates 00..12 2
	/* 3 */ 0,1, 1,1, 1,1, 1,1, // rates 00..12 3

	/* 4 */ 1,1, 1,1, 1,1, 1,1, // rate 13 0 (increment by 1)
	/* 5 */ 1,1, 1,2, 1,1, 1,2, // rate 13 1
	/* 6 */ 1,2, 1,2, 1,2, 1,2, // rate 13 2
	/* 7 */ 1,2, 2,2, 1,2, 2,2, // rate 13 3

	/* 8 */ 2,2, 2,2, 2,2, 2,2, // rate 14 0 (increment by 2)
	/* 9 */ 2,2, 2,4, 2,2, 2,4, // rate 14 1
	/*10 */ 2,4, 2,4, 2,4, 2,4, // rate 14 2
	/*11 */ 2,4, 4,4, 2,4, 4,4, // rate 14 3

	/*12 */ 4,4, 4,4, 4,4, 4,4, // rates 15 0, 15 1, 15 2, 15 3 (increment by 4)
	/*13 */ 8,8, 8,8, 8,8, 8,8, // rates 15 2, 15 3 for attack
	/*14 */ 0,0, 0,0, 0,0, 0,0, // infinity rates for attack and decay(s)
};

// note that there is no O(13) in this table - it's directly in the code
#define O(a) (a * RATE_STEPS)
static const byte eg_rate_select[16 + 64 + 16] =
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

//rate  0,    1,    2,    3,    4,   5,   6,   7,  8,  9, 10, 11, 12, 13, 14, 15
//shift 13,   12,   11,   10,   9,   8,   7,   6,  5,  4,  3,  2,  1,  0,  0,  0
//mask  8191, 4095, 2047, 1023, 511, 255, 127, 63, 31, 15, 7,  3,  1,  0,  0,  0

#define O(a) (a)
static const byte eg_rate_shift[16 + 64 + 16] =
{
	// Envelope Generator counter shifts (16 + 64 rates + 16 RKS)
	// 16 infinite time rates
	O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),
	O(0),O(0),O(0),O(0),O(0),O(0),O(0),O(0),

	// rates 00-12
	O(13),O(13),O(13),O(13),
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

	// rate 13
	O( 0),O( 0),O( 0),O( 0),

	// rate 14
	O( 0),O( 0),O( 0),O( 0),

	// rate 15
	O( 0),O( 0),O( 0),O( 0),

	// 16 dummy rates (same as 15 3)
	O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
	O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),O( 0),
};
#undef O


// multiple table
#define ML(x) (byte)(2 * x)
static const byte mul_tab[16] =
{
	// 1/2, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,10,12,12,15,15
	ML(0.50), ML(1.00),ML( 2.00),ML( 3.00),ML( 4.00),ML( 5.00),ML( 6.00),ML( 7.00),
	ML(8.00), ML(9.00),ML(10.00),ML(10.00),ML(12.00),ML(12.00),ML(15.00),ML(15.00)
};

//  TL_TAB_LEN is calculated as:
//  11 - sinus amplitude bits     (Y axis)
//  2  - sinus sign bit           (Y axis)
//  TL_RES_LEN - sinus resolution (X axis)
const int TL_TAB_LEN = 11 * 2 * TL_RES_LEN;
static int tl_tab[TL_TAB_LEN];
const int ENV_QUIET = TL_TAB_LEN >> 5;

// sin waveform table in 'decibel' scale
// two waveforms on OPLL type chips
static unsigned sin_tab[SIN_LEN * 2];

// LFO Amplitude Modulation table (verified on real YM3812)
// 27 output levels (triangle waveform); 1 level takes one of: 192, 256 or 448 samples
//
// Length: 210 elements.
//
//  Each of the elements has to be repeated
//  exactly 64 times (on 64 consecutive samples).
//  The whole table takes: 64 * 210 = 13440 samples.
//
// We use data>>1, until we find what it really is on real chip...

const unsigned LFO_AM_TAB_ELEMENTS = 210;
static const byte lfo_am_table[LFO_AM_TAB_ELEMENTS] =
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

// LFO Phase Modulation table (verified on real YM2413)
static const char lfo_pm_table[8 * 8] = 
{
	// FNUM2/FNUM = 0 00xxxxxx (0x0000)
	0, 0, 0, 0, 0, 0, 0, 0,

	// FNUM2/FNUM = 0 01xxxxxx (0x0040)
	1, 0, 0, 0,-1, 0, 0, 0,

	// FNUM2/FNUM = 0 10xxxxxx (0x0080)
	2, 1, 0,-1,-2,-1, 0, 1,

	// FNUM2/FNUM = 0 11xxxxxx (0x00C0)
	3, 1, 0,-1,-3,-1, 0, 1,

	// FNUM2/FNUM = 1 00xxxxxx (0x0100)
	4, 2, 0,-2,-4,-2, 0, 2,

	// FNUM2/FNUM = 1 01xxxxxx (0x0140)
	5, 2, 0,-2,-5,-2, 0, 2,

	// FNUM2/FNUM = 1 10xxxxxx (0x0180)
	6, 3, 0,-3,-6,-3, 0, 3,

	// FNUM2/FNUM = 1 11xxxxxx (0x01C0)
	7, 3, 0,-3,-7,-3, 0, 3,
};

// This is not 100% perfect yet but very close
//
// - multi parameters are 100% correct (instruments and drums)
// - LFO PM and AM enable are 100% correct
// - waveform DC and DM select are 100% correct
static const byte table[19][8] = {
	// MULT  MULT modTL DcDmFb AR/DR AR/DR SL/RR SL/RR
	//   0     1     2     3     4     5     6    7
	  {0x49, 0x4c, 0x4c, 0x12, 0x00, 0x00, 0x00, 0x00 },	//0
	  {0x61, 0x61, 0x1e, 0x17, 0xf0, 0x78, 0x00, 0x17 },	//1
	  {0x13, 0x41, 0x1e, 0x0d, 0xd7, 0xf7, 0x13, 0x13 },	//2
	  {0x13, 0x01, 0x99, 0x04, 0xf2, 0xf4, 0x11, 0x23 },	//3
	  {0x21, 0x61, 0x1b, 0x07, 0xaf, 0x64, 0x40, 0x27 },	//4
	//{0x22, 0x21, 0x1e, 0x09, 0xf0, 0x76, 0x08, 0x28 },	//5
	  {0x22, 0x21, 0x1e, 0x06, 0xf0, 0x75, 0x08, 0x18 },	//5
	//{0x31, 0x22, 0x16, 0x09, 0x90, 0x7f, 0x00, 0x08 },	//6
	  {0x31, 0x22, 0x16, 0x05, 0x90, 0x71, 0x00, 0x13 },	//6
	  {0x21, 0x61, 0x1d, 0x07, 0x82, 0x80, 0x10, 0x17 },	//7
	  {0x23, 0x21, 0x2d, 0x16, 0xc0, 0x70, 0x07, 0x07 },	//8
	  {0x61, 0x61, 0x1b, 0x06, 0x64, 0x65, 0x10, 0x17 },	//9
	//{0x61, 0x61, 0x0c, 0x08, 0x85, 0xa0, 0x79, 0x07 },	//A
	  {0x61, 0x61, 0x0c, 0x18, 0x85, 0xf0, 0x70, 0x07 },	//A
	  {0x23, 0x01, 0x07, 0x11, 0xf0, 0xa4, 0x00, 0x22 },	//B
	  {0x97, 0xc1, 0x24, 0x07, 0xff, 0xf8, 0x22, 0x12 },	//C
	//{0x61, 0x10, 0x0c, 0x08, 0xf2, 0xc4, 0x40, 0xc8 },	//D
	  {0x61, 0x10, 0x0c, 0x05, 0xf2, 0xf4, 0x40, 0x44 },	//D
	  {0x01, 0x01, 0x55, 0x03, 0xf3, 0x92, 0xf3, 0xf3 },	//E
	  {0x61, 0x41, 0x89, 0x03, 0xf1, 0xf4, 0xf0, 0x13 },	//F

	// drum instruments definitions
	// MULTI MULTI modTL  xxx  AR/DR AR/DR SL/RR SL/RR
	//   0     1     2     3     4     5     6    7
	  {0x01, 0x01, 0x16, 0x00, 0xfd, 0xf8, 0x2f, 0x6d },// BD(multi verified, modTL verified, mod env - verified(close), carr. env verifed)
	  {0x01, 0x01, 0x00, 0x00, 0xd8, 0xd8, 0xf9, 0xf8 },// HH(multi verified), SD(multi not used)
	  {0x05, 0x01, 0x00, 0x00, 0xf8, 0xba, 0x49, 0x55 },// TOM(multi,env verified), TOP CYM(multi verified, env verified)
};


Slot::Slot()
{
	ar = dr = rr = KSR = ksl = ksr = mul = 0;
	phase = freq = fb_shift = op1_out[0] = op1_out[1] = 0;
	eg_type = state = TL = TLL = volume = sl = 0;
	eg_sh_dp = eg_sel_dp = eg_sh_ar = eg_sel_ar = eg_sh_dr = 0;
	eg_sel_dr = eg_sh_rr = eg_sel_rr = eg_sh_rs = eg_sel_rs = 0;
	key = AMmask = vib = wavetable = 0;
}

Channel::Channel()
{
	block_fnum = fc = ksl_base = kcode = sus = 0;
}

// advance LFO to next sample
inline void OpenYM2413::advance_lfo()
{
	lfo_am_cnt += lfo_am_inc;
	if (lfo_am_cnt >= (LFO_AM_TAB_ELEMENTS << LFO_SH)) {
		// lfo_am_table is 210 elements long
		lfo_am_cnt -= (LFO_AM_TAB_ELEMENTS << LFO_SH);
	}
	LFO_AM = lfo_am_table[lfo_am_cnt >> LFO_SH ] >> 1;
	lfo_pm_cnt += lfo_pm_inc;
	LFO_PM = (lfo_pm_cnt >> LFO_SH) & 7;
}

// advance to next sample
inline void OpenYM2413::advance()
{
	// Envelope Generator
	eg_timer += eg_timer_add;

	while (eg_timer >= EG_TIMER_OVERFLOW) {
		eg_timer -= EG_TIMER_OVERFLOW;
		eg_cnt++;

		for (int i = 0; i < 9 * 2; i++) {
			Channel& ch = channels[i / 2];
			Slot& op = ch.slots[i & 1];

			switch (op.state) {
			case EG_DMP:	// dump phase
				// dump phase is performed by both operators in each channel
				// when CARRIER envelope gets down to zero level,
				// phases in BOTH opearators are reset (at the same time?)
				if (!(eg_cnt & ((1 << op.eg_sh_dp) - 1))) {
					op.volume += eg_inc[op.eg_sel_dp + ((eg_cnt >> op.eg_sh_dp) & 7)];
					if (op.volume >= MAX_ATT_INDEX) {
						op.volume = MAX_ATT_INDEX;
						op.state = EG_ATT;
						op.phase = 0;	// restart Phase Generator
					}
				}
				break;

			case EG_ATT:	// attack phase
				if (!(eg_cnt & ((1 << op.eg_sh_ar) - 1))) {
					op.volume += (~op.volume * (eg_inc[op.eg_sel_ar + ((eg_cnt >> op.eg_sh_ar) & 7)])) >> 2;
					if (op.volume <= MIN_ATT_INDEX) {
						op.volume = MIN_ATT_INDEX;
						op.state = EG_DEC;
					}
				}
				break;

			case EG_DEC:    // decay phase
				if (!(eg_cnt & ((1 << op.eg_sh_dr) - 1))) {
					op.volume += eg_inc[op.eg_sel_dr + ((eg_cnt >> op.eg_sh_dr) & 7)];
					if (op.volume >= op.sl) {
						op.state = EG_SUS;
					}
				}
				break;

			case EG_SUS:    // sustain phase
				// this is important behaviour:
				// one can change percusive/non-percussive modes on the fly and
				// the chip will remain in sustain phase - verified on real YM3812
				if (op.eg_type) {    
					// non-percussive mode (sustained tone)
					// do nothing
				} else {
					// percussive mode
					// during sustain phase chip adds Release Rate (in percussive mode)
					if (!(eg_cnt & ((1 << op.eg_sh_rr) - 1))) {
						op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt >> op.eg_sh_rr) & 7)];

						if (op.volume >= MAX_ATT_INDEX) {
							op.volume = MAX_ATT_INDEX;
						}
					}
					// else do nothing in sustain phase
				}
				break;

			case EG_REL:    // release phase
				// exclude modulators in melody channels from performing anything in this mode
				// allowed are only carriers in melody mode and rhythm slots in rhythm mode
				//
				// This table shows which operators and on what conditions are allowed to perform EG_REL:
				// (a) - always perform EG_REL
				// (n) - never perform EG_REL
				// (r) - perform EG_REL in Rhythm mode ONLY
				//   0: 0 (n),  1 (a)
				//   1: 2 (n),  3 (a)
				//   2: 4 (n),  5 (a)
				//   3: 6 (n),  7 (a)
				//   4: 8 (n),  9 (a)
				//   5: 10(n),  11(a)
				//   6: 12(r),  13(a)
				//   7: 14(r),  15(a)
				//   8: 16(r),  17(a)
				if ((i & 1) || (rhythm && (i >= 12))) {
					// exclude modulators
					if (op.eg_type) {
						// non-percussive mode (sustained tone)
						// this is correct: use RR when SUS = OFF
						// and use RS when SUS = ON
						if (ch.sus) {
							if (!(eg_cnt & ((1 << op.eg_sh_rs) - 1))) {
								op.volume += eg_inc[op.eg_sel_rs + ((eg_cnt >> op.eg_sh_rs) & 7)];
								if (op.volume >= MAX_ATT_INDEX) {
									op.volume = MAX_ATT_INDEX;
									op.state = EG_OFF;
								}
							}
						} else {
							if (!(eg_cnt & ((1 << op.eg_sh_rr) - 1))) {
								op.volume += eg_inc[op.eg_sel_rr + ((eg_cnt >> op.eg_sh_rr) & 7)];
								if (op.volume >= MAX_ATT_INDEX) {
									op.volume = MAX_ATT_INDEX;
									op.state = EG_OFF;
								}
							}
						}
					} else {
						// percussive mode
						if (!(eg_cnt & ((1 << op.eg_sh_rs) - 1))) {
							op.volume += eg_inc[op.eg_sel_rs + ((eg_cnt >> op.eg_sh_rs) & 7)];
							if (op.volume >= MAX_ATT_INDEX) {
								op.volume = MAX_ATT_INDEX;
								op.state = EG_OFF;
							}
						}
					}
				}
				break;

			default:
				break;
			}
		}
	}

    int i;
	for (i = 0; i < 9 * 2; i++) {
		Channel& ch = channels[i / 2];
		Slot& op = ch.slots[i & 1];

		// Phase Generator
		if (op.vib) {
			int fnum_lfo   = 8 * ((ch.block_fnum & 0x01C0) >> 6);
			int block_fnum = ch.block_fnum * 2;
			int lfo_fn_table_index_offset = lfo_pm_table[LFO_PM + fnum_lfo];
			if (lfo_fn_table_index_offset) {
				// LFO phase modulation active
				block_fnum += lfo_fn_table_index_offset;
				byte block = (block_fnum & 0x1C00) >> 10;
				op.phase += (fn_tab[block_fnum & 0x03FF] >> (7 - block)) * op.mul;
			} else {
				// LFO phase modulation = zero
				op.phase += op.freq;
			}
		} else {
			// LFO phase modulation disabled for this operator
			op.phase += op.freq;
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
	i = noise_p >> FREQ_SH;	// number of events (shifts of the shift register)
	noise_p &= FREQ_MASK;
	while (i--) {
		//  int j = ((noise_rng) ^ (noise_rng >> 14) ^ (noise_rng >> 15) ^ (noise_rng >> 22)) & 1;
		//  noise_rng = (j << 22) | (noise_rng >> 1);
		//  
		//    Instead of doing all the logic operations above, we
		//    use a trick here (and use bit 0 as the noise output).
		//    The difference is only that the noise bit changes one
		//    step ahead. This doesn't matter since we don't know
		//    what is real state of the noise_rng after the reset.

		if (noise_rng & 1) {
			noise_rng ^= 0x800302;
		}
		noise_rng >>= 1;
	}
}


inline int op_calc(unsigned phase, int env, int pm, int wave_tab)
{
	int i = (phase & ~FREQ_MASK) + (pm << 17);
	int p = (env << 5) + sin_tab[wave_tab + ((i >> FREQ_SH) & SIN_MASK)];
	if (p >= TL_TAB_LEN) {
		return 0;
	}
	return tl_tab[p];
}

inline int op_calc1(unsigned phase, int env, int pm, int wave_tab)
{
	int i = (phase & ~FREQ_MASK) + pm;
	int p = (env << 5) + sin_tab[wave_tab + ((i >> FREQ_SH) & SIN_MASK)];
	if (p >= TL_TAB_LEN) {
		return 0;
	}
	return tl_tab[p];
}

inline int Slot::volume_calc(byte LFO_AM)
{
	return TLL + volume + (LFO_AM & AMmask);
}

// calculate output
inline int Channel::chan_calc(byte LFO_AM)
{
	// SLOT 1
	int env = slots[SLOT1].volume_calc(LFO_AM);
	int out = slots[SLOT1].op1_out[0] + slots[SLOT1].op1_out[1];

	slots[SLOT1].op1_out[0] = slots[SLOT1].op1_out[1];
	int phase_modulation = slots[SLOT1].op1_out[0];	// phase modulation input (SLOT 2)
	slots[SLOT1].op1_out[1] = 0;

	if (env < ENV_QUIET) {
		if (!slots[SLOT1].fb_shift) {
			out = 0;
		}
		slots[SLOT1].op1_out[1] = op_calc1(slots[SLOT1].phase, env, (out << slots[SLOT1].fb_shift), slots[SLOT1].wavetable);
	}

	// SLOT 2
	env = slots[SLOT2].volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		return op_calc(slots[SLOT2].phase, env, phase_modulation, slots[SLOT2].wavetable);
	} else {
		return 0;
	}
}


// Operators used in the rhythm sounds generation process:
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
//    Phase Generator:
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
//    7     13,16     B7        A7            +     +           +
//    8     14,17     B8        A8            +           +     +

// calculate rhythm
inline int OpenYM2413::rhythm_calc(bool noise)
{
	int output = 0;
	Slot& SLOT6_1 = channels[6].slots[SLOT1];
	Slot& SLOT6_2 = channels[6].slots[SLOT2];
	Slot& SLOT7_1 = channels[7].slots[SLOT1];
	Slot& SLOT7_2 = channels[7].slots[SLOT2];
	Slot& SLOT8_1 = channels[8].slots[SLOT1];
	Slot& SLOT8_2 = channels[8].slots[SLOT2];
	
	// Bass Drum (verified on real YM3812):
	//  - depends on the channel 6 'connect' register:
	//    when connect = 0 it works the same as in normal (non-rhythm) mode (op1->op2->out)
	//    when connect = 1 _only_ operator 2 is present on output (op2->out), operator 1 is ignored
	//  - output sample always is multiplied by 2
	
	// SLOT 1
	int env = SLOT6_1.volume_calc(LFO_AM);

	int out = SLOT6_1.op1_out[0] + SLOT6_1.op1_out[1];
	SLOT6_1.op1_out[0] = SLOT6_1.op1_out[1];

	int phase_modulation = SLOT6_1.op1_out[0];	// phase modulation input (SLOT 2)

	SLOT6_1.op1_out[1] = 0;
	if (env < ENV_QUIET) {
		if (!SLOT6_1.fb_shift) {
			out = 0;
		}
		SLOT6_1.op1_out[1] = op_calc1(SLOT6_1.phase, env, (out << SLOT6_1.fb_shift), SLOT6_1.wavetable);
	}
	
	// SLOT 2
	env = SLOT6_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		output += op_calc(SLOT6_2.phase, env, phase_modulation, SLOT6_2.wavetable);
	}

	// Phase generation is based on:
	//   HH  (13) channel 7->slot 1 combined with channel 8->slot 2 (same combination as TOP CYMBAL but different output phases)
	//   SD  (16) channel 7->slot 1
	//   TOM (14) channel 8->slot 1
	//   TOP (17) channel 7->slot 1 combined with channel 8->slot 2 (same combination as HIGH HAT but different output phases)
	// Envelope generation based on:
	//   HH  channel 7->slot1
	//   SD  channel 7->slot2
	//   TOM channel 8->slot1
	//   TOP channel 8->slot2

	// The following formulas can be well optimized.
	//   I leave them in direct form for now (in case I've missed something).

	// High Hat (verified on real YM3812)
	env = SLOT7_1.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// high hat phase generation:
		// phase = D0 or 234 (based on frequency only)
		// phase = 34 or 2D0 (based on noise)

		// base frequency derived from operator 1 in channel 7
		bool bit7 = ((SLOT7_1.phase >> FREQ_SH) & 0x80) != 0;
		bool bit3 = ((SLOT7_1.phase >> FREQ_SH) & 0x08) != 0;
		bool bit2 = ((SLOT7_1.phase >> FREQ_SH) & 0x04) != 0;
		bool res1 = ((bit2 ^ bit7) | bit3) != 0;
		// when res1 = 0 phase = 0x000 |  0xD0;
		// when res1 = 1 phase = 0x200 | (0xD0 >> 2);
		unsigned phase = res1 ? (0x200 | (0xD0 >> 2)) : 0xD0;

		// enable gate based on frequency of operator 2 in channel 8
		bool bit5e= ((SLOT8_2.phase >> FREQ_SH) & 0x20) != 0;
		bool bit3e= ((SLOT8_2.phase >> FREQ_SH) & 0x08) != 0;
		bool res2 = (bit3e | bit5e) != 0;
		// when res2 = 0 pass the phase from calculation above (res1);
		// when res2 = 1 phase = 0x200 | (0xd0>>2);
		if (res2) {
			phase = (0x200 | (0xD0 >> 2));
		}

		// when phase & 0x200 is set and noise=1 then phase = 0x200 |  0xD0
		// when phase & 0x200 is set and noise=0 then phase = 0x200 | (0xD0 >> 2), ie no change
		if (phase & 0x200) {
			if (noise) {
				phase = 0x200 | 0xD0;
			}
		} else {
			// when phase & 0x200 is clear and noise=1 then phase = 0xD0 >> 2
			// when phase & 0x200 is clear and noise=0 then phase = 0xD0, ie no change
			if (noise) {
				phase = 0xD0 >> 2;
			}
		}
		output += op_calc(phase << FREQ_SH, env, 0, SLOT7_1.wavetable);
	}

	// Snare Drum (verified on real YM3812)
	env = SLOT7_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// base frequency derived from operator 1 in channel 7
		bool bit8 = ((SLOT7_1.phase >> FREQ_SH) & 0x100) != 0;

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
		output += op_calc(phase << FREQ_SH, env, 0, SLOT7_2.wavetable);
	}

	// Tom Tom (verified on real YM3812)
	env = SLOT8_1.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		output += op_calc(SLOT8_1.phase, env, 0, SLOT8_1.wavetable);
	}
	
	// Top Cymbal (verified on real YM2413)
	env = SLOT8_2.volume_calc(LFO_AM);
	if (env < ENV_QUIET) {
		// base frequency derived from operator 1 in channel 7
		bool bit7 = ((SLOT7_1.phase >> FREQ_SH) & 0x80) != 0;
		bool bit3 = ((SLOT7_1.phase >> FREQ_SH) & 0x08) != 0;
		bool bit2 = ((SLOT7_1.phase >> FREQ_SH) & 0x04) != 0;
		bool res1 = ((bit2 ^ bit7) | bit3) != 0;
		// when res1 = 0 phase = 0x000 | 0x100;
		// when res1 = 1 phase = 0x200 | 0x100;
		unsigned phase = res1 ? 0x300 : 0x100;

		// enable gate based on frequency of operator 2 in channel 8
		bool bit5e= ((SLOT8_2.phase >> FREQ_SH) & 0x20) != 0;
		bool bit3e= ((SLOT8_2.phase >> FREQ_SH) & 0x08) != 0;
		bool res2 = (bit3e | bit5e) != 0;
		// when res2 = 0 pass the phase from calculation above (res1);
		// when res2 = 1 phase = 0x200 | 0x100;
		if (res2) {
			phase = 0x300;
		}
		output += op_calc(phase << FREQ_SH, env, 0, SLOT8_2.wavetable);
	}
	return output * 2;
}


// generic table initialize
void OpenYM2413::init_tables()
{
	static bool alreadyInit = false;
	if (alreadyInit) {
		return;
	}
	alreadyInit = true;
	
	for (int x = 0; x < TL_RES_LEN; x++) {
		DoubleT m = (1 << 16) / pow(2.0, (x + 1) * (ENV_STEP / 4.0) / 8.0);
		m = floor(m);

		// we never reach (1 << 16) here due to the (x + 1)
		// result fits within 16 bits at maximum
		int n = (int)m;	// 16 bits here
		n >>= 4;	// 12 bits here
		if (n & 1) {
			// round to nearest
			n = (n >> 1) + 1;
		} else {
			n =  n >> 1;
		}
		// 11 bits here (rounded)
		tl_tab[x * 2 + 0] = n;
		tl_tab[x * 2 + 1] = -tl_tab[x * 2 + 0];

		for (int i = 1; i < 11; i++) {
			tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN] =  tl_tab[x * 2 + 0] >> i;
			tl_tab[x * 2 + 1 + i * 2 * TL_RES_LEN] = -tl_tab[x * 2 + 0 + i * 2 * TL_RES_LEN];
		}
	}

	for (int i = 0; i < SIN_LEN; i++) {
		// non-standard sinus
		DoubleT m = sin(((i * 2) + 1) * PI / SIN_LEN); // checked against the real chip

		// we never reach zero here due to ((i*2)+1)
		DoubleT o = (m > 0.0) ?
		           (8 * ::log( 1.0 / m) / ::log(2.0)) :	// convert to 'decibels'
		           (8 * ::log(-1.0 / m) / ::log(2.0));	// convert to 'decibels'
		o = o / (ENV_STEP / 4);

		int n = (int)(2.0 * o);
		if (n & 1) {
			// round to nearest
			n = (n >> 1) + 1;
		} else {
			n =  n >> 1;
		}
		// waveform 0: standard sinus
		sin_tab[i] = n * 2 + (m >= 0.0 ? 0: 1);

		// waveform 1:  __      __     
		//             /  \____/  \____
		// output only first half of the sinus waveform (positive one)
		if (i & (1 << (SIN_BITS - 1))) {
			sin_tab[SIN_LEN + i] = TL_TAB_LEN;
		} else {
			sin_tab[SIN_LEN + i] = sin_tab[i];
		}
	}
}


void OpenYM2413::setSampleRate(int sampleRate, int Oversampling)
{
    oplOversampling = Oversampling;
	const int CLOCK_FREQ = 3579545;
	DoubleT freqbase = (CLOCK_FREQ / 72.0) / (DoubleT)(sampleRate * oplOversampling);

	// make fnumber -> increment counter table 
	for (int i = 0 ; i < 1024; i++) {
		// OPLL (YM2413) phase increment counter = 18bit 
		// -10 because chip works with 10.10 fixed point, while we use 16.16 
		fn_tab[i] = (int)((DoubleT)i * 64 * freqbase * (1 << (FREQ_SH - 10)));
	}

	// Amplitude modulation: 27 output levels (triangle waveform)
	// 1 level takes one of: 192, 256 or 448 samples 
	// One entry from LFO_AM_TABLE lasts for 64 samples 
	lfo_am_inc = (unsigned)((1 << LFO_SH) * freqbase / 64);

	// Vibrato: 8 output levels (triangle waveform); 1 level takes 1024 samples 
	lfo_pm_inc = (unsigned)((1 << LFO_SH) * freqbase / 1024);

	// Noise generator: a step takes 1 sample 
	noise_f = (unsigned)((1 << FREQ_SH) * freqbase);

	eg_timer_add  = (unsigned)((1 << EG_SH) * freqbase);
}

inline void Slot::KEY_ON(byte key_set)
{
	if (!key) {
		// do NOT restart Phase Generator (verified on real YM2413)
		// phase -> Dump 
		state = EG_DMP;
	}
	key |= key_set;
}

inline void Slot::KEY_OFF(byte key_clr)
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
inline void Channel::CALC_FCSLOT(Slot* slot)
{
	// (frequency) phase increment counter 
	slot->freq = fc * slot->mul;
	int ksr = kcode >> slot->KSR;

	if (slot->ksr != ksr) {
		slot->ksr = ksr;

		// calculate envelope generator rates 
		if ((slot->ar + slot->ksr) < (16 + 62)) {
			slot->eg_sh_ar  = eg_rate_shift [slot->ar + slot->ksr];
			slot->eg_sel_ar = eg_rate_select[slot->ar + slot->ksr];
		} else {
			slot->eg_sh_ar  = 0;
			slot->eg_sel_ar = 13 * RATE_STEPS;
		}
		slot->eg_sh_dr  = eg_rate_shift [slot->dr + slot->ksr];
		slot->eg_sel_dr = eg_rate_select[slot->dr + slot->ksr];
		slot->eg_sh_rr  = eg_rate_shift [slot->rr + slot->ksr];
		slot->eg_sel_rr = eg_rate_select[slot->rr + slot->ksr];
	}

	int SLOT_rs = (sus) ? (16 + (5 << 2)) : (16 + (7 << 2));
	slot->eg_sh_rs  = eg_rate_shift [SLOT_rs + slot->ksr];
	slot->eg_sel_rs = eg_rate_select[SLOT_rs + slot->ksr];

	int SLOT_dp = 16 + (13 << 2);
	slot->eg_sh_dp  = eg_rate_shift [SLOT_dp + slot->ksr];
	slot->eg_sel_dp = eg_rate_select[SLOT_dp + slot->ksr];
}

// set multi,am,vib,EG-TYP,KSR,mul 
inline void OpenYM2413::set_mul(byte sl, byte v)
{
	Channel& ch = channels[sl / 2];
	Slot& slot = ch.slots[sl & 1];

	slot.mul     = mul_tab[v & 0x0F];
	slot.KSR     = (v & 0x10) ? 0 : 2;
	slot.eg_type = (v & 0x20);
	slot.vib     = (v & 0x40);
	slot.AMmask  = (v & 0x80) ? ~0 : 0;
	ch.CALC_FCSLOT(&slot);
}

// set ksl, tl 
inline void OpenYM2413::set_ksl_tl(byte chan, byte v)
{
	Channel& ch = channels[chan];
	Slot& slot = ch.slots[SLOT1]; // modulator 

	int ksl = v >> 6; // 0 / 1.5 / 3.0 / 6.0 dB/OCT 

	slot.ksl = ksl ? (3 - ksl) : 31;
	slot.TL  = (v & 0x3F) << (ENV_BITS - 2 - 7); // 7 bits TL (bit 6 = always 0) 
	slot.TLL = slot.TL + (ch.ksl_base >> slot.ksl);
}

// set ksl , waveforms, feedback 
inline void OpenYM2413::set_ksl_wave_fb(byte chan, byte v)
{
	Channel& ch = channels[chan];
	Slot& slot1 = ch.slots[SLOT1]; // modulator 
	slot1.wavetable = ((v & 0x08) >> 3) * SIN_LEN;
	slot1.fb_shift  = (v & 7) ? ((v & 7) + 8) : 0;

	Slot& slot2 = ch.slots[SLOT2];	//carrier
	int ksl = v >> 6; // 0 / 1.5 / 3.0 / 6.0 dB/OCT 
	slot2.ksl = ksl ? (3 - ksl) : 31;
	slot2.TLL = slot2.TL + (ch.ksl_base >> slot2.ksl);
	slot2.wavetable = ((v & 0x10) >> 4) * SIN_LEN;
}

// set attack rate & decay rate  
inline void OpenYM2413::set_ar_dr(byte sl, byte v)
{
	Channel& ch = channels[sl / 2];
	Slot& slot = ch.slots[sl & 1];

	slot.ar = (v >> 4) ? (16 + ((v >> 4) << 2)) : 0;

	if ((slot.ar + slot.ksr) < (16 + 62)) {
		slot.eg_sh_ar  = eg_rate_shift [slot.ar + slot.ksr];
		slot.eg_sel_ar = eg_rate_select[slot.ar + slot.ksr];
	} else {
		slot.eg_sh_ar  = 0;
		slot.eg_sel_ar = 13 * RATE_STEPS;
	}
	slot.dr    = (v & 0x0F) ? (16 + ((v & 0x0F) << 2)) : 0;
	slot.eg_sh_dr  = eg_rate_shift [slot.dr + slot.ksr];
	slot.eg_sel_dr = eg_rate_select[slot.dr + slot.ksr];
}

// set sustain level & release rate 
inline void OpenYM2413::set_sl_rr(byte sl, byte v)
{
	Channel& ch = channels[sl / 2];
	Slot& slot = ch.slots[sl & 1];

	slot.sl  = sl_tab[v >> 4];
	slot.rr  = (v & 0x0F) ? (16 + ((v & 0x0F) << 2)) : 0;
	slot.eg_sh_rr  = eg_rate_shift [slot.rr + slot.ksr];
	slot.eg_sel_rr = eg_rate_select[slot.rr + slot.ksr];
}

void OpenYM2413::load_instrument(byte chan, byte slot, byte* inst)
{
	set_mul        (slot,     inst[0]);
	set_mul        (slot + 1, inst[1]);
	set_ksl_tl     (chan,     inst[2]);
	set_ksl_wave_fb(chan,     inst[3]);
	set_ar_dr      (slot,     inst[4]);
	set_ar_dr      (slot + 1, inst[5]);
	set_sl_rr      (slot,     inst[6]);
	set_sl_rr      (slot + 1, inst[7]);
}

void OpenYM2413::update_instrument_zero(byte r)
{
	byte* inst = &inst_tab[0][0]; // point to user instrument 
    byte chan;

	byte chan_max = (rhythm) ? 6 : 9;
	switch (r) {
	case 0:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_mul(chan * 2, inst[0]);
			}
		}
		break;
	case 1:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_mul(chan * 2 + 1, inst[1]);
			}
		}
		break;
	case 2:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_ksl_tl(chan, inst[2]);
			}
		}
		break;
	case 3:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_ksl_wave_fb(chan, inst[3]);
			}
		}
		break;
	case 4:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_ar_dr(chan * 2, inst[4]);
			}
		}
		break;
	case 5:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_ar_dr(chan * 2 + 1, inst[5]);
			}
		}
		break;
	case 6:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_sl_rr(chan * 2, inst[6]);
			}
		}
		break;
	case 7:
		for (chan = 0; chan < chan_max; chan++) {
			if ((instvol_r[chan] & 0xF0) == 0) {
				set_sl_rr(chan * 2 + 1, inst[7]);
			}
		}
		break;
	}
}

void OpenYM2413::setRhythmMode(bool newMode)
{
	if (newMode == rhythm) {
		return;
	}
	rhythm = newMode;
	if (rhythm) {
		// OFF -> ON
		
		// Load instrument settings for channel seven
		load_instrument(6, 12, &inst_tab[16][0]);

		// Load instrument settings for channel eight. (High hat and snare drum)
		load_instrument(7, 14, &inst_tab[17][0]);
		Channel& ch7 = channels[7];
		Slot& slot71 = ch7.slots[SLOT1]; // modulator envelope is HH 
		slot71.TL  = ((instvol_r[7] >> 4) << 2) << (ENV_BITS - 2 - 7); // 7 bits TL (bit 6 = always 0) 
		slot71.TLL = slot71.TL + (ch7.ksl_base >> slot71.ksl);

		// Load instrument settings for channel nine. (Tom-tom and top cymbal) 
		load_instrument(8, 16, &inst_tab[18][0]);
		Channel& ch8 = channels[8];
		Slot& slot81 = ch8.slots[SLOT1]; // modulator envelope is TOM 
		slot81.TL  = ((instvol_r[8] >> 4) << 2) << (ENV_BITS - 2 - 7); // 7 bits TL (bit 6 = always 0) 
		slot81.TLL = slot81.TL + (ch8.ksl_base >> slot81.ksl);
	} else {
		// ON -> OFF
		
		// Load instrument settings for channel seven
		load_instrument(6, 12, &inst_tab[instvol_r[6] >> 4][0]);

		// Load instrument settings for channel eight.
		load_instrument(7, 14, &inst_tab[instvol_r[7] >> 4][0]);

		// Load instrument settings for channel nine.
		load_instrument(8, 16, &inst_tab[instvol_r[8] >> 4][0]);
		
		// BD key off 
		channels[6].slots[SLOT1].KEY_OFF(~2);
		channels[6].slots[SLOT2].KEY_OFF(~2);
		// HH key off 
		channels[7].slots[SLOT1].KEY_OFF(~2);
		// SD key off 
		channels[7].slots[SLOT2].KEY_OFF(~2);
		// TOM key off 
		channels[8].slots[SLOT1].KEY_OFF(~2);
		// TOP-CY off 
		channels[8].slots[SLOT2].KEY_OFF(~2);
	}
}

void OpenYM2413::writeReg(byte r, byte v, const EmuTime &time)
{
    regs[r] = v;

	switch (r & 0xF0) {
	case 0x00: {
		// 00-0F:control 
		switch (r & 0x0F) {
		case 0x00:  // AM/VIB/EGTYP/KSR/MULTI (modulator) 
		case 0x01:  // AM/VIB/EGTYP/KSR/MULTI (carrier) 
		case 0x02:  // Key Scale Level, Total Level (modulator) 
		case 0x03:  // Key Scale Level, carrier waveform, modulator waveform, Feedback 
		case 0x04:  // Attack, Decay (modulator) 
		case 0x05:  // Attack, Decay (carrier) 
		case 0x06:  // Sustain, Release (modulator) 
		case 0x07:  // Sustain, Release (carrier) 
			inst_tab[0][r & 0x07] = v;
			update_instrument_zero(r & 7);
			break;

		case 0x0E: {
			// x, x, r,bd,sd,tom,tc,hh 
			setRhythmMode((v & 0x20) != 0);
			if (rhythm) {
				// BD key on/off 
				if (v & 0x10) {
					channels[6].slots[SLOT1].KEY_ON ( 2);
					channels[6].slots[SLOT2].KEY_ON ( 2);
				} else {
					channels[6].slots[SLOT1].KEY_OFF(~2);
					channels[6].slots[SLOT2].KEY_OFF(~2);
				}
				// HH key on/off 
				if (v & 0x01) {
					channels[7].slots[SLOT1].KEY_ON ( 2);
				} else {
					channels[7].slots[SLOT1].KEY_OFF(~2);
				}
				// SD key on/off 
				if (v & 0x08) {
					channels[7].slots[SLOT2].KEY_ON ( 2);
				} else {
					channels[7].slots[SLOT2].KEY_OFF(~2);
				}
				// TOM key on/off 
				if (v & 0x04) {
					channels[8].slots[SLOT1].KEY_ON ( 2);
				} else {
					channels[8].slots[SLOT1].KEY_OFF(~2);
				}
				// TOP-CY key on/off 
				if (v & 0x02) {
					channels[8].slots[SLOT2].KEY_ON ( 2);
				} else {
					channels[8].slots[SLOT2].KEY_OFF(~2);
				}
			}
			break;
		}
		}
		break;
	}
	case 0x10:
	case 0x20: {
		byte chan = (r & 0x0F) % 9;	// verified on real YM2413 
		Channel& ch = channels[chan];

		int block_fnum;
		if (r & 0x10) {
			// 10-18: FNUM 0-7 
			block_fnum  = (ch.block_fnum & 0x0F00) | v;
		} else {
			// 20-28: suson, keyon, block, FNUM 8 
			block_fnum = ((v & 0x0F) << 8) | (ch.block_fnum & 0xFF);
			if (v & 0x10) {
				ch.slots[SLOT1].KEY_ON ( 1);
				ch.slots[SLOT2].KEY_ON ( 1);
			} else {
				ch.slots[SLOT1].KEY_OFF(~1);
				ch.slots[SLOT2].KEY_OFF(~1);
			}
			ch.sus = v & 0x20;
		}
		// update 
		if (ch.block_fnum != block_fnum) {
			ch.block_fnum = block_fnum;

			// BLK 2,1,0 bits -> bits 3,2,1 of kcode, FNUM MSB -> kcode LSB 
			ch.kcode    = (block_fnum & 0x0f00) >> 8;
			ch.ksl_base = ksl_tab[block_fnum >> 5];
			block_fnum  = block_fnum * 2;
			byte block  = (block_fnum & 0x1C00) >> 10;
			ch.fc       = fn_tab[block_fnum & 0x03FF] >> (7 - block);

			// refresh Total Level in both SLOTs of this channel 
			ch.slots[SLOT1].TLL = ch.slots[SLOT1].TL + (ch.ksl_base >> ch.slots[SLOT1].ksl);
			ch.slots[SLOT2].TLL = ch.slots[SLOT2].TL + (ch.ksl_base >> ch.slots[SLOT2].ksl);

			// refresh frequency counter in both SLOTs of this channel 
			ch.CALC_FCSLOT(&ch.slots[SLOT1]);
			ch.CALC_FCSLOT(&ch.slots[SLOT2]);
		}
		break;
	}

	case 0x30: {
		// inst 4 MSBs, VOL 4 LSBs 
		byte chan = (r & 0x0F) % 9;	// verified on real YM2413 
		byte old_instvol = instvol_r[chan];
		instvol_r[chan] = v;  // store for later use 

		Channel& ch = channels[chan];
		Slot& slot2 = ch.slots[SLOT2]; // carrier 
		slot2.TL  = ((v & 0x0F) << 2) << (ENV_BITS - 2 - 7); // 7 bits TL (bit 6 = always 0) 
		slot2.TLL = slot2.TL + (ch.ksl_base >> slot2.ksl);

		//check wether we are in rhythm mode and handle instrument/volume register accordingly
		if ((chan >= 6) && rhythm) {
			// we're in rhythm mode
			if (chan >= 7) {
				// only for channel 7 and 8 (channel 6 is handled in usual way)
				Slot& slot1 = ch.slots[SLOT1]; // modulator envelope is HH(chan=7) or TOM(chan=8) 
				slot1.TL  = ((instvol_r[chan] >> 4) << 2) << (ENV_BITS - 2 - 7); // 7 bits TL (bit 6 = always 0) 
				slot1.TLL = slot1.TL + (ch.ksl_base >> slot1.ksl);
			}
		} else {
			if (!((old_instvol & 0xF0) == (v & 0xF0))) {
				byte* inst = &inst_tab[instvol_r[chan] >> 4][0];
				byte sl = chan * 2;
				load_instrument(chan, sl, inst);
			}
		}
		break;
	}

	default:
		break;
	}

    checkMute();
}


void OpenYM2413::reset(const EmuTime &time)
{
	eg_timer = 0;
	eg_cnt   = 0;
	noise_rng = 1;    // noise shift register 

	// setup instruments table 
    int i;
	for (i = 0; i < 19; i++) {
		for (int c = 0; c < 8; c++) {
			inst_tab[i][c] = table[i][c];
		}
	}

    memset(regs, 0, sizeof(regs));

	// reset with register write 
	writeReg(0x0F, 0, time); //test reg
	for (i = 0x3F; i >= 0x10; i--) {
		writeReg(i, 0, time);
	}

	// reset operator parameters 
	for (int c = 0; c < 9; c++) {
		Channel& ch = channels[c];
		for (int s = 0; s < 2; s++) {
			// wave table 
			ch.slots[s].wavetable = 0;
			ch.slots[s].state     = EG_OFF;
			ch.slots[s].volume    = MAX_ATT_INDEX;
		}
	}
}


OpenYM2413::OpenYM2413(const string &name, short volume, const EmuTime &time)
{
	eg_cnt = eg_timer = 0;
	rhythm = 0;
	lfo_am_cnt = lfo_pm_cnt = 0;
	noise_rng = noise_p = 0;
	LFO_AM = LFO_PM = 0;
	for (int i = 0; i < 9; i++) {
		instvol_r[i] = 0;
	}
	
    oplOversampling = 1;

	init_tables();

	reset(time);
}

OpenYM2413::~OpenYM2413()
{
}

int OpenYM2413::filter(int input) {
    in[4] = in[3];
    in[3] = in[2];
    in[2] = in[1];
    in[1] = in[0];
    in[0] = input;

    return (in[0] + in[4] + 2 * (in[3] + in[1]) + 4 * in[2]) / 8;
}


int* OpenYM2413::updateBuffer(int length)
{
	if (isInternalMuted()) {
		return NULL;
	}
	
	int* buf = buffer;
	while (length--) {
		int output = 0;
        int count = oplOversampling;
        while (count--) {
		    advance_lfo();
		    output += channels[0].chan_calc(LFO_AM);
		    output += channels[1].chan_calc(LFO_AM);
		    output += channels[2].chan_calc(LFO_AM);
		    output += channels[3].chan_calc(LFO_AM);
		    output += channels[4].chan_calc(LFO_AM);
		    output += channels[5].chan_calc(LFO_AM);
		    if (!rhythm) {
			    output += channels[6].chan_calc(LFO_AM);
			    output += channels[7].chan_calc(LFO_AM);
			    output += channels[8].chan_calc(LFO_AM);
		    } else {
			    output += rhythm_calc(noise_rng & 1);
		    }
		    advance();
        }
        
//		*(buf++) = (output << 5) / oplOversampling;
		*(buf++) = filter((output << 5) / oplOversampling);
	}
	checkMute();
	return buffer;
}

void OpenYM2413::checkMute()
{
	setInternalMute(checkMuteHelper());
}
bool OpenYM2413::checkMuteHelper()
{
	for (int i = 0; i < 6; i++) {
		if (channels[i].slots[SLOT2].state != EG_OFF) {
			return false;
		}
	}
	if (!rhythm) {
		for (int i = 6; i < 9; i++) {
			if (channels[i].slots[SLOT2].state != EG_OFF) {
				return false;
			}
		}
	} else {
		if (channels[6].slots[SLOT2].state != EG_OFF) return false;
		if (channels[7].slots[SLOT1].state != EG_OFF) return false;
		if (channels[7].slots[SLOT2].state != EG_OFF) return false;
		if (channels[8].slots[SLOT1].state != EG_OFF) return false;
		if (channels[8].slots[SLOT2].state != EG_OFF) return false;
	}
	return true;	// nothing playing, mute
}

void OpenYM2413::setInternalVolume(short newVolume)
{
	maxVolume = newVolume;
}

void OpenYM2413::loadState()
{
    SaveState* state = saveStateOpenForRead("ym2413");
    char tag[32];
    int i;

    maxVolume    = (short)saveStateGet(state, "maxVolume",    0);

    eg_cnt       = saveStateGet(state, "eg_cnt",       0);
    eg_timer     = saveStateGet(state, "eg_timer",     0);
    eg_timer_add = saveStateGet(state, "eg_timer_add", 0);

    rhythm       = saveStateGet(state, "rhythm",       0) != 0;

    lfo_am_cnt   = saveStateGet(state, "lfo_am_cnt",   0);
    lfo_am_inc   = saveStateGet(state, "lfo_am_inc",   0);
    lfo_pm_cnt   = saveStateGet(state, "lfo_pm_cnt",   0);
    lfo_pm_inc   = saveStateGet(state, "lfo_pm_inc",   0);

    noise_rng    = saveStateGet(state, "noise_rng",    0);
    noise_p      = saveStateGet(state, "noise_p",      0);
    noise_f      = saveStateGet(state, "noise_f",      0);

    LFO_AM       = (byte)saveStateGet(state, "LFO_AM",       0);
    LFO_PM       = (byte)saveStateGet(state, "LFO_PM",       0);

    saveStateGetBuffer(state, "inst_tab", inst_tab, sizeof(inst_tab));

    for (i = 0; i < sizeof(fn_tab) / sizeof(fn_tab[0]); i++) {
        sprintf(tag, "fn_tab%.4d", i);
        fn_tab[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < 9; i++) {
        sprintf(tag, "instvol_r%d", i);
        instvol_r[i] = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "block_fnum%d", i);
        channels[i].block_fnum = saveStateGet(state, tag, 0);

        sprintf(tag, "fc%d", i);
        channels[i].fc = saveStateGet(state, tag, 0);

        sprintf(tag, "ksl_base%d", i);
        channels[i].ksl_base = saveStateGet(state, tag, 0);

        sprintf(tag, "kcode%d", i);
        channels[i].kcode = (byte)saveStateGet(state, tag, 0);

        sprintf(tag, "sus%d", i);
        channels[i].sus = (byte)saveStateGet(state, tag, 0);
        
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
            
            sprintf(tag, "phase%d_%d", i, j);
            channels[i].slots[j].phase = saveStateGet(state, tag, 0);
            
            sprintf(tag, "freq%d_%d", i, j);
            channels[i].slots[j].freq = saveStateGet(state, tag, 0);
            
            sprintf(tag, "fb_shift%d_%d", i, j);
            channels[i].slots[j].fb_shift = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "op1_out%d_%d_0", i, j);
            channels[i].slots[j].op1_out[0] = saveStateGet(state, tag, 0);
            
            sprintf(tag, "op1_out%d_%d_1", i, j);
            channels[i].slots[j].op1_out[1] = saveStateGet(state, tag, 0);
            
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
            
            sprintf(tag, "eg_sh_dp%d_%d", i, j);
            channels[i].slots[j].eg_sh_dp = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_dp%d_%d", i, j);
            channels[i].slots[j].eg_sel_dp = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_ar%d_%d", i, j);
            channels[i].slots[j].eg_sh_ar = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_ar%d_%d", i, j);
            channels[i].slots[j].eg_sel_ar = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_dr%d_%d", i, j);
            channels[i].slots[j].eg_sh_dr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_dr%d_%d", i, j);
            channels[i].slots[j].eg_sel_dr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_rr%d_%d", i, j);
            channels[i].slots[j].eg_sh_rr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_rr%d_%d", i, j);
            channels[i].slots[j].eg_sel_rr = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sh_rs%d_%d", i, j);
            channels[i].slots[j].eg_sh_rs = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_sel_rs%d_%d", i, j);
            channels[i].slots[j].eg_sel_rs =(byte) saveStateGet(state, tag, 0);
            
            sprintf(tag, "key%d_%d", i, j);
            channels[i].slots[j].key = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "AMmask%d_%d", i, j);
            channels[i].slots[j].AMmask = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "vib%d_%d", i, j);
            channels[i].slots[j].vib = (byte)saveStateGet(state, tag, 0);
            
            sprintf(tag, "wavetable%d_%d", i, j);
            channels[i].slots[j].wavetable = saveStateGet(state, tag, 0);
        }
    }

    saveStateClose(state);
}

void OpenYM2413::saveState()
{
    SaveState* state = saveStateOpenForWrite("ym2413");
    char tag[32];
    int i;

    saveStateSet(state, "maxVolume",    maxVolume);

    saveStateSet(state, "eg_cnt",       eg_cnt);
    saveStateSet(state, "eg_timer",     eg_timer);
    saveStateSet(state, "eg_timer_add", eg_timer_add);

    saveStateSet(state, "rhythm",       rhythm);

    saveStateSet(state, "lfo_am_cnt",   lfo_am_cnt);
    saveStateSet(state, "lfo_am_inc",   lfo_am_inc);
    saveStateSet(state, "lfo_pm_cnt",   lfo_pm_cnt);
    saveStateSet(state, "lfo_pm_inc",   lfo_pm_inc);

    saveStateSet(state, "noise_rng",    noise_rng);
    saveStateSet(state, "noise_p",      noise_p);
    saveStateSet(state, "noise_f",      noise_f);

    saveStateSet(state, "LFO_AM",       LFO_AM);
    saveStateSet(state, "LFO_PM",       LFO_PM);

    saveStateSetBuffer(state, "inst_tab", inst_tab, sizeof(inst_tab));

    for (i = 0; i < sizeof(fn_tab) / sizeof(fn_tab[0]); i++) {
        sprintf(tag, "fn_tab%.4d", i);
        saveStateSet(state, tag, fn_tab[i]);
    }

    for (i = 0; i < 9; i++) {
        sprintf(tag, "instvol_r%d", i);
        saveStateSet(state, tag, instvol_r[i]);
        
        sprintf(tag, "block_fnum%d", i);
        saveStateSet(state, tag, channels[i].block_fnum);

        sprintf(tag, "fc%d", i);
        saveStateSet(state, tag, channels[i].fc);

        sprintf(tag, "ksl_base%d", i);
        saveStateSet(state, tag, channels[i].ksl_base);

        sprintf(tag, "kcode%d", i);
        saveStateSet(state, tag, channels[i].kcode);

        sprintf(tag, "sus%d", i);
        saveStateSet(state, tag, channels[i].sus);
        
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
            
            sprintf(tag, "phase%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].phase);
            
            sprintf(tag, "freq%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].freq);
            
            sprintf(tag, "fb_shift%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].fb_shift);
            
            sprintf(tag, "op1_out%d_%d_0", i, j);
            saveStateSet(state, tag, channels[i].slots[j].op1_out[0]);
            
            sprintf(tag, "op1_out%d_%d_1", i, j);
            saveStateSet(state, tag, channels[i].slots[j].op1_out[1]);
            
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
            
            sprintf(tag, "eg_sh_dp%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_dp);
            
            sprintf(tag, "eg_sel_dp%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_dp);
            
            sprintf(tag, "eg_sh_ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_ar);
            
            sprintf(tag, "eg_sel_ar%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_ar);
            
            sprintf(tag, "eg_sh_dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_dr);
            
            sprintf(tag, "eg_sel_dr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_dr);
            
            sprintf(tag, "eg_sh_rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_rr);
            
            sprintf(tag, "eg_sel_rr%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_rr);
            
            sprintf(tag, "eg_sh_rs%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sh_rs);
            
            sprintf(tag, "eg_sel_rs%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].eg_sel_rs);
            
            sprintf(tag, "key%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].key);
            
            sprintf(tag, "AMmask%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].AMmask);
            
            sprintf(tag, "vib%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].vib);
            
            sprintf(tag, "wavetable%d_%d", i, j);
            saveStateSet(state, tag, channels[i].slots[j].wavetable);
        }
    }

    saveStateClose(state);
}
