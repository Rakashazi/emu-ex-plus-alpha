// The file	has	been modified to be	built in the blueMSX environment.

/*
**
** File: fmopl.c --	software implementation	of FM sound	generator
**
** Copyright (C) 1999,2000 Tatsuyuki Satoh 
**
** 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "Fmopl.h"
#include "Switches.h"
#include "SaveState.h"
#include "Board.h"

#ifndef	PI
#define	PI 3.14159265358979323846
#endif

extern void	y8950TimerSet(void* ref, int timer, int count);
extern void	y8950TimerStart(void* ref, int timer, int start);
extern int  y8950GetNoteOn(void* ref, int row);

/* --------------------	preliminary	define section --------------------- */
/* attack/decay	rate time rate */
#define	OPL_ARRATE	   141280  /* RATE 4 =	2826.24ms @	3.6MHz */
#define	OPL_DRRATE	  1956000  /* RATE 4 = 39280.64ms @	3.6MHz */

#define	DELTAT_MIXING_LEVEL	(1)	/* DELTA-T ADPCM MIXING	LEVEL */

#define	FREQ_BITS 24			/* frequency turn		   */

/* counter bits	= 20 , octerve 7 */
#define	FREQ_RATE	(1<<(FREQ_BITS-20))
#define	TL_BITS	   (FREQ_BITS+2)

/* final output	shift ,	limit minimum and maximum */
#define	OPL_OUTSB	(TL_BITS+3-16)		/* OPL output final	shift 16bit	*/
#define	OPL_MAXOUT (0x7fff<<OPL_OUTSB)
#define	OPL_MINOUT (-0x8000<<OPL_OUTSB)

/* --------------------	quality	selection ---------------------	*/

/* sinwave entries */
/* used	static memory =	SIN_ENT	* 4	(byte) */
#define	SIN_ENT	2048

/* output level	entries	(envelope,sinwave) */
/* envelope	counter	lower bits */
#define	ENV_BITS 16
/* envelope	output entries */
#define	EG_ENT	 4096
/* used	dynamic	memory = EG_ENT*4*4(byte)or	EG_ENT*6*4(byte) */
/* used	static	memory = EG_ENT*4 (byte)					 */

#define	EG_OFF	 ((2*EG_ENT)<<ENV_BITS)	 /*	OFF			 */
#define	EG_DED	 EG_OFF
#define	EG_DST	 (EG_ENT<<ENV_BITS)		 /*	DECAY  START */
#define	EG_AED	 EG_DST
#define	EG_AST	 0						 /*	ATTACK START */

#define	EG_STEP	(96.0/EG_ENT) /* OPL is	0.1875 dB step	*/

/* LFO table entries */
#define	VIB_ENT	512
#define	VIB_SHIFT (32-9)
#define	AMS_ENT	512
#define	AMS_SHIFT (32-9)

#define	VIB_RATE 256

/* --------------------	local defines ,	macros --------------------- */

/* register	number to channel number , slot	offset */
#define	SLOT1 0
#define	SLOT2 1

/* envelope	phase */
#define	ENV_MOD_RR	0x00
#define	ENV_MOD_DR	0x01
#define	ENV_MOD_AR	0x02

/* --------------------	tables --------------------- */
static const int slot_array[32]=
{
	0, 2, 4, 1,	3, 5,-1,-1,
		6, 8,10, 7,	9,11,-1,-1,
		12,14,16,13,15,17,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1
};

/* key scale level */
/* table is	3dB/OCT	, DV converts this in TL step at 6dB/OCT */
#define	DV (EG_STEP/2)
static const UINT32	KSL_TABLE[8*16]=
{
	/* OCT 0 */
	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		/* OCT 1 */
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(0.750/DV),	(UINT32)(1.125/DV),	(UINT32)(1.500/DV),
		(UINT32)(1.875/DV),	(UINT32)(2.250/DV),	(UINT32)(2.625/DV),	(UINT32)(3.000/DV),
		/* OCT 2 */
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),
		(UINT32)(0.000/DV),	(UINT32)(1.125/DV),	(UINT32)(1.875/DV),	(UINT32)(2.625/DV),
		(UINT32)(3.000/DV),	(UINT32)(3.750/DV),	(UINT32)(4.125/DV),	(UINT32)(4.500/DV),
		(UINT32)(4.875/DV),	(UINT32)(5.250/DV),	(UINT32)(5.625/DV),	(UINT32)(6.000/DV),
		/* OCT 3 */
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(1.875/DV),
		(UINT32)(3.000/DV),	(UINT32)(4.125/DV),	(UINT32)(4.875/DV),	(UINT32)(5.625/DV),
		(UINT32)(6.000/DV),	(UINT32)(6.750/DV),	(UINT32)(7.125/DV),	(UINT32)(7.500/DV),
		(UINT32)(7.875/DV),	(UINT32)(8.250/DV),	(UINT32)(8.625/DV),	(UINT32)(9.000/DV),
		/* OCT 4 */
		(UINT32)(0.000/DV),	(UINT32)(0.000/DV),	(UINT32)(3.000/DV),	(UINT32)(4.875/DV),
		(UINT32)(6.000/DV),	(UINT32)(7.125/DV),	(UINT32)(7.875/DV),	(UINT32)(8.625/DV),
		(UINT32)(9.000/DV),	(UINT32)(9.750/DV),(UINT32)(10.125/DV),(UINT32)(10.500/DV),
		(UINT32)(10.875/DV),(UINT32)(11.250/DV),(UINT32)(11.625/DV),(UINT32)(12.000/DV),
		/* OCT 5 */
		(UINT32)(0.000/DV),	(UINT32)(3.000/DV),	(UINT32)(6.000/DV),	(UINT32)(7.875/DV),
		(UINT32)(9.000/DV),(UINT32)(10.125/DV),(UINT32)(10.875/DV),(UINT32)(11.625/DV),
		(UINT32)(12.000/DV),(UINT32)(12.750/DV),(UINT32)(13.125/DV),(UINT32)(13.500/DV),
		(UINT32)(13.875/DV),(UINT32)(14.250/DV),(UINT32)(14.625/DV),(UINT32)(15.000/DV),
		/* OCT 6 */
		(UINT32)(0.000/DV),	(UINT32)(6.000/DV),	(UINT32)(9.000/DV),(UINT32)(10.875/DV),
		(UINT32)(12.000/DV),(UINT32)(13.125/DV),(UINT32)(13.875/DV),(UINT32)(14.625/DV),
		(UINT32)(15.000/DV),(UINT32)(15.750/DV),(UINT32)(16.125/DV),(UINT32)(16.500/DV),
		(UINT32)(16.875/DV),(UINT32)(17.250/DV),(UINT32)(17.625/DV),(UINT32)(18.000/DV),
		/* OCT 7 */
		(UINT32)(0.000/DV),	(UINT32)(9.000/DV),(UINT32)(12.000/DV),(UINT32)(13.875/DV),
		(UINT32)(15.000/DV),(UINT32)(16.125/DV),(UINT32)(16.875/DV),(UINT32)(17.625/DV),
		(UINT32)(18.000/DV),(UINT32)(18.750/DV),(UINT32)(19.125/DV),(UINT32)(19.500/DV),
		(UINT32)(19.875/DV),(UINT32)(20.250/DV),(UINT32)(20.625/DV),(UINT32)(21.000/DV)
};
#undef DV

/* sustain lebel table (3db	per	step) */
/* 0 - 15: 0, 3, 6,	9,12,15,18,21,24,27,30,33,36,39,42,93 (dB)*/
#define	SC(db) (INT32)((db*((3/EG_STEP)*(1<<ENV_BITS)))+EG_DST)
static const INT32 SL_TABLE[16]={
	SC(	0),SC( 1),SC( 2),SC(3 ),SC(4 ),SC(5	),SC(6 ),SC( 7),
		SC(	8),SC( 9),SC(10),SC(11),SC(12),SC(13),SC(14),SC(31)
};
#undef SC

#define	TL_MAX (EG_ENT*2) /* limit(tl +	ksr	+ envelope)	+ sinwave */
/* TotalLevel :	48 24 12  6	 3 1.5 0.75	(dB) */
/* TL_TABLE[ 0		to TL_MAX		   ] : plus	 section */
/* TL_TABLE[ TL_MAX	to TL_MAX+TL_MAX-1 ] : minus section */
static INT32 *TL_TABLE;

/* pointers	to TL_TABLE	with sinwave output	offset */
static INT32 **SIN_TABLE;

/* LFO table */
INT32 *AMS_TABLE;
INT32 *VIB_TABLE;

/* envelope	output curve table */
/* attack +	decay +	OFF	*/
static INT32 ENV_CURVE[2*EG_ENT+1];

/* multiple	table */
#define	ML 2
static const UINT32	MUL_TABLE[16]= {
	/* 1/2,	1, 2, 3, 4,	5, 6, 7, 8,	9,10,11,12,13,14,15	*/
	(UINT32)(0.50*ML), (UINT32)(1.00*ML), (UINT32)(2.00*ML), (UINT32)(3.00*ML),	
		(UINT32)(4.00*ML), (UINT32)(5.00*ML), (UINT32)(6.00*ML), (UINT32)(7.00*ML),
		(UINT32)(8.00*ML), (UINT32)(9.00*ML),(UINT32)(10.00*ML),(UINT32)(10.00*ML),
		(UINT32)(12.00*ML),(UINT32)(12.00*ML),(UINT32)(15.00*ML),(UINT32)(15.00*ML)
};
#undef ML

/* dummy attack	/ decay	rate ( when	rate ==	0 )	*/
static INT32 RATE_0[16]=
{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/* --------------------	static state --------------------- */

/* lock	level of common	table */

/* work	table */
void *cur_chip = NULL;	/* current chip	point */
/* currenct	chip state */
/* static OPLSAMPLE	 *bufL,*bufR; */
OPL_CH *S_CH;
OPL_CH *E_CH;
OPL_SLOT *SLOT7_1,*SLOT7_2,*SLOT8_1,*SLOT8_2;

int	num_lock = 0;
INT32  *ams_table;
INT32  *vib_table;
INT32 amsIncr;
INT32 vibIncr;
INT32 outd;
INT32 ams;
INT32 vib;
INT32 feedback2;		/* connect for SLOT	2 */

/* --------------------- subroutines  ---------------------	*/

int	Limit( int val,	int	max, int min ) {
	if ( val > max )
		val	= max;
	else if	( val <	min	)
		val	= min;

	return val;
}

/* status set and IRQ handling */
void OPL_STATUS_SET(FM_OPL *OPL,int	flag)
{
	/* set status flag */
	OPL->status	|= flag;

	if(!(OPL->status & 0x80))
	{
		if(OPL->status & OPL->statusmask)
		{	/* IRQ on */
			OPL->status	|= 0x80;
			boardSetInt(0x10);
		}
	}
}

/* status reset	and	IRQ	handling */
void OPL_STATUS_RESET(FM_OPL *OPL,int flag)
{
	/* reset status	flag */
	OPL->status	&=~flag;
	if((OPL->status	& 0x80))
	{
		if (!(OPL->status &	OPL->statusmask) )
		{
			OPL->status	&= 0x7f;
			boardClearInt(0x10);
		}
	}
}

/* IRQ mask	set	*/
void OPL_STATUSMASK_SET(FM_OPL *OPL,int	flag)
{
	OPL->statusmask	= flag;
	/* IRQ handling	check */
	OPL_STATUS_SET(OPL,0);
	OPL_STATUS_RESET(OPL,0);
}

/* ----- key on	 ----- */
void OPL_KEYON(OPL_SLOT	*SLOT)
{
	/* sin wave	restart	*/
	SLOT->Cnt =	0;
	/* set attack */
	SLOT->evm =	ENV_MOD_AR;
	SLOT->evs =	SLOT->evsa;
	SLOT->evc =	EG_AST;
	SLOT->eve =	EG_AED;
}
/* ----- key off ----- */
void OPL_KEYOFF(OPL_SLOT *SLOT)
{
	if(	SLOT->evm >	ENV_MOD_RR)
	{
		/* set envelope	counter	from envleope output */
		SLOT->evm =	ENV_MOD_RR;
		if(	!(SLOT->evc&EG_DST)	)
			/*SLOT->evc	= (ENV_CURVE[SLOT->evc>>ENV_BITS]<<ENV_BITS) + EG_DST; */
			SLOT->evc =	EG_DST;
		SLOT->eve =	EG_DED;
		SLOT->evs =	SLOT->evsr;
	}
}

/* ---------- calcrate Envelope	Generator &	Phase Generator	---------- */
/* return :	envelope output	*/
UINT32 OPL_CALC_SLOT( OPL_SLOT *SLOT )
{
	/* calcrate	envelope generator */
	if(	(SLOT->evc+=SLOT->evs) >= SLOT->eve	)
	{
		switch(	SLOT->evm ){
		case ENV_MOD_AR: /*	ATTACK -> DECAY1 */
			/* next	DR */
			SLOT->evm =	ENV_MOD_DR;
			SLOT->evc =	EG_DST;
			SLOT->eve =	SLOT->SL;
			SLOT->evs =	SLOT->evsd;
			break;
		case ENV_MOD_DR: /*	DECAY -> SL	or RR */
			SLOT->evc =	SLOT->SL;
			SLOT->eve =	EG_DED;
			if(SLOT->eg_typ)
			{
				SLOT->evs =	0;
			}
			else
			{
				SLOT->evm =	ENV_MOD_RR;
				SLOT->evs =	SLOT->evsr;
			}
			break;
		case ENV_MOD_RR: /*	RR -> OFF */
			SLOT->evc =	EG_OFF;
			SLOT->eve =	EG_OFF+1;
			SLOT->evs =	0;
			break;
		}
	}
	/* calcrate	envelope */
	return SLOT->TLL+ENV_CURVE[SLOT->evc>>ENV_BITS]+(SLOT->ams ? ams : 0);
}

/* ---------- frequency	counter	for	operater update	---------- */
void CALC_FCSLOT(FM_OPL	*OPL,OPL_CH	*CH,OPL_SLOT *SLOT)
{
	int	ksr;

	/* frequency step counter */
	SLOT->Incr = CH->fc	* SLOT->mul;
	ksr	= CH->kcode	>> SLOT->KSR;

	if(	SLOT->ksr != ksr )
	{
		SLOT->ksr =	ksr;
		/* attack ,	decay rate recalcration	*/
		SLOT->evsa = (SLOT->AR ? &OPL->AR_TABLE[SLOT->AR<<2] : RATE_0)[ksr];
		SLOT->evsd = (SLOT->DR ? &OPL->DR_TABLE[SLOT->DR<<2] : RATE_0)[ksr];
		SLOT->evsr = (&OPL->DR_TABLE[SLOT->RR])[ksr];
	}
	SLOT->TLL =	SLOT->TL + (CH->ksl_base>>SLOT->ksl);
}

/* set multi,am,vib,EG-TYP,KSR,mul */
void set_mul(FM_OPL	*OPL,int slot,int v)
{
	OPL_CH	 *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];

	SLOT->mul	 = MUL_TABLE[v&0x0f];
	SLOT->KSR	 = (v&0x10)	? 0	: 2;
	SLOT->eg_typ = (v&0x20)>>5;
	SLOT->vib	 = (v&0x40);
	SLOT->ams	 = (v&0x80);
	CALC_FCSLOT(OPL,CH,SLOT);
}

/* set ksl & tl	*/
void set_ksl_tl(FM_OPL *OPL,int	slot,int v)
{
	OPL_CH	 *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int	ksl	= v>>6;	/* 0 / 1.5 / 3 / 6 db/OCT */

	SLOT->ksl =	ksl	? 3-ksl	: 31;
	SLOT->TL  =	(INT32)((v&0x3f)*(0.75/EG_STEP)); /* 0.75db	step */

	if(	!(OPL->mode&0x80) )
	{	/* not CSM latch total level */
		SLOT->TLL =	SLOT->TL + (CH->ksl_base>>SLOT->ksl);
	}
}

/* set attack rate & decay rate	 */
void set_ar_dr(FM_OPL *OPL,int slot,int	v)
{
	OPL_CH	 *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int	ar = v>>4;
	int	dr = v&0x0f;

	SLOT->AR = ar;
	SLOT->evsa = (SLOT->AR ? &OPL->AR_TABLE[SLOT->AR<<2] : RATE_0)[SLOT->ksr];
	if(	SLOT->evm == ENV_MOD_AR	) SLOT->evs	= SLOT->evsa;

	SLOT->DR = dr;
	SLOT->evsd = (SLOT->DR ? &OPL->DR_TABLE[SLOT->DR<<2] : RATE_0)[SLOT->ksr];
	if(	SLOT->evm == ENV_MOD_DR	) SLOT->evs	= SLOT->evsd;
}

/* set sustain level & release rate	*/
void set_sl_rr(FM_OPL *OPL,int slot,int	v)
{
	OPL_CH	 *CH   = &OPL->P_CH[slot/2];
	OPL_SLOT *SLOT = &CH->SLOT[slot&1];
	int	sl = v>>4;
	int	rr = v & 0x0f;

	SLOT->SL = SL_TABLE[sl];
	if(	SLOT->evm == ENV_MOD_DR	) SLOT->eve	= SLOT->SL;
	SLOT->RR = rr << 2;
	SLOT->evsr = (&OPL->DR_TABLE[SLOT->RR])[SLOT->ksr];
	if(	SLOT->evm == ENV_MOD_RR	) SLOT->evs	= SLOT->evsr;
}

/* operator	output calcrator */
#define	OP_OUT(slot,env,con)   (&SIN_TABLE[slot->wavetableidx])[((slot->Cnt+con)/(0x1000000/SIN_ENT))&(SIN_ENT-1)][env]
/* ---------- calcrate one of channel ---------- */
void OPL_CALC_CH( OPL_CH *CH )
{
	UINT32 env_out;
	OPL_SLOT *SLOT;

	feedback2 =	0;
	/* SLOT	1 */
	SLOT = &CH->SLOT[SLOT1];
	env_out=OPL_CALC_SLOT(SLOT);
	if(	env_out	< EG_ENT-1 )
	{
		/* PG */
		if(SLOT->vib) SLOT->Cnt	+= (SLOT->Incr*vib/VIB_RATE);
		else		  SLOT->Cnt	+= SLOT->Incr;
		/* connectoion */
		if(CH->FB)
		{
			int	feedback1 =	(CH->op1_out[0]+CH->op1_out[1])>>CH->FB;
			CH->op1_out[1] = CH->op1_out[0];
			*(CH->CON ?	&outd :	&feedback2)	+= CH->op1_out[0] =	OP_OUT(SLOT,env_out,feedback1);
		}
		else
		{
			*(CH->CON ?	&outd :	&feedback2)	+= OP_OUT(SLOT,env_out,0);
		}
	}else
	{
		CH->op1_out[1] = CH->op1_out[0];
		CH->op1_out[0] = 0;
	}
	/* SLOT	2 */
	SLOT = &CH->SLOT[SLOT2];
	env_out=OPL_CALC_SLOT(SLOT);
	if(	env_out	< EG_ENT-1 )
	{
		/* PG */
		if(SLOT->vib) SLOT->Cnt	+= (SLOT->Incr*vib/VIB_RATE);
		else		  SLOT->Cnt	+= SLOT->Incr;
		/* connectoion */
		outd +=	OP_OUT(SLOT,env_out, feedback2);
	}
}

/* ---------- calcrate rythm block ---------- */
#define	WHITE_NOISE_db 6.0
void OPL_CALC_RH( OPL_CH *CH )
{
	UINT32 env_tam,env_sd,env_top,env_hh;
	int	whitenoise = (int)((rand()&1)*(WHITE_NOISE_db/EG_STEP));
	INT32 tone8;

	OPL_SLOT *SLOT;
	int	env_out;

	/* BD :	same as	FM serial mode and output level	is large */
	feedback2 =	0;
	/* SLOT	1 */
	SLOT = &CH[6].SLOT[SLOT1];
	env_out=OPL_CALC_SLOT(SLOT);
	if(	env_out	< EG_ENT-1 )
	{
		/* PG */
		if(SLOT->vib) SLOT->Cnt	+= (SLOT->Incr*vib/VIB_RATE);
		else		  SLOT->Cnt	+= SLOT->Incr;
		/* connectoion */
		if(CH[6].FB)
		{
			int	feedback1 =	(CH[6].op1_out[0]+CH[6].op1_out[1])>>CH[6].FB;
			CH[6].op1_out[1] = CH[6].op1_out[0];
			feedback2 =	CH[6].op1_out[0] = OP_OUT(SLOT,env_out,feedback1);
		}
		else
		{
			feedback2 =	OP_OUT(SLOT,env_out,0);
		}
	}else
	{
		feedback2 =	0;
		CH[6].op1_out[1] = CH[6].op1_out[0];
		CH[6].op1_out[0] = 0;
	}
	/* SLOT	2 */
	SLOT = &CH[6].SLOT[SLOT2];
	env_out=OPL_CALC_SLOT(SLOT);
	if(	env_out	< EG_ENT-1 )
	{
		/* PG */
		if(SLOT->vib) SLOT->Cnt	+= (SLOT->Incr*vib/VIB_RATE);
		else		  SLOT->Cnt	+= SLOT->Incr;
		/* connectoion */
		outd +=	OP_OUT(SLOT,env_out, feedback2)*2;
	}

	/* SD  (17)	= mul14[fnum7] + white noise */
	/* TAM (15)	= mul15[fnum8] */
	/* TOP (18)	= fnum6(mul18[fnum8]+whitenoise) */
	/* HH  (14)	= fnum7(mul18[fnum8]+whitenoise) + white noise */
	env_sd =OPL_CALC_SLOT(SLOT7_2) + whitenoise;
	env_tam=OPL_CALC_SLOT(SLOT8_1);
	env_top=OPL_CALC_SLOT(SLOT8_2);
	env_hh =OPL_CALC_SLOT(SLOT7_1) + whitenoise;

	/* PG */
	if(SLOT7_1->vib) SLOT7_1->Cnt += (2*SLOT7_1->Incr*vib/VIB_RATE);
	else			 SLOT7_1->Cnt += 2*SLOT7_1->Incr;
	if(SLOT7_2->vib) SLOT7_2->Cnt += ((CH[7].fc*8)*vib/VIB_RATE);
	else			 SLOT7_2->Cnt += (CH[7].fc*8);
	if(SLOT8_1->vib) SLOT8_1->Cnt += (SLOT8_1->Incr*vib/VIB_RATE);
	else			 SLOT8_1->Cnt += SLOT8_1->Incr;
	if(SLOT8_2->vib) SLOT8_2->Cnt += ((CH[8].fc*48)*vib/VIB_RATE);
	else			 SLOT8_2->Cnt += (CH[8].fc*48);

	tone8 =	OP_OUT(SLOT8_2,whitenoise,0	);

	/* SD */
	if(	env_sd < EG_ENT-1 )
		outd +=	OP_OUT(SLOT7_1,env_sd, 0)*8;
	/* TAM */
	if(	env_tam	< EG_ENT-1 )
		outd +=	OP_OUT(SLOT8_1,env_tam,	0)*2;
	/* TOP-CY */
	if(	env_top	< EG_ENT-1 )
		outd +=	OP_OUT(SLOT7_2,env_top,tone8)*2;
	/* HH */
	if(	env_hh	< EG_ENT-1 )
		outd +=	OP_OUT(SLOT7_2,env_hh,tone8)*2;
}

/* ----------- initialize time tabls ----------- */
static void	init_timetables( FM_OPL	*OPL , int ARRATE ,	int	DRRATE )
{
	int	i;
	DoubleT rate;

	/* make	attack rate	& decay	rate tables	*/
	for	(i = 0;i < 4;i++) OPL->AR_TABLE[i] = OPL->DR_TABLE[i] =	0;
	for	(i = 4;i <=	60;i++){
		rate  =	OPL->freqbase;						/* frequency rate */
		if(	i <	60 ) rate *= 1.0+(i&3)*0.25;		/* b0-1	: x1 , x1.25 , x1.5	, x1.75	*/
		rate *=	1<<((i>>2)-1);						/* b2-5	: shift	bit	*/
		rate *=	(DoubleT)(EG_ENT<<ENV_BITS);
		OPL->AR_TABLE[i] = (INT32)(rate	/ ARRATE);
		OPL->DR_TABLE[i] = (INT32)(rate	/ DRRATE);
	}
	for	(i = 60;i <	76;i++)
	{
		OPL->AR_TABLE[i] = EG_AED-1;
		OPL->DR_TABLE[i] = OPL->DR_TABLE[60];
	}
}

/* ---------- generic table	initialize ---------- */
static int OPLOpenTable( void )
{
	int	s,t;
	DoubleT rate;
	int	i,j;
	DoubleT pom;

	/* allocate	dynamic	tables */
	if(	(TL_TABLE =	malloc(TL_MAX*2*sizeof(INT32)))	== NULL)
		return 0;
	if(	(SIN_TABLE = malloc(SIN_ENT*4 *sizeof(INT32	*))) ==	NULL)
	{
		free(TL_TABLE);
		return 0;
	}
	if(	(AMS_TABLE = malloc(AMS_ENT*2 *sizeof(INT32))) == NULL)
	{
		free(TL_TABLE);
		free(SIN_TABLE);
		return 0;
	}
	if(	(VIB_TABLE = malloc(VIB_ENT*2 *sizeof(INT32))) == NULL)
	{
		free(TL_TABLE);
		free(SIN_TABLE);
		free(AMS_TABLE);
		return 0;
	}
	/* make	total level	table */
	for	(t = 0;t < EG_ENT-1	;t++){
		rate = ((1<<TL_BITS)-1)/pow(10,EG_STEP*t/20);	/* dB -> voltage */
		TL_TABLE[		t] =  (int)rate;
		TL_TABLE[TL_MAX+t] = -TL_TABLE[t];
	}
	/* fill	volume off area	*/
	for	( t	= EG_ENT-1;	t <	TL_MAX ;t++){
		TL_TABLE[t]	= TL_TABLE[TL_MAX+t] = 0;
	}

	/* make	sinwave	table (total level offet) */
	/* degree 0	= degree 180				   = off */
	SIN_TABLE[0] = SIN_TABLE[SIN_ENT/2]			= &TL_TABLE[EG_ENT-1];
	for	(s = 1;s <=	SIN_ENT/4;s++){
		pom	= sin(2*PI*s/SIN_ENT); /* sin	  */
		pom	= 20*log10(1/pom);	   /* decibel */
		j =	(int)(pom /	EG_STEP);		  /* TL_TABLE steps	*/

		/* degree 0	  -	 90	   , degree	180	-  90 :	plus section */
		SIN_TABLE[			s] = SIN_TABLE[SIN_ENT/2-s]	= &TL_TABLE[j];
		/* degree 180 -	270	   , degree	360	- 270 :	minus section */
		SIN_TABLE[SIN_ENT/2+s] = SIN_TABLE[SIN_ENT	-s]	= &TL_TABLE[TL_MAX+j];
	}
	for	(s = 0;s < SIN_ENT;s++)
	{
		SIN_TABLE[SIN_ENT*1+s] = s<(SIN_ENT/2) ? SIN_TABLE[s] :	&TL_TABLE[EG_ENT];
		SIN_TABLE[SIN_ENT*2+s] = SIN_TABLE[s % (SIN_ENT/2)];
		SIN_TABLE[SIN_ENT*3+s] = (s/(SIN_ENT/4))&1 ? &TL_TABLE[EG_ENT] : SIN_TABLE[SIN_ENT*2+s];
	}

	/* envelope	counter	-> envelope	output table */
	for	(i=0; i<EG_ENT;	i++)
	{
		/* ATTACK curve	*/
		pom	= pow( ((DoubleT)(EG_ENT-1-i)/EG_ENT) , 8 ) * EG_ENT;
		/* if( pom >= EG_ENT ) pom = EG_ENT-1; */
		ENV_CURVE[i] = (int)pom;
		/* DECAY ,RELEASE curve	*/
		ENV_CURVE[(EG_DST>>ENV_BITS)+i]= i;
	}
	/* off */
	ENV_CURVE[EG_OFF>>ENV_BITS]= EG_ENT-1;
	/* make	LFO	ams	table */
	for	(i=0; i<AMS_ENT; i++)
	{
		pom	= (1.0+sin(2*PI*i/AMS_ENT))/2; /* sin */
		AMS_TABLE[i]		 = (INT32)((1.0/EG_STEP)*pom); /* 1dB	*/
		AMS_TABLE[AMS_ENT+i] = (INT32)((4.8/EG_STEP)*pom); /* 4.8dB	*/
	}
	/* make	LFO	vibrate	table */
	for	(i=0; i<VIB_ENT; i++)
	{
		/* 100cent = 1seminote = 6%	?? */
		pom	= (DoubleT)VIB_RATE*0.06*sin(2*PI*i/VIB_ENT); /*	+-100sect step */
		VIB_TABLE[i]		 = (INT32)(VIB_RATE	+ (pom*0.07)); /* +- 7cent */
		VIB_TABLE[VIB_ENT+i] = (INT32)(VIB_RATE	+ (pom*0.14)); /* +-14cent */
	}
	return 1;
}


static void	OPLCloseTable( void	)
{
	free(TL_TABLE);
	free(SIN_TABLE);
	free(AMS_TABLE);
	free(VIB_TABLE);
}

/* CSM Key Controll	*/
void CSMKeyControll(OPL_CH *CH)
{
	OPL_SLOT *slot1	= &CH->SLOT[SLOT1];
	OPL_SLOT *slot2	= &CH->SLOT[SLOT2];
	/* all key off */
	OPL_KEYOFF(slot1);
	OPL_KEYOFF(slot2);
	/* total level latch */
	slot1->TLL = slot1->TL + (CH->ksl_base>>slot1->ksl);
	slot1->TLL = slot1->TL + (CH->ksl_base>>slot1->ksl);
	/* key on */
	CH->op1_out[0] = CH->op1_out[1]	= 0;
	OPL_KEYON(slot1);
	OPL_KEYON(slot2);
}

/* ---------- opl initialize ----------	*/
static void	OPL_initalize(FM_OPL *OPL)
{
	int	fn;

#if 0
	/* frequency base */
	OPL->freqbase =	(OPL->rate)	? ((DoubleT)OPL->clock /	OPL->rate) / 72	 : 0;
	/* Timer base time */
	OPL->TimerBase = 1.0/((DoubleT)OPL->clock / 72.0	);
#else
    if (OPL->baseRate == OPL->clock / 72) {
	    OPL->freqbase =	OPL->baseRate / OPL->rate;
	    OPL->TimerBase = 1.0 / OPL->baseRate;
    }
    else {
	    OPL->freqbase =	(OPL->rate)	? ((DoubleT)OPL->clock /	OPL->rate) / 72	 : 0;
	    OPL->TimerBase = 1.0/((DoubleT)OPL->clock / 72.0	);
    }
#endif
	/* make	time tables	*/
	init_timetables( OPL , OPL_ARRATE ,	OPL_DRRATE );
	/* make	fnumber	-> increment counter table */
	for( fn=0 ;	fn < 1024 ;	fn++ )
	{
		OPL->FN_TABLE[fn] =	(UINT32)(OPL->freqbase * fn	* FREQ_RATE	* (1<<7) / 2);
	}
	/* LFO freq.table */
	OPL->amsIncr = (INT32)(OPL->rate ? (DoubleT)AMS_ENT*(1<<AMS_SHIFT) /	OPL->rate *	3.7	* ((DoubleT)OPL->clock/3600000) : 0);
	OPL->vibIncr = (INT32)(OPL->rate ? (DoubleT)VIB_ENT*(1<<VIB_SHIFT) /	OPL->rate *	6.4	* ((DoubleT)OPL->clock/3600000) : 0);
}

/* ---------- write	a OPL registers	---------- */
void OPLWriteReg(FM_OPL	*OPL, int r, int v)
{
	OPL_CH *CH;
	int	slot;
	int	block_fnum;

    OPL->regs[r&0xff] = v;

	switch(r&0xe0)
	{
	case 0x00: /* 00-1f:controll */
		switch(r&0x1f)
		{
		case 0x01:
			/* wave	selector enable	*/
			if(OPL->type&OPL_TYPE_WAVESEL)
			{
				OPL->wavesel = v&0x20;
				if(!OPL->wavesel)
				{
					/* preset compatible mode */
					int	c;
					for(c=0;c<OPL->max_ch;c++)
					{
						OPL->P_CH[c].SLOT[SLOT1].wavetableidx =	0;
						OPL->P_CH[c].SLOT[SLOT2].wavetableidx =	0;
					}
				}
			}
			return;
		case 0x02:	/* Timer 1 */
			y8950TimerSet(OPL->ref, 0, 1 * (256 -	v));
			break;
		case 0x03:	/* Timer 2 */
			y8950TimerSet(OPL->ref, 1, 4 * (256 -	v));
			return;
		case 0x04:	/* IRQ clear / mask	and	Timer enable */
			if(v&0x80)
			{	/* IRQ flag	clear */
				OPL_STATUS_RESET(OPL,0x7f);
			}
			else
			{	/* set IRQ mask	,timer enable*/
				/* IRQRST,T1MSK,t2MSK,EOSMSK,BRMSK,x,ST2,ST1 */
				OPL_STATUS_RESET(OPL,v&0x78);
				OPL_STATUSMASK_SET(OPL,((~v)&0x78)|0x01);

				y8950TimerStart(OPL->ref, 0, v & 1);
				y8950TimerStart(OPL->ref, 1, v & 2);
			}
			return;
		case 0x06:		/* Key Board OUT */
			if(OPL->type&OPL_TYPE_KEYBOARD)
			{
                OPL->reg6 = v;
			}
			return;
		case 0x07:	/* DELTA-T controll	: START,REC,MEMDATA,REPT,SPOFF,x,x,RST */
			if(OPL->type&OPL_TYPE_ADPCM)
				YM_DELTAT_ADPCM_Write(OPL->deltat,r-0x07,v);
			return;
		case 0x08:	/* MODE,DELTA-T	: CSM,NOTESEL,x,x,smpl,da/ad,64k,rom */
			OPL->mode =	v;
			v&=0x1f;	/* for DELTA-T unit	*/
		case 0x09:		/* START ADD */
		case 0x0a:
		case 0x0b:		/* STOP	ADD	 */
		case 0x0c:
		case 0x0d:		/* PRESCALE	  */
		case 0x0e:
		case 0x0f:		/* ADPCM data */
		case 0x10:		/* DELTA-N	  */
		case 0x11:		/* DELTA-N	  */
		case 0x12:		/* EG-CTRL	  */
			if(OPL->type&OPL_TYPE_ADPCM)
				YM_DELTAT_ADPCM_Write(OPL->deltat,r-0x07,v);
			return;
		case 0x15:		/* DAC data	   */
            OPL->reg15 = v;
			if (OPL->mode & 0x04) {
                static int damp[] = { 256, 279, 304, 332, 362, 395, 431, 470 };
				int sample = (short)(256 * OPL->reg15 + OPL->reg16) * 128 / damp[OPL->reg17];
                OPL->dacSampleVolume = sample;
                OPL->dacEnabled = 1;
			}
		case 0x16:
            OPL->reg16 = v & 0xc0;
            return;
		case 0x17:
            OPL->reg17 = v & 0x07;
			return;
		case 0x18:		/* I/O CTRL	(Direction)	*/
			if(OPL->type&OPL_TYPE_IO)
				OPL->portDirection = v&0x0f;
			return;
		case 0x19:		/* I/O DATA	*/
			if(OPL->type&OPL_TYPE_IO)
			{
				OPL->portLatch = v;
			}
			return;
		case 0x1a:		/* PCM data	*/
			return;
		}
		break;
	case 0x20:	/* am,vib,ksr,eg type,mul */
		slot = slot_array[r&0x1f];
		if(slot	== -1) return;
		set_mul(OPL,slot,v);
		return;
	case 0x40:
		slot = slot_array[r&0x1f];
		if(slot	== -1) return;
		set_ksl_tl(OPL,slot,v);
		return;
	case 0x60:
		slot = slot_array[r&0x1f];
		if(slot	== -1) return;
		set_ar_dr(OPL,slot,v);
		return;
	case 0x80:
		slot = slot_array[r&0x1f];
		if(slot	== -1) return;
		set_sl_rr(OPL,slot,v);
		return;
	case 0xa0:
		switch(r)
		{
		case 0xbd:
			/* amsep,vibdep,r,bd,sd,tom,tc,hh */
			{
				UINT8 rkey = OPL->rythm^v;
				OPL->ams_table_idx = v&0x80	? AMS_ENT :	0;
				OPL->vib_table_idx = v&0x40	? VIB_ENT :	0;
				OPL->rythm	= v&0x3f;
				if(OPL->rythm&0x20)
				{
					/* BD key on/off */
					if(rkey&0x10)
					{
						if(v&0x10)
						{
							OPL->P_CH[6].op1_out[0]	= OPL->P_CH[6].op1_out[1] =	0;
							OPL_KEYON(&OPL->P_CH[6].SLOT[SLOT1]);
							OPL_KEYON(&OPL->P_CH[6].SLOT[SLOT2]);
						}
						else
						{
							OPL_KEYOFF(&OPL->P_CH[6].SLOT[SLOT1]);
							OPL_KEYOFF(&OPL->P_CH[6].SLOT[SLOT2]);
						}
					}
					/* SD key on/off */
					if(rkey&0x08)
					{
						if(v&0x08) OPL_KEYON(&OPL->P_CH[7].SLOT[SLOT2]);
						else	   OPL_KEYOFF(&OPL->P_CH[7].SLOT[SLOT2]);
					}/*	TAM	key	on/off */
					if(rkey&0x04)
					{
						if(v&0x04) OPL_KEYON(&OPL->P_CH[8].SLOT[SLOT1]);
						else	   OPL_KEYOFF(&OPL->P_CH[8].SLOT[SLOT1]);
					}
					/* TOP-CY key on/off */
					if(rkey&0x02)
					{
						if(v&0x02) OPL_KEYON(&OPL->P_CH[8].SLOT[SLOT2]);
						else	   OPL_KEYOFF(&OPL->P_CH[8].SLOT[SLOT2]);
					}
					/* HH key on/off */
					if(rkey&0x01)
					{
						if(v&0x01) OPL_KEYON(&OPL->P_CH[7].SLOT[SLOT1]);
						else	   OPL_KEYOFF(&OPL->P_CH[7].SLOT[SLOT1]);
					}
				}
			}
			return;
		}
		/* keyon,block,fnum	*/
		if(	(r&0x0f) > 8) return;
		CH = &OPL->P_CH[r&0x0f];
		if(!(r&0x10))
		{	/* a0-a8 */
			block_fnum	= (CH->block_fnum&0x1f00) |	v;
		}
		else
		{	/* b0-b8 */
			int	keyon =	(v>>5)&1;
			block_fnum = ((v&0x1f)<<8) | (CH->block_fnum&0xff);
			if(CH->keyon !=	keyon)
			{
				if(	(CH->keyon=keyon) )
				{
					CH->op1_out[0] = CH->op1_out[1]	= 0;
					OPL_KEYON(&CH->SLOT[SLOT1]);
					OPL_KEYON(&CH->SLOT[SLOT2]);
				}
				else
				{
					OPL_KEYOFF(&CH->SLOT[SLOT1]);
					OPL_KEYOFF(&CH->SLOT[SLOT2]);
				}
			}
		}
		/* update */
		if(CH->block_fnum != block_fnum)
		{
			int	blockRv	= 7-(block_fnum>>10);
			int	fnum   = block_fnum&0x3ff;
			CH->block_fnum = block_fnum;

			CH->ksl_base = KSL_TABLE[block_fnum>>6];
			CH->fc = OPL->FN_TABLE[fnum]>>blockRv;
			CH->kcode =	CH->block_fnum>>9;
			if(	(OPL->mode&0x40) &&	CH->block_fnum&0x100) CH->kcode	|=1;
			CALC_FCSLOT(OPL,CH,&CH->SLOT[SLOT1]);
			CALC_FCSLOT(OPL,CH,&CH->SLOT[SLOT2]);
		}
		return;
	case 0xc0:
		/* FB,C	*/
		if(	(r&0x0f) > 8) return;
		CH = &OPL->P_CH[r&0x0f];
		{
			int	feedback = (v>>1)&7;
			CH->FB	 = feedback	? (8+1)	- feedback : 0;
			CH->CON	= v&1;
		}
		return;
	case 0xe0: /* wave type	*/
		slot = slot_array[r&0x1f];
		if(slot	== -1) return;
		CH = &OPL->P_CH[slot/2];
		if(OPL->wavesel)
		{
			/* LOG(LOG_INF,("OPL SLOT %d wave select %d\n",slot,v&3)); */
			CH->SLOT[slot&1].wavetableidx =	(v&0x03)*SIN_ENT;
		}
		return;
	}
}

/* lock/unlock for common table	*/
static int OPL_LockTable(void)
{
	num_lock++;
	if(num_lock>1) return 0;
	/* first time */
	cur_chip = NULL;
	/* allocate	total level	table (128kb space)	*/
	if(	!OPLOpenTable()	)
	{
		num_lock--;
		return -1;
	}
	return 0;
}

static void	OPL_UnLockTable(void)
{
	if(num_lock) num_lock--;
	if(num_lock) return;
	/* last	time */
	cur_chip = NULL;
	OPLCloseTable();
}


int	Y8950UpdateOne(FM_OPL *OPL)
{
	int	data;
    int count;
	UINT32 amsCnt  = OPL->amsCnt;
	UINT32 vibCnt  = OPL->vibCnt;
	UINT8 rythm	= OPL->rythm&0x20;
	OPL_CH *CH,*R_CH;
	YM_DELTAT *DELTAT =	OPL->deltat;

	/* setup DELTA-T unit */
	YM_DELTAT_DECODE_PRESET(DELTAT);

	if(	(void *)OPL	!= cur_chip	){
		cur_chip = (void *)OPL;
		/* channel pointers	*/
		S_CH = OPL->P_CH;
		E_CH = &S_CH[9];
		/* rythm slot */
		SLOT7_1	= &S_CH[7].SLOT[SLOT1];
		SLOT7_2	= &S_CH[7].SLOT[SLOT2];
		SLOT8_1	= &S_CH[8].SLOT[SLOT1];
		SLOT8_2	= &S_CH[8].SLOT[SLOT2];
		/* LFO state */
		amsIncr	= OPL->amsIncr;
		vibIncr	= OPL->vibIncr;
		ams_table =	&AMS_TABLE[OPL->ams_table_idx];
		vib_table =	&VIB_TABLE[OPL->vib_table_idx];			
	}

	R_CH = rythm ? &S_CH[6]	: E_CH;
	/*			  channel A			channel	B		  channel C		 */
	/* LFO */
	ams	= ams_table[(amsCnt+=amsIncr)>>AMS_SHIFT];
	vib	= vib_table[(vibCnt+=vibIncr)>>VIB_SHIFT];
	/* FM part */
	outd = 0;
    count = OPL->rate / OPL->baseRate;
    while (count--) {
	    for(CH=S_CH	; CH < R_CH	; CH++)
		    OPL_CALC_CH(CH);
	    /* Rythn part */
	    if(rythm)
		    OPL_CALC_RH(S_CH);
    }
    outd /= OPL->rate / OPL->baseRate;

    OPL->dacCtrlVolume = OPL->dacSampleVolume - OPL->dacOldSampleVolume + 0x3fe7 * OPL->dacCtrlVolume / 0x4000;
    OPL->dacOldSampleVolume = OPL->dacSampleVolume;
    OPL->dacDaVolume += 2 * (OPL->dacCtrlVolume - OPL->dacDaVolume) / 3;
    OPL->dacEnabled = OPL->dacDaVolume;
    outd += OPL->dacDaVolume << 14;

	/* deltaT ADPCM	*/
	if(	DELTAT->flag )
		YM_DELTAT_ADPCM_CALC(DELTAT);
	/* limit check */
	data = outd;//Limit( outd ,	OPL_MAXOUT,	OPL_MINOUT );
	OPL->amsCnt	= amsCnt;
	OPL->vibCnt	= vibCnt;
	/* deltaT START	flag */
	if(	!DELTAT->flag )
		OPL->status	&= 0xfe;
	/* return result */
	return (data / (1 << (OPL_OUTSB - 3))) * 9 / 10;
}

/* ---------- reset	one	of chip	---------- */
void OPLResetChip(FM_OPL *OPL)
{
	int	c,s;
	int	i;

	/* reset chip */
	OPL->mode	= 0;	/* normal mode */
	OPL_STATUS_RESET(OPL,0x7f);
	/* reset with register write */
    memset(OPL->regs, 0, sizeof(OPL->regs));
	OPLWriteReg(OPL,0x01,0); /*	wabesel	disable	*/
	OPLWriteReg(OPL,0x02,0); /*	Timer1 */
	OPLWriteReg(OPL,0x03,0); /*	Timer2 */
	OPLWriteReg(OPL,0x04,0); /*	IRQ	mask clear */
	for(i =	0xff ; i >=	0x20 ; i-- ) OPLWriteReg(OPL,i,0);
	/* reset OPerator paramater	*/
	for( c = 0 ; c < OPL->max_ch ; c++ )
	{
		OPL_CH *CH = &OPL->P_CH[c];
		/* OPL->P_CH[c].PAN	= OPN_CENTER; */
		for(s =	0 ;	s <	2 ;	s++	)
		{
			/* wave	table */
			CH->SLOT[s].wavetableidx = 0;
			/* CH->SLOT[s].evm = ENV_MOD_RR; */
			CH->SLOT[s].evc	= EG_OFF;
			CH->SLOT[s].eve	= EG_OFF+1;
			CH->SLOT[s].evs	= 0;
		}
	}
	OPL->statusmask	= 0;
	if(OPL->type&OPL_TYPE_ADPCM)
	{
		YM_DELTAT *DELTAT =	OPL->deltat;

		DELTAT->freqbase = OPL->freqbase;
		DELTAT->output_pointer = &outd;
#ifdef MSX_AUDIO
		DELTAT->portshift =	2;
#else
		DELTAT->portshift =	5;
#endif
		DELTAT->output_range = DELTAT_MIXING_LEVEL<<TL_BITS;
		YM_DELTAT_ADPCM_Reset(DELTAT,0);
	}
    
    OPL->dacSampleVolume = 0;
    OPL->dacOldSampleVolume = 0;
    OPL->dacSampleVolumeSum = 0;
    OPL->dacCtrlVolume = 0;
    OPL->dacDaVolume = 0;
    OPL->dacEnabled = 0;

    OPL->reg6  = 0;
    OPL->reg15 = 0;
    OPL->reg16 = 0;
    OPL->reg17 = 0;
}


void OPLSetOversampling(FM_OPL *OPL, int oversampling)
{
    OPL->rate = OPL->baseRate * oversampling;
	OPL_initalize(OPL);
}


/* ----------  Create one of vietual YM3812	----------		 */
/* 'rate'  is sampling rate	and	'bufsiz' is	the	size of	the	 */
FM_OPL *OPLCreate(int type,	int	clock, int rate, int sampleram, void* ref)
{
	char *ptr;
	FM_OPL *OPL;
	int	state_size;
	int	max_ch = 9;	/* normaly 9 channels */

	if(	OPL_LockTable()	==-1) return NULL;
	/* allocate	OPL	state space	*/
	state_size	= sizeof(FM_OPL);
	state_size += sizeof(OPL_CH)*max_ch;
	if(type&OPL_TYPE_ADPCM)	state_size+= sizeof(YM_DELTAT);
	/* allocate	memory block */
	ptr	= malloc(state_size);
	if(ptr==NULL) return NULL;
	/* clear */
	memset(ptr,0,state_size);
	OPL		   = (FM_OPL *)ptr;	ptr+=sizeof(FM_OPL);
	OPL->P_CH  = (OPL_CH *)ptr;	ptr+=sizeof(OPL_CH)*max_ch;
	if(type&OPL_TYPE_ADPCM)	{
		OPL->deltat	= (YM_DELTAT *)ptr;
		OPL->deltat->memory	= malloc(sampleram*1024*sizeof(UINT8));	/* size	of sample bank */
		memset(OPL->deltat->memory,	0xff, 1024 * sampleram);
		OPL->deltat->memory_size = sampleram*1024;
	}
	ptr+=sizeof(YM_DELTAT);

	YM_DELTAT_DECODE_PRESET(OPL->deltat);

	/* set channel state pointer */

    OPL->deltat->OPL = OPL;

    OPL->ref   = ref;
	OPL->type  = type;
	OPL->clock = clock;
	OPL->rate  = rate;
    OPL->baseRate = rate;
	OPL->max_ch	= max_ch;
	/* init	grobal tables */
	OPL_initalize(OPL);
	/* reset chip */
	OPLResetChip(OPL);
	return OPL;
}

/* ----------  Destroy one of vietual YM3812 ----------	*/
void OPLDestroy(FM_OPL *OPL)
{
	OPL_UnLockTable();
	free(OPL->deltat->memory);
	free(OPL);
}

/* ---------- YM3812 I/O interface ---------- */
int	OPLWrite(FM_OPL	*OPL,int a,int v)
{
	if(	!(a&1) )
	{	/* address port	*/
		OPL->address = v & 0xff;
	}
	else
	{	/* data	port */
		OPLWriteReg(OPL,OPL->address,v);
	}
	return OPL->status>>7;
}

unsigned char OPLRead(FM_OPL *OPL,int a)
{
	if(	!(a&1) )
	{	
		/* buffer ready	 */
		OPL_STATUS_SET(OPL,0x08); 

		/* end of sample flag */
		if(OPL->deltat->eos)	  
			OPL_STATUS_SET(OPL,0x10);
		else OPL_STATUS_RESET(OPL,0x10);

		return (OPL->status	& (0x80	| OPL->statusmask))	| 6;
	}

	/* data	port */
	switch(OPL->address)
	{
	case 0x05: /* KeyBoard IN */
		if(OPL->type&OPL_TYPE_KEYBOARD)
		{
            return y8950GetNoteOn(OPL->ref, OPL->reg6);
		}
		return 0xff;
	case 0x14:
		return YM_DELTAT_ADPCM_Read2(OPL->deltat);
	case 0x0f: /* ADPCM-DATA  */
		return YM_DELTAT_ADPCM_Read(OPL->deltat);
	case 0x13:
	case 0x1a:
		return 0;
	case 0x19: /* I/O DATA	  */
		if(OPL->type&OPL_TYPE_IO)
		{
		}
		return ~(switchGetAudio() ?	0 :	0x04);
	}
	return 0xff;
}

unsigned char OPLPeek(FM_OPL *OPL,int a)
{
	if(!(a & 1)) {	
		return (OPL->status	& (0x80	| OPL->statusmask))	| 6;
	}

	/* data	port */
	switch(OPL->address) {
	case 0x05: /* KeyBoard IN */
		return 0xff;
	case 0x14:
		return YM_DELTAT_ADPCM_Peek2(OPL->deltat);
	case 0x0f: /* ADPCM-DATA  */
		return YM_DELTAT_ADPCM_Peek(OPL->deltat);
	case 0x13:
	case 0x1a:
		return 0;
	case 0x19: /* I/O DATA	  */
		return ~(switchGetAudio() ?	0 :	0x04);
	}
	return 0xff;
}

int	OPLTimerOver(FM_OPL	*OPL,int c)
{
	if(	c )
	{	/* Timer B */
		OPL_STATUS_SET(OPL,0x20);
	}
	else
	{	/* Timer A */
		OPL_STATUS_SET(OPL,0x40);
		/* CSM mode	key,TL controll	*/
		if(	OPL->mode &	0x80 )
		{	/* CSM mode	total level	latch and auto key on */
			int	ch;
			for(ch=0;ch<9;ch++)
				CSMKeyControll(	&OPL->P_CH[ch] );
		}
	}
	/* reload timer	*/
	return OPL->status>>7;
}

void Y8950LoadState(FM_OPL *OPL)
{
    SaveState* state = saveStateOpenForRead("fmopl");
    char tag[32];
    int i;
    int j;

    OPL->type               = (UInt8)saveStateGet(state, "type",               0);
    OPL->address            = (UInt8)saveStateGet(state, "address",            0);
    OPL->status             = (UInt8)saveStateGet(state, "status",             0);
    OPL->statusmask         = (UInt8)saveStateGet(state, "statusmask",         0);
    OPL->mode               = saveStateGet(state, "mode",               0);
    OPL->max_ch             = saveStateGet(state, "max_ch",             0);
    OPL->rythm              = (UInt8)saveStateGet(state, "rythm",              0);
    OPL->portDirection      = (UInt8)saveStateGet(state, "portDirection",      0);
    OPL->portLatch          = (UInt8)saveStateGet(state, "portLatch",          0);
    OPL->ams_table_idx      = saveStateGet(state, "ams_table_idx",      0);
    OPL->vib_table_idx      = saveStateGet(state, "vib_table_idx",      0);
    OPL->amsCnt             = saveStateGet(state, "amsCnt",             0);
    OPL->amsIncr            = saveStateGet(state, "amsIncr",            0);
    OPL->vibCnt             = saveStateGet(state, "vibCnt",             0);
    OPL->vibIncr            = saveStateGet(state, "vibIncr",            0);
    OPL->wavesel            = (UInt8)saveStateGet(state, "wavesel",            0);
    OPL->dacSampleVolume    = saveStateGet(state, "dacSampleVolume",    0);
    OPL->dacOldSampleVolume = saveStateGet(state, "dacOldSampleVolume", 0);
    OPL->dacSampleVolumeSum = saveStateGet(state, "dacSampleVolumeSum", 0);
    OPL->dacCtrlVolume      = saveStateGet(state, "dacCtrlVolume",      0);
    OPL->dacDaVolume        = saveStateGet(state, "dacDaVolume",        0);
    OPL->dacEnabled         = saveStateGet(state, "dacEnabled",         0);
    OPL->reg6               = saveStateGet(state, "reg6",               0);
    OPL->reg15              = saveStateGet(state, "reg15",              0);
    OPL->reg16              = saveStateGet(state, "reg16",              0);
    OPL->reg17              = saveStateGet(state, "reg17",              0);

    for (i = 0; i < sizeof(OPL->AR_TABLE) / sizeof(OPL->AR_TABLE[0]); i++) {
        sprintf(tag, "AR_TABLE%d", i);
        OPL->AR_TABLE[i] = saveStateGet(state, tag, 0);

        sprintf(tag, "DR_TABLE%d", i);
        OPL->DR_TABLE[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < sizeof(OPL->FN_TABLE) / sizeof(OPL->FN_TABLE[0]); i++) {
        sprintf(tag, "FN_TABLE%d", i);
        OPL->FN_TABLE[i] = saveStateGet(state, tag, 0);
    }

    for (i = 0; i < OPL->max_ch; i++) {
        sprintf(tag, "CON%d", i);
        OPL->P_CH[i].CON = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "FB%d", i);
        OPL->P_CH[i].FB = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "op1_out%d_0", i);
        OPL->P_CH[i].op1_out[0] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "op1_out%d_1", i);
        OPL->P_CH[i].op1_out[1] = saveStateGet(state, tag, 0);
        
        sprintf(tag, "block_fnum%d", i);
        OPL->P_CH[i].block_fnum = saveStateGet(state, tag, 0);
        
        sprintf(tag, "kcode%d", i);
        OPL->P_CH[i].kcode = (UInt8)saveStateGet(state, tag, 0);
        
        sprintf(tag, "fc%d", i);
        OPL->P_CH[i].fc = saveStateGet(state, tag, 0);
        
        sprintf(tag, "ksl_base%d", i);
        OPL->P_CH[i].ksl_base = saveStateGet(state, tag, 0);
        
        sprintf(tag, "keyon%d", i);
        OPL->P_CH[i].keyon = (UInt8)saveStateGet(state, tag, 0);
        
        for (j = 0; j < 2; j++) {
            sprintf(tag, "TL%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].TL = saveStateGet(state, tag, 0);
            
            sprintf(tag, "TLL%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].TLL = saveStateGet(state, tag, 0);
            
            sprintf(tag, "KSR%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].KSR = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "AR%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].AR = saveStateGet(state, tag, 0);
            
            sprintf(tag, "DR%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].DR = saveStateGet(state, tag, 0);
            
            sprintf(tag, "SL%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].SL = saveStateGet(state, tag, 0);
            
            sprintf(tag, "RR%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].RR = saveStateGet(state, tag, 0);
            
            sprintf(tag, "ksl%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].ksl = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "ksr%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].ksr = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "mul%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].mul = saveStateGet(state, tag, 0);
            
            sprintf(tag, "Cnt%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].Cnt = saveStateGet(state, tag, 0);
            
            sprintf(tag, "Incr%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].Incr = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eg_typ%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].eg_typ = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "evm%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evm = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "evc%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evc = saveStateGet(state, tag, 0);
            
            sprintf(tag, "eve%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].eve = saveStateGet(state, tag, 0);
            
            sprintf(tag, "evs%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evs = saveStateGet(state, tag, 0);
            
            sprintf(tag, "evsa%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evsa = saveStateGet(state, tag, 0);
            
            sprintf(tag, "evsd%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evsd = saveStateGet(state, tag, 0);
            
            sprintf(tag, "evsr%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].evsr = saveStateGet(state, tag, 0);
            
            sprintf(tag, "ams%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].ams = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "vib%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].vib = (UInt8)saveStateGet(state, tag, 0);
            
            sprintf(tag, "wavetableidx%d_%d", i, j);
            OPL->P_CH[i].SLOT[j].wavetableidx = saveStateGet(state, tag, 0);
        }
    }

    saveStateClose(state);
}

void Y8950SaveState(FM_OPL *OPL)
{
    SaveState* state = saveStateOpenForWrite("fmopl");
    char tag[32];
    int i;
    int j;

    saveStateSet(state, "type",               OPL->type);
    saveStateSet(state, "address",            OPL->address);
    saveStateSet(state, "status",             OPL->status);
    saveStateSet(state, "statusmask",         OPL->statusmask);
    saveStateSet(state, "mode",               OPL->mode);
    saveStateSet(state, "max_ch",             OPL->max_ch);
    saveStateSet(state, "rythm",              OPL->rythm);
    saveStateSet(state, "portDirection",      OPL->portDirection);
    saveStateSet(state, "portLatch",          OPL->portLatch);
    saveStateSet(state, "ams_table_idx",      OPL->ams_table_idx);
    saveStateSet(state, "vib_table_idx",      OPL->vib_table_idx);
    saveStateSet(state, "amsCnt",             OPL->amsCnt);
    saveStateSet(state, "amsIncr",            OPL->amsIncr);
    saveStateSet(state, "vibCnt",             OPL->vibCnt);
    saveStateSet(state, "vibIncr",            OPL->vibIncr);
    saveStateSet(state, "wavesel",            OPL->wavesel);
    saveStateSet(state, "dacSampleVolume",    OPL->dacSampleVolume);
    saveStateSet(state, "dacOldSampleVolume", OPL->dacOldSampleVolume);
    saveStateSet(state, "dacSampleVolumeSum", OPL->dacSampleVolumeSum);
    saveStateSet(state, "dacCtrlVolume",      OPL->dacCtrlVolume);
    saveStateSet(state, "dacDaVolume",        OPL->dacDaVolume);
    saveStateSet(state, "dacEnabled",         OPL->dacEnabled);
    saveStateSet(state, "reg6",               OPL->reg6);
    saveStateSet(state, "reg15",              OPL->reg15);
    saveStateSet(state, "reg16",              OPL->reg16);
    saveStateSet(state, "reg17",              OPL->reg17);

    for (i = 0; i < sizeof(OPL->AR_TABLE) / sizeof(OPL->AR_TABLE[0]); i++) {
        sprintf(tag, "AR_TABLE%d", i);
        saveStateSet(state, tag, OPL->AR_TABLE[i]);

        sprintf(tag, "DR_TABLE%d", i);
        saveStateSet(state, tag, OPL->DR_TABLE[i]);
    }

    for (i = 0; i < sizeof(OPL->FN_TABLE) / sizeof(OPL->FN_TABLE[0]); i++) {
        sprintf(tag, "FN_TABLE%d", i);
        saveStateSet(state, tag, OPL->FN_TABLE[i]);
    }

    for (i = 0; i < OPL->max_ch; i++) {
        sprintf(tag, "CON%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].CON);
        
        sprintf(tag, "FB%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].FB);
        
        sprintf(tag, "op1_out%d_0", i);
        saveStateSet(state, tag, OPL->P_CH[i].op1_out[0]);
        
        sprintf(tag, "op1_out%d_1", i);
        saveStateSet(state, tag, OPL->P_CH[i].op1_out[1]);
        
        sprintf(tag, "block_fnum%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].block_fnum);
        
        sprintf(tag, "kcode%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].kcode);
        
        sprintf(tag, "fc%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].fc);
        
        sprintf(tag, "ksl_base%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].ksl_base);
        
        sprintf(tag, "keyon%d", i);
        saveStateSet(state, tag, OPL->P_CH[i].keyon);
        
        for (j = 0; j < 2; j++) {
            sprintf(tag, "TL%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].TL);
            
            sprintf(tag, "TLL%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].TLL);
            
            sprintf(tag, "KSR%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].KSR);
            
            sprintf(tag, "AR%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].AR);
            
            sprintf(tag, "DR%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].DR);
            
            sprintf(tag, "SL%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].SL);
            
            sprintf(tag, "RR%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].RR);
            
            sprintf(tag, "ksl%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].ksl);
            
            sprintf(tag, "ksr%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].ksr);
            
            sprintf(tag, "mul%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].mul);
            
            sprintf(tag, "Cnt%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].Cnt);
            
            sprintf(tag, "Incr%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].Incr);
            
            sprintf(tag, "eg_typ%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].eg_typ);
            
            sprintf(tag, "evm%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evm);
            
            sprintf(tag, "evc%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evc);
            
            sprintf(tag, "eve%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].eve);
            
            sprintf(tag, "evs%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evs);
            
            sprintf(tag, "evsa%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evsa);
            
            sprintf(tag, "evsd%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evsd);
            
            sprintf(tag, "evsr%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].evsr);
            
            sprintf(tag, "ams%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].ams);
            
            sprintf(tag, "vib%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].vib);
            
            sprintf(tag, "wavetableidx%d_%d", i, j);
            saveStateSet(state, tag, OPL->P_CH[i].SLOT[j].wavetableidx);
        }
    }

    saveStateClose(state);
}

