/***************************************************************************

  2610intf.c

  The YM2610 emulator supports up to 2 chips.
  Each chip has the following connections:
  - Status Read / Control Write A
  - Port Read / Data Write A
  - Control Write B
  - Data Write B

***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <stdio.h>
#include <math.h>
#include "2610intf.h"
#include "../emu.h"
#include "../memory.h"
#include "../timer.h"

static timer_struct *Timer[2] = { 0 };

/*------------------------- TM2610 -------------------------------*/
/* IRQ Handler */
/*
static void IRQHandler(int n, int irq)
{
    //printf("IRQ!!!\n");
    neogeo_sound_irq(irq);
}*/

/* Timer overflow callback from timer.c */
void timer_callback_2610(int param)
{
    int c = param;

    Timer[c] = 0;
    YM2610TimerOver(c);
}

/* TimerHandler from fm.c */
static void TimerHandler(int c, int count, float stepTime)
//static void TimerHandler(int c, int count, Uint32 stepTime)
{
	//printf("TimerHandler %d %d %f\n",c,count,stepTime);
	if (count == 0) {		/* Reset FM Timer */
		if (Timer[c]) {
			del_timer(Timer[c]);
			Timer[c] = 0;
		}
	} else {			/* Start FM Timer */
		//logMsg("fm timer @ %d", count);
		AudioTime timeSec = (AudioTime) roundf(count * stepTime);
		//Uint32 timeSec = count * (Uint32)(stepTime*(1<<TIMER_SH));
		
		if (Timer[c] == 0) {
			Timer[c] =
				(timer_struct *) insert_timer(timeSec, c,
							      timer_callback_2610);
		}
	}
}
void FMTimerInit(void)
{
    Timer[0] = Timer[1] = 0;
    free_all_timer();
}
#if 0
/* update request from fm.c */
void YM2610UpdateRequest(void)
{
	static double old_tc;
	//static Uint32 old_tc;
	double tc=timer_count-old_tc;
	//Uint32 tc=timer_count-old_tc;
    int len=(int)((conf.sample_rate*tc)>>TIMER_SH)<<2;
    if (len >4 ) {
	old_tc=timer_count;
	streamupdate(len);
    }
}
#endif

int YM2610_sh_start(void)
{
    int rate = conf.sample_rate;
    //char buf[YM2610_NUMBUF][40];
    void *pcmbufa, *pcmbufb;
    int pcmsizea, pcmsizeb;

    /*
    if (AY8910_sh_start())
	return 1;
    */

    /* Timer Handler set */
    FMTimerInit();
/*
    for (j = 0; j < YM2610_NUMBUF; j++) {
	buf[j][0] = 0;
    }
    stream = stream_init_multi(YM2610_NUMBUF, 0, YM2610UpdateOne);
*/
    pcmbufa = (void *) memory.rom.adpcma.p;
    pcmsizea = memory.rom.adpcma.size;
    pcmbufb = (void *) memory.rom.adpcmb.p;
    pcmsizeb = memory.rom.adpcmb.size;

    //}

  /**** initialize YM2610 ****/
    /*
       if (YM2610Init(8000000,rate,
       pcmbufa,pcmsizea,pcmbufb,pcmsizeb,
       TimerHandler,IRQHandler) == 0)
     */
    YM2610Init(8000000, rate,
	       pcmbufa, pcmsizea, pcmbufb, pcmsizeb,
	       TimerHandler, neogeo_sound_irq);
    return 0;
}


/************************************************/
/* Sound Hardware Stop				*/
/************************************************/
void YM2610_sh_stop(void)
{
}

/* reset */
void YM2610_sh_reset(void)
{

    YM2610Reset();
}

/************************************************/
/* Status Read for YM2610 - Chip 0		*/
/************************************************/
Uint32 YM2610_status_port_A_r(Uint32 offset)
{
    return YM2610Read(0);
}

Uint32 YM2610_status_port_B_r(Uint32 offset)
{
    return YM2610Read(2);
}

/************************************************/
/* Port Read for YM2610 - Chip 0		*/
/************************************************/
Uint32 YM2610_read_port_r(Uint32 offset)
{
    return YM2610Read(1);
}


/************************************************/
/* Control Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_control_port_A_w(Uint32 offset, Uint32 data)
{
    YM2610Write(0, data);
}

void YM2610_control_port_B_w(Uint32 offset, Uint32 data)
{
    YM2610Write(2, data);
}

/************************************************/
/* Data Write for YM2610 - Chip 0		*/
/* Consists of 2 addresses			*/
/************************************************/
void YM2610_data_port_A_w(Uint32 offset, Uint32 data)
{
    YM2610Write(1, data);
}

void YM2610_data_port_B_w(Uint32 offset, Uint32 data)
{
    YM2610Write(3, data);
}

