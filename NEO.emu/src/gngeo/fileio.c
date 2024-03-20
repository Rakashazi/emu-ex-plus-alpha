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

//#include "SDL.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>

#include "unzip.h"
#include "memory.h"
#include "video.h"
#include "emu.h"
#include "fileio.h"
#include "neocrypt.h"
#include "conf.h"
#include "sound.h"
#include "menu.h"
#include <imagine/logger/logger.h>

#ifdef GP2X
#include "ym2610-940/940shared.h"
#endif

#if defined (WII)
#define ROOTPATH "sd:/apps/gngeo/"
#elif defined (__AMIGA__)
#define ROOTPATH "/PROGDIR/data/"
#else
#define ROOTPATH ""
#endif

//Uint8 *current_buf;
//char *rom_file;

void chomp(char *str) {
    int i = 0;
    if (str) {
        while (str[i] != 0) {
            printf(" %d ", str[i]);
            i++;
        }
        printf("\n");
        if (str[i - 1] == 0x0A || str[i - 1] == 0x0D) str[i - 1] = 0;
        if (str[i - 2] == 0x0A || str[i - 2] == 0x0D) str[i - 2] = 0;

    }
}


char *file_basename(char *filename) {
    char *t;
    t = strrchr(filename, '/');
    if (t) return t + 1;
    return filename;
}

/* check if dir_name exist. Create it if not */
bool check_dir(char *dir_name) {
    DIR *d;

    if (!(d = opendir(dir_name)) && (errno == ENOENT)) {
#ifdef WIN32
        mkdir(dir_name);
#else
        mkdir(dir_name, 0755);
#endif
        return false;
    }
    if(d)
    	closedir(d);
    return true;
}

int close_game(void) {
    if (conf.game == NULL) return false;

    dr_free_roms(&memory.rom);
    trans_pack_free();

    return true;
}

bool load_game_config(void *contextPtr, char *rom_name) {
	const char *gpath;
	char *drconf;
#ifdef EMBEDDED_FS
    gpath=ROOTPATH"conf/";
#else
    gpath=get_gngeo_dir(contextPtr);
#endif
	cf_reset_to_default();
	cf_open_file(NULL); /* Reset possible previous setting */
	if (rom_name) {
		if (strstr(rom_name,".gno")!=NULL) {
			char *name=dr_gno_romname(rom_name);
			if (name) {
				logMsg("Tring to load a gno file %s %s\n",rom_name,name);
				drconf=alloca(strlen(gpath)+strlen(name)+strlen(".cf")+1);
				sprintf(drconf,"%s%s.cf",gpath,name);
			} else {
				logMsg("Error while loading %s\n",rom_name);
				return false;
			}
		} else {
			drconf=alloca(strlen(gpath)+strlen(rom_name)+strlen(".cf")+1);
			sprintf(drconf,"%s%s.cf",gpath,rom_name);
		}
		cf_open_file(drconf);
	}
	return true;
}

int init_game(void *contextPtr, char *rom_name, char romerror[1024]) {
	//logMsg("AAA Blitter %s effect %s\n",CF_STR(cf_get_item_by_name("blitter")),CF_STR(cf_get_item_by_name("effect")));

	load_game_config(contextPtr, rom_name);
	/* reinit screen if necessary */
	//screen_change_blitter_and_effect(NULL,NULL);
	screen_reinit();
	//logMsg("BBB Blitter %s effect %s\n",CF_STR(cf_get_item_by_name("blitter")),CF_STR(cf_get_item_by_name("effect")));
    /* open transpack if need */
    //trans_pack_open(CF_STR(cf_get_item_by_name("transpack")));

    if (strstr(rom_name, ".gno") != NULL) {
        if(dr_open_gno(contextPtr, rom_name, romerror) == false)
        {
        	//sprintf(romerror, "Can't load %s", rom_name);
        	return false;
        }

    } else {

        //open_rom(rom_name);
	if (dr_load_game(contextPtr, rom_name, romerror) == false) {
#if defined(GP2X)
            gn_popup_error(" Error! :", "Couldn't load %s",
                    file_basename(rom_name));
#else
            logMsg("Can't load %s\n", rom_name);
#endif
            return false;
        }

    }

    init_neo();
    setup_misc_patch(conf.game);

    fix_usage = memory.fix_board_usage;
    current_pal = memory.vid.pal_neo[0];
    current_fix = memory.rom.bios_sfix.p;
    current_pc_pal = (Uint32 *) memory.vid.pal_host[0];

	memory.vid.currentpal=0;
	memory.vid.currentfix=0;


    return true;
}
