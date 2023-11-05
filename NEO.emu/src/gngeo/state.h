#ifndef _STATE_H_
#define _STATE_H_

#include <gngeoTypes.h>

#include <zlib.h>
//#include "SDL.h"
#include <stdbool.h>

typedef enum ST_MODULE_TYPE {
    ST_68k=0,
    ST_Z80,
    ST_YM2610,
    ST_YM2610_FM,
    ST_YM2610_ADPCMA,
    ST_YM2610_ADPCMB,
    ST_TIMER,
    ST_PD4990A,
    ST_NEOGEO, /* all the other reg will go here */
    ST_MODULE_END
}ST_MODULE_TYPE;

typedef enum ST_DATA_TYPE {
    REG_UINT8=1,
    REG_UINT16,
    REG_UINT32,
    REG_INT8,
    REG_INT16,
    REG_INT32
}ST_DATA_TYPE;


typedef struct ST_REG {
    char *reg_name;
    void *data;
    Uint8 num;
    Uint32 size;
    ST_DATA_TYPE type;
    struct ST_REG *next;
}ST_REG;

typedef struct ST_MODULE {
    void (*pre_save_state)(void);
    void (*post_load_state)(void);
    ST_REG *reglist;
}ST_MODULE;


typedef struct M68K_STATE {
    Uint32 dreg[8];
    Uint32 areg[8];
    Uint32 asp;
    Uint32 pc;
    Uint32 sr;
    Uint32 bank;
    Uint8  ram[0x10000];
}M68K_STATE;

typedef struct Z80_STATE {
    Uint16 PC,SP,AF,BC,DE,HL,IX,IY;
    Uint16 AF2,BC2,DE2,HL2;
    Uint8  R,R2,IFF1,IFF2,IM,I;
    Uint8  IRQV,IRQL;
    Uint16 bank[4];
    Uint8  ram[0x800];
}Z80_STATE;

typedef struct NEOGEO_STATE {
    Uint16 vptr;
    Sint16 modulo;
    Uint8 current_pal;
    Uint8 current_fix;
    Uint8 sram_lock;
    Uint8 sound_code;
    Uint8 pending_command;
    Uint8 result_code;
    Uint8 sram[0x10000];
    Uint8 video[0x20000];
    Uint8 pal1[0x2000], pal2[0x2000];
}NEOGEO_STATE;

//SDL_Surface *state_img;

#define STREAD  0
#define STWRITE 1

#if 0
void create_state_register(ST_MODULE_TYPE module,const char *reg_name,Uint8 num,void *data,int size,ST_DATA_TYPE type);
void set_pre_save_function(ST_MODULE_TYPE module,void (*func)(void));
void set_post_load_function(ST_MODULE_TYPE module,void (*func)(void));
#endif
typedef void Stream;
//SDL_Surface *load_state_img(char *game,int slot);
int save_stateWithName(void *contextPtr, const char *name);
int load_stateWithName(void *contextPtr, const char *name);
Uint32 how_many_slot(char *game);
int mkstate_data(Stream *gzf,void *data,int size,int mode);
gzFile gzopenHelper(void *contextPtr, const char *filename, const char *mode);

//void neogeo_init_save_state(void);

#endif

