/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include "emu.h"
#include "timer.h"
#include "state.h"
#include "ym2610/ym2610.h"
#ifdef GP2X
#include "ym2610-940/940shared.h"
#endif
#include <imagine/logger/logger.h>

AudioTime timer_count = 0;
//Uint32 timer_count;

//timer_struct *timer_list;
#define MAX_TIMER 3
timer_struct timers[MAX_TIMER] = { { 0 } };
//int nb_timer=0;

AudioTime timer_get_time(void) {
//Uint32 timer_get_time(void) {
    return timer_count;
}

timer_struct *insert_timer(AudioTime duration, int param, void (*func) (int))
//timer_struct *insert_timer(Uint32 duration, int param, void (*func) (int))
{
    for (int i = 0; i < MAX_TIMER; i++) {
	if (timers[i].del_it) {
	    timers[i].time = timer_count + duration;
	    timers[i].param = param;
	    //timers[i].func = func;
	    //logMsg("Insert_timer %d duration=%d param=%d",i,duration,timers[i].param);
	    timers[i].del_it = 0;
	    return &timers[i];
	}
    }
    logMsg("YM2610: No timer free!\n");
    return NULL;		/* No timer free */
}

void free_all_timer(void) {
    for (int i = 0; i < MAX_TIMER; i++) {
	timers[i].del_it=1;
    }
}

#ifdef ENABLE_940T
void timer_callback_2610(int param){}
#else
void timer_callback_2610(int param);
#endif
void timer_post_load_state(void)
{
#if 0
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timers[i].del_it) {
			timers[i].func=NULL;
		} else {
			/* FIXME: Ugly.... */
			timers[i].func=timer_callback_2610;
		}
	}
#endif
}

void timer_pre_save_state(void)
{

}

#if 0
void timer_init_save_state(void) {
    int i;
    create_state_register(ST_TIMER,"timer_count",1,&timer_count,sizeof(AudioTime),REG_UINT32);

    for(i=0;i<MAX_TIMER;i++) {
	create_state_register(ST_TIMER,"t.del_it",i,&timers[i].del_it,sizeof(Uint32),REG_UINT32);
	create_state_register(ST_TIMER,"t.time",i,&timers[i].time,sizeof(AudioTime),REG_UINT32);
	create_state_register(ST_TIMER,"t.param",i,&timers[i].param,sizeof(Sint32),REG_INT32);
    }
    set_post_load_function(ST_TIMER,timer_post_load_state);
    set_pre_save_function(ST_TIMER,timer_pre_save_state);
}
#endif

void del_timer(timer_struct * ts)
{
    ts->del_it = 1;
}

//static AudioTime inc;
//static Uint32 inc;

void init_timer()
{
#if 0
	static int init = 1;
	if (init)
	{
		timer_init_save_state();
		init = 0;
	}

	if (conf.pal) {
			inc = ((AudioTime) (0.02) / nb_interlace);/* *(1<<TIMER_SH);*/
				  //(conf.sound ? (double) nb_interlace : 1.0);
		} else {
			inc = ((AudioTime) (1./(60./1.001)) / nb_interlace); /* *(1<<TIMER_SH); */
			//(conf.sound ? (double) nb_interlace : 1.0);
		}
#endif
	logMsg("init timer");
	timer_count = 0;
	for (int i = 0; i < MAX_TIMER; i++)
	  timers[i].del_it = 1;
}

void timer_mkstate(Stream *gzf,int mode) {
	mkstate_data(gzf, &timer_count, sizeof(timer_count), mode);
	mkstate_data(gzf, timers, sizeof(timers), mode);
}

void my_timer(void)
{
    //logMsg("timer update");
    //timer_count += inc;		/* 16ms par frame */
    timer_count+= 1;

    for (int i = 0; i < MAX_TIMER; i++) {
	if (timer_count >= timers[i].time && timers[i].del_it == 0) {
	    //logMsg("Timer_expire %d duration=%d param=%d",i,timers[i].time,timers[i].param);
	    //if (timers[i].func) timers[i].func(timers[i].param);
	    timer_callback_2610(timers[i].param);
	    timers[i].del_it = 1;
	}
    }
}
