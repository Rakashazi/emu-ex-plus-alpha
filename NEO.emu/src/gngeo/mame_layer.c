#include "mame_layer.h"
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <imagine/logger/logger.h>

/*
  fixed
  main
  mainbios
  sprites
  ym

audiocpu
audiocrypt
fixed
mainbios
maincpu
sprites
ym

*/

UINT8 *memory_region( GAME_ROMS *r, const char *region ) {
	assert(region);
	if (strcmp(region,"audiocpu")==0) return r->cpu_z80.p;
	if (strcmp(region,"audiocrypt")==0) return r->cpu_z80c.p;
	if (strcmp(region,"fixed")==0) return r->game_sfix.p;
	if (strcmp(region,"maincpu")==0) return r->cpu_m68k.p;
	if (strcmp(region,"mainbios")==0) return r->bios_m68k.p;
	if (strcmp(region,"sprites")==0) return r->tiles.p;
	if (strcmp(region,"ym")==0) return r->adpcma.p;
	logMsg("memory_region %s not found",region);
	
	return NULL;
}
UINT32 memory_region_length( GAME_ROMS *r, const char *region ) {
	assert(region);
	if (strcmp(region,"audiocpu")==0) return r->cpu_z80.size;
	if (strcmp(region,"audiocrypt")==0) return r->cpu_z80c.size;
	if (strcmp(region,"fixed")==0) return r->game_sfix.size;
	if (strcmp(region,"maincpu")==0) return r->cpu_m68k.size;
	if (strcmp(region,"mainbios")==0) return r->bios_m68k.size;
	if (strcmp(region,"sprites")==0) return r->tiles.size;
	if (strcmp(region,"ym")==0) return r->adpcma.size;
	logMsg("memory_region_length %s not found",region);
	
	return 0;
}

void *malloc_or_die(UINT32 b) {
	void *a=malloc(b);
	if (a) return a;
	logMsg("Not enough memory :( exiting\n");
	exit(1);
	return NULL;
}
