// The file has been modified to be built in the blueMSX environment.

/*
**
** File: ymdeltat.c
**
** YAMAHA DELTA-T adpcm sound emulation subroutine
** used by fmopl.c(v0.36e-) and fm.c(v0.36c-)
**
** Base program is YM2610 emulator by Hiromitsu Shioya.
** Written by Tatsuyuki Satoh
**
** sound chips who has this unit
**
** YM2608   OPNA
** YM2610/B OPNB
** Y8950    MSX AUDIO
**
**
*/
 
#include "Ymdeltat.h"
#include "SaveState.h"

void OPL_STATUS_SET(void *OPL,int	flag);
void OPL_STATUS_RESET(void *OPL,int flag);

UINT8 *ym_deltat_memory;      /* memory pointer */

/* Forecast to next Forecast (rate = *8) */
/* 1/8 , 3/8 , 5/8 , 7/8 , 9/8 , 11/8 , 13/8 , 15/8 */
const INT32 ym_deltat_decode_tableB1[16] = {
  1,   3,   5,   7,   9,  11,  13,  15,
  -1,  -3,  -5,  -7,  -9, -11, -13, -15,
};
/* delta to next delta (rate= *64) */
/* 0.9 , 0.9 , 0.9 , 0.9 , 1.2 , 1.6 , 2.0 , 2.4 */
const INT32 ym_deltat_decode_tableB2[16] = {
  57,  57,  57,  57, 77, 102, 128, 153,
  57,  57,  57,  57, 77, 102, 128, 153
};


/* DELTA-T-ADPCM write register */
void YM_DELTAT_ADPCM_Write(YM_DELTAT *DELTAT,int r,int v)
{
	if(r>=0x10) return;
 	DELTAT->reg[r] = v; /* stock data */
	DELTAT->memread=0; /* reset mem->cpu transfer */
	switch( r ){
	case 0x00:	/* START,REC,MEMDATA,REPEAT,SPOFF,--,--,RESET */
	case 0x60:	/* write buffer MEMORY from PCM data port */
	case 0x20:	/* read  buffer MEMORY to   PCM data port */
		if( v&0x80 ){
			DELTAT->portstate = v&0x90; /* start req,memory mode,repeat flag copy */
			/**** start ADPCM ****/
			DELTAT->volume_w_step = (INT32)((DoubleT)DELTAT->volume * DELTAT->step / (1<<YM_DELTAT_SHIFT));
			DELTAT->now_addr = (DELTAT->start)<<1;
			DELTAT->now_step = (1<<YM_DELTAT_SHIFT)-DELTAT->step;
			/*adpcm->adpcmm   = 0;*/
			DELTAT->adpcmx   = 0;
			DELTAT->adpcml   = 0;
			DELTAT->adpcmd   = YM_DELTAT_DELTA_DEF;
			DELTAT->next_leveling=0;
			DELTAT->flag     = 1; /* start ADPCM */
			DELTAT->eos      = 0;

			if( !DELTAT->step )
			{
				DELTAT->flag = 0;
				DELTAT->eos  = 1;
				DELTAT->portstate = 0x00;
			}
			/**** PCM memory check & limit check ****/
			if(DELTAT->memory_size == 0){			/* Check memory Mapped */
				DELTAT->flag = 0;
				DELTAT->eos = 1;
				//DELTAT->portstate = 0x00;
			}else{
				if( DELTAT->end >= DELTAT->memory_size )
				{		/* Check End in Range */
					DELTAT->end = DELTAT->memory_size - 1;
				}
				if( DELTAT->start >= DELTAT->memory_size )
				{		/* Check Start in Range */
					DELTAT->flag = 0; 
					DELTAT->eos = 1;
					DELTAT->portstate = 0x00;
				}
			}
		} else if( v&0x01 ){
			DELTAT->flag = 0;
			DELTAT->eos  = 1;
			//DELTAT->start         = 0;
			//DELTAT->end           = 0;
			//DELTAT->read_pointer  = 0;
			//DELTAT->write_pointer = 0;
			DELTAT->portstate     = 0x00;
		}
		break;
	case 0x01:	/* L,R,-,-,SAMPLE,DA/AD,RAMTYPE,ROM */
		DELTAT->portcontrol = v&0xff;
		DELTAT->pan = &DELTAT->output_pointer[(v>>6)&0x03];
		break;
	case 0x02:	/* Start Address L */
	case 0x03:	/* Start Address H */
		DELTAT->start  = (DELTAT->reg[0x3]*0x0100 | DELTAT->reg[0x2]) << DELTAT->portshift;
		DELTAT->write_pointer = 0;
		DELTAT->read_pointer = 0;
		break;
	case 0x04:	/* Stop Address L */
	case 0x05:	/* Stop Address H */
		DELTAT->end    = (DELTAT->reg[0x5]*0x0100 | DELTAT->reg[0x4]) << DELTAT->portshift;
		DELTAT->end   += (1<<DELTAT->portshift) - 1;
		break;
	case 0x06:	/* Prescale L (PCM and Recoard frq) */
	case 0x07:	/* Proscale H */
		break;
	case 0x08:	/* ADPCM data */
		if ( (DELTAT->start+DELTAT->write_pointer) <  DELTAT->memory_size &&
			 (DELTAT->start+DELTAT->write_pointer) <= DELTAT->end )
		{
			DELTAT->memory[DELTAT->start+DELTAT->write_pointer] = v;
		 	DELTAT->write_pointer++;
			DELTAT->eos=0;
		}
		else
		{
			DELTAT->write_pointer=0;
			DELTAT->start=0;
			DELTAT->eos=1;
		}
	    break;
	case 0x09:	/* DELTA-N L (ADPCM Playback Prescaler) */
	case 0x0a:	/* DELTA-N H */
		DELTAT->delta  = (DELTAT->reg[0xa]*0x0100 | DELTAT->reg[0x9]);
		DELTAT->step   = (UINT32)((DoubleT)(DELTAT->delta*(1<<(YM_DELTAT_SHIFT-16)))*(DELTAT->freqbase));
		DELTAT->volume_w_step = (INT32)((DoubleT)DELTAT->volume * DELTAT->step / (1<<YM_DELTAT_SHIFT));
		break;
	case 0x0b:	/* Level control (volume , voltage flat) */
		{
			INT32 oldvol = DELTAT->volume;
			DELTAT->volume = (v&0xff)*(DELTAT->output_range/256) / YM_DELTAT_DECODE_RANGE;
			if( oldvol != 0 )
			{
				DELTAT->adpcml      = (int)((DoubleT)DELTAT->adpcml      / (DoubleT)oldvol * (DoubleT)DELTAT->volume);
				DELTAT->sample_step = (int)((DoubleT)DELTAT->sample_step / (DoubleT)oldvol * (DoubleT)DELTAT->volume);
			}
			DELTAT->volume_w_step = (int)((DoubleT)DELTAT->volume * (DoubleT)DELTAT->step / (DoubleT)(1<<YM_DELTAT_SHIFT));
		}
		break;
	}
}


UINT8 YM_DELTAT_ADPCM_Read(YM_DELTAT *DELTAT)
{
	UINT8 v = 0;
	
	if (DELTAT->memread<2) 
	{ 
		DELTAT->eos=0; 
		DELTAT->memread++; 
		return 0;
	}
	if ( (DELTAT->start+DELTAT->read_pointer) <  DELTAT->memory_size &&
		 (DELTAT->start+DELTAT->read_pointer) <= DELTAT->end ) 
	{
		v = DELTAT->memory[DELTAT->start+DELTAT->read_pointer];
		DELTAT->read_pointer++;
		DELTAT->eos=0;
	}
	else 
	{
		DELTAT->read_pointer=0;
		DELTAT->start=0;
		DELTAT->eos=1;
	}

	return v;
}

UINT8 YM_DELTAT_ADPCM_Peek(YM_DELTAT *DELTAT)
{
	if (DELTAT->memread >= 2 &&
        (DELTAT->start+DELTAT->read_pointer) <  DELTAT->memory_size &&
	 (DELTAT->start+DELTAT->read_pointer) <= DELTAT->end ) 
	{
        return DELTAT->memory[DELTAT->start+DELTAT->read_pointer];
    }
	return 0;
}

UINT8 YM_DELTAT_ADPCM_Read2(YM_DELTAT *DELTAT)
{
    return DELTAT->adpcmx / 256;
}

UINT8 YM_DELTAT_ADPCM_Peek2(YM_DELTAT *DELTAT)
{
    return DELTAT->adpcmx / 256;
}


void YM_DELTAT_ADPCM_Reset(YM_DELTAT *DELTAT,int pan)
{
	DELTAT->now_addr  = 0;
	DELTAT->now_step  = 0;
	DELTAT->step      = 0;
	DELTAT->start     = 0;
	DELTAT->end       = 0;
	DELTAT->eos       = 0;
	/* F2610->adpcm[i].delta     = 21866; */
	DELTAT->volume    = 0;
	DELTAT->pan       = &DELTAT->output_pointer[pan];
	/* DELTAT->flagMask  = 0; */
	DELTAT->arrivedFlag = 0;
	DELTAT->flag      = 0;
	DELTAT->adpcmx    = 0;
	DELTAT->adpcmd    = 127;
	DELTAT->adpcml    = 0;
	/*DELTAT->adpcmm    = 0;*/
	DELTAT->volume_w_step = 0;
    DELTAT->next_leveling = 0;
	DELTAT->portstate = 0;
	DELTAT->memread = 0;
	/* DELTAT->portshift = 8; */
}


void YM_DELTAT_ADPCM_CALC(YM_DELTAT *DELTAT)
{
	UINT32 step;
	int data;
	INT32 old_m;
	INT32 now_leveling;
	INT32 delta_next;

	DELTAT->now_step += DELTAT->step;
	if ( DELTAT->now_step >= (1<<YM_DELTAT_SHIFT) )
	{
		step = DELTAT->now_step >> YM_DELTAT_SHIFT;
		DELTAT->now_step &= (1<<YM_DELTAT_SHIFT)-1;
		do{
			if ( DELTAT->now_addr > (DELTAT->end<<1) ) {
			    if( DELTAT->portstate&0x10 ){
					/**** repeat start ****/
					DELTAT->now_addr = DELTAT->start<<1;
					DELTAT->adpcmx   = 0;
					DELTAT->adpcmd   = YM_DELTAT_DELTA_DEF;
					DELTAT->next_leveling = 0;
					DELTAT->flag     = 1;
					DELTAT->eos      = 0;

				}else{
					DELTAT->arrivedFlag |= DELTAT->flagMask;
					DELTAT->flag = 0;
					DELTAT->eos  = 1;
					DELTAT->adpcml = 0;
					now_leveling = 0;
					return;
				}
			}
			if( DELTAT->now_addr&1 ) data = DELTAT->now_data & 0x0f;
			else
			{
				DELTAT->now_data = *(ym_deltat_memory+(DELTAT->now_addr>>1));
				data = DELTAT->now_data >> 4;
			}
			DELTAT->now_addr++;
			/* shift Measurement value */
			old_m      = DELTAT->adpcmx/*adpcmm*/;
			/* ch->adpcmm = YM_DELTAT_Limit( ch->adpcmx + (decode_tableB3[data] * ch->adpcmd / 8) ,YM_DELTAT_DECODE_MAX, YM_DELTAT_DECODE_MIN ); */
			/* Forecast to next Forecast */
			DELTAT->adpcmx += (ym_deltat_decode_tableB1[data] * DELTAT->adpcmd / 8);
			YM_DELTAT_Limit(DELTAT->adpcmx,YM_DELTAT_DECODE_MAX, YM_DELTAT_DECODE_MIN);
			/* delta to next delta */
			DELTAT->adpcmd = (DELTAT->adpcmd * ym_deltat_decode_tableB2[data] ) / 64;
			YM_DELTAT_Limit(DELTAT->adpcmd,YM_DELTAT_DELTA_MAX, YM_DELTAT_DELTA_MIN );
			/* shift leveling value */
			delta_next        = DELTAT->adpcmx/*adpcmm*/ - old_m;
			now_leveling      = DELTAT->next_leveling;
			DELTAT->next_leveling = old_m + (delta_next / 2);
		}while(--step);
/*#define YM_DELTAT_CUT_RE_SAMPLING */
#ifdef YM_DELTAT_CUT_RE_SAMPLING
		DELTAT->adpcml  = DELTAT->next_leveling * DELTAT->volume;
		DELTAT->adpcml  = DELTAT->adpcmx/*adpcmm*/ * DELTAT->volume;
	}
#else
		/* delta step of re-sampling */
		DELTAT->sample_step = (DELTAT->next_leveling - now_leveling) * DELTAT->volume_w_step;
		/* output of start point */
		DELTAT->adpcml  = now_leveling * DELTAT->volume;
		/* adjust to now */
		DELTAT->adpcml += (int)((DoubleT)DELTAT->sample_step * ((DoubleT)DELTAT->now_step/(DoubleT)DELTAT->step));
	}
	DELTAT->adpcml += DELTAT->sample_step;
#endif
	/* output for work of output channels (outd[OPNxxxx])*/
	*(DELTAT->pan) += DELTAT->adpcml;
}


void YM_DELTAT_ADPCM_LoadState(YM_DELTAT *DELTAT)
{
    SaveState* state = saveStateOpenForRead("ymdeltat");

    DELTAT->memory_size   = saveStateGet(state, "memory_size",   0);
    DELTAT->output_range  = saveStateGet(state, "output_range",  0);
    DELTAT->portstate     = (UInt8)saveStateGet(state, "portstate",     0);
    DELTAT->portcontrol   = (UInt8)saveStateGet(state, "portcontrol",   0);
    DELTAT->portshift     = saveStateGet(state, "portshift",     0);
    DELTAT->memread       = saveStateGet(state, "memread",       0);
    
    DELTAT->flag          = (UInt8)saveStateGet(state, "flag",          0);
    DELTAT->eos           = (UInt8)saveStateGet(state, "eos",           0);
    DELTAT->flagMask      = (UInt8)saveStateGet(state, "flagMask",      0);
    DELTAT->now_data      = (UInt8)saveStateGet(state, "now_data",      0);
    DELTAT->now_addr      = saveStateGet(state, "now_addr",      0);
    DELTAT->now_step      = saveStateGet(state, "now_step",      0);
    DELTAT->step          = saveStateGet(state, "step",          0);
    DELTAT->start         = saveStateGet(state, "start",         0);
    DELTAT->end           = saveStateGet(state, "end",           0);
    DELTAT->read_pointer  = saveStateGet(state, "read_pointer",  0);
    DELTAT->write_pointer = saveStateGet(state, "write_pointer", 0);
    DELTAT->delta         = saveStateGet(state, "delta",         0);
    DELTAT->volume        = saveStateGet(state, "volume",        0);
    DELTAT->adpcmx        = saveStateGet(state, "adpcmx",        0);
    DELTAT->adpcmd        = saveStateGet(state, "adpcmd",        0);
    DELTAT->adpcml        = saveStateGet(state, "adpcml",        0);
    
    DELTAT->volume_w_step = saveStateGet(state, "volume_w_step", 0);
    DELTAT->next_leveling = saveStateGet(state, "next_leveling", 0);
    DELTAT->sample_step   = saveStateGet(state, "sample_step",   0);
    DELTAT->arrivedFlag   = (UInt8)saveStateGet(state, "arrivedFlag",   0);

    saveStateGetBuffer(state, "memory",  DELTAT->memory, DELTAT->memory_size);
    saveStateGetBuffer(state, "reg",     DELTAT->reg, sizeof(DELTAT->reg));

    saveStateClose(state);

	DELTAT->pan = &DELTAT->output_pointer[(DELTAT->portcontrol>>6)&0x03];
}

void YM_DELTAT_ADPCM_SaveState(YM_DELTAT *DELTAT)
{
    SaveState* state = saveStateOpenForWrite("ymdeltat");

    saveStateSet(state, "memory_size",   DELTAT->memory_size);
    saveStateSet(state, "output_range",  DELTAT->output_range);
    saveStateSet(state, "portstate",     DELTAT->portstate);
    saveStateSet(state, "portcontrol",   DELTAT->portcontrol);
    saveStateSet(state, "portshift",     DELTAT->portshift);
    saveStateSet(state, "memread",       DELTAT->memread);
    
    saveStateSet(state, "flag",          DELTAT->flag);
    saveStateSet(state, "eos",           DELTAT->eos);
    saveStateSet(state, "flagMask",      DELTAT->flagMask);
    saveStateSet(state, "now_data",      DELTAT->now_data);
    saveStateSet(state, "now_addr",      DELTAT->now_addr);
    saveStateSet(state, "now_step",      DELTAT->now_step);
    saveStateSet(state, "step",          DELTAT->step);
    saveStateSet(state, "start",         DELTAT->start);
    saveStateSet(state, "end",           DELTAT->end);
    saveStateSet(state, "read_pointer",  DELTAT->read_pointer);
    saveStateSet(state, "write_pointer", DELTAT->write_pointer);
    saveStateSet(state, "delta",         DELTAT->delta);
    saveStateSet(state, "volume",        DELTAT->volume);
    saveStateSet(state, "adpcmx",        DELTAT->adpcmx);
    saveStateSet(state, "adpcmd",        DELTAT->adpcmd);
    saveStateSet(state, "adpcml",        DELTAT->adpcml);
    
    saveStateSet(state, "volume_w_step", DELTAT->volume_w_step);
    saveStateSet(state, "next_leveling", DELTAT->next_leveling);
    saveStateSet(state, "sample_step",   DELTAT->sample_step);
    saveStateSet(state, "arrivedFlag",   DELTAT->arrivedFlag);

    saveStateSetBuffer(state, "memory",  DELTAT->memory, DELTAT->memory_size);
    saveStateSetBuffer(state, "reg",     DELTAT->reg, sizeof(DELTAT->reg));

    saveStateClose(state);
}

