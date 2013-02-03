/***************************************************************************************
 *  Genesis Plus
 *  Virtual System Emulation
 *
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003  Charles Mac Donald (original code)
 *  Eke-Eke (2007-2011), additional code & fixes for the GCN/Wii port
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ****************************************************************************************/

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#include "genplus-config.h"

#ifndef NO_SYSTEM_PBC
#define SYSTEM_PBC        0x00
#endif
#define SYSTEM_GENESIS    0x01
#define SYSTEM_MEGADRIVE  0x02
#define SYSTEM_PICO       0x03

#define MCYCLES_PER_LINE  3420

/* Horizontal timing offsets when running in Z80 mode */
#define SMS_CYCLE_OFFSET  520
#define PBC_CYCLE_OFFSET  550

struct t_bitmap
{
  uint8 *data;      /* Bitmap data */
  int height;       /* Bitmap height */
  int pitch;        /* Width of bitmap in bytes */
  struct
  {
    int x;          /* X offset of viewport within bitmap */
    int y;          /* Y offset of viewport within bitmap */
    int w;          /* Width of viewport */
    int h;          /* Height of viewport */
    int ow;         /* Previous width of viewport */
    int oh;         /* Previous height of viewport */
    int changed;    /* 1= Viewport width or height have changed */
  } viewport;
};

typedef struct
{
  int sample_rate;  /* Output Sample rate (8000-48000) */
  float frame_rate; /* Output Frame rate (usually 50 or 60 frames per second) */
  float cddaRatio;
  int enabled;      /* 1= sound emulation is enabled */
  int buffer_size;  /* Size of sound buffer (in bytes) */
  struct
  {
	FMSampleType *pos;
	FMSampleType *buffer;
  } fm;
  struct
  {
    int16 *pos;
    int16 *buffer;
  } psg;
} t_snd;


/* Global variables */
extern t_bitmap bitmap;
extern t_snd snd;
//extern uint32 mcycles_z80;
//extern uint32 mcycles_68k;
extern uint32 mcycles_vdp;
extern uint8 system_hw;

/* Function prototypes */
extern int audio_init(int samplerate,float framerate);
extern void audio_reset(void);
extern void audio_shutdown(void);
extern int audio_update(int16 *sb);
extern void audio_set_equalizer(void);
extern void system_init(void);
extern void system_reset(void);
extern void system_shutdown(void);
extern void (*system_frame)(int do_skip, uint renderGfx);

static bool emuSystemIs16Bit()
{
	return system_hw != SYSTEM_PBC;
}

#endif /* _SYSTEM_H_ */

