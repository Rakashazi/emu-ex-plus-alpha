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
#include <config.h>
#endif

#ifdef USE_GUI
#include "gui_interf.h"
#endif

#include "SDL.h"

#ifdef GP2X
#include "messages.h"
#include "screen.h"
#include "menu.h"
/*
static int y=8;
static SDL_Rect pbar={0,0,0,0};
*/

#endif

/* progress bar function */
static Uint8 pg_last;
static Uint8 pg_size;
static Uint32 oldpos=0;

void create_progress_bar(const char *desc) {
    int i;
#ifdef GP2X
    /*
    SDL_textout(screen, 8, y, desc);
    pbar.x=120;pbar.w=182;
    pbar.y=y+1;pbar.h=6;
    SDL_FillRect(screen,&pbar,0xFFFF);
    pbar.x=121;pbar.w=180;
    pbar.y=y+2;pbar.h=4;
    SDL_FillRect(screen,&pbar,0);

    SDL_Flip(screen);
    */
    gn_init_pbar(desc);
#else
#ifdef USE_GUI
    loading_pbar_set_label(desc);
    oldpos=0;
#else
    pg_size=62;//74-strlen(desc);
    pg_last=0;

    printf("%12s [",desc);
    for (i=0;i<pg_size;i++)
	printf("-");
    printf("]\r%12s [",desc);
    fflush(stdout);
#endif
#endif
}

void update_progress_bar(Uint32 current_pos,Uint32 size) {
#ifdef GP2X
	if (((current_pos-oldpos)*100.0/(float)size)>=5.0) {
		/*
		  pbar.x=121;pbar.w=(current_pos*180)/size;
		  pbar.y=y+2;pbar.h=4;
		  SDL_FillRect(screen,&pbar,0x02FF);
		  oldpos=current_pos;
		  SDL_Flip(screen);
		*/
		gn_update_pbar(current_pos,size);
		oldpos=current_pos;
	}	
#else
#ifdef USE_GUI
    //printf("%ul %f\n",current_pos-oldpos,((current_pos-oldpos)*100.0/(float)size));
    if (((current_pos-oldpos)*100.0/(float)size)>=5.0) {
	loading_pbar_update(current_pos,size);
	oldpos=current_pos;
    }
#else
    Uint8 pg_current=(pg_size*current_pos)/(double)size;
    int i;

    
    for(i=pg_last;i<pg_current;i++) {
	putchar('*');
    }
    pg_last=pg_current;
    fflush(stdout);
#endif
#endif
}

void terminate_progress_bar(void) {
#ifdef GP2X
	/*
	pbar.x=121;pbar.w=180;
	pbar.y=y+2;pbar.h=4;
	SDL_FillRect(screen,&pbar,0x02FF);
	SDL_Flip(screen);
	y+=8;
	if (y>220) { 
		y=8; 
		SDL_FillRect(screen,NULL,0);
	}
	*/
	gn_terminate_pbar();
#else
#ifdef USE_GUI
    loading_pbar_set_label(NULL);
#else
    int i;
    for(i=pg_last;i<pg_size;i++) {
	putchar('*');
    }
    printf("\r");
#endif
#endif
}

