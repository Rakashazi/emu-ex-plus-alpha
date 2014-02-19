/*
 * unzip.h
 * Basic unzip interface
 *
 *  Created on: 1 janv. 2010
 *      Author: Mathieu Peponas
 */

#ifndef UNZIP_H_
#define UNZIP_H_

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
#include "zlib.h"
#else
#include "stb_zlib.h"
#endif

#include <stdio.h>
#include <stdint.h>

typedef struct ZFILE {
	//char *name;
	int pos;
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
	z_streamp zb;
	uint8_t *inbuf;
#else
	zbuf *zb;
#endif
	FILE *f;
	int csize,uncsize;
	int cmeth; /* compression method */
	int readed;
}ZFILE;

typedef struct PKZIP {
	FILE *file;
	unsigned int cd_offset; /* Central dir offset */
	unsigned int cd_size;
	unsigned int cde_offset;
	uint16_t nb_item;
	uint8_t *map;
}PKZIP;

void gn_unzip_fclose(ZFILE *z);
int gn_unzip_fread(ZFILE *z,uint8_t *data,unsigned int size);
ZFILE *gn_unzip_fopen(PKZIP *zf,const char *filename,uint32_t file_crc);
PKZIP *gn_open_zip(char *file);
uint8_t *gn_unzip_file_malloc(PKZIP *zf,const char *filename,uint32_t file_crc,unsigned int *outlen);
void gn_close_zip(PKZIP *zf);
int gn_strictROMChecking();

#endif /* UNZIP_H_ */
