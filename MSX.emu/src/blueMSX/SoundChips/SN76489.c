/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SN76489.c,v $
**
** $Revision: 1.21 $
**
** $Date: 2009-04-10 04:38:10 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "SN76489.h"
#include "IoPort.h"
#include "SaveState.h"
#include "DebugDeviceManager.h"
#include "Language.h"
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <math.h>
#include <limits.h>



#if 1

#define FB_BBCMICRO  0x0005
#define FB_SC3000    0x0006
#define FB_SEGA      0x0009
#define FB_COLECO    0x0003

#define SRW_SEGA     16
#define SRW_COLECO   15

#define VOL_TRUNC    0
#define VOL_FULL     0

#define SR_INIT       0x4000
#define PSG_CUTOFF    0x6

static int VoltTables[2][16] = 
{
    { 9897, 9897, 9897, 8432, 6912, 5514, 4482, 3584, 2851, 2196, 1764, 1365, 1065,  832,  666, 0 },
    { 9897, 7867, 6248, 4962, 3937, 3127, 2487, 1978, 1567, 1247,  992,  783,  627,  496,  392, 0 }
};

#define DELTA_CLOCK  ((float)3579545 / 16 / 44100)

struct SN76489 {
    /* Framework params */
    Mixer* mixer;
    Int32  handle;
    Int32  debugHandle;

    /* Configuration params */
    int voltTableIdx;
    int whiteNoiseFeedback;
    int shiftRegisterWidth;
    
    /* State params */
    float clock;

    int regs[8];
    int latch;
    int shiftReg;
    int noiseFreq;
    
    int toneFrequency[4];
    int toneFlipFlop[4];
    float toneInterpol[4];

    /* Filter params */
    Int32  ctrlVolume;
    Int32  oldSampleVolume;
    Int32  daVolume;

    /* Audio buffer */
    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};


static Int32* sn76489Sync(void* ref, UInt32 count);


void sn76489LoadState(SN76489* sn76489)
{
    SaveState* state = saveStateOpenForRead("sn76489");
    char tag[32];
    int i;
    
    sn76489->latch            = saveStateGet(state, "latch",           0);
    sn76489->shiftReg         = saveStateGet(state, "shiftReg",        0);
    sn76489->noiseFreq        = saveStateGet(state, "noiseFreq",       1);

    sn76489->ctrlVolume       = saveStateGet(state, "ctrlVolume",      0);
    sn76489->oldSampleVolume  = saveStateGet(state, "oldSampleVolume", 0);
    sn76489->daVolume         = saveStateGet(state, "daVolume",        0);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        sn76489->regs[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "toneFrequency%d", i);
        sn76489->toneFrequency[i] = saveStateGet(state, tag, 0);

        sprintf(tag, "toneFlipFlop%d", i);
        sn76489->toneFlipFlop[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
}

void sn76489SaveState(SN76489* sn76489)
{
    SaveState* state = saveStateOpenForWrite("sn76489");
    char tag[32];
    int i;

    saveStateSet(state, "latch",           sn76489->latch);
    saveStateSet(state, "shiftReg",        sn76489->shiftReg);
    saveStateSet(state, "noiseFreq",       sn76489->noiseFreq);

    saveStateSet(state, "ctrlVolume",      sn76489->ctrlVolume);
    saveStateSet(state, "oldSampleVolume", sn76489->oldSampleVolume);
    saveStateSet(state, "daVolume",        sn76489->daVolume);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        saveStateSet(state, tag, sn76489->regs[i]);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "toneFrequency%d", i);
        saveStateSet(state, tag, sn76489->toneFrequency[i]);

        sprintf(tag, "toneFlipFlop%d", i);
        saveStateSet(state, tag, sn76489->toneFlipFlop[i]);
        
        sn76489->toneInterpol[i] = 0;
    }

    sn76489->clock = 0;

    saveStateClose(state);
}

static void getDebugInfo(SN76489* sn76489, DbgDevice* dbgDevice)
{
    DbgRegisterBank* regBank;
    int i;

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegs(), 8);

    for (i = 0; i < 4; i++) {
        char reg[4];
        sprintf(reg, "V%d", i + 1);
        dbgRegisterBankAddRegister(regBank,  i, reg, 8, sn76489->regs[2 * i + 1] & 0x0f);
    }
    
    for (i = 0; i < 4; i++) {
        char reg[4];
        sprintf(reg, "T%d", i + 1);
        if (i < 3) {
            dbgRegisterBankAddRegister(regBank,  i + 4, reg, 16, sn76489->regs[2 * i] & 0x03ff);
        }
        else {
            dbgRegisterBankAddRegister(regBank,  i + 4, reg, 8, sn76489->regs[2 * i] & 0x03);
        }
    }

}

void sn76489Destroy(SN76489* sn76489)
{
    debugDeviceUnregister(sn76489->debugHandle);
    mixerUnregisterChannel(sn76489->mixer, sn76489->handle);
    free(sn76489);
}

void sn76489Reset(SN76489* sn76489)
{
    SN76489* p = sn76489;
    int i;

    for( i = 0; i <= 3; i++ )
    {
        p->regs[2 * i]      = 1;
        p->regs[2 * i + 1]  = 0xf;
        p->noiseFreq        = 0x10;
        p->toneFrequency[i] = 0;
        p->toneFlipFlop[i]  = 1;
        p->toneInterpol[i]  = FLT_MIN;
    }

    p->clock    = 0;
    p->latch    = 0;
    p->shiftReg = 1 << (sn76489->shiftRegisterWidth - 1);
}

SN76489* sn76489Create(Mixer* mixer)
{
    DebugCallbacks dbgCallbacks = { getDebugInfo, NULL, NULL, NULL };
    SN76489* sn76489 = (SN76489*)calloc(1, sizeof(SN76489));

    sn76489->mixer = mixer;

    sn76489->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_PSG, 0, sn76489Sync, NULL, sn76489);
    sn76489->debugHandle = debugDeviceRegister(DBGTYPE_AUDIO, "SN76489 PSG", &dbgCallbacks, sn76489);


    sn76489->voltTableIdx       = VOL_FULL;
    sn76489->whiteNoiseFeedback = FB_COLECO;
    sn76489->shiftRegisterWidth = SRW_COLECO;

    sn76489Reset(sn76489);

    return sn76489;
}

void sn76489WriteData(SN76489* sn76489, UInt16 ioPort, UInt8 data)
{
    SN76489* p = sn76489;

    mixerSync(p->mixer);

    if (data & 0x80) {
        p->latch = ( data >> 4 ) & 0x07;
        p->regs[p->latch] = (p->regs[p->latch] & 0x3f0) | (data & 0x0f);
    } 
    else {
        if ((p->latch & 1) == 0 && p->latch < 5) {
            p->regs[p->latch] = (p->regs[p->latch] & 0x0f) | ((data & 0x3f) << 4);
        }
        else {
            p->regs[p->latch] = data & 0x0f;
        }
    }
    switch (p->latch) {
    case 0:
    case 2:
    case 4:
        if (p->latch == 4 && (p->regs[6] & 3) == 0x03) {
            p->noiseFreq = p->regs[4];
        }
        break;
    case 6:
        p->shiftReg = SR_INIT;
        if ((p->regs[6] & 3) == 0x03) {
            p->noiseFreq = p->regs[4];
        }
        else {
            p->noiseFreq = 0x10 << (p->regs[6] & 0x3);
        }
        break;
    }
}

static Int32* sn76489Sync(void* ref, UInt32 count)
{
    SN76489* p = (SN76489*)ref;
    int clocksPerSample;
    UInt32 j;
    int i;

    for(j = 0; j < count; j++) {
        Int32 sampleVolume = 0;

        for (i = 0; i < 3; i++) {
            if (p->toneInterpol[i] > FLT_MIN) {
                sampleVolume += (int)(VoltTables[p->voltTableIdx][p->regs[2 * i + 1]] * p->toneInterpol[i]);
            }
            else {
                sampleVolume += VoltTables[p->voltTableIdx][p->regs[2 * i + 1]] * p->toneFlipFlop[i];
            }
        }

        sampleVolume += VoltTables[p->voltTableIdx][p->regs[7]] * ( p->shiftReg & 0x1 ) * 2;

        /* Perform DC offset filtering */
        p->ctrlVolume = sampleVolume - p->oldSampleVolume + 0x3fe7 * p->ctrlVolume / 0x4000;
        p->oldSampleVolume = sampleVolume;

        /* Perform simple 1 pole low pass IIR filtering */
        p->daVolume += 2 * (p->ctrlVolume - p->daVolume) / 3;
        
        /* Store calclulated sample value */
        p->buffer[j] = 4 * p->daVolume;

        /* Increment clock by 1 sample length */
        p->clock += DELTA_CLOCK;
        clocksPerSample = (int)p->clock;
        p->clock -= clocksPerSample;
    
        for (i = 0; i <= 2; i++) {
            p->toneFrequency[i] -= clocksPerSample;
        }

        if (p->noiseFreq == 0x80) {
            p->toneFrequency[3] = p->toneFrequency[2];
        }
        else {
            p->toneFrequency[3] -= clocksPerSample;
        }
    
        for (i = 0; i <= 2; i++) {
            if (p->regs[2 * i] == 0) {
                p->toneFlipFlop[i] = 1;
                p->toneInterpol[i] = FLT_MIN;
                p->toneFrequency[i] = 0;
            }
            else if (p->toneFrequency[i] <= 0) {
                if (p->regs[i * 2] > PSG_CUTOFF) {
                    p->toneInterpol[i] = (clocksPerSample - p->clock + 2 * p->toneFrequency[i]) * p->toneFlipFlop[i] / (clocksPerSample + p->clock);
                    p->toneFlipFlop[i] = -p->toneFlipFlop[i];
                }
                else {
                    p->toneFlipFlop[i] = 1;
                    p->toneInterpol[i] = FLT_MIN;
                }
                p->toneFrequency[i] += p->regs[i*2] * (clocksPerSample / p->regs[i*2] + 1);
            }
            else {
                p->toneInterpol[i] = FLT_MIN;
            }
        }

        if (p->noiseFreq == 0) {
            p->toneFlipFlop[3] = 1;
            p->toneFrequency[3] = 0;
        }
        else if (p->toneFrequency[3] <= 0) {
            p->toneFlipFlop[3] = -p->toneFlipFlop[3];
            if (p->noiseFreq != 0x80) {
                p->toneFrequency[3] += p->noiseFreq * (clocksPerSample / p->noiseFreq + 1);
            }
            if (p->toneFlipFlop[3] == 1) {
                int feedback;
                if ( p->regs[6] & 0x4 ) {
                    feedback = p->shiftReg & p->whiteNoiseFeedback;
                    feedback ^= feedback >> 8;
                    feedback ^= feedback >> 4;
                    feedback ^= feedback >> 2;
                    feedback ^= feedback >> 1;
                    feedback &= 1;
                } else {
                    feedback = p->shiftReg & 1;
                }

                p->shiftReg = (p->shiftReg >> 1) | (feedback << (p->shiftRegisterWidth - 1));
            }
        }
    }

    return p->buffer;
}

#else

#define BASE_PHASE_STEP 0x28959becUL  /* = (1 << 28) * 3579545 / 32 / 44100 */

static const Int16 voltTableIdx[16] = {
    0x26a9, 0x1eb5, 0x1864, 0x1360, 0x0f64, 0x0c39, 0x09b6, 0x07b6, 
    0x0620, 0x04dd, 0x03dd, 0x0312, 0x0270, 0x01f0, 0x018a, 0x0000
};

static Int32* sn76489Sync(void* ref, UInt32 count);


struct SN76489 {
    Mixer* mixer;
    Int32  handle;
    Int32  debugHandle;

    UInt16 latch;
    UInt32 noiseRand;

    UInt16 regs[8];

    UInt32 tonePhase[4];
    UInt32 toneStep[4];

    Int32  ctrlVolume;
    Int32  oldSampleVolume;
    Int32  daVolume;

    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};

void sn76489LoadState(SN76489* sn76489)
{
    SaveState* state = saveStateOpenForRead("sn76489");
    char tag[32];
    int i;

    sn76489->latch            = (UInt16)saveStateGet(state, "latch",           0);
    sn76489->noiseRand        =         saveStateGet(state, "noiseRand",       1);
    sn76489->ctrlVolume       =         saveStateGet(state, "ctrlVolume",      0);
    sn76489->oldSampleVolume  =         saveStateGet(state, "oldSampleVolume", 0);
    sn76489->daVolume         =         saveStateGet(state, "daVolume",        0);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        sn76489->regs[i] = (UInt16)saveStateGet(state, tag, 0);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "phase%d", i);
        sn76489->tonePhase[i] = saveStateGet(state, tag, 0);

        sprintf(tag, "toneStep%d", i);
        sn76489->toneStep[i] = saveStateGet(state, tag, 0);
    }

    saveStateClose(state);
}

void sn76489SaveState(SN76489* sn76489)
{
    SaveState* state = saveStateOpenForWrite("sn76489");
    char tag[32];
    int i;

    saveStateSet(state, "latch",           sn76489->latch);
    saveStateSet(state, "noiseRand",       sn76489->noiseRand);
    saveStateSet(state, "ctrlVolume",      sn76489->ctrlVolume);
    saveStateSet(state, "oldSampleVolume", sn76489->oldSampleVolume);
    saveStateSet(state, "daVolume",        sn76489->daVolume);

    for (i = 0; i < 8; i++) {
        sprintf(tag, "reg%d", i);
        saveStateSet(state, tag, sn76489->regs[i]);
    }

    for (i = 0; i < 4; i++) {
        sprintf(tag, "phase%d", i);
        saveStateSet(state, tag, sn76489->tonePhase[i]);

        sprintf(tag, "toneStep%d", i);
        saveStateSet(state, tag, sn76489->toneStep[i]);
    }

    saveStateClose(state);
}

static void setDebugInfo(SN76489* sn76489, DbgDevice* dbgDevice)
{
    DbgRegisterBank* regBank;
    int i;

    regBank = dbgDeviceAddRegisterBank(dbgDevice, langDbgRegs(), 16);

    for (i = 0; i < 16; i++) {
        char reg[4];
        sprintf(reg, "R%d", i);
        dbgRegisterBankAddRegister(regBank,  i, reg, 8, sn76489->regs[i]);
    }
}

SN76489* sn76489Create(Mixer* mixer)
{
    SN76489* sn76489 = (SN76489*)calloc(1, sizeof(SN76489));
    int i;

    sn76489->mixer = mixer;

    sn76489->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_PSG, 0, sn76489Sync, sn76489);

    sn76489Reset(sn76489);

    {
    	DoubleT v = 0x26a9;
        for (i = 0; i < 15; i++) {
            v /= 1.258925412;
        }
    }

    return sn76489;
}

void sn76489Reset(SN76489* sn76489)
{
    if (sn76489 != NULL) {
        int i;
    
        for (i = 0; i < 4; i++) {
            sn76489->regs[2 * i] = 1;
            sn76489->regs[2 * i + 1] = 0x0f;
            sn76489->tonePhase[i] = 0;
            sn76489->toneStep[i]  = 1 << 31;
        }

        sn76489->latch = 0;
        sn76489->noiseRand = 0x8000;
    }
}

void sn76489Destroy(SN76489* sn76489)
{
    debugDeviceUnregister(sn76489->debugHandle);
    mixerUnregisterChannel(sn76489->mixer, sn76489->handle);
    free(sn76489);
}

extern int framecounter;

void sn76489WriteData(SN76489* sn76489, UInt16 ioPort, UInt8 data)
{
    UInt32 period;
    int reg;

//    printf("W %d:\t %.2x  %.2x\n", framecounter, ioPort, data);

    mixerSync(sn76489->mixer);

    if (data & 0x80) {
		reg = (data >> 4) & 0x07;
		sn76489->latch = reg;

		sn76489->regs[reg] = (sn76489->regs[reg] & 0x3f0) | (data & 0x0f);

//        if (reg >=4) printf("W %d:\t %.2x  %.4x\n", framecounter, reg, sn76489->regs[reg]);
    } 
    else {
		reg = sn76489->latch;

        if ( !(reg & 1) && (reg < 5)) {
            sn76489->regs[reg] = (sn76489->regs[reg] & 0x00f) | ((data & 0x3f) << 4);
        }
        else {
            sn76489->regs[reg] = data & 0x0f;
        }
//        if (reg >=4) printf("W %d:\t %.2x  %.4x\n", framecounter, reg, sn76489->regs[reg]);
    }

    switch (reg) {
    case 0:
    case 2:
    case 4: /* Tone channels */
		period = sn76489->regs[reg];
        sn76489->toneStep[reg >> 1] = period > 0 ? BASE_PHASE_STEP / period : 1 << 31;

		if (reg == 4 && (sn76489->regs[6] & 0x03) == 0x03) {
			period = sn76489->regs[4] * 16;
            sn76489->toneStep[3] = period > 0 ? BASE_PHASE_STEP / period : 1 << 31;
		}
        break;
    case 6: /* Noise */
		if ((sn76489->regs[6] & 0x03) == 0x03) {
			period = sn76489->regs[4] * 16;
            sn76489->toneStep[3] = period > 0 ? BASE_PHASE_STEP / period : 1 << 31;
		}
        else {
		    period = 256 << (sn76489->regs[6] & 0x03);
            sn76489->toneStep[3] = period > 0 ? BASE_PHASE_STEP / period : 1 << 31;
        }

		sn76489->noiseRand = 0x4000;
        break;
    }
}

static Int32* sn76489Sync(void* ref, UInt32 count)
{
    SN76489* sn76489 = (SN76489*)ref;
    Int32   channel;
    UInt32  index;

    for (index = 0; index < count; index++) {
        Int32 sampleVolume = 0;

        UInt32 phaseStep = sn76489->toneStep[3];
        UInt32 tonePhase = sn76489->tonePhase[3];
        UInt32 tone = 0;
        Int32  count = 16;
    
        while (count--) {
            tonePhase += phaseStep;
            while (tonePhase >> 28) {
                tonePhase -= 1 << 28;
                sn76489->noiseRand = (sn76489->noiseRand >> 1) | 
                    ((sn76489->regs[6] & 0x04) ? 
                        ((sn76489->noiseRand ^ (sn76489->noiseRand >> 1)) & 1) << 14 : 
                        (sn76489->noiseRand & 1) << 14);
            }
            tone += sn76489->noiseRand & 1;
        }
    
        /* Store phase */
        sn76489->tonePhase[3] = tonePhase;

        /* Amplify sample using either envelope volume or channel volume */
        sampleVolume += (Int16)tone * voltTableIdx[sn76489->regs[7]] / 16;

        /* Calculate and add channel samples to buffer */
        for (channel = 0; channel < 3; channel++) {
            UInt32 phaseStep = sn76489->toneStep[channel];
            UInt32 tonePhase = sn76489->tonePhase[channel];
            UInt32 tone = 0;
            Int32  count = 16;
            
            /* Perform 16x oversampling */
            while (count--) {
                /* Update phase of tone */
                tonePhase += phaseStep;
     
                /* Calculate if tone is on or off */
                tone += tonePhase >> 31;
            }

            /* Store phase */
            sn76489->tonePhase[channel] = tonePhase;

            /* Amplify sample using either envelope volume or channel volume */
            sampleVolume += (Int16)tone * voltTableIdx[sn76489->regs[channel * 2 + 1]] / 16;
        }

        /* Perform DC offset filtering */
        sn76489->ctrlVolume = sampleVolume - sn76489->oldSampleVolume + 0x3fe7 * sn76489->ctrlVolume / 0x4000;
        sn76489->oldSampleVolume = sampleVolume;

        /* Perform simple 1 pole low pass IIR filtering */
        sn76489->daVolume += 2 * (sn76489->ctrlVolume - sn76489->daVolume) / 3;
        
        /* Store calclulated sample value */
        sn76489->buffer[index] = 9 * sn76489->daVolume;
    }

    return sn76489->buffer;
}

#endif
