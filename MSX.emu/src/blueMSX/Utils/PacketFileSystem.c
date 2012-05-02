/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Utils/PacketFileSystem.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-30 18:38:47 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strcmpnocase.h"

#include "blowfish.h"
#include "authkey.h"


typedef struct {
    char path[100];
    int  length;
    int  offset;
} FileInfo;

typedef struct 
{
    int offset;
    int length;
    int pos;
} PKG_FILE;


#define PKG_FILE_CNT 32

static PKG_FILE pkg_files[PKG_FILE_CNT];

#define PKG_HDR         "blueMSX Pkg 001"
#define PKG_HDR_SIZE    16

static char* pkg_buf = NULL;

int pkg_load(const char* filename, char* key, int keyLen)
{
    BLOWFISH_CTX userEnc;
    BLOWFISH_CTX emuEnc;
    FileInfo* fi;
    int i;
    int len;
    FILE* f;
    
    f = fopen(filename, "rb");
    if (f == NULL) {
        return 0;
    }

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        fclose(f);
        return 0;
    }

    pkg_buf = (char*)malloc(len);

    len = fread(pkg_buf, 1, len, f);
    if (len <= 0) {
        free(pkg_buf);
        pkg_buf = NULL;
        fclose(f);
        return 0;
    }

    fclose(f);

    // Decrypt using private emulator key
    Blowfish_Init(&emuEnc, (unsigned char*)AuthKey, (int)strlen(AuthKey));
    
    for (i = 0; i < len; i += 8) {
        unsigned long* l = (unsigned long*)(pkg_buf + i + 0);
        unsigned long* r = (unsigned long*)(pkg_buf + i + 4);

        Blowfish_Decrypt(&emuEnc, l, r);
    }

    // Decrypt using user key
    if (keyLen > 0) {
        Blowfish_Init(&userEnc, (unsigned char*)key, keyLen);
    }
    else {
        unsigned char val = 0;
        Blowfish_Init(&userEnc, &val, 1);
    }

    for (i = 0; i < len; i += 8) {
        unsigned long* l = (unsigned long*)(pkg_buf + i + 0);
        unsigned long* r = (unsigned long*)(pkg_buf + i + 4);

        Blowfish_Decrypt(&userEnc, l, r);
    }

    // Unsalt data
    for (i = 0; i < len - 8; i++) {
        pkg_buf[i] ^= pkg_buf[len - 8 + (i & 7)];
    }

    if (memcmp(pkg_buf, PKG_HDR, PKG_HDR_SIZE) != 0) {
        free(pkg_buf);
        pkg_buf = NULL;
        return 0;
    }

    fi = (FileInfo*)(pkg_buf + 16);

    while (fi->offset != 0) {
        printf("%d\t%d\t%s\n", fi->offset, fi->length, fi->path);
        fi++;
    }

    return 1;
}

void pkg_unload()
{
    if (pkg_buf != NULL) {
        free(pkg_buf);
        pkg_buf = NULL;
    }
}

int pkg_file_exists(const char* fname)
{
    FileInfo* fi = (FileInfo*)(pkg_buf + 16);

    if (pkg_buf != NULL) {
        while (fi->offset != 0) {
            if (strcmpnocase(fi->path, fname) == 0) {
                return 1;
            }
            fi++;
        }
    }
    return 0;
}

FILE* pkg_fopen(const char* fname, const char* mode)
{
    FileInfo* fi = (FileInfo*)(pkg_buf + 16);

    if (pkg_buf != NULL) {
        while (fi->offset != 0) {
            if (strcmpnocase(fi->path, fname) == 0) {
                int i;

                for (i = 0; i < PKG_FILE_CNT; i++) {
                    if (pkg_files[i].offset == 0) {
                        pkg_files[i].offset = fi->offset;
                        pkg_files[i].length = fi->length;
                        pkg_files[i].pos    = 0;
                        return (FILE*)&pkg_files[i];
                    }
                }
                return NULL;
            }
            fi++;
        }
    }

    return fopen(fname, mode);
}

int pkg_fclose(FILE* file)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return fclose(file);
    }
    pkg_file->offset = 0;
    return 0;
}

size_t pkg_fwrite(const void* buffer, size_t size, size_t count, FILE* file)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return fwrite(buffer, size, count, file);
    }
    return 0;
}

size_t pkg_fread(void* buffer, size_t size, size_t count, FILE* file)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return fread(buffer, size, count, file);
    }

    if (pkg_file->offset == 0) {
        return -1;
    }

    if (pkg_file->pos + (int)(count * size) > pkg_file->length) {
        count = (pkg_file->length - pkg_file->pos) / size;
    }

    memcpy(buffer, pkg_buf + pkg_file->offset + pkg_file->pos, count * size);

    pkg_file->pos += count * size;
    
    return count;
}

int pkg_fseek(FILE* file, long offset, int origin)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;
    int newPos = 0;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return fseek(file, offset, origin);
    }

    if (pkg_file->offset == 0) {
        return -1;
    }

    switch (origin) {
    case 0:
        newPos = offset;
        break;
    case 1:
        newPos = pkg_file->pos + offset;
        break;
    case 2:
        newPos = pkg_file->length + offset;
        break;
    default:
        return -1;
    }

    if (newPos < 0) {
        newPos = 0;
    }
    if (newPos >= pkg_file->length) {
        newPos = pkg_file->length;
    }

    pkg_file->pos = newPos;
    return 0;
}

long pkg_ftell(FILE* file)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return ftell(file);
    }

    if (pkg_file->offset == 0) {
        return -1;
    }

    return pkg_file->pos;
}


char *pkg_fgets(char* string, int n, FILE* file)
{
    PKG_FILE* pkg_file = (PKG_FILE*)file;
    char* s;
    char* ptr;

    if ((char*)file < (char*)pkg_files || (char*)file >= ((char*)pkg_files + PKG_FILE_CNT)) {
        return fgets(string, n, file);
    }

    if (pkg_file->pos + n > pkg_file->length) {
        n = pkg_file->length - pkg_file->pos;
        string[n] = 0;
    }

    if (n == 0) {
        return NULL;
    }

    ptr = pkg_buf + pkg_file->offset + pkg_file->pos;

    s = string;

    while (n > 0) {
        char c = *ptr++;
        *s++ = c;
        n--;
        pkg_file->pos++;
        if (c == 0) {
            break;
        }
        if (n > 0 && c == '\n') {
            char c = *ptr;
            if (c == '\r') {
                *s++ = c;
                n--;
                pkg_file->pos++;
                break;
            }
        }
        if (c == '\r') {
            char c = *ptr;
            if (c == '\n') {
                *s++ = c;
                n--;
                pkg_file->pos++;
                break;
            }
        }
    }

    if (n > 0) {
        *s = 0;
    }

    return string;
}
