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
//#include "SDL.h"
//#include "SDL_types.h"
#define Uintptr uintptr_t


typedef enum SYSTEM {
    SYS_ARCADE=0,
    SYS_HOME,
    SYS_UNIBIOS, // selects 2.3 for backwards compatibility
    SYS_UNIBIOS_3_0,
    SYS_UNIBIOS_3_1,
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

    Uint8 sound;
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

enum {
    HOTKEY_MASK_A = 0x1,
    HOTKEY_MASK_B = 0x2,
    HOTKEY_MASK_C = 0x4,
    HOTKEY_MASK_D = 0x8,
};

enum {
    BUT_A = 0,
    BUT_B,
    BUT_C,
    BUT_D,
    BUT_START,
    BUT_COIN,
    KB_UP,
    KB_DOWN,
    KB_LEFT,
    KB_RIGHT,
    BUT_HOTKEY0,
    BUT_HOTKEY1,
    BUT_HOTKEY2,
    BUT_HOTKEY3
};
enum {
    AXE_X = 6,
    AXE_Y,
    AXE_X_DIR,
    AXE_Y_DIR
};

//config conf;

//Uint8 key[SDLK_LAST];
extern Uint8 *joy_button[2];
extern Sint32 *joy_axe[2];
extern Uint32 joy_numaxes[2];

void debug_loop(void);
void main_loop(void);
void init_neo(void);
void cpu_68k_dpg_step(void);
void setup_misc_patch(char *name);
void neogeo_reset(void);

#ifdef ENABLE_PROFILER
#define PROFILER_START profiler_start
#define PROFILER_STOP profiler_stop

#else
#define PROFILER_START(a)
#define PROFILER_STOP(a)
#endif

/* LOG generation */
#define GNGEO_LOG(...)
#define DEBUG_LOG logMsg
//#define GNGEO_LOG printf

#endif
