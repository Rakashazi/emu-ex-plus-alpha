#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

//#include "SDL.h"
//#include "SDL_endian.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
#include <zlib.h>
#endif

#include "memory.h"
#include "state.h"
#include "fileio.h"
#include "screen.h"
#include "sound.h"
#include "emu.h"
#include "timer.h"
//#include "streams.h"

#ifdef USE_STARSCREAM
static int m68k_flag=0x1;
#elif USE_GENERATOR68K
static int m68k_flag=0x2;
#elif USE_CYCLONE
static int m68k_flag=0x3;
#endif

#ifdef USE_RAZE
static int z80_flag=0x4;
#elif USE_MAMEZ80
static int z80_flag=0x8;
#elif USE_DRZ80
static int z80_flag=0xC;
#endif

#ifdef WORDS_BIGENDIAN
static int endian_flag=0x10;
#else
static int endian_flag=0x0;
#endif

#if defined (WII)
#define ROOTPATH "sd:/apps/gngeo/"
#elif defined (__AMIGA__)
#define ROOTPATH "/PROGDIR/data/"
#else
#define ROOTPATH ""
#endif

#if !defined(HAVE_LIBZ) || !defined (HAVE_MMAP)
#define gzopen fopen
#define gzread(f,data,size) fread(data,size,1,f)
#define gzwrite(f,data,size) fwrite(data,size,1,f)
#define gzclose fclose
#define gzFile FILE
#define gzeof feof
#define gzseek fseek

#endif

static ST_REG *reglist;
static ST_MODULE st_mod[ST_MODULE_END];
static GN_Rect buf_rect    =	{24, 16, 304, 224};
static GN_Rect screen_rect =	{ 0,  0, 304, 224};
//SDL_Surface *state_img_tmp;

void cpu_68k_mkstate(gzFile gzf,int mode);
void cpu_z80_mkstate(gzFile gzf,int mode);
void ym2610_mkstate(gzFile gzf,int mode);
void timer_mkstate(gzFile gzf,int mode);
void pd4990a_mkstate(gzFile gzf,int mode);

#if 0
void create_state_register(ST_MODULE_TYPE module,const char *reg_name,
			   Uint8 num,void *data,int size,ST_DATA_TYPE type) {
    ST_REG *t=(ST_REG*)calloc(1,sizeof(ST_REG));
    t->next=st_mod[module].reglist;
    st_mod[module].reglist=t;
    t->reg_name=strdup(reg_name);
    t->data=data;
    t->size=size;
    t->type=type;
    t->num=num;
}

void set_pre_save_function(ST_MODULE_TYPE module,void (*func)(void)) {
    st_mod[module].pre_save_state=func;
}

void set_post_load_function(ST_MODULE_TYPE module,void (*func)(void)) {
    st_mod[module].post_load_state=func;
}

static void *find_data_by_name(ST_MODULE_TYPE module,Uint8 num,char *name) {
    ST_REG *t=st_mod[module].reglist;
    while(t) {
	if ((!strcmp(name,t->reg_name)) && (t->num==num)) {
	    /*
	     *len=t->size;
	     *type=t->type;
	     */
	    return t->data;
	}
	t=t->next;
    }
    return NULL;
}

static int sizeof_st_type(ST_DATA_TYPE type) {
    switch (type) {
    case REG_UINT8:
    case REG_INT8:
	return 1;
    case REG_UINT16:
    case REG_INT16:
	return 2;
    case REG_UINT32:
    case REG_INT32:
	return 4;
    }
    return 0; /* never go here */
}

void swap_buf16_if_need(Uint8 src_endian,Uint16* buf,Uint32 size)
{
    int i;
#ifdef WORDS_BIGENDIAN
    Uint8  my_endian=1;
#else
    Uint8  my_endian=0;
#endif
    if (my_endian!=src_endian) {
	for (i=0;i<size;i++)
	    SDL_Swap16(buf[i]);
    }
}

void swap_buf32_if_need(Uint8 src_endian,Uint32* buf,Uint32 size)
{
    int i;
#ifdef WORDS_BIGENDIAN
    Uint8  my_endian=1;
#else
    Uint8  my_endian=0;
#endif
    if (my_endian!=src_endian) {
	for (i=0;i<size;i++)
	    buf[i]=SDL_Swap32(buf[i]);
    }
}

Uint32 how_many_slot(char *game) {
	char *st_name;
	FILE *f;
//    char *st_name_len;
#ifdef EMBEDDED_FS
	char *gngeo_dir=ROOTPATH"save/";
#else
	char *gngeo_dir=get_gngeo_dir();
#endif
	Uint32 slot=0;
	st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
	while (1) {
		sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);
		if ((f=fopen(st_name,"rb"))) {
			fclose(f);
			slot++;
		} else
		    return slot;
	}
}
#endif

#if 0
SDL_Surface *load_state_img(char *game,int slot) {
	char *st_name;
//    char *st_name_len;
#ifdef EMBEDDED_FS
	char *gngeo_dir="save/";
#else
	char *gngeo_dir=get_gngeo_dir();
#endif
	
#ifdef WORDS_BIGENDIAN
	Uint8  my_endian=1;
#else
	Uint8  my_endian=0;
#endif
	char string[20];
	gzFile gzf;
	Uint8  endian;
	Uint32 rate;

    st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
    sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);

    if ((gzf=gzopen(st_name,"rb"))==NULL) {
    	logMsg("%s not found\n",st_name);
	return NULL;
    }

    memset(string,0,20);
    gzread(gzf,string,6);

    if (strcmp(string,"GNGST1")) {
    	logMsg("%s is not a valid gngeo st file\n",st_name);
	gzclose(gzf);
	return NULL; 
    }

    gzread(gzf,&endian,1);

    if (my_endian!=endian) {
    	logMsg("This save state comme from a different endian architecture.\n"
	       "This is not currently supported :(\n");
	return NULL;
    }

    gzread(gzf,&rate,4); // don't care
    
    gzread(gzf,state_img_tmp->pixels,304*224*2);
    gzclose(gzf);
    return state_img_tmp;
}

bool load_state(char *game,int slot) {
    char *st_name;
//    char *st_name_len;
#ifdef EMBEDDED_FS
    char *gngeo_dir="save/";
#else
    char *gngeo_dir=get_gngeo_dir();
#endif

#ifdef WORDS_BIGENDIAN
    Uint8  my_endian=1;
#else
    Uint8  my_endian=0;
#endif

    int i;
    gzFile gzf;
    char string[20];
    Uint8 a,num;
    ST_DATA_TYPE type;
    void *data;
    Uint32 len;

    Uint8  endian;
    Uint32 rate;

    st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
    sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);

    if ((gzf=gzopen(st_name,"rb"))==NULL) {
    	logMsg("%s not found\n",st_name);
	return false;
    }

    memset(string,0,20);
    gzread(gzf,string,6);

    if (strcmp(string,"GNGST1")) {
    	logMsg("%s is not a valid gngeo st file\n",st_name);
	gzclose(gzf);
	return false;
    }


    gzread(gzf,&endian,1);

    if (my_endian!=endian) {
    	logMsg("This save state comme from a different endian architecture.\n"
	       "This is not currently supported :(\n");
	return false;
    }

    gzread(gzf,&rate,4);
    swap_buf32_if_need(endian,&rate,1);

#ifdef GP2X
    if (rate==0 && conf.sound) {
	    gn_popup_error("Failed!",
			   "This save state is incompatible "
			   "because you have sound enabled "
			   "and this save state don't have sound data");
	    return false;
    }
    if (rate!=0 && conf.sound==0) {
	    gn_popup_error("Failed!",
			   "This save state is incompatible "
			   "because you don't have sound enabled "
			   "and this save state need it");
	    return false;
    } else if (rate!=conf.sample_rate && conf.sound) {
	    conf.sample_rate=rate;
	    close_sdl_audio();
	    init_sdl_audio();
    }
#else
    if (rate==0 && conf.sound) {
	/* disable sound */
	conf.sound=0;
	pause_audio(1);
	close_sdl_audio();
    } else if (rate!=0 && conf.sound==0) {
	/* enable sound */
	conf.sound=1;
	conf.sample_rate=rate;
	if (!conf.snd_st_reg_create) {
	    cpu_z80_init();
	    init_sdl_audio();
	    //streams_sh_start();
	    YM2610_sh_start();
	    conf.snd_st_reg_create=1;
	} else 
	    init_sdl_audio();
	pause_audio(0);
    } else if (rate!=conf.sample_rate && conf.sound) {
	conf.sample_rate=rate;
	close_sdl_audio();
	init_sdl_audio();
    }
#endif


    gzread(gzf,state_img->pixels,304*224*2);
    swap_buf16_if_need(endian,state_img->pixels,304*224);

    

    while(!gzeof(gzf)) {
	gzread(gzf,&a,1); /* name size */
	memset(string,0,20);
	gzread(gzf,string,a); /* regname */
	gzread(gzf,&num,1); /* regname num */
	gzread(gzf,&a,1); /* module id */
	gzread(gzf,&len,4);
	gzread(gzf,&type,1);
	data=find_data_by_name(a,num,string);
	if (data) {
	    gzread(gzf,data,len);
	    switch(type) {
	    case REG_UINT16:
	    case REG_INT16:
		swap_buf16_if_need(endian,data,len>>1);
		break;
	    case REG_UINT32:
	    case REG_INT32:
		swap_buf32_if_need(endian,data,len>>2);
		break;
	    case REG_INT8:
	    case REG_UINT8:
		/* nothing */
		break;
	    }
	} else {
	    /* unknow reg, ignore it*/
		logMsg("skeeping unknow reg %s\n",string);
	    gzseek(gzf,len,SEEK_CUR);
	}
    
    
	// /*if (a==ST_68k)*/ printf("LO %02d %20s %02x %08x \n",a,string,num,len/*,*(Uint32*)data*/);
    }
    gzclose(gzf);

    for(i=0;i<ST_MODULE_END;i++) {
	if (st_mod[i].post_load_state) st_mod[i].post_load_state();
    }

    return true;
}

bool save_state(char *game,int slot) {
     char *st_name;
//    char *st_name_len;
#ifdef EMBEDDED_FS
     char *gngeo_dir="save/";
#else
     char *gngeo_dir=get_gngeo_dir();
#endif

    Uint8 i;
    gzFile gzf;
    char string[20];
    Uint8 a,num;
    ST_DATA_TYPE type;
    void *data;
    Uint32 len;
#ifdef WORDS_BIGENDIAN
    Uint8  endian=1;
#else
    Uint8  endian=0;
#endif
    Uint32 rate=(conf.sound?conf.sample_rate:0);

    st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
    sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);

    if ((gzf=gzopen(st_name,"wb"))==NULL) {
    	logMsg("can't write to %s\n",st_name);
	return false;
    }

/*
#ifndef GP2X
    SDL_BlitSurface(buffer, &buf_rect, state_img, &screen_rect);
#endif
*/

    gzwrite(gzf,"GNGST1",6);
    gzwrite(gzf,&endian,1);
    gzwrite(gzf,&rate,4);
    gzwrite(gzf,state_img->pixels,304*224*2);
    for(i=0;i<ST_MODULE_END;i++) {
	ST_REG *t=st_mod[i].reglist;
	if (st_mod[i].pre_save_state) st_mod[i].pre_save_state();
	while(t) {
	    // /*if (i==ST_68k)*/ printf("SV %02d %20s %02x %08x \n",i,t->reg_name,t->num,t->size/*,*(Uint32*)t->data*/);
	    
	    a=strlen(t->reg_name);
	    gzwrite(gzf,&a,1); /* strlen(regname) */
	    gzwrite(gzf,t->reg_name,strlen(t->reg_name)); /* regname */
	    gzwrite(gzf,&t->num,1); /* regname num */
	    gzwrite(gzf,&i,1); /* module id */
	    gzwrite(gzf,&t->size,4);
	    gzwrite(gzf,&t->type,1);
	    gzwrite(gzf,t->data,t->size);

	    t=t->next;
	}
    }
    gzclose(gzf);
   return true;

}

#else

static const char *getGngeoDir()
{
#ifdef EMBEDDED_FS
	return ROOTPATH"save/";
#else
	return get_gngeo_dir();
#endif
}

static void make_stateName(char *game,int slot,char *st_name_out)
{
	sprintf(st_name_out,"%s%s.%03d",getGngeoDir(),game,slot);
}

static gzFile open_state(/*char *game,int slot,*/char *st_name,int mode) {
	/*char *st_name;
//    char *st_name_len;
#ifdef EMBEDDED_FS
	char *gngeo_dir=ROOTPATH"save/";
#else
	char *gngeo_dir=get_gngeo_dir();
#endif*/
	char string[20];
	char *m=(mode==STWRITE?"wb":"rb");
	gzFile gzf;
	int  flags;
	Uint32 rate;

    /*st_name=(char*)alloca(strlen(gngeo_dir)+strlen(game)+5);
    sprintf(st_name,"%s%s.%03d",gngeo_dir,game,slot);*/

	if ((gzf = gzopen(st_name, m)) == NULL) {
		logMsg("%s not found\n", st_name);
		return NULL;
    }

	static const char *stateSig = "GNGST3";

	if(mode==STREAD) {

		memset(string, 0, 20);
		gzread(gzf, string, 6);

		if (strcmp(string, stateSig)) {
			logMsg("%s is not a valid gngeo st file", st_name);
			gzclose(gzf);
			return NULL;
		}

		gzread(gzf, &flags, sizeof (int));

		if (flags != (m68k_flag | z80_flag | endian_flag)) {
			logMsg("This save state comes from a different endian architecture.\n"
					"This is not currently supported :(");
			gzclose(gzf);
			return NULL;
		}
	} else {
		int flags=m68k_flag | z80_flag | endian_flag;
		gzwrite(gzf, stateSig, 6);
		gzwrite(gzf, &flags, sizeof(int));
	}
	return gzf;
}

/*static gzFile open_state(char *game,int slot,int mode) {
	char *st_name=(char*)alloca(strlen(getGngeoDir())+strlen(game)+5);
	make_state_name(game,slot,st_name);
	return open_stateWithName(st_name, mode);
}*/

int mkstate_data(gzFile gzf,void *data,int size,int mode) {
	if (mode==STREAD)
		return gzread(gzf,data,size);
	return gzwrite(gzf,data,size);
}

/*SDL_Surface *load_state_img(char *game,int slot) {
	gzFile *gzf;

	if ((gzf = open_state(game, slot, STREAD)) == NULL)
		return NULL;

	gzread(gzf, state_img_tmp->pixels, 304 * 224 * 2);


    gzclose(gzf);
    return state_img_tmp;
}*/

static void neogeo_mkstate(gzFile gzf,int mode) {
	GAME_ROMS r;
	memcpy(&r,&memory.rom,sizeof(GAME_ROMS));
	mkstate_data(gzf, &memory, sizeof (memory), mode);

	/* Roms info are needed (at least) for z80 bankswitch, so we need to restore
	 * it asap */
	if (mode==STREAD) memcpy(&memory.rom,&r,sizeof(GAME_ROMS));


	mkstate_data(gzf, &bankaddress, sizeof (Uint32), mode);
	mkstate_data(gzf, &sram_lock, sizeof (Uint8), mode);
	mkstate_data(gzf, &sound_code, sizeof (Uint8), mode);
	mkstate_data(gzf, &pending_command, sizeof (Uint8), mode);
	mkstate_data(gzf, &result_code, sizeof (Uint8), mode);
	mkstate_data(gzf, &neogeo_frame_counter_speed, sizeof (Uint32), mode);
	mkstate_data(gzf, &neogeo_frame_counter, sizeof (Uint32), mode);
	cpu_68k_mkstate(gzf, mode);
#ifndef ENABLE_940T
	mkstate_data(gzf, z80_bank,sizeof(Uint16)*4, mode);
	cpu_z80_mkstate(gzf, mode);
	ym2610_mkstate(gzf, mode);
	timer_mkstate(gzf, mode);
#else
/* TODO */
#endif
	pd4990a_mkstate(gzf, mode);
}

int save_stateWithName(char *name) {
	gzFile gzf;

	if ((gzf = open_state(name, STWRITE)) == NULL)
		return false;

	//gzwrite(gzf, state_img->pixels, 304 * 224 * 2);

	neogeo_mkstate(gzf,STWRITE);

	gzclose(gzf);
	return true;
}

int save_state(char *game,int slot) {
	char *st_name=(char*)alloca(strlen(getGngeoDir())+strlen(game)+5);
	make_stateName(game,slot,st_name);
	return save_stateWithName(st_name);
}

int load_stateWithName(char *name) {
	gzFile gzf;
	/* Save pointers */
	Uint8 *ng_lo = memory.ng_lo;
	Uint8 *fix_game_usage=memory.fix_game_usage;
	Uint8 *bksw_unscramble = memory.bksw_unscramble;
	int *bksw_offset=memory.bksw_offset;
//	GAME_ROMS r;
//	memcpy(&r,&memory.rom,sizeof(GAME_ROMS));
	
	if ((gzf = open_state(name, STREAD))==NULL)
		return false;

	//gzread(gzf,state_img_tmp->pixels,304*224*2);

	neogeo_mkstate(gzf,STREAD);

	/* Restore them */
	memory.ng_lo=ng_lo;
	memory.fix_game_usage=fix_game_usage;
	memory.bksw_unscramble=bksw_unscramble;
	memory.bksw_offset=bksw_offset;
//	memcpy(&memory.rom,&r,sizeof(GAME_ROMS));

	cpu_68k_bankswitch(bankaddress);

	if (memory.current_vector==0)
		memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
	else
		memcpy(memory.rom.cpu_m68k.p, memory.game_vector, 0x80);

	if (memory.vid.currentpal) {
		current_pal = memory.vid.pal_neo[1];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[1];
	} else {
		current_pal = memory.vid.pal_neo[0];
		current_pc_pal = (Uint32 *) memory.vid.pal_host[0];
	}

	if (memory.vid.currentfix) {
		current_fix = memory.rom.game_sfix.p;
		fix_usage = memory.fix_game_usage;
	} else {
		current_fix = memory.rom.bios_sfix.p;
		fix_usage = memory.fix_board_usage;
	}

	gzclose(gzf);
	return true;
}

int load_state(char *game,int slot) {
	char *st_name=(char*)alloca(strlen(getGngeoDir())+strlen(game)+5);
	make_stateName(game,slot,st_name);
	return load_stateWithName(st_name);
}
#endif

#if 0
/* neogeo state register */ 
static Uint8 st_current_pal,st_current_fix;

static void neogeo_pre_save_state(void) {

    //st_current_pal=(current_pal==memory.pal1?0:1);
    //st_current_fix=(current_fix==memory.rom.bios_sfix.p?0:1);
    //printf("%d %d\n",st_current_pal,st_current_fix);
    
}

static void neogeo_post_load_state(void) {
	int i;
	//printf("%d %d\n",st_current_pal,st_current_fix);
    //current_pal=(st_current_pal==0?memory.pal1:memory.pal2);
    //current_pc_pal=(Uint32 *)(st_current_pal==0?memory.pal_pc1:memory.pal_pc2);
    current_fix=(st_current_fix==0?memory.rom.bios_sfix.p:memory.rom.game_sfix.p);
    update_all_pal();
 
}

void clear_state_reg(void) {
    int i;
    ST_REG *t,*s;
    for(i=0;i<ST_MODULE_END;i++) {
	t=st_mod[i].reglist;
	while (t) {
	    s=t;t=t->next;
	    free(s);
	}
	st_mod[i].reglist=NULL;
    }
}

void neogeo_init_save_state(void) {
    int i;
    ST_REG *t,*s;
    /*if (!state_img)
		state_img=SDL_CreateRGBSurface(SDL_SWSURFACE,304, 224, 16, 0xF800, 0x7E0, 0x1F, 0);
	if (!state_img_tmp)
		state_img_tmp=SDL_CreateRGBSurface(SDL_SWSURFACE,304, 224, 16, 0xF800, 0x7E0, 0x1F, 0);*/

/*
    for(i=0;i<ST_MODULE_END;i++) {
	t=st_mod[i].reglist;
	while (t) {
	    s=t;t=t->next;
	    free(s);
	}
	st_mod[i].reglist=NULL;
    }
*/

    //create_state_register(ST_NEOGEO,"vptr",1,(void *)&vptr,sizeof(Sint32),REG_INT32);
    //create_state_register(ST_NEOGEO,"modulo",1,(void *)&modulo,sizeof(Sint16),REG_INT16);
    create_state_register(ST_NEOGEO,"current_pal",1,(void *)&st_current_pal,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_NEOGEO,"current_fix",1,(void *)&st_current_fix,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_NEOGEO,"sram_lock",1,(void *)&sram_lock,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_NEOGEO,"sound_code",1,(void *)&sound_code,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_NEOGEO,"pending_command",1,(void *)&pending_command,sizeof(Uint8),REG_UINT8);
    create_state_register(ST_NEOGEO,"result_code",1,(void *)&result_code,sizeof(Uint8),REG_UINT8);
    //create_state_register(ST_NEOGEO,"sram",1,(void *)memory.sram,0x10000,REG_UINT8);
    //create_state_register(ST_NEOGEO,"pal1",1,(void *)memory.pal1,0x2000,REG_UINT8);
    //create_state_register(ST_NEOGEO,"pal2",1,(void *)memory.pal2,0x2000,REG_UINT8);
    create_state_register(ST_NEOGEO,"video",1,(void *)memory.vid.ram,0x20000,REG_UINT8);
//    create_state_register(ST_NEOGEO,"irq2enable",1,(void *)&irq2enable,sizeof(Uint16),REG_UINT16);
//    create_state_register(ST_NEOGEO,"irq2start",1,(void *)&irq2start,sizeof(Uint16),REG_UINT16);
//    create_state_register(ST_NEOGEO,"irq2repeat",1,(void *)&irq2repeat,sizeof(Uint16),REG_UINT16);
//    create_state_register(ST_NEOGEO,"irq2control",1,(void *)&irq2control,sizeof(Uint16),REG_UINT16);
//    create_state_register(ST_NEOGEO,"lastirq2line",1,(void *)&lastirq2line,sizeof(Uint16),REG_UINT16);
    create_state_register(ST_NEOGEO,"fc_speed",1,(void *)&neogeo_frame_counter_speed,sizeof(Sint32),REG_INT32);
    create_state_register(ST_NEOGEO,"fc",1,(void *)&neogeo_frame_counter,sizeof(Sint32),REG_INT32);


    set_post_load_function(ST_NEOGEO,neogeo_post_load_state);
    set_pre_save_function(ST_NEOGEO,neogeo_pre_save_state);
    
}
#endif
