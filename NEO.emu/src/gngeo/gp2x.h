#ifndef _GP2X_H_
#define _GP2X_H_

#include "unzip.h"

/* GP2X upper Memory management */
//Uint32 gp2x_upmem_base;
int gp2x_dev_mem;
int gp2x_gfx_dump;
//unzFile *gp2x_gfx_dump_gz;
int gp2x_mixer;

volatile Uint8 *gp2x_ram;
volatile Uint8 *gp2x_ram2;
volatile Uint8 *gp2x_ram2_uncached;
volatile Uint16 *gp2x_memregs;
volatile Uint32 *gp2x_memregl;

void gp2x_ram_init(void);
Uint8 *gp2x_ram_malloc(size_t size,Uint32 page);
void gp2x_quit(void);
void gp2x_sound_volume_set(int l, int r);
int gp2x_sound_volume_get(void);
void gp2x_init(void);
void gp2x_video_RGB_setscaling(int W, int H);
void gp2x_set_cpu_speed(void);
Uint32 gp2x_is_tvout_on(void);
void gp2x_init_940(void);
void gp2x_add_job940(int job);
void gp2x_ram_ptr_reset(void);

enum  { GP2X_UP=0,
	GP2X_UP_LEFT,
	GP2X_LEFT,
	GP2X_DOWN_LEFT,
	GP2X_DOWN,
	GP2X_DOWN_RIGHT,
	GP2X_RIGHT,
	GP2X_UP_RIGHT,
	GP2X_START,
	GP2X_SELECT,
	GP2X_R,
	GP2X_L,
	GP2X_A,
	GP2X_B,
	GP2X_X,
	GP2X_Y,
	GP2X_VOL_UP,
	GP2X_VOL_DOWN,
	GP2X_PUSH
};
typedef enum
{
        LCDR_60 = 0,    /* as close as possible to 60.00Hz, currently only managed to set to ~59.998Hz, has interlacing problems */
        LCDR_50,        /* 50Hz, has interlacing problems */
        LCDR_120_20,    /* ~60.10*2Hz, used by FCE Ultra */
        LCDR_100_02,    /* ~50.01*2Hz, used by FCE Ultra */
} lcd_rate_t;

extern void set_LCD_custom_rate(lcd_rate_t rate);
extern void unset_LCD_custom_rate(void);

#define CHECK_BUSY(job) \
        (gp2x_memregs[0x3b46>>1] & (1<<(job-1)))

#endif
