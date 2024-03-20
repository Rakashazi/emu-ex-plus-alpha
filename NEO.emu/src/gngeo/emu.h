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

#ifndef _EMU_H_
#define _EMU_H_

#include <gngeoTypes.h>

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <string.h>
#include <stdint.h>
#define Uintptr uintptr_t


typedef enum SYSTEM {
    SYS_ARCADE=0,
    SYS_HOME,
    SYS_UNIBIOS, // selects 2.3 for backwards compatibility
    SYS_UNIBIOS_3_0,
    SYS_UNIBIOS_3_1,
    SYS_UNIBIOS_3_2,
    SYS_UNIBIOS_3_3,
    SYS_UNIBIOS_4_0,
    SYS_UNIBIOS_LAST = SYS_UNIBIOS_4_0,
    SYS_MAX
} SYSTEM;

typedef enum COUNTRY {
    CTY_JAPAN=0,
    CTY_EUROPE,
    CTY_USA,
    CTY_ASIA,
    CTY_MAX
} COUNTRY;

typedef struct {
    char *game;
    /*Uint16 x_start;
    Uint16 y_start;
    Uint16 res_x;
    Uint16 res_y;*/
    Uint16 sample_rate;
    Uint16 test_switch;

    //Uint8 vsync;
    Uint8 snd_st_reg_create;
    //Uint8 do_message;
    //Uint8 nb_joy;
    Uint8 raster;
    Uint8 debug;
    //Uint8 rom_type;
    //Uint8 special_bios;
    Uint8 extra_xor;
    Uint8 pal;
    //Uint8 accurate940;
    SYSTEM system;
    COUNTRY country;

    //Uint8 autoframeskip;
    //Uint8 show_fps;
    //Uint8 sleep_idle;
    //Uint8 screen320;

    //char message[128];
    //char fps[4];

    //int *p1_key;
    //int *p2_key;

    //SDL_Joystick **joy;
    /*int *p1_joy;
    int *p2_joy;

    int *p1_hotkey0, *p1_hotkey1, *p1_hotkey2, *p1_hotkey3;
    int *p2_hotkey0, *p2_hotkey1, *p2_hotkey2, *p2_hotkey3;

    int p1_hotkey[4];
    int p2_hotkey[4];*/
} CONFIG;
extern CONFIG conf;

extern unsigned int fc;
extern int last_line;

void debug_loop(void);
void main_loop(void);
void init_neo(void);
void cpu_68k_dpg_step(int skip_this_frame, void *emuTaskPtr, void *neoSystemPtr, void *emuVideoPtr);
void setup_misc_patch(char *name);
void neogeo_reset(void);
int currentZ80Timeslice();
void syncZ80();

/* LOG generation */
#define GNGEO_LOG(...)
#define DEBUG_LOG logMsg
//#define GNGEO_LOG printf

#endif
