/*
 * unzip.c
 * Basic unzip interface
 *
 *  Created on: 1 janv. 2010
 *      Author: Mathieu Peponas
 */

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
#include <zlib.h>
//#define ZLIB_IN_CHUNK 128*1024
#include <unistd.h>
#include <sys/mman.h>
#include <assert.h>
#else
#include "stb_zlib.h"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "unzip.h"
#include <imagine/logger/logger.h>

static int fget8(FILE *f) {
	if (f) {
		int c = fgetc(f);
		return c == EOF ? 0 : c;
	}
	return 0;
}
static uint8_t fget8u(FILE *f) {
	return (uint8_t) fget8(f);
}
static void fskip(FILE *f, int n) {
	uint8_t a, i;
    size_t tr;
	if (!f)
		return;//fseek(f,n,SEEK_CUR);
	for (i = 0; i < n; i++)
		tr=fread(&a, 1, 1, f);
}
static uint16_t fget16(FILE *f) {
	int z = fget8(f);
	return (z << 8) + fget8(f);
}

static uint32_t fget32(FILE *f) {
	uint32_t z = fget16(f);
	return (z << 16) + fget16(f);
}

static uint16_t fget16le(FILE *f) {
	int z = fget8(f);
	return z + (fget8(f) << 8);
}

static uint32_t fget32le(FILE *f) {
	uint32_t z = fget16le(f);
	return z + (fget16le(f) << 16);
}

static void freadvle(FILE *f, char *fmt, ...) {
	va_list v;
	va_start(v,fmt);
	//printf("%d\n",sizeof(uint16_t));
	while (*fmt) {
		switch (*fmt++) {
		case ' ':
			break;
		case '1': {
			uint8_t *x = (uint8_t*) va_arg(v, int*);
			//printf("%p\n",x);
			if (x)
				(*x) = fget8(f);
			else
				fskip(f, 1);
			break;
		}
		case '2': {
			uint16_t *x = (uint16_t*) va_arg(v, int*);
			//printf("%p\n",x);
			if (x)
				(*x) = fget16le(f);
			else
				fskip(f, 2);
			break;
		}
		case '4': {
			uint32_t *x = (uint32_t*) va_arg(v, int*);
			//printf("%p\n",x);
			if (x)
				(*x) = fget32le(f);
			else
				fskip(f, 4);
			break;
		}
		default:
			va_end(v);
			return;
		}
	}
	va_end(v);
}

static int search_sig32(PKZIP *zf, uint8_t sig[4]) {
	int i = 0, pos;
	uint8_t a;
	int t = 0;
    size_t tr=0;

	if (!zf || !zf->file)
		return -1;
	//pos=ftell(zf->file);
	while (!feof(zf->file)) {
		tr+=fread(&a, sizeof(char), 1, zf->file);
		//printf("Search sig %d\n",a);
		if (sig[i] == a) {
			//printf("%02x %d\n",a,t);
			if (i == 3) {
				//fseek(zf->file,-4,SEEK_CUR);
				pos = ftell(zf->file) - 4;
				return pos;
				break;
			}
			i++;
		} else
			i = 0;
		t++;
	}
	return -1;
}

static int search_central_dir(PKZIP *zf) {
	int i = 0;
	unsigned char sig_cde[4] = { 0x50, 0x4b, 0x05, 0x06 };
	unsigned char sig_cd[4] = { 0x50, 0x4b, 0x01, 0x02 };
	uint16_t nbdsk;

	if (!zf || !zf->file)
		return -1;
	zf->cde_offset = 0;
	for (i = 0; i < 0xFFFF; i += 0x100) {
		fseek(zf->file, -i - 22, SEEK_END);
		/* Max comment size + size of [end of central dir record ] */
		//printf("search ecd iteration %d\n", i);
		zf->cde_offset = search_sig32(zf, sig_cde);

		if (zf->cde_offset != -1) {
			break;
		}
	}
	if (zf->cde_offset == -1) {
		logMsg("Couldn't find central dir, Corrupted zip\n");
		return -1;
	}
	//printf("End of central dir here %08x\n", zf->cde_offset);
	//freadvle(zf->file,"4 22 22 44",0,&nbdsk,0,0,&zf->nb_item,&zf->cd_size,&zf->cd_offset);
	freadvle(zf->file, "22 22 44", &nbdsk, 0, 0, &zf->nb_item, &zf->cd_size,
			&zf->cd_offset);
	if (nbdsk != 0) {
		logMsg("Multi disk not supported (%d)\n", nbdsk);
		return -1;
	}
	/*fskip(zf->file,12);
	 zf->cd_size=fget32le(zf->file);
	 zf->cd_offset=fget32le(zf->file);*/
	//printf("CD off=%08x CD size=%08x\n", zf->cd_offset, zf->cd_size);
	/* check sig */
	fseek(zf->file, zf->cd_offset, SEEK_SET);
	if (search_sig32(zf, sig_cd) != zf->cd_offset) {
		logMsg("Corrupted zip\n");
		return -1;
	}
	//printf("ZIP seems ok\n");
	return 0;
}

static int unzip_locate_file(PKZIP *zf, const char *filename, uint32_t file_crc) {
	int pos;
	uint32_t crc, offset;
	uint16_t xf_len, fcomment_len, fname_len;
	uint32_t sig;
	char *fname = NULL;
    size_t tr=0;

	fseek(zf->file, zf->cd_offset, SEEK_SET);
	pos = ftell(zf->file);
	if (file_crc == 0)
		file_crc = (uint32_t) -1; /* because crc=0=dir */
	while (1) {
		sig = fget32le(zf->file);
		//printf("SIG=%08x\n",sig);
		if (sig == 0x02014b50) { /* File header */
			freadvle(zf->file, "222222 4 44 222 224 4 ", NULL, NULL, NULL,
					NULL, NULL, NULL, &crc, NULL, NULL, &fname_len, &xf_len,
					&fcomment_len, NULL, NULL, NULL, &offset);
			if (fname)
				free(fname);
			fname = calloc(fname_len + 1, sizeof(unsigned char));
			tr+=fread(fname, fname_len, 1, zf->file);
			fskip(zf->file, xf_len + fcomment_len);
			//printf("0x%08x %s=%s?\n",crc,fname,filename);
			int loadByName = file_crc == (uint32_t)-1 || !gn_strictROMChecking();
			if ((loadByName && (strcmp(fname, filename) == 0 && strlen(fname) == strlen(
					filename))) || crc == file_crc) {
				//logMsg("Found 0x%08x %s", crc, fname);
				free(fname);
				fseek(zf->file, offset, SEEK_SET);
				return 0;
			}
		} else
			break;
	}
	if (fname)
		free(fname);
	return -1;
}

ZFILE *gn_unzip_fopen(PKZIP *zf, const char *filename, uint32_t file_crc) {
	ZFILE *z;
	uint32_t sig;
	int cmeth, xf_len, fname_len;
	int csize, uncsize;

	if (unzip_locate_file(zf, filename, file_crc) == 0) {
		sig = fget32le(zf->file);
		//printf("SIG=%08x\n", sig);
		if (sig != 0x04034b50) {
			logMsg("Error\n");
			return NULL;
		}
		fskip(zf->file, 2 + 2);
		cmeth = fget16le(zf->file);
		if (cmeth != 0 && cmeth != 8) {
			logMsg("Error: Unsupported compression method\n");
			return NULL;
		}
		fskip(zf->file, 2 + 2 + 4);
		csize = fget32le(zf->file);
		uncsize = fget32le(zf->file);

		fname_len = fget16le(zf->file);
		xf_len = fget16le(zf->file);

		fskip(zf->file, xf_len + fname_len);

		//printf("compressed size %d uncompressed size %d method=%d\n", csize,uncsize, cmeth);

		z = malloc(sizeof(ZFILE));
		if (!z)
			return NULL;
		//z->name=fname;
		z->pos = ftell(zf->file);
		z->csize = csize;
		z->uncsize = uncsize;
		if (cmeth == 8) {
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
			z->zb = malloc(sizeof(z_stream));
			z->inbuf = zf->map+z->pos;
			//printf("inbuf=%p %d\n",z->inbuf,fileno(zf->file));
			//perror("ERROR:");
			memset(z->zb, 0, sizeof(z_stream));

			z->zb->avail_in=csize;
			z->zb->next_in=z->inbuf;
			inflateInit2(z->zb, -MAX_WBITS);
#else
			z->zb=stbi_zlib_create_zbuf(NULL,zf->file,csize);
#endif
		}
		z->cmeth = cmeth;
		z->readed = 0;
		z->f = zf->file;
		return z;
	}

	return NULL;

}

int gn_unzip_fread(ZFILE *z, uint8_t *data, unsigned int size) {
	int readed;
	int todo;
	int ret;
    size_t tr=0;
	if (!z)
		return -1;
	//if (z->pos!=ftell(z->f))
	//fseek(z->f,z->pos,SEEK_SET);
	if (z->cmeth == 8) {
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
				z->zb->next_out = data;
				z->zb->avail_out = size;
				ret = inflate(z->zb, Z_NO_FLUSH);
				//printf("ret=%d\n",ret);
				assert(ret != Z_STREAM_ERROR); /* state not clobbered */
				switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR; /* and fall through */
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void) inflateEnd(z->zb);
					return ret;
				}
				readed = size - z->zb->avail_out;
#else
		readed=stbi_zlib_decode_noheader_stream(z->zb,data,size);
#endif
	} else { /* Stored */
		todo = z->readed - z->uncsize;
		if (todo < size)
			todo = size;

		readed = fread(data, 1, size, z->f);
		z->readed += readed;
	}

	z->pos = ftell(z->f);
	return readed;
}

uint8_t *gn_unzip_file_malloc(PKZIP *zf, const char *filename, uint32_t file_crc,
		unsigned int *outlen) {
	ZFILE *z = gn_unzip_fopen(zf, filename, file_crc);
	int readed;
	if (!z)
		return NULL;
	uint8_t *data = malloc(z->uncsize);
	if (!data)
		return NULL;
	if (z->cmeth == 8) {
#if !defined(HAVE_LIBZ) || !defined (HAVE_MMAP)
		readed = stbi_zlib_decode_noheader_stream(z->zb, data, z->uncsize);
#else
		readed=gn_unzip_fread(z,data,z->uncsize);
		logMsg("Readed=%d",readed);
#endif
	} else
		readed = fread(data, 1, z->uncsize, z->f);
	if (readed != z->uncsize)
		logMsg(
				"GNZIP: Readed data size different from uncompressed size %d!=%d \n",
				readed, z->uncsize);
	*outlen = z->uncsize;
	gn_unzip_fclose(z);
	return data;
}

void gn_unzip_fclose(ZFILE *z) {
	if (!z)
		return;
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
	if (z->cmeth==8) {
		inflateEnd(z->zb);
		free(z->zb);
	}
#else
	if (z->cmeth==8) free(z->zb->cbuf);
#endif
	free(z);
}

PKZIP *gn_open_zip(char *file) {
	PKZIP *zf = malloc(sizeof(PKZIP));
	int size;
	int e;
	zf->file = fopen(file, "rb");
	if (zf->file == NULL) {
		logMsg("Couldn't open %s\n", file);
		free(zf);
		return NULL;
	}
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
	fseek(zf->file,0,SEEK_END);
	size=ftell(zf->file);
	zf->map=mmap(0,size,PROT_READ,MAP_SHARED,fileno(zf->file),0);

#endif
	e = search_central_dir(zf);
	if (e) {
		logMsg("Strange %d %s\n", e, file);
		fclose(zf->file);
		free(zf);
		return NULL;
	}
	return zf;
}
void gn_close_zip(PKZIP *zf) {
	//logMsg("closing file %p", zf);
#if defined(HAVE_LIBZ) && defined (HAVE_MMAP)
	fseek(zf->file,0,SEEK_END);
	int size=ftell(zf->file);
	munmap(zf->map, size);
#endif
	fclose(zf->file);
	free(zf);
}

