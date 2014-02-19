#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#include <stdlib.h>

//#include "SDL.h"
#include "roms.h"
#include "resfile.h"
#include "unzip.h"
//#include "stb_zlib.h"
#include "conf.h"
#include "stb_image.h"


void zread_char(ZFILE *gz, char *c, int len) {
	int rc;
	rc = gn_unzip_fread(gz, (Uint8*)c, len);
	//printf("HS  %s %d\n",c,rc);
}
void zread_uint8(ZFILE *gz, Uint8 *c) {
	int rc;
	rc = gn_unzip_fread(gz, c, 1);
	//printf("H8  %02x %d\n",*c,rc);
}
void zread_uint32le(ZFILE *gz, Uint32 *c) {
	int rc;
	rc = gn_unzip_fread(gz, (Uint8*)c, sizeof(Uint32));
#ifdef WORDS_BIGENDIAN
	*c=SDL_Swap32(*c);
#endif
	//printf("H32  %08x %d\n",*c,rc);
}

/*
 * Load a rom definition file from gngeo.dat (rom/name.drv)
 * return ROM_DEF*, NULL on error
 */
ROM_DEF *res_load_drv(char *name) {
	char *gngeo_dat = CF_STR(cf_get_item_by_name("datafile"));
	ROM_DEF *drv;
	char drvfname[32];
	PKZIP *pz;
	ZFILE *z;
	int i;

	drv = calloc(sizeof(ROM_DEF), 1);

	/* Open the rom driver def */
	pz = gn_open_zip(gngeo_dat);
	if (pz == NULL) {
		free(drv);
		fprintf(stderr, "Can't open the %s\n", gngeo_dat);
		return NULL;
	}
	sprintf(drvfname, DATAFILE_PREFIX "rom/%s.drv", name);

	if ((z=gn_unzip_fopen(pz,drvfname,0x0)) == NULL) {
		free(drv);
		fprintf(stderr, "Can't open rom driver for %s\n", name);
		return NULL;
	}

	//Fill the driver struct
	zread_char(z, drv->name, 32);
	zread_char(z, drv->parent, 32);
	zread_char(z, drv->longname, 128);
	zread_uint32le(z, &drv->year);
	for (i = 0; i < 10; i++)
		zread_uint32le(z, &drv->romsize[i]);
	zread_uint32le(z, &drv->nb_romfile);
	for (i = 0; i < drv->nb_romfile; i++) {
		zread_char(z, drv->rom[i].filename, 32);
		zread_uint8(z, &drv->rom[i].region);
		zread_uint32le(z, &drv->rom[i].src);
		zread_uint32le(z, &drv->rom[i].dest);
		zread_uint32le(z, &drv->rom[i].size);
		zread_uint32le(z, &drv->rom[i].crc);
	}
	gn_unzip_fclose(z);
	gn_close_zip(pz);
	return drv;
}



/*
 * Load a stb image from gngeo.dat
 * return a SDL_Surface, NULL on error
 * supported format: bmp, tga, jpeg, png, psd
 * 24&32bpp only
 */
/*SDL_Surface *res_load_stbi(char *bmp) {
	PKZIP *pz;
	SDL_Surface *s;
	Uint8 * buffer;
	unsigned int size;
	int x, y, comp;
	stbi_uc *data = NULL;

	pz = gn_open_zip(CF_STR(cf_get_item_by_name("datafile")));
	if (!pz)
		return NULL;
	buffer = gn_unzip_file_malloc(pz, bmp, 0x0, &size);
	if (!buffer)
		return NULL;

	data = stbi_load_from_memory(buffer, size, &x, &y, &comp, 0);

	printf("STBILOAD %p %d %d %d %d\n", data, x, y, comp, x * comp);
	switch (comp) {
#ifdef WORDS_BIGENDIAN
	case 3:
		s = SDL_CreateRGBSurfaceFrom((void*) data, x, y, comp * 8, x * comp,
				0xFF0000, 0xFF00, 0xFF, 0);
		break;
	case 4:
		s = SDL_CreateRGBSurfaceFrom((void*) data, x, y, comp * 8, x * comp,
				0xFF000000, 0xFF0000, 0xFF00, 0xFF);
		break;
#else
	case 3:
		s = SDL_CreateRGBSurfaceFrom((void*) data, x, y, comp * 8, x * comp,
				0xFF, 0xFF00, 0xFF0000, 0);
		break;
	case 4:
		s = SDL_CreateRGBSurfaceFrom((void*) data, x, y, comp * 8, x * comp,
				0xFF, 0xFF00, 0xFF0000, 0xFF000000);
		break;
#endif
	default:
		printf("RES load STBI: Unhandled bpp surface\n");
		s = NULL;
		break;
	}
	free(buffer);
	if (s == NULL)
		printf("RES load STBI: Couldn't create surface\n");
	gn_close_zip(pz);
	return s;
}*/
/*
 * Load a Microsoft BMP from gngeo.dat
 * return a SDL Surface, NULL on error
 */
#if 0
SDL_Surface *res_load_bmp(char *bmp) {
	PKZIP *pz;
	SDL_Surface *s;
	Uint8 * buffer;
	SDL_RWops *rw;
	unsigned int size;

	pz = gn_open_zip(CF_STR(cf_get_item_by_name("datafile")));
	if (!pz)
		return NULL;
	buffer = gn_unzip_file_malloc(pz, bmp, 0x0, &size);
	if (!buffer) {
		gn_close_zip(pz);
		return NULL;
	}

	rw = SDL_RWFromMem(buffer, size);
	if (!rw) {
		printf("Error with SDL_RWFromMem: %s\n", SDL_GetError());
		gn_close_zip(pz);
		return NULL;
	}

	/* The function that does the loading doesn't change at all */
	s = SDL_LoadBMP_RW(rw, 0);
	if (!s) {
		printf("Error loading to SDL_Surface: %s\n", SDL_GetError());
		gn_close_zip(pz);
		return NULL;
	}

	/* Clean up after ourselves */
	free(buffer);
	SDL_FreeRW(rw);
	gn_close_zip(pz);
	return s;
}
#endif
void *res_load_data(char *name) {
	PKZIP *pz;
	Uint8 * buffer;
	unsigned int size;

	pz = gn_open_zip(CF_STR(cf_get_item_by_name("datafile")));
	if (!pz)
		return NULL;
	buffer = gn_unzip_file_malloc(pz, name, 0x0, &size);
	gn_close_zip(pz);
	return buffer;
}
