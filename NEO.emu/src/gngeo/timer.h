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

#ifndef _TIMER_H_
#define _TIMER_H_

//#include "SDL.h"
#include <gngeoTypes.h>

static const int nb_interlace = 256;

//typedef double AudioTime;
typedef uint AudioTime;

typedef struct timer_struct {
	AudioTime time;		// when
//	Uint32 time;		// when
    //Uint32 odo_debut;
    //Uint32 nb_cycle;
    int param;
    Uint32 del_it;
    //void (*func) (int param);
    //struct timer_struct *next;
} timer_struct;

extern AudioTime timer_count;
//extern Uint32 timer_count;

void init_timer();
timer_struct *insert_timer(AudioTime duration, int param, void (*func) (int));
//timer_struct *insert_timer(Uint32 duration, int param, void (*func) (int));
void del_timer(timer_struct * ts);
void my_timer(void);
AudioTime timer_get_time(void);
//Uint32 timer_get_time(void);
void free_all_timer(void);

#endif
