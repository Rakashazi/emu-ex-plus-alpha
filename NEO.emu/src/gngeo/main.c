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

#include <signal.h>

#include "SDL.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ym2610/2610intf.h"
#include "font.h"
#include "fileio.h"
#include "video.h"
#include "screen.h"
#include "emu.h"
#include "sound.h"
#include "messages.h"
#include "memory.h"
#include "debug.h"
#include "blitter.h"
#include "effect.h"
#include "conf.h"
#include "transpack.h"
#include "gngeo_icon.h"
#include "event.h"
#include "menu.h"
#include "frame_skip.h"

#ifdef USE_GUI
#include "gui_interf.h"
#endif
#ifdef GP2X
#include "gp2x.h"
#include "ym2610-940/940shared.h"
#endif
#ifdef WII
extern bool fatInitDefault(void);
#endif

#ifdef __AMIGA__
# include <proto/dos.h>
#endif

void calculate_hotkey_bitmasks()
{
    int *p;
    int i, j, mask;
    const char *p1_key_list[] = { "p1hotkey0", "p1hotkey1", "p1hotkey2", "p1hotkey3" };
    const char *p2_key_list[] = { "p2hotkey0", "p2hotkey1", "p2hotkey2", "p2hotkey3" };


    for ( i = 0; i < 4; i++ ) {
	p=CF_ARRAY(cf_get_item_by_name(p1_key_list[i]));
	for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
	conf.p1_hotkey[i] = mask;
    }

    for ( i = 0; i < 4; i++ ) {
	p=CF_ARRAY(cf_get_item_by_name(p2_key_list[i]));
	for ( mask = 0, j = 0; j < 4; j++ ) mask |= p[j];
	conf.p2_hotkey[i] = mask;
    }

}

void sdl_set_title(char *name) {
    char *title;
    if (name) {
	title = malloc(strlen("Gngeo : ")+strlen(name)+1);
	if (title) {
	  sprintf(title,"Gngeo : %s",name);
	  SDL_WM_SetCaption(title, NULL);
	}
    } else {
	SDL_WM_SetCaption("Gngeo", NULL);
    }
}

void init_sdl(void /*char *rom_name*/) {
    int surface_type = (CF_BOOL(cf_get_item_by_name("hwsurface"))? SDL_HWSURFACE : SDL_SWSURFACE);


    char *nomouse = getenv("SDL_NOMOUSE");
    SDL_Surface *icon;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);

#ifdef GP2X
    atexit(gp2x_quit);
#else
    atexit(SDL_Quit);
#endif

    if (screen_init() == SDL_FALSE) {
	printf("Screen initialization failed.\n");
	exit(-1);
    }

    buffer = SDL_CreateRGBSurface(surface_type, 352, 256, 16, 0xF800, 0x7E0,
				  0x1F, 0);
    SDL_FillRect(buffer,NULL,SDL_MapRGB(buffer->format,0xE5,0xE5,0xE5));

    fontbuf = SDL_CreateRGBSurfaceFrom(font_image.pixel_data, font_image.width, font_image.height
				       , 24, font_image.width * 3, 0xFF0000, 0xFF00, 0xFF, 0);
    SDL_SetColorKey(fontbuf,SDL_SRCCOLORKEY,SDL_MapRGB(fontbuf->format,0xFF,0,0xFF));
    fontbuf=SDL_DisplayFormat(fontbuf);
    icon = SDL_CreateRGBSurfaceFrom(gngeo_icon.pixel_data, gngeo_icon.width,
				    gngeo_icon.height, gngeo_icon.bytes_per_pixel*8,
				    gngeo_icon.width * gngeo_icon.bytes_per_pixel,
				    0xFF, 0xFF00, 0xFF0000, 0);
    
    SDL_WM_SetIcon(icon,NULL);

    calculate_hotkey_bitmasks();    
	init_event();

    //if (nomouse == NULL)
	//SDL_ShowCursor(SDL_DISABLE);
}
static void catch_me(int signo) {
	printf("Catch a sigsegv\n");
	//SDL_Quit();
	exit(-1);
}
int main(int argc, char *argv[])
{
    char *rom_name;



#ifdef __AMIGA__
    BPTR file_lock = GetProgramDir();
    SetProgramDir(file_lock);
#endif
	signal(SIGSEGV, catch_me);

#ifdef WII
	//   SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);

	fatInitDefault();
#endif

    cf_init(); /* must be the first thing to do */
    cf_init_cmd_line();
    cf_open_file(NULL); /* Open Default configuration file */

    rom_name=cf_parse_cmd_line(argc,argv);

    /* print effect/blitter list if asked by user */
    if (!strcmp(CF_STR(cf_get_item_by_name("effect")),"help")) {
	print_effect_list();
	exit(0);
    }
    if (!strcmp(CF_STR(cf_get_item_by_name("blitter")),"help")) {
	print_blitter_list();
	exit(0);
    }

	init_sdl();

/* GP2X stuff */
#ifdef GP2X
    gp2x_init();
#endif
    if (gn_init_skin()!=SDL_TRUE) {
	    printf("Can't load skin...\n");
            exit(1);
    }    

	reset_frame_skip();

    if (conf.debug) conf.sound=0;

/* Launch the specified game, or the rom browser if no game was specified*/
	if (!rom_name) {
	//	rom_browser_menu();
		run_menu();
		printf("GAME %s\n",conf.game);
		if (conf.game==NULL) return 0;
	} else {

		if (init_game(rom_name)!=SDL_TRUE) {
			printf("Can't init %s...\n",rom_name);
            exit(1);
		}    
	}

	/* If asked, do a .gno dump and exit*/
    if (CF_BOOL(cf_get_item_by_name("dump"))) {
        char dump[8+4+1];
        sprintf(dump,"%s.gno",rom_name);
        dr_save_gno(&memory.rom,dump);
        close_game();
        return 0;
    }

    if (conf.debug)
	    debug_loop();
    else
	    main_loop();

    close_game();

    return 0;
}
