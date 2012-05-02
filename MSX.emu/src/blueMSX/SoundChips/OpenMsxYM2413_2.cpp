// This file is taken from the openMSX project. 
// The file has been modified to be built in the blueMSX environment.
//
// The file was originally written by Mitsutaka Okazaki.
//
#include "OpenMsxYM2413_2.h"

extern "C" {
#include "SaveState.h"
}

#ifdef assert
#undef assert
#endif
#define assert(x)

#include <stdio.h>
#include <math.h>
#include <algorithm>

//using std::string;

static const int CLOCK_FREQ = 3579545;
static const DoubleT PI = 3.14159265358979323846;

int OpenYM2413_2::pmtable[PM_PG_WIDTH];
int OpenYM2413_2::amtable[AM_PG_WIDTH];
unsigned int OpenYM2413_2::tllTable[16][8][1 << TL_BITS][4];
int OpenYM2413_2::rksTable[2][8][2];
word OpenYM2413_2::AR_ADJUST_TABLE[1 << EG_BITS];
word OpenYM2413_2::fullsintable[PG_WIDTH];
word OpenYM2413_2::halfsintable[PG_WIDTH];
word* OpenYM2413_2::waveform[2] = {fullsintable, halfsintable};
short OpenYM2413_2::dB2LinTab[(2 * DB_MUTE) * 2];
unsigned int OpenYM2413_2::dphaseARTable[16][16];
unsigned int OpenYM2413_2::dphaseDRTable[16][16];
unsigned int OpenYM2413_2::dphaseTable[512][8][16];
unsigned int OpenYM2413_2::pm_dphase;
unsigned int OpenYM2413_2::am_dphase;


//***************************************************//
//                                                   //
//  Helper functions                                 //
//                                                   //
//***************************************************//

int OpenYM2413_2::Slot::EG2DB(int d) 
{
	return d * (int)(EG_STEP / DB_STEP);
}
int OpenYM2413_2::TL2EG(int d)
{ 
	return d * (int)(TL_STEP / EG_STEP);
}

unsigned int OpenYM2413_2::DB_POS(DoubleT x)
{
	return (unsigned int)(x / DB_STEP);
}
unsigned int OpenYM2413_2::DB_NEG(DoubleT x)
{
	return (unsigned int)(2 * DB_MUTE + x / DB_STEP);
}

// Cut the lower b bit off
template <typename T>
static inline T HIGHBITS(T c, int b)
{
	return c >> b;
}
// Expand x which is s bits to d bits
static inline unsigned EXPAND_BITS(unsigned x, int s, int d)
{
	return x << (d - s);
}
// Adjust envelope speed which depends on sampling rate
static inline unsigned int rate_adjust(DoubleT x, int rate)
{
	DoubleT tmp = x * CLOCK_FREQ / 72 / rate + 0.5; // +0.5 to round
	assert (tmp <= 4294967295U);
	return (unsigned int)tmp;
}

static inline bool BIT(int s, int b)
{
	return (s >> b) & 1;
}


//***************************************************//
//                                                   //
//                  Create tables                    //
//                                                   //
//***************************************************//

// Table for AR to LogCurve.
void OpenYM2413_2::makeAdjustTable()
{
	AR_ADJUST_TABLE[0] = (1 << EG_BITS) - 1;
	for (int i = 1; i < (1 << EG_BITS); ++i) {
		AR_ADJUST_TABLE[i] = (unsigned short)((DoubleT)(1 << EG_BITS) - 1 -
		                     ((1 << EG_BITS) - 1) * ::log((DoubleT)i) / ::log(127.0));
	}
}

// Table for dB(0 .. (1<<DB_BITS)-1) to lin(0 .. DB2LIN_AMP_WIDTH)
void OpenYM2413_2::makeDB2LinTable()
{
	for (int i = 0; i < 2 * DB_MUTE; ++i) {
		dB2LinTab[i] = (i < DB_MUTE)
		             ?  (short)((DoubleT)((1 << DB2LIN_AMP_BITS) - 1) *
		                    ::pow(10.0, -(DoubleT)i * DB_STEP / 20))
		             : 0;
		dB2LinTab[i + 2 * DB_MUTE] = -dB2LinTab[i];
	}
}

// lin(+0.0 .. +1.0) to  dB((1<<DB_BITS)-1 .. 0)
int OpenYM2413_2::lin2db(DoubleT d)
{
	return (d == 0)
		? DB_MUTE - 1
		: std::min(-(int)(20.0 * log10(d) / DB_STEP), DB_MUTE - 1); // 0 - 127
}

// Sin Table
void OpenYM2413_2::makeSinTable()
{
	for (int i = 0; i < PG_WIDTH / 4; ++i)
		fullsintable[i] = lin2db(sin(2.0 * PI * i / PG_WIDTH));
	for (int i = 0; i < PG_WIDTH / 4; ++i)
		fullsintable[PG_WIDTH / 2 - 1 - i] = fullsintable[i];
	for (int i = 0; i < PG_WIDTH / 2; ++i)
		fullsintable[PG_WIDTH / 2 + i] = 2 * DB_MUTE + fullsintable[i];

	for (int i = 0; i < PG_WIDTH / 2; ++i)
		halfsintable[i] = fullsintable[i];
	for (int i = PG_WIDTH / 2; i < PG_WIDTH; ++i)
		halfsintable[i] = fullsintable[0];
}

static inline DoubleT saw(DoubleT phase)
{
  if (phase <= (PI / 2)) {
    return phase * 2 / PI;
  } else if (phase <= (PI * 3 / 2)) {
    return 2.0 - (phase * 2 / PI);
  } else {
    return -4.0 + phase * 2 / PI;
  }
}

// Table for Pitch Modulator
void OpenYM2413_2::makePmTable()
{
	for (int i = 0; i < PM_PG_WIDTH; ++i) {
		 pmtable[i] = (int)((DoubleT)PM_AMP *
		     ::pow(2.0, (DoubleT)PM_DEPTH *
		            saw(2.0 * PI * i / PM_PG_WIDTH) / 1200));
	}
}

// Table for Amp Modulator
void OpenYM2413_2::makeAmTable()
{
	for (int i = 0; i < AM_PG_WIDTH; ++i) {
		amtable[i] = (int)((DoubleT)AM_DEPTH / 2 / DB_STEP *
		                   (1.0 + saw(2.0 * PI * i / PM_PG_WIDTH)));
	}
}

// Phase increment counter table
void OpenYM2413_2::makeDphaseTable(int sampleRate)
{
	unsigned int mltable[16] = {
		1,   1*2,  2*2,  3*2,  4*2,  5*2,  6*2,  7*2,
		8*2, 9*2, 10*2, 10*2, 12*2, 12*2, 15*2, 15*2
	};

	for (unsigned fnum = 0; fnum < 512; ++fnum) {
		for (unsigned block = 0; block < 8; ++block) {
			for (unsigned ML = 0; ML < 16; ++ML) {
				dphaseTable[fnum][block][ML] = 
				    rate_adjust(((fnum * mltable[ML]) << block) >>
				                (20 - DP_BITS),
				                sampleRate);
			}
		}
	}
}

void OpenYM2413_2::makeTllTable()
{
	DoubleT kltable[16] = {
		( 0.000 * 2), ( 9.000 * 2), (12.000 * 2), (13.875 * 2),
		(15.000 * 2), (16.125 * 2), (16.875 * 2), (17.625 * 2),
		(18.000 * 2), (18.750 * 2), (19.125 * 2), (19.500 * 2),
		(19.875 * 2), (20.250 * 2), (20.625 * 2), (21.000 * 2)
	};
  
	for (int fnum = 0; fnum < 16; ++fnum) {
		for (int block = 0; block < 8; ++block) {
			for (int TL = 0; TL < 64; ++TL) {
				for (int KL = 0; KL < 4; ++KL) {
					if (KL == 0) {
						tllTable[fnum][block][TL][KL] = TL2EG(TL);
					} else {
						int tmp = (int)(kltable[fnum] - (3.000 * 2) * (7 - block));
						tllTable[fnum][block][TL][KL] =
						    (tmp <= 0) ?
						    TL2EG(TL) :
						    (unsigned int)((tmp >> (3 - KL)) / EG_STEP) + TL2EG(TL);
					}
				}
			}
		}
	}
}

// Rate Table for Attack
void OpenYM2413_2::makeDphaseARTable(int sampleRate)
{
	for (int AR = 0; AR < 16; ++AR) {
		for (int Rks = 0; Rks < 16; ++Rks) {
			int RM = AR + (Rks >> 2);
			int RL = Rks & 3;
			if (RM > 15) RM = 15;
			switch (AR) { 
			case 0:
				dphaseARTable[AR][Rks] = 0;
				break;
			case 15:
				dphaseARTable[AR][Rks] = 0; // EG_DP_WIDTH
				break;
			default:
				dphaseARTable[AR][Rks] = rate_adjust(3 * (RL + 4) << (RM + 1), sampleRate);
				break;
			}
		}
	}
}

// Rate Table for Decay
void OpenYM2413_2::makeDphaseDRTable(int sampleRate)
{
	for (int DR = 0; DR < 16; ++DR) {
		for (int Rks = 0; Rks < 16; ++Rks) {
			int RM = DR + (Rks >> 2);
			int RL = Rks & 3;
			if (RM > 15) RM = 15;
			switch(DR) { 
			case 0:
				dphaseDRTable[DR][Rks] = 0;
				break;
			default:
				dphaseDRTable[DR][Rks] = rate_adjust((RL + 4) << (RM - 1), sampleRate);
				break;
			}
		}
	}
}

void OpenYM2413_2::makeRksTable()
{
	for (int fnum8 = 0; fnum8 < 2; ++fnum8) {
		for (int block = 0; block < 8; ++block) {
			for (int KR = 0; KR < 2; ++KR) {
				rksTable[fnum8][block][KR] = (KR != 0)
					? (block << 1) + fnum8
					:  block >> 1;
			}
		}
	}
}

//************************************************************//
//                                                            //
//                      Patch                                 //
//                                                            //
//************************************************************//

OpenYM2413_2::Patch::Patch()
	: AM(false), PM(false), EG(false)
	, KR(0), ML(0), KL(0), TL(0), FB(0)
	, WF(0), AR(0), DR(0), SL(0), RR(0)
{
}

OpenYM2413_2::Patch::Patch(int n, const byte* data)
{
	if (n == 0) {
		AM = (data[0] >> 7) & 1;
		PM = (data[0] >> 6) & 1;
		EG = (data[0] >> 5) & 1;
		KR = (data[0] >> 4) & 1;
		ML = (data[0] >> 0) & 15;
		KL = (data[2] >> 6) & 3;
		TL = (data[2] >> 0) & 63;
		FB = (data[3] >> 0) & 7;
		WF = (data[3] >> 3) & 1;
		AR = (data[4] >> 4) & 15;
		DR = (data[4] >> 0) & 15;
		SL = (data[6] >> 4) & 15;
		RR = (data[6] >> 0) & 15;
	} else {
		AM = (data[1] >> 7) & 1;
		PM = (data[1] >> 6) & 1;
		EG = (data[1] >> 5) & 1;
		KR = (data[1] >> 4) & 1;
		ML = (data[1] >> 0) & 15;
		KL = (data[3] >> 6) & 3;
		TL = 0;
		FB = 0;
		WF = (data[3] >> 4) & 1;
		AR = (data[5] >> 4) & 15;
		DR = (data[5] >> 0) & 15;
		SL = (data[7] >> 4) & 15;
		RR = (data[7] >> 0) & 15;
	}
}

//************************************************************//
//                                                            //
//                      Slot                                  //
//                                                            //
//************************************************************//

OpenYM2413_2::Slot::Slot(bool type)
{
	reset(type);
}

void OpenYM2413_2::Slot::reset(bool type_)
{
	type = type_;
    sintblIdx = 0;
	sintbl = waveform[sintblIdx];
	phase = 0;
	dphase = 0;
	output[0] = 0;
	output[1] = 0;
	feedback = 0;
	eg_mode = FINISH;
	eg_phase = EG_DP_WIDTH;
	eg_dphase = 0;
	rks = 0;
	tll = 0;
	sustine = false;
	fnum = 0;
	block = 0;
	volume = 0;
	pgout = 0;
	egout = 0;
	slot_on_flag = false;

    setPatch(NULL_PATCH_IDX);
}


void OpenYM2413_2::Slot::updatePG()
{
	dphase = dphaseTable[fnum][block][patches[patchIdx].ML];
}

void OpenYM2413_2::Slot::updateTLL()
{
	tll = type ? tllTable[fnum >> 5][block][volume]   [patches[patchIdx].KL]:
	             tllTable[fnum >> 5][block][patches[patchIdx].TL][patches[patchIdx].KL];
}

void OpenYM2413_2::Slot::updateRKS()
{
	rks = rksTable[fnum >> 8][block][patches[patchIdx].KR];
}

void OpenYM2413_2::Slot::updateWF()
{
    sintblIdx = patches[patchIdx].WF;
	sintbl = waveform[sintblIdx];
}

void OpenYM2413_2::Slot::updateEG()
{
	switch (eg_mode) {
	case ATTACK:
		eg_dphase = dphaseARTable[patches[patchIdx].AR][rks];
		break;
	case DECAY:
		eg_dphase = dphaseDRTable[patches[patchIdx].DR][rks];
		break;
	case SUSTINE:
		eg_dphase = dphaseDRTable[patches[patchIdx].RR][rks];
		break;
	case RELEASE:
		if (sustine) {
			eg_dphase = dphaseDRTable[5][rks];
		} else if (patches[patchIdx].EG) {
			eg_dphase = dphaseDRTable[patches[patchIdx].RR][rks];
		} else {
			eg_dphase = dphaseDRTable[7][rks];
		}
		break;
	case SETTLE:
		eg_dphase = dphaseDRTable[15][0];
		break;
	case SUSHOLD:
	case FINISH:
	default:
		eg_dphase = 0;
		break;
	}
}

void OpenYM2413_2::Slot::updateAll()
{
	updatePG();
	updateTLL();
	updateRKS();
	updateWF();
	updateEG(); // EG should be updated last
}


// Slot key on
void OpenYM2413_2::Slot::slotOn()
{
	eg_mode = ATTACK;
	eg_phase = 0;
	phase = 0;
	updateEG();
}

// Slot key on, without resetting the phase
void OpenYM2413_2::Slot::slotOn2()
{
	eg_mode = ATTACK;
	eg_phase = 0;
	updateEG();
}

// Slot key off
void OpenYM2413_2::Slot::slotOff()
{
	if (eg_mode == ATTACK) {
		eg_phase = EXPAND_BITS(
			AR_ADJUST_TABLE[HIGHBITS(
				eg_phase, EG_DP_BITS - EG_BITS)],
			EG_BITS, EG_DP_BITS);
	}
	eg_mode = RELEASE;
	updateEG();
}


// Change a rhythm voice
void OpenYM2413_2::Slot::setPatch(int idx)
{
    patchIdx = idx;
}

void OpenYM2413_2::Slot::setVolume(int newVolume)
{
	volume = newVolume;
}



//***********************************************************//
//                                                           //
//               Channel                                     //
//                                                           //
//***********************************************************//


OpenYM2413_2::Channel::Channel()
	: mod(false), car(true)
{
	reset();
}

// reset channel
void OpenYM2413_2::Channel::reset()
{
	mod.reset(false);
	car.reset(true);
	setPatch(0);
}

// Change a voice
void OpenYM2413_2::Channel::setPatch(int num)
{
	patch_number = num;
	mod.setPatch(2 * num + 0);
	car.setPatch(2 * num + 1);
}

// Set sustine parameter
void OpenYM2413_2::Channel::setSustine(bool sustine)
{
	car.sustine = sustine;
	if (mod.type) {
		mod.sustine = sustine;
	}
}

// Volume : 6bit ( Volume register << 2 )
void OpenYM2413_2::Channel::setVol(int volume)
{
	car.volume = volume;
}

// Set F-Number (fnum : 9bit)
void OpenYM2413_2::Channel::setFnumber(int fnum)
{
	car.fnum = fnum;
	mod.fnum = fnum;
}

// Set Block data (block : 3bit)
void OpenYM2413_2::Channel::setBlock(int block)
{
	car.block = block;
	mod.block = block;
}

// Channel key on
void OpenYM2413_2::Channel::keyOn()
{
	if (!mod.slot_on_flag) mod.slotOn();
	if (!car.slot_on_flag) car.slotOn();
}

// Channel key off
void OpenYM2413_2::Channel::keyOff()
{
	// Note: no mod.slotOff() in original code!!!
	if (car.slot_on_flag) car.slotOff();
}



//***********************************************************//
//                                                           //
//               OpenYM2413_2                                      //
//                                                           //
//***********************************************************//

static byte inst_data[16 + 3][8] = {
	{ 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }, // user instrument
	{ 0x61,0x61,0x1e,0x17,0xf0,0x7f,0x00,0x17 }, // violin
	{ 0x13,0x41,0x16,0x0e,0xfd,0xf4,0x23,0x23 }, // guitar
	{ 0x03,0x01,0x9a,0x04,0xf3,0xf3,0x13,0xf3 }, // piano
	{ 0x11,0x61,0x0e,0x07,0xfa,0x64,0x70,0x17 }, // flute
	{ 0x22,0x21,0x1e,0x06,0xf0,0x76,0x00,0x28 }, // clarinet
	{ 0x21,0x22,0x16,0x05,0xf0,0x71,0x00,0x18 }, // oboe
	{ 0x21,0x61,0x1d,0x07,0x82,0x80,0x17,0x17 }, // trumpet
	{ 0x23,0x21,0x2d,0x16,0x90,0x90,0x00,0x07 }, // organ
	{ 0x21,0x21,0x1b,0x06,0x64,0x65,0x10,0x17 }, // horn
	{ 0x21,0x21,0x0b,0x1a,0x85,0xa0,0x70,0x07 }, // synthesizer
	{ 0x23,0x01,0x83,0x10,0xff,0xb4,0x10,0xf4 }, // harpsichord
	{ 0x97,0xc1,0x20,0x07,0xff,0xf4,0x22,0x22 }, // vibraphone
	{ 0x61,0x00,0x0c,0x05,0xc2,0xf6,0x40,0x44 }, // synthesizer bass
	{ 0x01,0x01,0x56,0x03,0x94,0xc2,0x03,0x12 }, // acoustic bass
	{ 0x21,0x01,0x89,0x03,0xf1,0xe4,0xf0,0x23 }, // electric guitar
	{ 0x07,0x21,0x14,0x00,0xee,0xf8,0xff,0xf8 },
	{ 0x01,0x31,0x00,0x00,0xf8,0xf7,0xf8,0xf7 },
	{ 0x25,0x11,0x00,0x00,0xf8,0xfa,0xf8,0x55 }
};

OpenYM2413_2::OpenYM2413_2(const char * name_, short volume, const EmuTime& time)
	: name(name_)
{
	for (int i = 0; i < 16 + 3; ++i) {
		patches[2 * i + 0] = Patch(0, inst_data[i]);
		patches[2 * i + 1] = Patch(1, inst_data[i]);
	}

	for (int i = 0; i < 0x40; ++i) {
		reg[i] = 0; // avoid UMR
	}

	for (int i = 0; i < 9; ++i) {
		// TODO cleanup
		ch[i].patches = patches;
        ch[i].mod.patches = patches;
        ch[i].car.patches = patches;
	}

	makePmTable();
	makeAmTable();
	makeDB2LinTable();
	makeAdjustTable();
	makeTllTable();
	makeRksTable();
	makeSinTable();

	reset(time);
}

OpenYM2413_2::~OpenYM2413_2()
{
}

const char * OpenYM2413_2::getName() const
{
	return name;
}

const char * OpenYM2413_2::getDescription() const
{
	static const char * desc = "MSX-MUSIC";
	return desc;
}

// Reset whole of OPLL except patch datas
void OpenYM2413_2::reset(const EmuTime &time)
{
	pm_phase = 0;
	am_phase = 0;
	noise_seed = 0xFFFF;

	for(int i = 0; i < 9; i++) {
		ch[i].reset();
	}
	for (int i = 0; i < 0x40; i++) {
		writeReg(i, 0, time);
	}
	setInternalMute(true);	// set muted
}

void OpenYM2413_2::setSampleRate(int sampleRate, int Oversampling)
{
	makeDphaseTable(sampleRate);
	makeDphaseARTable(sampleRate);
	makeDphaseDRTable(sampleRate);
	pm_dphase = rate_adjust(PM_SPEED * PM_DP_WIDTH / (CLOCK_FREQ / 72), sampleRate);
	am_dphase = rate_adjust(AM_SPEED * AM_DP_WIDTH / (CLOCK_FREQ / 72), sampleRate);
}


// Drum key on
void OpenYM2413_2::keyOn_BD()  { ch[6].keyOn(); }
void OpenYM2413_2::keyOn_HH()  { if (!ch[7].mod.slot_on_flag) ch[7].mod.slotOn2(); }
void OpenYM2413_2::keyOn_SD()  { if (!ch[7].car.slot_on_flag) ch[7].car.slotOn (); }
void OpenYM2413_2::keyOn_TOM() { if (!ch[8].mod.slot_on_flag) ch[8].mod.slotOn (); }
void OpenYM2413_2::keyOn_CYM() { if (!ch[8].car.slot_on_flag) ch[8].car.slotOn2(); }

// Drum key off
void OpenYM2413_2::keyOff_BD() { ch[6].keyOff(); }
void OpenYM2413_2::keyOff_HH() { if (ch[7].mod.slot_on_flag) ch[7].mod.slotOff(); }
void OpenYM2413_2::keyOff_SD() { if (ch[7].car.slot_on_flag) ch[7].car.slotOff(); }
void OpenYM2413_2::keyOff_TOM(){ if (ch[8].mod.slot_on_flag) ch[8].mod.slotOff(); }
void OpenYM2413_2::keyOff_CYM(){ if (ch[8].car.slot_on_flag) ch[8].car.slotOff(); }

void OpenYM2413_2::update_rhythm_mode()
{
	if (ch[6].patch_number & 0x10) {
		if (!(ch[6].car.slot_on_flag ||
		      (reg[0x0e] & 0x20))) {
			ch[6].mod.eg_mode = FINISH;
			ch[6].car.eg_mode = FINISH;
			ch[6].setPatch(reg[0x36] >> 4);
		}
	} else if (reg[0x0e] & 0x20) {
		ch[6].mod.eg_mode = FINISH;
		ch[6].car.eg_mode = FINISH;
		ch[6].setPatch(16);
	}

	if (ch[7].patch_number & 0x10) {
		if (!((ch[7].mod.slot_on_flag && ch[7].car.slot_on_flag) ||
		      (reg[0x0e] & 0x20))) {
			ch[7].mod.type = false;
			ch[7].mod.eg_mode = FINISH;
			ch[7].car.eg_mode = FINISH;
			ch[7].setPatch(reg[0x37] >> 4);
		}
	} else if (reg[0x0e] & 0x20) {
		ch[7].mod.type = true;
		ch[7].mod.eg_mode = FINISH;
		ch[7].car.eg_mode = FINISH;
		ch[7].setPatch(17);
	}

	if (ch[8].patch_number & 0x10) {
		if (!((ch[8].mod.slot_on_flag && ch[8].car.slot_on_flag) ||
		      (reg[0x0e] & 0x20))) {
			ch[8].mod.type = false;
			ch[8].mod.eg_mode = FINISH;
			ch[8].car.eg_mode = FINISH;
			ch[8].setPatch(reg[0x38] >> 4);
		}
	} else if (reg[0x0e] & 0x20) {
		ch[8].mod.type = true;
		ch[8].mod.eg_mode = FINISH;
		ch[8].car.eg_mode = FINISH;
		ch[8].setPatch(18);
	}
}

void OpenYM2413_2::update_key_status()
{
	for (int i = 0; i < 9; ++i) {
		ch[i].mod.slot_on_flag = ch[i].car.slot_on_flag =
			(reg[0x20 + i] & 0x10) != 0;
	}
	if (reg[0x0e] & 0x20) {
		ch[6].mod.slot_on_flag |= 0 != (reg[0x0e] & 0x10); // BD1
		ch[6].car.slot_on_flag |= 0 != (reg[0x0e] & 0x10); // BD2
		ch[7].mod.slot_on_flag |= 0 != (reg[0x0e] & 0x01); // HH
		ch[7].car.slot_on_flag |= 0 != (reg[0x0e] & 0x08); // SD
		ch[8].mod.slot_on_flag |= 0 != (reg[0x0e] & 0x04); // TOM
		ch[8].car.slot_on_flag |= 0 != (reg[0x0e] & 0x02); // SYM
	}
}


//******************************************************//
//                                                      //
//                 Generate wave data                   //
//                                                      //
//******************************************************//

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 4PI)
int OpenYM2413_2::Slot::wave2_4pi(int e) 
{
	int shift =  SLOT_AMP_BITS - PG_BITS - 1;
	if (shift > 0) {
		return e >> shift;
	} else {
		return e << -shift;
	}
}

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 8PI)
int OpenYM2413_2::Slot::wave2_8pi(int e) 
{
	int shift = SLOT_AMP_BITS - PG_BITS - 2;
	if (shift > 0) {
		return e >> shift;
	} else {
		return e << -shift;
	}
}

// Update AM, PM unit
inline void OpenYM2413_2::update_ampm()
{
	pm_phase = (pm_phase + pm_dphase) & (PM_DP_WIDTH - 1);
	am_phase = (am_phase + am_dphase) & (AM_DP_WIDTH - 1);
	lfo_am = amtable[HIGHBITS(am_phase, AM_DP_BITS - AM_PG_BITS)];
	lfo_pm = pmtable[HIGHBITS(pm_phase, PM_DP_BITS - PM_PG_BITS)];
}

// PG
void OpenYM2413_2::Slot::calc_phase(int lfo_pm)
{
	if (patches[patchIdx].PM) {
		phase += (dphase * lfo_pm) >> PM_AMP_BITS;
	} else {
		phase += dphase;
	}
	phase &= (DP_WIDTH - 1);
	pgout = HIGHBITS(phase, DP_BASE_BITS);
}

// Update Noise unit
inline void OpenYM2413_2::update_noise()
{
   if (noise_seed & 1) noise_seed ^= 0x8003020;
   noise_seed >>= 1;
}

// EG
void OpenYM2413_2::Slot::calc_envelope(int lfo_am)
{
	#define S2E(x) (unsigned int)(SL2EG((int)(x / SL_STEP)) << (EG_DP_BITS - EG_BITS))
	/*constexpr	static*/ unsigned int SL[16] = {
		S2E( 0.0), S2E( 3.0), S2E( 6.0), S2E( 9.0),
		S2E(12.0), S2E(15.0), S2E(18.0), S2E(21.0),
		S2E(24.0), S2E(27.0), S2E(30.0), S2E(33.0),
		S2E(36.0), S2E(39.0), S2E(42.0), S2E(48.0)
	};

	unsigned out;
	switch (eg_mode) {
	case ATTACK:
		out = AR_ADJUST_TABLE[HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS)];
		eg_phase += eg_dphase;
		if ((EG_DP_WIDTH & eg_phase) || (patches[patchIdx].AR == 15)) {
			out = 0;
			eg_phase = 0;
			eg_mode = DECAY;
			updateEG();
		}
		break;
	case DECAY:
		out = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		eg_phase += eg_dphase;
		if (eg_phase >= SL[patches[patchIdx].SL]) {
			eg_phase = SL[patches[patchIdx].SL];
			if (patches[patchIdx].EG) {
				eg_mode = SUSHOLD;
			} else {
				eg_mode = SUSTINE;
			}
			updateEG();
		}
		break;
	case SUSHOLD:
		out = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		if (patches[patchIdx].EG == 0) {
			eg_mode = SUSTINE;
			updateEG();
		}
		break;
	case SUSTINE:
	case RELEASE:
		out = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		eg_phase += eg_dphase;
		if (out >= (1 << EG_BITS)) {
			eg_mode = FINISH;
			out = (1 << EG_BITS) - 1;
		}
		break;
	case SETTLE:
		out = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		eg_phase += eg_dphase;
		if (out >= (1 << EG_BITS)) {
			eg_mode = ATTACK;
			out = (1 << EG_BITS) - 1;
			updateEG();
		}
		break;
	case FINISH:
	default:
		out = (1 << EG_BITS) - 1;
		break;
	}
	if (patches[patchIdx].AM) {
		out = EG2DB(out + tll) + lfo_am;
	} else {
		out = EG2DB(out + tll);
	}
	if (out >= (unsigned)DB_MUTE) {
		out = DB_MUTE - 1;
	}
	
	egout = out | 3;
}

// CARRIOR
int OpenYM2413_2::Slot::calc_slot_car(int fm)
{
	if (egout >= (DB_MUTE - 1)) {
		output[0] = 0;
	} else {
		output[0] = dB2LinTab[sintbl[(pgout + wave2_8pi(fm)) & (PG_WIDTH - 1)]
		                       + egout];
	}
	output[1] = (output[1] + output[0]) >> 1;
	return output[1];
}

// MODULATOR
int OpenYM2413_2::Slot::calc_slot_mod()
{
	output[1] = output[0];

	if (egout >= (DB_MUTE - 1)) {
		output[0] = 0;
	} else if (patches[patchIdx].FB != 0) {
		int fm = wave2_4pi(feedback) >> (7 - patches[patchIdx].FB);
		output[0] = dB2LinTab[sintbl[(pgout + fm) & (PG_WIDTH - 1)] + egout];
	} else {
		output[0] = dB2LinTab[sintbl[pgout] + egout];
	}
	feedback = (output[1] + output[0]) >> 1;
	return feedback;
}

// TOM
int OpenYM2413_2::Slot::calc_slot_tom()
{
	return (egout >= (DB_MUTE - 1))
	     ? 0
	     : dB2LinTab[sintbl[pgout] + egout];
}

// SNARE
int OpenYM2413_2::Slot::calc_slot_snare(bool noise)
{
	if (egout >= (DB_MUTE - 1)) {
		return 0;
	} 
	if (BIT(pgout, 7)) {
		return dB2LinTab[(noise ? DB_POS(0.0) : DB_POS(15.0)) + egout];
	} else {
		return dB2LinTab[(noise ? DB_NEG(0.0) : DB_NEG(15.0)) + egout];
	}
}

// TOP-CYM
int OpenYM2413_2::Slot::calc_slot_cym(unsigned int pgout_hh)
{
	if (egout >= (DB_MUTE - 1)) {
		return 0;
	}
	unsigned int dbout
	    = (((BIT(pgout_hh, PG_BITS - 8) ^ BIT(pgout_hh, PG_BITS - 1)) |
	        BIT(pgout_hh, PG_BITS - 7)) ^
	       (BIT(pgout, PG_BITS - 7) & !BIT(pgout, PG_BITS - 5)))
	    ? DB_NEG(3.0)
	    : DB_POS(3.0);
	return dB2LinTab[dbout + egout];
}

// HI-HAT
int OpenYM2413_2::Slot::calc_slot_hat(int pgout_cym, bool noise)
{
	if (egout >= (DB_MUTE - 1)) {
		return 0;
	}
	unsigned int dbout;
	if (((BIT(pgout, PG_BITS - 8) ^ BIT(pgout, PG_BITS - 1)) |
	     BIT(pgout, PG_BITS - 7)) ^
	    (BIT(pgout_cym, PG_BITS - 7) & !BIT(pgout_cym, PG_BITS - 5))) {
		dbout = noise ? DB_NEG(12.0) : DB_NEG(24.0);
	} else {
		dbout = noise ? DB_POS(12.0) : DB_POS(24.0);
	}
	return dB2LinTab[dbout + egout];
}


int OpenYM2413_2::filter(int input) {
    in[4] = in[3];
    in[3] = in[2];
    in[2] = in[1];
    in[1] = in[0];
    in[0] = input;

    return (0 * (in[0] + in[4]) + 1 * (in[3] + in[1]) + 2 * in[2]) / 4;
}

inline int OpenYM2413_2::calcSample()
{
	// while muted updated_ampm() and update_noise() aren't called, probably ok
	update_ampm();
	update_noise();

	for (int i = 0; i < 9; ++i) {
		ch[i].mod.calc_phase(lfo_pm);
		ch[i].mod.calc_envelope(lfo_am);
		ch[i].car.calc_phase(lfo_pm);
		ch[i].car.calc_envelope(lfo_am);
	}

	int channelMask = 0;
	for (int i = 0; i < 9; ++i) {
		if (ch[i].car.eg_mode != FINISH) {
			channelMask |= (1 << i);
		}
	}

	int mix = 0;
	if (ch[6].patch_number & 0x10) {
		if (channelMask & (1 << 6)) {
			mix += ch[6].car.calc_slot_car(ch[6].mod.calc_slot_mod());
			channelMask &= ~(1 << 6);
		}
	}
	if (ch[7].patch_number & 0x10) {
		if (ch[7].mod.eg_mode != FINISH) {
			mix += ch[7].mod.calc_slot_hat(ch[8].car.pgout, noise_seed & 1);
		}
		if (channelMask & (1 << 7)) {
			mix -= ch[7].car.calc_slot_snare(noise_seed & 1);
			channelMask &= ~(1 << 7);
		}
	}
	if (ch[8].patch_number & 0x10) {
		if (ch[8].mod.eg_mode != FINISH) {
			mix += ch[8].mod.calc_slot_tom();
		}
		if (channelMask & (1 << 8)) {
			mix -= ch[8].car.calc_slot_cym(ch[7].mod.pgout);
			channelMask &= ~(1 << 8);
		}
	}
	mix *= 2;

	for (Channel* cp = ch; channelMask; channelMask >>= 1, ++cp) {
		if (channelMask & 1) {
			mix += cp->car.calc_slot_car(cp->mod.calc_slot_mod());
		}
	}
	return filter((maxVolume * mix) >> (DB2LIN_AMP_BITS - 1)); 
}

void OpenYM2413_2::checkMute()
{
	setInternalMute(checkMuteHelper());
}
bool OpenYM2413_2::checkMuteHelper()
{
	for (int i = 0; i < 6; i++) {
		if (ch[i].car.eg_mode != FINISH) return false;
	}
	if (!(reg[0x0e] & 0x20)) {
		for(int i = 6; i < 9; i++) {
			 if (ch[i].car.eg_mode != FINISH) return false;
		}
	} else {
		if (ch[6].car.eg_mode != FINISH) return false;
		if (ch[7].mod.eg_mode != FINISH) return false;
		if (ch[7].car.eg_mode != FINISH) return false;
		if (ch[8].mod.eg_mode != FINISH) return false;
		if (ch[8].car.eg_mode != FINISH) return false;
	}
	return true;	// nothing is playing, then mute
}

int* OpenYM2413_2::updateBuffer(int length)
{
    int* buf = buffer;

	while (length--) {
		*(buf++) = calcSample();
	}
	checkMute();

    return buffer;
}

void OpenYM2413_2::setInternalVolume(short newVolume)
{
	maxVolume = newVolume;
}


//**************************************************//
//                                                  //
//                       I/O Ctrl                   //
//                                                  //
//**************************************************//

void OpenYM2413_2::writeReg(byte regis, byte data, const EmuTime &time)
{

	assert (regis < 0x40);
	reg[regis] = data;

	switch (regis) {
	case 0x00:
		patches[0].AM = (data >> 7) & 1;
		patches[0].PM = (data >> 6) & 1;
		patches[0].EG = (data >> 5) & 1;
		patches[0].KR = (data >> 4) & 1;
		patches[0].ML = (data >> 0) & 15;
		for (int i = 0; i < 9; ++i) {
			if (ch[i].patch_number == 0) {
				ch[i].mod.updatePG();
				ch[i].mod.updateRKS();
				ch[i].mod.updateEG();
			}
		}
		break;
	case 0x01:
		patches[1].AM = (data >> 7) & 1;
		patches[1].PM = (data >> 6) & 1;
		patches[1].EG = (data >> 5) & 1;
		patches[1].KR = (data >> 4) & 1;
		patches[1].ML = (data >> 0) & 15;
		for (int i = 0; i < 9; ++i) {
			if(ch[i].patch_number == 0) {
				ch[i].car.updatePG();
				ch[i].car.updateRKS();
				ch[i].car.updateEG();
			}
		}
		break;
	case 0x02:
		patches[0].KL = (data >> 6) & 3;
		patches[0].TL = (data >> 0) & 63;
		for (int i = 0; i < 9; ++i) {
			if (ch[i].patch_number == 0) {
				ch[i].mod.updateTLL();
			}
		}
		break;
	case 0x03:
		patches[1].KL = (data >> 6) & 3;
		patches[1].WF = (data >> 4) & 1;
		patches[0].WF = (data >> 3) & 1;
		patches[0].FB = (data >> 0) & 7;
		for (int i = 0; i < 9; ++i) {
			if (ch[i].patch_number == 0) {
				ch[i].mod.updateWF();
				ch[i].car.updateWF();
			}
		}
		break;
	case 0x04:
		patches[0].AR = (data >> 4) & 15;
		patches[0].DR = (data >> 0) & 15;
		for (int i = 0; i < 9; ++i) {
			if(ch[i].patch_number == 0) {
				ch[i].mod.updateEG();
			}
		}
		break;
	case 0x05:
		patches[1].AR = (data >> 4) & 15;
		patches[1].DR = (data >> 0) & 15;
		for (int i = 0; i < 9; ++i) {
			if (ch[i].patch_number == 0) {
				ch[i].car.updateEG();
			}
		}
		break;
	case 0x06:
		patches[0].SL = (data >> 4) & 15;
		patches[0].RR = (data >> 0) & 15;
		for (int i = 0; i < 9; ++i) {
			if (ch[i].patch_number == 0) {
				ch[i].mod.updateEG();
			}
		}
		break;
	case 0x07:
		patches[1].SL = (data >> 4) & 15;
		patches[1].RR = (data >> 0) & 15;
		for (int i = 0; i < 9; i++) {
			if (ch[i].patch_number == 0) {
				ch[i].car.updateEG();
			}
		}
		break;
	case 0x0e:
		update_rhythm_mode();
		if (data & 0x20) {
			if (data & 0x10) keyOn_BD();  else keyOff_BD();
			if (data & 0x08) keyOn_SD();  else keyOff_SD();
			if (data & 0x04) keyOn_TOM(); else keyOff_TOM();
			if (data & 0x02) keyOn_CYM(); else keyOff_CYM();
			if (data & 0x01) keyOn_HH();  else keyOff_HH();
		}
		update_key_status();

		ch[6].mod.updateAll();
		ch[6].car.updateAll();
		ch[7].mod.updateAll();
		ch[7].car.updateAll();
		ch[8].mod.updateAll();
		ch[8].car.updateAll();        
		break;

	case 0x10:  case 0x11:  case 0x12:  case 0x13:
	case 0x14:  case 0x15:  case 0x16:  case 0x17:
	case 0x18:
	{
		int cha = regis & 0x0F;
		ch[cha].setFnumber(data + ((reg[0x20 + cha] & 1) << 8));
		ch[cha].mod.updateAll();
		ch[cha].car.updateAll();
		break;
	}
	case 0x20:  case 0x21:  case 0x22:  case 0x23:
	case 0x24:  case 0x25:  case 0x26:  case 0x27:
	case 0x28:
	{
		int cha = regis & 0x0F;
		int fNum = ((data & 1) << 8) + reg[0x10 + cha];
		int block = (data >> 1) & 7;
		ch[cha].setFnumber(fNum);
		ch[cha].setBlock(block);
		ch[cha].setSustine((data >> 5) & 1);
		if (data & 0x10) {
			ch[cha].keyOn();
		} else {
			ch[cha].keyOff();
		}
		ch[cha].mod.updateAll();
		ch[cha].car.updateAll();
		update_key_status();
		update_rhythm_mode();
		break;
	}
	case 0x30: case 0x31: case 0x32: case 0x33: case 0x34:
	case 0x35: case 0x36: case 0x37: case 0x38: 
	{
		int cha = regis & 0x0F;
		int j = (data >> 4) & 15;
		int v = data & 15;
		if ((reg[0x0e] & 0x20) && (regis >= 0x36)) {
			switch(regis) {
			case 0x37:
				ch[7].mod.setVolume(j << 2);
				break;
			case 0x38:
				ch[8].mod.setVolume(j << 2);
				break;
			}
		} else { 
			ch[cha].setPatch(j);
		}
		ch[cha].setVol(v << 2);
		ch[cha].mod.updateAll();
		ch[cha].car.updateAll();
		break;
	}
	default:
		break;
	}
	checkMute();
}


// Debuggable

unsigned OpenYM2413_2::getSize() const
{
	return 0x40;
}

byte OpenYM2413_2::read(unsigned address)
{
	return reg[address];
}

void OpenYM2413_2::write(unsigned address, byte value)
{
	writeReg(address, value, 0 /*Scheduler::instance().getCurrentTime() */);
}

void OpenYM2413_2::loadState()
{
    SaveState* state = saveStateOpenForRead("ym2413_2");
    char tag[32];
    int i;

    for (i = 0; i < sizeof(reg) / sizeof(reg[0]); i++) {
        sprintf(tag, "reg%.4d", i);
        reg[i] = (byte)saveStateGet(state, tag, 0);
    }

    maxVolume  = saveStateGet(state, "maxVolume",     0);
    pm_phase   = saveStateGet(state, "pm_phase",      0);
    lfo_pm     = saveStateGet(state, "lfo_pm",        0);
    am_phase   = saveStateGet(state, "am_phase",      0);
    lfo_am     = saveStateGet(state, "lfo_am",        0);
    noise_seed = saveStateGet(state, "noise_seed",    0);

    for (i = 0; i < sizeof(patches) / sizeof(patches[0]); i++) {
        sprintf(tag, "AM%d", i);
        patches[i].AM = 0 != saveStateGet(state, tag, 0);
        
        sprintf(tag, "PM%d", i);
        patches[i].PM = 0 != saveStateGet(state, tag, 0);
        
        sprintf(tag, "EG%d", i);
        patches[i].EG = 0 != saveStateGet(state, tag, 0);

        sprintf(tag, "KR%d", i);
        patches[i].KR = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "ML%d", i);
        patches[i].ML = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "KL%d", i);
        patches[i].KL = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "TL%d", i);
        patches[i].TL = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "FB%d", i);
        patches[i].FB = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "WF%d", i);
        patches[i].WF = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "AR%d", i);
        patches[i].AR = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "DR%d", i);
        patches[i].DR = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "SL%d", i);
        patches[i].SL = (byte)saveStateGet(state, tag, 0);
        
        sprintf(tag, "RR%d", i);
        patches[i].RR = (byte)saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(ch) / sizeof(ch[0]); i++) {
        sprintf(tag, "patch_number%d", i);
        ch[i].patch_number = saveStateGet(state, tag, 0);

        ch[i].setPatch(ch[i].patch_number);

        sprintf(tag, "mod.output0%d", i);
        ch[i].mod.output[0] = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.output1%d", i);
        ch[i].mod.output[1] = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.output2%d", i);
        ch[i].mod.output[2] = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.output3%d", i);
        ch[i].mod.output[3] = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.output4%d", i);
        ch[i].mod.output[4] = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.sintblIdx%d", i);
        ch[i].mod.sintblIdx = saveStateGet(state, tag, 0);
	    ch[i].mod.sintbl = waveform[ch[i].mod.sintblIdx];
        
        sprintf(tag, "mod.type%d", i);
        ch[i].mod.type = 0 != saveStateGet(state, tag, 0);

        sprintf(tag, "mod.slot_on_flag%d", i);
        ch[i].mod.slot_on_flag = 0 != saveStateGet(state, tag, 0);

        sprintf(tag, "mod.phase%d", i);
        ch[i].mod.phase = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.dphase%d", i);
        ch[i].mod.dphase = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.pgout%d", i);
        ch[i].mod.pgout = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.fnum%d", i);
        ch[i].mod.fnum = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.block%d", i);
        ch[i].mod.block = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.volume%d", i);
        ch[i].mod.volume = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.sustine%d", i);
        ch[i].mod.sustine = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.tll%d", i);
        ch[i].mod.tll = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.rks%d", i);
        ch[i].mod.rks = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.eg_mode%d", i);
        ch[i].mod.eg_mode = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.eg_phase%d", i);
        ch[i].mod.eg_phase = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.eg_dphase%d", i);
        ch[i].mod.eg_dphase = saveStateGet(state, tag, 0);

        sprintf(tag, "mod.egout%d", i);
        ch[i].mod.egout = saveStateGet(state, tag, 0);


        sprintf(tag, "car.output0%d", i);
        ch[i].car.output[0] = saveStateGet(state, tag, 0);

        sprintf(tag, "car.output1%d", i);
        ch[i].car.output[1] = saveStateGet(state, tag, 0);

        sprintf(tag, "car.output2%d", i);
        ch[i].car.output[2] = saveStateGet(state, tag, 0);

        sprintf(tag, "car.output3%d", i);
        ch[i].car.output[3] = saveStateGet(state, tag, 0);

        sprintf(tag, "car.output4%d", i);
        ch[i].car.output[4] = saveStateGet(state, tag, 0);

        sprintf(tag, "car.sintblIdx%d", i);
        ch[i].car.sintblIdx = saveStateGet(state, tag, 0);
	    ch[i].car.sintbl = waveform[ch[i].car.sintblIdx];
        
        sprintf(tag, "car.type%d", i);
        ch[i].car.type = 0 != saveStateGet(state, tag, 0);

        sprintf(tag, "car.slot_on_flag%d", i);
        ch[i].car.slot_on_flag = 0 != saveStateGet(state, tag, 0);

        sprintf(tag, "car.phase%d", i);
        ch[i].car.phase = saveStateGet(state, tag, 0);

        sprintf(tag, "car.dphase%d", i);
        ch[i].car.dphase = saveStateGet(state, tag, 0);

        sprintf(tag, "car.pgout%d", i);
        ch[i].car.pgout = saveStateGet(state, tag, 0);

        sprintf(tag, "car.fnum%d", i);
        ch[i].car.fnum = saveStateGet(state, tag, 0);

        sprintf(tag, "car.block%d", i);
        ch[i].car.block = saveStateGet(state, tag, 0);

        sprintf(tag, "car.volume%d", i);
        ch[i].car.volume = saveStateGet(state, tag, 0);

        sprintf(tag, "car.sustine%d", i);
        ch[i].car.sustine = saveStateGet(state, tag, 0);

        sprintf(tag, "car.tll%d", i);
        ch[i].car.tll = saveStateGet(state, tag, 0);

        sprintf(tag, "car.rks%d", i);
        ch[i].car.rks = saveStateGet(state, tag, 0);

        sprintf(tag, "car.eg_mode%d", i);
        ch[i].car.eg_mode = saveStateGet(state, tag, 0);

        sprintf(tag, "car.eg_phase%d", i);
        ch[i].car.eg_phase = saveStateGet(state, tag, 0);

        sprintf(tag, "car.eg_dphase%d", i);
        ch[i].car.eg_dphase = saveStateGet(state, tag, 0);

        sprintf(tag, "car.egout%d", i);
        ch[i].car.egout = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
}


void OpenYM2413_2::saveState()
{
    SaveState* state = saveStateOpenForWrite("ym2413_2");
    char tag[32];
    int i;

    for (i = 0; i < sizeof(reg) / sizeof(reg[0]); i++) {
        sprintf(tag, "reg%.4d", i);
        saveStateSet(state, tag, reg[i]);
    }

    saveStateSet(state, "maxVolume",     maxVolume);
    saveStateSet(state, "pm_phase",      pm_phase);
    saveStateSet(state, "lfo_pm",        lfo_pm);
    saveStateSet(state, "am_phase",      am_phase);
    saveStateSet(state, "lfo_am",        lfo_am);
    saveStateSet(state, "noise_seed",    noise_seed);

    for (i = 0; i < sizeof(patches) / sizeof(patches[0]); i++) {
        sprintf(tag, "AM%d", i);
        saveStateSet(state, tag, patches[i].AM);
        
        sprintf(tag, "PM%d", i);
        saveStateSet(state, tag, patches[i].PM);
        
        sprintf(tag, "EG%d", i);
        saveStateSet(state, tag, patches[i].EG);

        sprintf(tag, "KR%d", i);
        saveStateSet(state, tag, patches[i].KR);
        
        sprintf(tag, "ML%d", i);
        saveStateSet(state, tag, patches[i].ML);
        
        sprintf(tag, "KL%d", i);
        saveStateSet(state, tag, patches[i].KL);
        
        sprintf(tag, "TL%d", i);
        saveStateSet(state, tag, patches[i].TL);
        
        sprintf(tag, "FB%d", i);
        saveStateSet(state, tag, patches[i].FB);
        
        sprintf(tag, "WF%d", i);
        saveStateSet(state, tag, patches[i].WF);
        
        sprintf(tag, "AR%d", i);
        saveStateSet(state, tag, patches[i].AR);
        
        sprintf(tag, "DR%d", i);
        saveStateSet(state, tag, patches[i].DR);
        
        sprintf(tag, "SL%d", i);
        saveStateSet(state, tag, patches[i].SL);
        
        sprintf(tag, "RR%d", i);
        saveStateSet(state, tag, patches[i].RR);
    }

    for (i = 0; i < sizeof(ch) / sizeof(ch[0]); i++) {
        sprintf(tag, "patch_number%d", i);
        saveStateSet(state, tag, ch[i].patch_number);

        sprintf(tag, "mod.output0%d", i);
        saveStateSet(state, tag, ch[i].mod.output[0]);

        sprintf(tag, "mod.output1%d", i);
        saveStateSet(state, tag, ch[i].mod.output[1]);

        sprintf(tag, "mod.output2%d", i);
        saveStateSet(state, tag, ch[i].mod.output[2]);

        sprintf(tag, "mod.output3%d", i);
        saveStateSet(state, tag, ch[i].mod.output[3]);

        sprintf(tag, "mod.output4%d", i);
        saveStateSet(state, tag, ch[i].mod.output[4]);

        sprintf(tag, "mod.sintblIdx%d", i);
        saveStateSet(state, tag, ch[i].mod.sintblIdx);
        
        sprintf(tag, "mod.type%d", i);
        saveStateSet(state, tag, ch[i].mod.type);

        sprintf(tag, "mod.slot_on_flag%d", i);
        saveStateSet(state, tag, ch[i].mod.slot_on_flag);

        sprintf(tag, "mod.phase%d", i);
        saveStateSet(state, tag, ch[i].mod.phase);

        sprintf(tag, "mod.dphase%d", i);
        saveStateSet(state, tag, ch[i].mod.dphase);

        sprintf(tag, "mod.pgout%d", i);
        saveStateSet(state, tag, ch[i].mod.pgout);

        sprintf(tag, "mod.fnum%d", i);
        saveStateSet(state, tag, ch[i].mod.fnum);

        sprintf(tag, "mod.block%d", i);
        saveStateSet(state, tag, ch[i].mod.block);

        sprintf(tag, "mod.volume%d", i);
        saveStateSet(state, tag, ch[i].mod.volume);

        sprintf(tag, "mod.sustine%d", i);
        saveStateSet(state, tag, ch[i].mod.sustine);

        sprintf(tag, "mod.tll%d", i);
        saveStateSet(state, tag, ch[i].mod.tll);

        sprintf(tag, "mod.rks%d", i);
        saveStateSet(state, tag, ch[i].mod.rks);

        sprintf(tag, "mod.eg_mode%d", i);
        saveStateSet(state, tag, ch[i].mod.eg_mode);

        sprintf(tag, "mod.eg_phase%d", i);
        saveStateSet(state, tag, ch[i].mod.eg_phase);

        sprintf(tag, "mod.eg_dphase%d", i);
        saveStateSet(state, tag, ch[i].mod.eg_dphase);

        sprintf(tag, "mod.egout%d", i);
        saveStateSet(state, tag, ch[i].mod.egout);


        sprintf(tag, "car.output0%d", i);
        saveStateSet(state, tag, ch[i].car.output[0]);

        sprintf(tag, "car.output1%d", i);
        saveStateSet(state, tag, ch[i].car.output[1]);

        sprintf(tag, "car.output2%d", i);
        saveStateSet(state, tag, ch[i].car.output[2]);

        sprintf(tag, "car.output3%d", i);
        saveStateSet(state, tag, ch[i].car.output[3]);

        sprintf(tag, "car.output4%d", i);
        saveStateSet(state, tag, ch[i].car.output[4]);

        sprintf(tag, "car.sintblIdx%d", i);
        saveStateSet(state, tag, ch[i].car.sintblIdx);
        
        sprintf(tag, "car.type%d", i);
        saveStateSet(state, tag, ch[i].car.type);

        sprintf(tag, "car.slot_on_flag%d", i);
        saveStateSet(state, tag, ch[i].car.slot_on_flag);

        sprintf(tag, "car.phase%d", i);
        saveStateSet(state, tag, ch[i].car.phase);

        sprintf(tag, "car.dphase%d", i);
        saveStateSet(state, tag, ch[i].car.dphase);

        sprintf(tag, "car.pgout%d", i);
        saveStateSet(state, tag, ch[i].car.pgout);

        sprintf(tag, "car.fnum%d", i);
        saveStateSet(state, tag, ch[i].car.fnum);

        sprintf(tag, "car.block%d", i);
        saveStateSet(state, tag, ch[i].car.block);

        sprintf(tag, "car.volume%d", i);
        saveStateSet(state, tag, ch[i].car.volume);

        sprintf(tag, "car.sustine%d", i);
        saveStateSet(state, tag, ch[i].car.sustine);

        sprintf(tag, "car.tll%d", i);
        saveStateSet(state, tag, ch[i].car.tll);

        sprintf(tag, "car.rks%d", i);
        saveStateSet(state, tag, ch[i].car.rks);

        sprintf(tag, "car.eg_mode%d", i);
        saveStateSet(state, tag, ch[i].car.eg_mode);

        sprintf(tag, "car.eg_phase%d", i);
        saveStateSet(state, tag, ch[i].car.eg_phase);

        sprintf(tag, "car.eg_dphase%d", i);
        saveStateSet(state, tag, ch[i].car.eg_dphase);

        sprintf(tag, "car.egout%d", i);
        saveStateSet(state, tag, ch[i].car.egout);
    }

    saveStateClose(state);
}
