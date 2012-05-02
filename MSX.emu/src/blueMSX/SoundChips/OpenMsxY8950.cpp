// $Id: OpenMsxY8950.cpp,v 1.6 2008-03-31 22:07:05 hap-hap Exp $

/*
  * Based on:
  *    emu8950.c -- Y8950 emulator written by Mitsutaka Okazaki 2001
  * heavily rewritten to fit openMSX structure
  */

#include <cmath>
#include "OpenMsxY8950.h"

extern "C" {
#include "SaveState.h"
}

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

short Y8950::dB2LinTab[(2*DB_MUTE)*2];
int   Y8950::Slot::sintable[PG_WIDTH];
int   Y8950::Slot::tllTable[16][8][1<<TL_BITS][4];
int   Y8950::Slot::rksTable[2][8][2];
int   Y8950::Slot::AR_ADJUST_TABLE[1<<EG_BITS];
unsigned int Y8950::dphaseNoiseTable[1024][8];
unsigned int Y8950::Slot::dphaseARTable[16][16];
unsigned int Y8950::Slot::dphaseDRTable[16][16];
unsigned int Y8950::Slot::dphaseTable[1024][8][16];


extern "C" UInt32 boardSystemTime();
extern "C" int switchGetAudio();

//**************************************************//
//                                                  //
//  Helper functions                                //
//                                                  //
//**************************************************//

int Y8950::Slot::ALIGN(int d, DoubleT SS, DoubleT SD)
{ 
	return d*(int)(SS/SD);
}

int Y8950::DB_POS(int x)
{
	return (int)(x/DB_STEP);
}
int Y8950::DB_NEG(int x)
{
	return (int)(2*DB_MUTE+x/DB_STEP);
}

// Cut the lower b bits off
int Y8950::HIGHBITS(int c, int b)
{
	return c >> b;
}
// Leave the lower b bits
int Y8950::LOWBITS(int c, int b)
{
	return c & ((1<<b)-1);
}
// Expand x which is s bits to d bits
int Y8950::EXPAND_BITS(int x, int s, int d)
{
	return x << (d-s);
}
// Adjust envelope speed which depends on sampling rate
unsigned int Y8950::rate_adjust(DoubleT x, int rate)
{
	DoubleT tmp = x * CLK_FREQ / 72 / rate + 0.5; // +0.5 to round
//	assert (tmp <= 4294967295U);
	return (unsigned int)tmp;
}

//**************************************************//
//                                                  //
//                  Create tables                   //
//                                                  //
//**************************************************//

// Table for AR to LogCurve. 
void Y8950::Slot::makeAdjustTable()
{
	AR_ADJUST_TABLE[0] = 1 << EG_BITS;
	for (int i = 1; i < (1 << EG_BITS); i++)
		AR_ADJUST_TABLE[i] = (int)((DoubleT)(1 << EG_BITS) - 1 -
		         (1 << EG_BITS) * ::log((DoubleT)i) / ::log((DoubleT)(1 << EG_BITS))) >> 1;
}

// Table for dB(0 -- (1<<DB_BITS)) to Liner(0 -- DB2LIN_AMP_WIDTH) 
void Y8950::Slot::makeDB2LinTable()
{
	for (int i=0; i < 2*DB_MUTE; i++) {
		dB2LinTab[i] = (i<DB_MUTE) ?
			(int)((DoubleT)((1<<DB2LIN_AMP_BITS)-1)*pow((DoubleT)10,-(DoubleT)i*DB_STEP/20)) :
			0;
		dB2LinTab[i + 2*DB_MUTE] = -dB2LinTab[i];
	}
}

// Liner(+0.0 - +1.0) to dB((1<<DB_BITS) - 1 -- 0) 
int Y8950::Slot::lin2db(DoubleT d)
{
	if (d < 1e-4) {
		// (almost) zero
		return DB_MUTE-1;
	}
	int tmp = -(int)(20.0*log10(d)/DB_STEP);
	if (tmp < DB_MUTE-1)
		return tmp;
	else
		return DB_MUTE-1;
}

// Sin Table 
void Y8950::Slot::makeSinTable()
{
	for (int i=0; i < PG_WIDTH/4; i++)
		sintable[i] = lin2db(sin(2.0*PI*i/PG_WIDTH));
	for (int i=0; i < PG_WIDTH/4; i++)
		sintable[PG_WIDTH/2 - 1 - i] = sintable[i];
	for (int i=0; i < PG_WIDTH/2; i++)
		sintable[PG_WIDTH/2 + i] = 2*DB_MUTE + sintable[i];
}

void Y8950::makeDphaseNoiseTable(int sampleRate)
{
	for (int i=0; i<1024; i++)
		for (int j=0; j<8; j++)
			dphaseNoiseTable[i][j] = rate_adjust(i<<j, sampleRate);
}

// Table for Pitch Modulator 
void Y8950::makePmTable()
{
	for (int i=0; i<PM_PG_WIDTH; i++)
		pmtable[0][i] = (int)((DoubleT)PM_AMP * pow(2.,(DoubleT)PM_DEPTH*sin(2.0*PI*i/PM_PG_WIDTH)/1200));
	for (int i=0; i < PM_PG_WIDTH; i++)
		pmtable[1][i] = (int)((DoubleT)PM_AMP * pow(2.,(DoubleT)PM_DEPTH2*sin(2.0*PI*i/PM_PG_WIDTH)/1200));
}

// Table for Amp Modulator 
void Y8950::makeAmTable()
{
	for (int i=0; i<AM_PG_WIDTH; i++)
		amtable[0][i] = (int)((DoubleT)AM_DEPTH/2/DB_STEP * (1.0 + sin(2.0*PI*i/PM_PG_WIDTH)));
	for (int i=0; i<AM_PG_WIDTH; i++)
		amtable[1][i] = (int)((DoubleT)AM_DEPTH2/2/DB_STEP * (1.0 + sin(2.0*PI*i/PM_PG_WIDTH)));
}

// Phase increment counter table  
void Y8950::Slot::makeDphaseTable(int sampleRate)
{
	int mltable[16] = {
		1,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,10*2,12*2,12*2,15*2,15*2
	};

	for (int fnum=0; fnum<1024; fnum++)
		for (int block=0; block<8; block++)
			for (int ML=0; ML<16; ML++)
				dphaseTable[fnum][block][ML] = 
					rate_adjust((((fnum * mltable[ML]) << block) >> (21 - DP_BITS)), sampleRate);
}

void Y8950::Slot::makeTllTable()
{
	#define dB2(x) (int)((x)*2)
	static int kltable[16] = {
		dB2( 0.000),dB2( 9.000),dB2(12.000),dB2(13.875),
		dB2(15.000),dB2(16.125),dB2(16.875),dB2(17.625),
		dB2(18.000),dB2(18.750),dB2(19.125),dB2(19.500),
		dB2(19.875),dB2(20.250),dB2(20.625),dB2(21.000)
	};

	for (int fnum=0; fnum<16; fnum++)
		for (int block=0; block<8; block++)
			for (int TL=0; TL<64; TL++)
				for (int KL=0; KL<4; KL++) {
					if (KL==0) {
						tllTable[fnum][block][TL][KL] = ALIGN(TL, TL_STEP, EG_STEP);
					} else {
						int tmp = kltable[fnum] - dB2(3.000) * (7 - block);
						if (tmp <= 0)
							tllTable[fnum][block][TL][KL] = ALIGN(TL, TL_STEP, EG_STEP);
						else 
							tllTable[fnum][block][TL][KL] = (int)((tmp>>(3-KL))/EG_STEP) + ALIGN(TL, TL_STEP, EG_STEP);
					}
				}
}


// Rate Table for Attack 
void Y8950::Slot::makeDphaseARTable(int sampleRate)
{
	for (int AR=0; AR<16; AR++)
		for (int Rks=0; Rks<16; Rks++) {
			int RM = AR + (Rks>>2);
			int RL = Rks&3;
			if (RM>15) RM=15;
			switch (AR) { 
			case 0:
				dphaseARTable[AR][Rks] = 0;
				break;
			case 15:
				dphaseARTable[AR][Rks] = EG_DP_WIDTH;
				break;
			default:
				dphaseARTable[AR][Rks] = rate_adjust((3*(RL+4) << (RM+1)), sampleRate);
				break;
			}
		}
}

// Rate Table for Decay 
void Y8950::Slot::makeDphaseDRTable(int sampleRate)
{
	for (int DR=0; DR<16; DR++)
		for (int Rks=0; Rks<16; Rks++) {
			int RM = DR + (Rks>>2);
			int RL = Rks&3;
			if (RM>15) RM=15;
			switch (DR) { 
			case 0:
				dphaseDRTable[DR][Rks] = 0;
				break;
			default:
				dphaseDRTable[DR][Rks] = rate_adjust((RL+4) << (RM-1), sampleRate);
				break;
			}
		}
}

void Y8950::Slot::makeRksTable()
{
	for (int fnum9=0; fnum9<2; fnum9++)
		for (int block=0; block<8; block++)
			for (int KR=0; KR<2; KR++) {
				rksTable[fnum9][block][KR] = (KR != 0) ?
					(block<<1) + fnum9:
					 block>>1;
			}
}

//**********************************************************//
//                                                          //
//  Patch                                                   //
//                                                          //
//**********************************************************//

Y8950::Patch::Patch()
{
	reset();
}

void Y8950::Patch::reset()
{
	AM = PM = EG = false;
	KR = ML = KL = TL = FB = AR = DR = SL = RR = 0;
}


//**********************************************************//
//                                                          //
//  Slot                                                    //
//                                                          //
//**********************************************************//

Y8950::Slot::Slot()
{
}

Y8950::Slot::~Slot()
{
}

void Y8950::Slot::reset()
{
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
	fnum = 0;
	block = 0;
	pgout = 0;
	egout = 0;
	slotStatus = false;
	patch.reset();
	updateAll();
}

void Y8950::Slot::updatePG()
{
	dphase = dphaseTable[fnum][block][patch.ML];
}

void Y8950::Slot::updateTLL()
{
	tll = tllTable[fnum>>6][block][patch.TL][patch.KL];
}

void Y8950::Slot::updateRKS()
{
	rks = rksTable[fnum>>9][block][patch.KR];
}

void Y8950::Slot::updateEG()
{
	switch (eg_mode) {
		case ATTACK:
			eg_dphase = dphaseARTable[patch.AR][rks];
			break;
		case DECAY:
			eg_dphase = dphaseDRTable[patch.DR][rks];
			break;
		case SUSTINE:
			eg_dphase = dphaseDRTable[patch.RR][rks];
			break;
		case RELEASE:
			eg_dphase = patch.EG ?
			            dphaseDRTable[patch.RR][rks]:
			            dphaseDRTable[7]       [rks];
			break;
		case SUSHOLD:
		case FINISH:
			eg_dphase = 0;
			break;
	}
}

void Y8950::Slot::updateAll()
{
	updatePG();
	updateTLL();
	updateRKS();
	updateEG(); // EG should be last 
}

// Slot key on  
void Y8950::Slot::slotOn()
{
	if (!slotStatus) {
		slotStatus = true;
		eg_mode = ATTACK;
		phase = 0;
		eg_phase = 0;
	}
}

// Slot key off 
void Y8950::Slot::slotOff()
{
	if (slotStatus) {
		slotStatus = false;
		if (eg_mode == ATTACK)
			eg_phase = EXPAND_BITS(AR_ADJUST_TABLE[HIGHBITS(eg_phase, EG_DP_BITS-EG_BITS)], EG_BITS, EG_DP_BITS);
		eg_mode = RELEASE;
	}
}


//**********************************************************//
//                                                          //
//  Channel                                                 //
//                                                          //
//**********************************************************//

Y8950::Channel::Channel()
{
	reset();
}

Y8950::Channel::~Channel()
{
}

void Y8950::Channel::reset()
{
	mod.reset();
	car.reset();
	alg = false;
}

// Set F-Number ( fnum : 10bit ) 
void Y8950::Channel::setFnumber(int fnum)
{
	car.fnum = fnum;
	mod.fnum = fnum;
}

// Set Block data (block : 3bit ) 
void Y8950::Channel::setBlock(int block)
{
	car.block = block;
	mod.block = block;
}

// Channel key on 
void Y8950::Channel::keyOn()
{
	mod.slotOn();
	car.slotOn();
}

// Channel key off 
void Y8950::Channel::keyOff()
{
	mod.slotOff();
	car.slotOff();
}


//**********************************************************//
//                                                          //
//  Y8950                                                   //
//                                                          //
//**********************************************************//


void Y8950::callback(byte flag)
{
	setStatus(flag);
}

Y8950::Y8950(const string& name_, int sampleRam, const EmuTime& time)
	: timer1(this), timer2(this), adpcm(*this, name_, sampleRam), /*connector(),*/
          name(name_)
{
	makePmTable();
	makeAmTable();
	Slot::makeAdjustTable();
	Slot::makeDB2LinTable();
	Slot::makeTllTable();
	Slot::makeRksTable();
	Slot::makeSinTable();

	for (int i=0; i<9; i++) {
		// TODO cleanup
		slot[i*2+0] = &(ch[i].mod);
		slot[i*2+1] = &(ch[i].car);
		ch[i].mod.plfo_am = &lfo_am;
		ch[i].mod.plfo_pm = &lfo_pm;
		ch[i].car.plfo_am = &lfo_am;
		ch[i].car.plfo_pm = &lfo_pm;
	}

	reset(time);
//	registerSound(config);
//	Debugger::instance().registerDebuggable(name + " regs", *this);
}

Y8950::~Y8950()
{
//	Debugger::instance().unregisterDebuggable(name + " regs", *this);
//	unregisterSound();
}

const string& Y8950::getName() const
{
	return name;
}

const string& Y8950::getDescription() const
{
	static const string desc("MSX-AUDIO");
	return desc;
}

void Y8950::setSampleRate(int sampleRate, int oversampling)
{
	adpcm.setSampleRate(sampleRate);
	Y8950::Slot::makeDphaseTable(sampleRate);
	Y8950::Slot::makeDphaseARTable(sampleRate);
	Y8950::Slot::makeDphaseDRTable(sampleRate);
	makeDphaseNoiseTable(sampleRate);
	pm_dphase = rate_adjust(PM_SPEED * PM_DP_WIDTH / (CLK_FREQ/72), sampleRate);
	am_dphase = rate_adjust(AM_SPEED * AM_DP_WIDTH / (CLK_FREQ/72), sampleRate);
}

// Reset whole of opl except patch datas.
void Y8950::reset(const EmuTime &time)
{
	for (int i=0; i<9; i++)
		ch[i].reset();
	output[0] = 0;
	output[1] = 0;


    dacSampleVolume = 0;
    dacOldSampleVolume = 0;
    dacSampleVolumeSum = 0;
    dacCtrlVolume = 0;
    dacDaVolume = 0;
    dacEnabled = 0;

	rythm_mode = false;
	am_mode = 0;
	pm_mode = 0;
	pm_phase = 0;
	am_phase = 0;
	noise_seed = 0xffff;
	noiseA = 0;
	noiseB = 0;
	noiseA_phase = 0;
	noiseB_phase = 0;
	noiseA_dphase = 0;
	noiseB_dphase = 0;

	// update the output buffer before changing the register
//	Mixer::instance().updateStream(time);
	for (int i = 0; i < 0x100; ++i) 
		reg[i] = 0x00;

	reg[0x04] = 0x18;
	reg[0x19] = 0x0F; // fixes 'Thunderbirds are Go'
	status = 0x00;
	statusMask = 0;
	irq.reset();
	
	adpcm.reset(time);
	setInternalMute(true);	// muted
}


// Drum key on
void Y8950::keyOn_BD()  { ch[6].keyOn(); }
void Y8950::keyOn_HH()  { ch[7].mod.slotOn(); }
void Y8950::keyOn_SD()  { ch[7].car.slotOn(); }
void Y8950::keyOn_TOM() { ch[8].mod.slotOn(); }
void Y8950::keyOn_CYM() { ch[8].car.slotOn(); }

// Drum key off
void Y8950::keyOff_BD() { ch[6].keyOff(); }
void Y8950::keyOff_HH() { ch[7].mod.slotOff(); }
void Y8950::keyOff_SD() { ch[7].car.slotOff(); }
void Y8950::keyOff_TOM(){ ch[8].mod.slotOff(); }
void Y8950::keyOff_CYM(){ ch[8].car.slotOff(); }

// Change Rhythm Mode
void Y8950::setRythmMode(int data)
{
	bool newMode = (data & 32) != 0;
	if (rythm_mode != newMode) {
		rythm_mode = newMode;
		if (!rythm_mode) {
			// ON->OFF
			ch[6].mod.eg_mode = FINISH; // BD1
			ch[6].mod.slotStatus = false;
			ch[6].car.eg_mode = FINISH; // BD2
			ch[6].car.slotStatus = false;
			ch[7].mod.eg_mode = FINISH; // HH
			ch[7].mod.slotStatus = false;
			ch[7].car.eg_mode = FINISH; // SD
			ch[7].car.slotStatus = false;
			ch[8].mod.eg_mode = FINISH; // TOM
			ch[8].mod.slotStatus = false;
			ch[8].car.eg_mode = FINISH; // CYM
			ch[8].car.slotStatus = false;
		}
	}
}

//********************************************************//
//                                                        //
// Generate wave data                                     //
//                                                        //
//********************************************************//

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 4PI). 
int Y8950::Slot::wave2_4pi(int e)
{
	int shift =  SLOT_AMP_BITS - PG_BITS - 1;
	if (shift > 0)
		return e >> shift;
	else
		return e << -shift;
}

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 8PI). 
int Y8950::Slot::wave2_8pi(int e)
{
	int shift = SLOT_AMP_BITS - PG_BITS - 2;
	if (shift > 0)
		return e >> shift;
	else
		return e << -shift;
}


void Y8950::update_noise()
{
	if (noise_seed & 1)
		noise_seed ^= 0x24000;
	noise_seed >>= 1;
	whitenoise = noise_seed&1 ? DB_POS(6) : DB_NEG(6);

	noiseA_phase += noiseA_dphase;
	noiseB_phase += noiseB_dphase;

	noiseA_phase &= (0x40<<11) - 1;
	if ((noiseA_phase>>11)==0x3f)
		noiseA_phase = 0;
	noiseA = noiseA_phase&(0x03<<11)?DB_POS(6):DB_NEG(6);

	noiseB_phase &= (0x10<<11) - 1;
	noiseB = noiseB_phase&(0x0A<<11)?DB_POS(6):DB_NEG(6);
}

void Y8950::update_ampm()
{
	pm_phase = (pm_phase + pm_dphase)&(PM_DP_WIDTH - 1);
	am_phase = (am_phase + am_dphase)&(AM_DP_WIDTH - 1);
	lfo_am = amtable[am_mode][HIGHBITS(am_phase, AM_DP_BITS - AM_PG_BITS)];
	lfo_pm = pmtable[pm_mode][HIGHBITS(pm_phase, PM_DP_BITS - PM_PG_BITS)];
}

void Y8950::Slot::calc_phase()
{
	if (patch.PM)
		phase += (dphase * (*plfo_pm)) >> PM_AMP_BITS;
	else
		phase += dphase;
	phase &= (DP_WIDTH - 1);
	pgout = HIGHBITS(phase, DP_BASE_BITS);
}

void Y8950::Slot::calc_envelope()
{
	#define S2E(x) (ALIGN((unsigned int)(x/SL_STEP),SL_STEP,EG_STEP)<<(EG_DP_BITS-EG_BITS)) 
	static unsigned int SL[16] = {
		S2E( 0), S2E( 3), S2E( 6), S2E( 9), S2E(12), S2E(15), S2E(18), S2E(21),
		S2E(24), S2E(27), S2E(30), S2E(33), S2E(36), S2E(39), S2E(42), S2E(93)
	};
	
	switch (eg_mode) {
	case ATTACK:
		eg_phase += eg_dphase;
		if (EG_DP_WIDTH & eg_phase) {
			egout = 0;
			eg_phase= 0;
			eg_mode = DECAY;
			updateEG();
		} else {
			egout = AR_ADJUST_TABLE[HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS)];
		}
		break;

	case DECAY:
		eg_phase += eg_dphase;
		egout = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		if (eg_phase >= SL[patch.SL]) {
			if (patch.EG) {
				eg_phase = SL[patch.SL];
				eg_mode = SUSHOLD;
				updateEG();
			} else {
				eg_phase = SL[patch.SL];
				eg_mode = SUSTINE;
				updateEG();
			}
			egout = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		}
		break;

	case SUSHOLD:
		egout = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS);
		if (!patch.EG) {
			eg_mode = SUSTINE;
			updateEG();
		}
		break;

	case SUSTINE:
	case RELEASE:
		eg_phase += eg_dphase;
		egout = HIGHBITS(eg_phase, EG_DP_BITS - EG_BITS); 
		if (egout >= (1<<EG_BITS)) {
			eg_mode = FINISH;
			egout = (1<<EG_BITS) - 1;
		}
		break;

	case FINISH:
		egout = (1<<EG_BITS) - 1;
		break;
	}

	if (patch.AM)
		egout = ALIGN(egout+tll,EG_STEP,DB_STEP) + (*plfo_am);
	else 
		egout = ALIGN(egout+tll,EG_STEP,DB_STEP);
	if (egout >= DB_MUTE)
		egout = DB_MUTE-1;
}

int Y8950::Slot::calc_slot_car(int fm)
{
	calc_envelope(); 
	calc_phase();
	if (egout>=(DB_MUTE-1))
		return 0;
	return dB2LinTab[sintable[(pgout+wave2_8pi(fm))&(PG_WIDTH-1)] + egout];
}

int Y8950::Slot::calc_slot_mod()
{
	output[1] = output[0];
	calc_envelope(); 
	calc_phase();

	if (egout>=(DB_MUTE-1)) {
		output[0] = 0;
	} else if (patch.FB!=0) {
		int fm = wave2_4pi(feedback) >> (7-patch.FB);
		output[0] = dB2LinTab[sintable[(pgout+fm)&(PG_WIDTH-1)] + egout];
	} else
		output[0] = dB2LinTab[sintable[pgout] + egout];
	
	feedback = (output[1] + output[0])>>1;
	return feedback;
}

// TOM
int Y8950::Slot::calc_slot_tom()
{
	calc_envelope(); 
	calc_phase();
	if (egout>=(DB_MUTE-1))
		return 0;
	return dB2LinTab[sintable[pgout] + egout];
}

// SNARE
int Y8950::Slot::calc_slot_snare(int whitenoise)
{
	calc_envelope();
	calc_phase();
	if (egout>=(DB_MUTE-1))
		return 0;
	if (pgout & (1<<(PG_BITS-1))) {
		return (dB2LinTab[egout] + dB2LinTab[egout+whitenoise]) >> 1;
	} else {
		return (dB2LinTab[2*DB_MUTE + egout] + dB2LinTab[egout+whitenoise]) >> 1;
	}
}

// TOP-CYM
int Y8950::Slot::calc_slot_cym(int a, int b)
{
	calc_envelope();
	if (egout>=(DB_MUTE-1)) {
		return 0;
	} else {
		return (dB2LinTab[egout+a] + dB2LinTab[egout+b]) >> 1;
	}
}

// HI-HAT
int Y8950::Slot::calc_slot_hat(int a, int b, int whitenoise)
{
	calc_envelope();
	if (egout>=(DB_MUTE-1)) {
		return 0;
	} else {
		return (dB2LinTab[egout+whitenoise] + dB2LinTab[egout+a] + dB2LinTab[egout+b]) >>2;
	}
}


int Y8950::calcSample(int channelMask)
{
	// while muted update_ampm() and update_noise() aren't called, probably ok
	update_ampm();
	update_noise();      

	int mix = 0;

	if (rythm_mode) {
		// TODO wasn't in original source either
		ch[7].mod.calc_phase();
		ch[8].car.calc_phase();

		if (channelMask & (1 << 6))
			mix += ch[6].car.calc_slot_car(ch[6].mod.calc_slot_mod());
		if (ch[7].mod.eg_mode != FINISH)
			mix += ch[7].mod.calc_slot_hat(noiseA, noiseB, whitenoise);
		if (channelMask & (1 << 7))
			mix += ch[7].car.calc_slot_snare(whitenoise);
		if (ch[8].mod.eg_mode != FINISH)
			mix += ch[8].mod.calc_slot_tom();
		if (channelMask & (1 << 8))
			mix += ch[8].car.calc_slot_cym(noiseA, noiseB);	

		channelMask &= (1<< 6) - 1;
		mix *= 2;
	}
	for (Channel *cp = ch; channelMask; channelMask >>=1, cp++) {
		if (channelMask & 1) {
			if (cp->alg)
				mix += cp->car.calc_slot_car(0) +
				       cp->mod.calc_slot_mod();
			else
				mix += cp->car.calc_slot_car( 
				         cp->mod.calc_slot_mod());
		}
	}
	
	mix += adpcm.calcSample();

	return (mix*maxVolume) >> (DB2LIN_AMP_BITS - 1);
}


void Y8950::checkMute()
{
	bool mute = checkMuteHelper();
	//PRT_DEBUG("Y8950: muted " << mute);
	setInternalMute(mute);
}
bool Y8950::checkMuteHelper()
{
	for (int i = 0; i < 6; i++) {
		if (ch[i].car.eg_mode != FINISH) return false;
	}
	if (!rythm_mode) {
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
	
	return adpcm.muted();
}

int* Y8950::updateBuffer(int length)
{
	//PRT_DEBUG("Y8950: update buffer");

	if (isInternalMuted() && !dacEnabled) {
		return NULL;
	}

    dacCtrlVolume = dacSampleVolume - dacOldSampleVolume + 0x3fe7 * dacCtrlVolume / 0x4000;
    dacOldSampleVolume = dacSampleVolume;

	int channelMask = 0;
	for (int i = 9; i--; ) {
		channelMask <<= 1;
		if (ch[i].car.eg_mode != FINISH) channelMask |= 1;
	}

	int* buf = buffer;
	while (length--) {
        int sample = calcSample(channelMask);

        dacCtrlVolume = 0x3fe7 * dacCtrlVolume / 0x4000;
        dacDaVolume += 2 * (dacCtrlVolume - dacDaVolume) / 3;
        sample += 48 * dacDaVolume;
		*(buf++) = sample;
	}

    dacEnabled = dacDaVolume;

	checkMute();
	return buffer;
}

void Y8950::setInternalVolume(short newVolume)
{
	maxVolume = newVolume;
}

//**************************************************//
//                                                  //
//                       I/O Ctrl                   //
//                                                  //
//**************************************************//

void Y8950::writeReg(byte rg, byte data, const EmuTime& time)
{
	//PRT_DEBUG("Y8950 write " << (int)rg << " " << (int)data);
	int stbl[32] = {
		 0, 2, 4, 1, 3, 5,-1,-1,
		 6, 8,10, 7, 9,11,-1,-1,
		12,14,16,13,15,17,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1
	};

	//TODO only for registers that influence sound
	//TODO also ADPCM
	//if (rg>=0x20) {
		// update the output buffer before changing the register
//		Mixer::instance().updateStream(time);
	//}
//	Mixer::instance().lock();

	switch (rg & 0xe0) {
	case 0x00: {
		switch (rg) {
		case 0x01: // TEST
			// TODO
			// Y8950 MSX-AUDIO Test register $01 (write only)
			//
			// Bit	Description
			//
			// 7	Reset LFOs - seems to force the LFOs to their initial values (eg.
			//	maximum amplitude, zero phase deviation)
			//
			// 6	something to do with ADPCM - bit 0 of the status register is
			//	affected by setting this bit (PCM BSY)
			//
			// 5	No effect? - Waveform select enable in YM3812 OPL2 so seems
			//	reasonable that this bit wouldn't have been used in OPL
			//
			// 4	No effect?
			//
			// 3	Faster LFOs - increases the frequencies of the LFOs and (maybe)
			//	the timers (cf. YM2151 test register)
			//
			// 2	Reset phase generators - No phase generator output, but envelope
			//	generators still work (can hear a transient when they are gated)
			//
			// 1	No effect?
			//
			// 0	Reset envelopes - Envelope generator outputs forced to maximum,
			//	so all enabled voices sound at maximum
			reg[rg] = data;
			break;

		case 0x02: // TIMER1 (reso. 80us)
			timer1.setValue(data);
			reg[rg] = data;
			break;

		case 0x03: // TIMER2 (reso. 320us) 
			timer2.setValue(data);
			reg[rg] = data;
			break;

		case 0x04: // FLAG CONTROL 
			if (data & R04_IRQ_RESET) {
				resetStatus(0x78);	// reset all flags
			} else {
				changeStatusMask((~data) & 0x78);
				timer1.setStart((data & R04_ST1) != 0, time);
				timer2.setStart((data & R04_ST2) != 0, time);
				reg[rg] = data;
			}
			break;

		case 0x06: // (KEYBOARD OUT) 
//			connector.write(data, time); //FIXME
			reg[rg] = data;
			break;
		
		case 0x07: // START/REC/MEM DATA/REPEAT/SP-OFF/-/-/RESET
		case 0x08: // CSM/KEY BOARD SPLIT/-/-/SAMPLE/DA AD/64K/ROM 
		case 0x09: // START ADDRESS (L) 
		case 0x0A: // START ADDRESS (H) 
		case 0x0B: // STOP ADDRESS (L) 
		case 0x0C: // STOP ADDRESS (H) 
		case 0x0D: // PRESCALE (L) 
		case 0x0E: // PRESCALE (H) 
		case 0x0F: // ADPCM-DATA 
		case 0x10: // DELTA-N (L) 
		case 0x11: // DELTA-N (H) 
		case 0x12: // ENVELOP CONTROL
		case 0x1A: // PCM-DATA
			reg[rg] = data;
			adpcm.writeReg(rg, data, time);
			break;
		
		case 0x15: // DAC-DATA  (bit9-2)
			reg[rg] = data;
			if (reg[0x08] & 0x04) {
                static int damp[] = { 256, 279, 304, 332, 362, 395, 431, 470 };
				int sample = (short)(256 * reg[0x15] + reg[0x16]) * 128 / damp[reg[0x17]];
                dacSampleVolume = sample;
                dacEnabled = 1;
			}
			break;
		case 0x16: //           (bit1-0)
			reg[rg] = data & 0xC0;
			break;
		case 0x17: //           (exponent)
			reg[rg] = data & 0x07;
			break;
		
		case 0x18: // I/O-CONTROL (bit3-0)
			// TODO
			// 0 -> input
			// 1 -> output
			reg[rg] = data;
			break;
		
		case 0x19: // I/O-DATA (bit3-0)
			// TODO
			reg[rg] = data;
			break;
		}
		
		break;
	}
	case 0x20: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			slot[s]->patch.AM = (data>>7)&1;
			slot[s]->patch.PM = (data>>6)&1;
			slot[s]->patch.EG = (data>>5)&1;
			slot[s]->patch.KR = (data>>4)&1;
			slot[s]->patch.ML = (data)&15;
			slot[s]->updateAll();
		}
		reg[rg] = data;
		break;
	}
	case 0x40: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			slot[s]->patch.KL = (data>>6)&3;
			slot[s]->patch.TL = (data)&63;
			slot[s]->updateAll();
		}
		reg[rg] = data;
		break;
	} 
	case 0x60: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			slot[s]->patch.AR = (data>>4)&15;
			slot[s]->patch.DR = (data)&15;
			slot[s]->updateEG();
		}
		reg[rg] = data;
		break;
	} 
	case 0x80: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			slot[s]->patch.SL = (data>>4)&15;
			slot[s]->patch.RR = (data)&15;
			slot[s]->updateEG();
		}
		reg[rg] = data;
		break;
	} 
	case 0xa0: {
		if (rg==0xbd) {
			am_mode = (data>>7)&1;
			pm_mode = (data>>6)&1;
			
			setRythmMode(data);
			if (rythm_mode) {
				if (data&0x10) keyOn_BD();  else keyOff_BD();
				if (data&0x08) keyOn_SD();  else keyOff_SD();
				if (data&0x04) keyOn_TOM(); else keyOff_TOM();
				if (data&0x02) keyOn_CYM(); else keyOff_CYM();
				if (data&0x01) keyOn_HH();  else keyOff_HH();
			}
			ch[6].mod.updateAll();
			ch[6].car.updateAll();
			ch[7].mod.updateAll();
			ch[7].car.updateAll();
			ch[8].mod.updateAll();
			ch[8].car.updateAll();

			reg[rg] = data;
			break;
		}
		if ((rg&0xf) > 8) {
			// 0xa9-0xaf 0xb9-0xbf
			break;
		}
		if (!(rg&0x10)) {
			// 0xa0-0xa8
			int c = rg-0xa0;
			int fNum = data + ((reg[rg+0x10]&3)<<8);
			int block = (reg[rg+0x10]>>2)&7;
			ch[c].setFnumber(fNum);
			switch (c) {
				case 7: noiseA_dphase = dphaseNoiseTable[fNum][block];
					break;
				case 8: noiseB_dphase = dphaseNoiseTable[fNum][block];
					break;
			}
			ch[c].car.updateAll();
			ch[c].mod.updateAll();
			reg[rg] = data;
		} else {
			// 0xb0-0xb8
			int c = rg-0xb0;
			int fNum = ((data&3)<<8) + reg[rg-0x10];
			int block = (data>>2)&7;
			ch[c].setFnumber(fNum);
			ch[c].setBlock(block);
			switch (c) {
				case 7: noiseA_dphase = dphaseNoiseTable[fNum][block];
					break;
				case 8: noiseB_dphase = dphaseNoiseTable[fNum][block];
					break;
			}
			if (data&0x20)
				ch[c].keyOn();
			else
				ch[c].keyOff();
			ch[c].mod.updateAll();
			ch[c].car.updateAll();
			reg[rg] = data;
		}
		break;
	}
	case 0xc0: {
		if (rg > 0xc8)
			break;
		int c = rg-0xC0;
		slot[c*2]->patch.FB = (data>>1)&7;
		ch[c].alg = data&1;
		reg[rg] = data;
	}
	}
//	Mixer::instance().unlock();
	//TODO only for registers that influence sound
	checkMute();
}

byte Y8950::readReg(byte rg, const EmuTime &time)
{
	// TODO only when necessary
//	Mixer::instance().updateStream(time);
	
	byte result;
	switch (rg) {
		case 0x05: // (KEYBOARD IN)
//			result = connector.read(time); FIXME
		    result = 0xff;
			break;
		
		case 0x0f: // ADPCM-DATA
		case 0x13: //  ???
		case 0x14: //  ???
		case 0x1a: // PCM-DATA
			result = adpcm.readReg(rg);
			break;
		
		case 0x19: // I/O DATA   TODO
		    result =  ~(switchGetAudio() ?	0 :	0x04);
            break;
		default:
			result = 255;
	}
	//PRT_DEBUG("Y8950 read " << (int)rg<<" "<<(int)result);
	return result;
}

byte Y8950::readStatus()
{
	setStatus(STATUS_BUF_RDY);	// temp hack
	byte tmp = status & (0x80 | statusMask);
	//PRT_DEBUG("Y8950 read status " << (int)tmp);
	return tmp | 0x06;	// bit 1 and 2 are always 1
}


void Y8950::setStatus(byte flags)
{
	status |= flags;
	if (status & statusMask) {
		status |= 0x80;
		irq.set();
	}
}
void Y8950::resetStatus(byte flags)
{
	status &= ~flags;
	if (!(status & statusMask)) {
		status &= 0x7f;
		irq.reset();
	}
}
void Y8950::changeStatusMask(byte newMask)
{
	statusMask = newMask;
	status &= statusMask;
	if (status) {
		status |= 0x80;
		irq.set();
	} else {
		status &= 0x7f;
		irq.reset();
	}
}


// Debuggable

unsigned Y8950::getSize() const
{
	return 0x100;
}

byte Y8950::read(unsigned address)
{
	return reg[address];
}

void Y8950::write(unsigned address, byte value)
{
	writeReg(address, value, boardSystemTime());
}


void Y8950::pushTime(const EmuTime &time) 
{
    adpcm.pushTime(time);
}

void Y8950::loadState()
{
    SaveState* state = saveStateOpenForRead("y8950");

    saveStateClose(state);

    adpcm.loadState();
}

void Y8950::saveState()
{
    SaveState* state = saveStateOpenForWrite("y8950");

    saveStateClose(state);

    adpcm.saveState();
}
