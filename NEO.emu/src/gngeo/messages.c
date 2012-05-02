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

#include <string.h>
#include "SDL.h"
#include "messages.h"
#include "video.h"
#include "emu.h"
#include "timer.h"
#include "frame_skip.h"
#include "screen.h"
#include "sound.h"
#include <stdarg.h>


static int font_w=8;
static int font_h=9;

void SDL_putchar(SDL_Surface * dest, int x, int y, unsigned char c)
{
    static SDL_Rect font_rect, dest_rect;
    int indice = c - 32;

    if (c < 32 || c > 127)
	return;

    font_rect.x = indice *  font_w;
    font_rect.y = 0;
    font_rect.w =  font_w;
    font_rect.h =  font_h;
    dest_rect.x = x;
    dest_rect.y = y;
    dest_rect.w =  font_w;
    dest_rect.h =  font_h;

    SDL_BlitSurface(fontbuf, &font_rect, dest, &dest_rect);

}

void SDL_textout(SDL_Surface * dest, int x, int y, const char *string)
{
	int i;int xx=x;
	for (i = 0; i < strlen(string); i++) {
		if (string[i]=='\n') {xx=x;y+=font_h;continue;}
		SDL_putchar(dest, xx , y, string[i]);
		xx+=font_w;
	}
}

#if 0

/* TODO: Use blitter instead of direct screen access */
void error_box(char *fmt,...) {
	char buf[512];
	va_list pvar;
	va_start(pvar,fmt);
	SDL_Rect r={32,32,320-64,240-64};

	SDL_FillRect(screen,&r,0xF011);
	
	vsnprintf(buf,511,fmt,pvar);
	SDL_textout(screen,40,40,buf);

	sleep(5);
}

#endif
//static timer_struct *msg_timer;
/*
void stop_message(int param)
{
  conf.do_message=0;
  msg_timer=NULL;
}
*/
void draw_message(const char *string)
{
    /*
       if (msg_timer!=NULL)
       del_timer(msg_timer);
       msg_timer=NULL;
     */
    strcpy(conf.message, string);
    conf.do_message = 75;
    //msg_timer=insert_timer(1.0,0,stop_message);
}


#define LEFT 1
#define RIGHT 2
#define BACKSPACE 3
#define DEL 4

int SDL_getchar(void)
{
    SDL_Event event;
    SDL_WaitEvent(&event);
    //while(SDL_PollEvent(&event)){}
    switch (event.type) {
    case SDL_KEYDOWN:
	switch(event.key.keysym.sym) {
	case SDLK_RETURN:
	    return -1;
	case SDLK_LEFT:
	    return LEFT;
	case SDLK_RIGHT:
	    return RIGHT;
	case SDLK_DELETE:
	    return DEL;
	case SDLK_BACKSPACE:
	    return BACKSPACE;
	default:
	    break;
	}
	    
	if ( (event.key.keysym.unicode & 0xFF80) == 0 ) {
	    return (event.key.keysym.unicode & 0x7F);
	}
	break;
    }
    return 0;
}

void text_input(const char *message,int x,int y,char *string,int size)
{
    int sx;
    int a;
    int s=0;
    int i;
    int pos=0;
    static SDL_Surface *save=NULL;
    SDL_Rect clear_rect={16,227,320,16};
    if (!save)
	save=SDL_CreateRGBSurface(SDL_SWSURFACE,320,16,16, 0xF800, 0x7E0,0x1F, 0);

    if (conf.sound) pause_audio(1);

    memset(string,0,size+1);

    SDL_FillRect(buffer,&clear_rect,0);
//    SDL_BlitSurface(buffer,&clear_rect,save,NULL);
    SDL_textout(buffer,x,y,message);
    sx=x+strlen(message)* font_w;
    SDL_EnableUNICODE(1);
    while((a=SDL_getchar())!=-1) {
	if (a==LEFT && pos>0) pos--;
	if (a==RIGHT && pos<s) pos++;
	if (a==BACKSPACE && pos>0) {
	    for(i=pos-1;i<s;i++)
		string[i]=string[i+1];
	    s--;
	    pos--;
	}
	if (a==DEL && pos<s) {
	    for(i=pos;i<s;i++)
		string[i]=string[i+1];
	    s--;
	}
	if (a>32  && s<size ) {
	    for(i=s;i>pos;i--)
		string[i]=string[i-1];
	    string[pos]=(char)a;

	    s++;
	    pos++;
	}
	SDL_FillRect(buffer,&clear_rect,0);
	//SDL_BlitSurface(save,NULL,buffer,&clear_rect);
	SDL_textout(buffer,x,y,message);
	SDL_textout(buffer,sx,y,string);
	/* cursor */
	((Uint16*)buffer->pixels)[352*(16+222)+sx+pos* font_w-1]=0;
	for(i=sx+pos* font_w;i<sx+pos* font_w+ font_w;i++) {
	    ((Uint16*)buffer->pixels)[352*(16+222)+i]=0xFFFF;
	    ((Uint16*)buffer->pixels)[352*(16+221)+i]=0;
	    ((Uint16*)buffer->pixels)[352*(16+223)+i]=0;
	}
	((Uint16*)buffer->pixels)[352*(16+222)+sx+pos* font_w+font_w]=0;	
	screen_update();
    }
    SDL_EnableUNICODE(0);
    if (conf.sound) pause_audio(0);
    reset_frame_skip();
}
