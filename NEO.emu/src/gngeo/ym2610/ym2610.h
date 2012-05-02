/***************************************************************************

  ym2610.h

  Header file for software emulation for YAMAHA YM-2610 sound generator

***************************************************************************/

#ifndef _YM2610_H_
#define _YM2610_H_

#include <gngeoTypes.h>

#include "mvs.h"
#include "../timer.h"

/* for busy flag emulation , function FM_GET_TIME_NOW() should be */
/* return the present time in second unit with (double) value     */
/* in timer.c */
#define FM_GET_TIME_NOW() timer_get_time()

typedef s16 FMSAMPLE;
typedef s32 FMSAMPLE_MIX;
#define TIMER_SH		16  /* 16.16 fixed point (timers calculations)    */

typedef void (*FM_TIMERHANDLER)(int channel, int count, float stepTime);
typedef void (*FM_IRQHANDLER)(int irq);

void YM2610Init(int baseclock, int rate,
		void *pcmroma, int pcmsizea,
		void *pcmromb, int pcmsizeb,
		FM_TIMERHANDLER TimerHandler,
		FM_IRQHANDLER IRQHandler);
void YM2610ChangeSamplerate(int rate);
void YM2610Reset(void);
int  YM2610Write(int addr, u8 value);
u8   YM2610Read(int addr);
int  YM2610TimerOver(int channel);

void YM2610Update(int *p);
void YM2610Update_stream(int length);

#ifdef SOUND_TEST
void YM2610Update_SoundTest(int p);
#endif

#ifdef SAVE_STATE
STATE_SAVE( ym2610 );
STATE_LOAD( ym2610 );
#endif

#endif /* _YM2610_H_ */
