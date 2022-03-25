/* Roms/Ram driver interface */
#ifndef H_ROMS
#define H_ROMS

//#include "SDL.h"
#include <gngeoTypes.h>
#include <stdbool.h>

#define REGION_AUDIO_CPU_BIOS        0
#define REGION_AUDIO_CPU_CARTRIDGE   1
#define REGION_AUDIO_CPU_ENCRYPTED   2
#define REGION_AUDIO_DATA_1          3
#define REGION_AUDIO_DATA_2          4
#define REGION_FIXED_LAYER_BIOS      5
#define REGION_FIXED_LAYER_CARTRIDGE 6
#define REGION_MAIN_CPU_BIOS         7
#define REGION_MAIN_CPU_CARTRIDGE    8
#define REGION_SPRITES               9
#define REGION_SPR_USAGE             10
#define REGION_GAME_FIX_USAGE        11

#define HAS_CUSTOM_CPU_BIOS 0x1
#define HAS_CUSTOM_AUDIO_BIOS 0x2
#define HAS_CUSTOM_SFIX_BIOS 0x4

typedef struct ROM_DEF{
	char name[32];
	char parent[32];
	char longname[128];
	Uint32 year;
	Uint32 romsize[10];
	Uint32 nb_romfile;
	struct romfile{
		char filename[32];
		Uint8 region;
		Uint32 src;
		Uint32 dest;
		Uint32 size;
		Uint32 crc;
	}rom[32];
}ROM_DEF;

typedef struct GAME_INFO {
	char name[32];
	char longname[128];
	int year;
	Uint32 flags;
}GAME_INFO;

typedef struct ROM_REGION {
	Uint8* p;
	Uint32 size;
}ROM_REGION;


typedef struct GAME_ROMS {
	GAME_INFO info;
	ROM_REGION cpu_m68k;
	ROM_REGION cpu_z80;
	ROM_REGION tiles;
	ROM_REGION game_sfix;
	ROM_REGION bios_sfix;
	ROM_REGION bios_audio;
	//ROM_REGION zoom_table;
	ROM_REGION bios_m68k;
	ROM_REGION adpcma;
	ROM_REGION adpcmb;
	ROM_REGION spr_usage;
	ROM_REGION gfix_usage;  /* Game fix char usage */
	//ROM_REGION bfix_usage;  /* Bios fix char usage */
	ROM_REGION cpu_z80c; /* Crypted z80 program rom */
}GAME_ROMS;



int dr_load_roms(void *contextPtr, GAME_ROMS *r,char *rom_path,char *name, char romerror[1024]);
void dr_free_roms(GAME_ROMS *r);
int dr_save_gno(GAME_ROMS *r,char *filename);
int dr_load_game(void *contextPtr, char *zip, char romerror[1024]);
ROM_DEF *dr_check_zip(void *contextPtr, const char *filename);
char *dr_gno_romname(char *filename);
int dr_open_gno(void *contextPtr, char *filename, char romerror[1024]);

struct PathArray
{
	char data[4096];
};

struct PathArray get_rom_path(void *contextPtr);

#endif
