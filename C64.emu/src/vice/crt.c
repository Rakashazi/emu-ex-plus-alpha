/*
 * crt.c - CRT image handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "archdep.h"
#include "cartridge.h"
#include "crt.h"
#include "log.h"
#include "machine.h"
#include "resources.h"
#include "types.h"
#include "c64cart.h"  /* FIXME: for C64CART_IMAGE_LIMIT */
#include "util.h"

/* #define DEBUGCRT */

#ifdef DEBUGCRT
#define DBG(x)  log_debug x
#else
#define DBG(x)
#endif

/*
 * CRT image "strings".
 */

                                      /*1234567890123456*/
static const char CRT_HEADER_C64[]   = "C64 CARTRIDGE   ";
static const char CRT_HEADER_C128[]  = "C128 CARTRIDGE  ";
static const char CRT_HEADER_VIC20[] = "VIC20 CARTRIDGE ";
static const char CRT_HEADER_PLUS4[] = "PLUS4 CARTRIDGE ";

static const char CHIP_HEADER[] = "CHIP";

static void expected_header_error(void)
{
    switch (machine_class) {
        case VICE_MACHINE_C64:
        case VICE_MACHINE_C64SC:
        case VICE_MACHINE_SCPU64:
            log_error(LOG_DEFAULT, "CRT header invalid (expected:'%s').", CRT_HEADER_C64);
            break;
        case VICE_MACHINE_C128:
            log_error(LOG_DEFAULT, "CRT header invalid (expected:'%s' or '%s').", CRT_HEADER_C64, CRT_HEADER_C128);
            break;
        case VICE_MACHINE_VIC20:
            log_error(LOG_DEFAULT, "CRT header invalid (expected:'%s').", CRT_HEADER_VIC20);
            break;
        case VICE_MACHINE_PLUS4:
            log_error(LOG_DEFAULT, "CRT header invalid (expected:'%s').", CRT_HEADER_PLUS4);
            break;
    }
}

/*
    Open a crt file and read header

    return NULL on fault, fd otherwise
*/
FILE *crt_open(const char *filename, crt_header_t *header)
{
    uint8_t crt_header[0x40];
    uint32_t skip;
    FILE *fd;

    fd = fopen(filename, MODE_READ);

    if (fd == NULL) {
        return NULL;
    }

    do {
        if (fread(crt_header, sizeof(crt_header), 1, fd) < 1) {
            log_error(LOG_DEFAULT, "could not read CRT header.");
            break;
        }

        header->machine = -1;
        /*printf("CRT TAG:'%s'\n", crt_header);*/

        if (memcmp(crt_header, CRT_HEADER_C64, 16) == 0) {
            DBG(("Found header: '%s'\n", CRT_HEADER_C64));
            header->machine = VICE_MACHINE_C64;
            if (!(machine_class == VICE_MACHINE_C64 ||
                  machine_class == VICE_MACHINE_C64SC ||
                  machine_class == VICE_MACHINE_C128 ||
                  machine_class == VICE_MACHINE_SCPU64)) {
                expected_header_error();
                break;
            }
        } else if (memcmp(crt_header, CRT_HEADER_C128, 16) == 0) {
            DBG(("Found header: '%s'\n", CRT_HEADER_C128));
            header->machine = VICE_MACHINE_C128;
            if (!(machine_class == VICE_MACHINE_C128)) {
                expected_header_error();
                break;
            }
        } else if (memcmp(crt_header, CRT_HEADER_VIC20, 16) == 0) {
            DBG(("Found header: '%s'\n", CRT_HEADER_VIC20));
            header->machine = VICE_MACHINE_VIC20;
            if (!(machine_class == VICE_MACHINE_VIC20)) {
                expected_header_error();
                break;
            }
        } else if (memcmp(crt_header, CRT_HEADER_PLUS4, 16) == 0) {
            DBG(("Found header: '%s'\n", CRT_HEADER_PLUS4));
            header->machine = VICE_MACHINE_PLUS4;
            if (!(machine_class == VICE_MACHINE_PLUS4)) {
                expected_header_error();
                break;
            }
        } else {
            log_error(LOG_DEFAULT, "no CRT header found.");
            break;
        }
        DBG(("CRT Machine:'%d'", header->machine));

        skip = util_be_buf_to_dword(&crt_header[0x10]);

        if (skip < sizeof(crt_header)) {
            log_error(LOG_DEFAULT, "CRT header size is wrong (is 0x%02x, expected 0x%02x).",
                (unsigned int)skip, (unsigned int)sizeof(crt_header));
            break; /* invalid header size */
        }
        skip -= sizeof(crt_header); /* without header */

        header->version = util_be_buf_to_word(&crt_header[0x14]);
        header->type = util_be_buf_to_word(&crt_header[0x16]);
        header->subtype = crt_header[0x1a];
        header->exrom = crt_header[0x18];
        header->game = crt_header[0x19];
        memset(header->name, 0, sizeof(header->name));
        strncpy(header->name, (char *)(crt_header + 0x20),
                sizeof(header->name) - 1);

        fseek(fd, skip, SEEK_CUR); /* skip the rest */

        return fd; /* Ok, exit */
    } while (0);

    fclose(fd);
    return NULL; /* Fault */
}
/*
    returns -1 on error, else a positive CRT ID
*/
int crt_getid(const char *filename)
{
    int id;
    crt_header_t header;
    FILE *fd;

    fd = crt_open(filename, &header);

    if (fd == NULL) {
        return -1;
    }

    fclose(fd);

    id = header.type;

    /* if we have loaded a C128 cartridge, convert the C128 crt id to something
       else (that can coexist with C64 crt ids) */
    if (header.machine == VICE_MACHINE_C128) {
        id = CARTRIDGE_C128_MAKEID(id);
    }

    return id;
}

/*
    Read and parse chip header, return -1 on fault
*/
int crt_read_chip_header(crt_chip_header_t *header, FILE *fd)
{
    uint8_t chipheader[0x10];

    if (fread(chipheader, sizeof(chipheader), 1, fd) < 1) {
        return -1; /* couldn't read header */
    }
    if (memcmp(chipheader, CHIP_HEADER, 4)) {
        return -1; /* invalid header signature */
    }

    /* 1: grab size of chip packet from the file */
    header->skip = util_be_buf_to_dword(&chipheader[4]);

    if (header->skip < sizeof(chipheader)) {
        return -1; /* invalid packet size */
    }
    /* 2: subtract header size, we now have the payload size */
    header->skip -= sizeof(chipheader); /* without header */

    header->size = util_be_buf_to_word(&chipheader[14]);
    if (header->size > header->skip) {
        return -1; /* rom bigger then total size?! */
    }
    /* 3: subtract ROM size, we get the unused portion at the end,
          which we need to skip */
    header->skip -= header->size; /* skip size after image */

    header->type = util_be_buf_to_word(&chipheader[8]);
    header->bank = util_be_buf_to_word(&chipheader[10]);
    header->start = util_be_buf_to_word(&chipheader[12]);

    if (header->start + header->size > 0x10000) {
        return -1; /* rom crossing the 64k boundary?! */
    }

    return 0;
}
/*
    Read chip data, return -1 on error
*/
int crt_read_chip(uint8_t *rawcart, int offset, crt_chip_header_t *chip, FILE *fd)
{
    if (offset + chip->size > C64CART_IMAGE_LIMIT) {
        return -1; /* overflow */
    }
    if (fread(&rawcart[offset], chip->size, 1, fd) < 1) {
        return -1; /* eof?! */
    }
    fseek(fd, chip->skip, SEEK_CUR); /* skip the rest */

    return 0;
}
/*
    Write chip header and data, return -1 on fault
*/
int crt_write_chip(uint8_t *data, crt_chip_header_t *header, FILE *fd)
{
    uint8_t chipheader[0x10];

    memcpy(chipheader, CHIP_HEADER, 4);
    util_dword_to_be_buf(&chipheader[4], header->size + sizeof(chipheader));
    util_word_to_be_buf(&chipheader[8], header->type);
    util_word_to_be_buf(&chipheader[10], header->bank);
    util_word_to_be_buf(&chipheader[12], header->start);
    util_word_to_be_buf(&chipheader[14], header->size);

    if (fwrite(chipheader, sizeof(chipheader), 1, fd) < 1) {
        return -1; /* could not write chip header */
    }

    if (fwrite(data, header->size, 1, fd) < 1) {
        return -1; /* could not write chip content */
    }

    return 0;
}
/*
    Create crt file with header, return NULL on fault, fd otherwise
*/
FILE *crt_create(const char *filename, int type, int exrom, int game, const char *name)
{
    uint8_t crt_header[0x40];
    FILE *fd;

    if (filename == NULL) {
        return NULL;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return NULL;
    }

    memset(&crt_header, 0, sizeof(crt_header));
    memcpy(crt_header, CRT_HEADER_C64, 16); /* FIXME */
    util_dword_to_be_buf(&crt_header[0x10], sizeof(crt_header));
    util_word_to_be_buf(&crt_header[0x14], 0x100); /* version */
    util_word_to_be_buf(&crt_header[0x16], (uint16_t)type);
    crt_header[0x18] = exrom ? 1 : 0;
    crt_header[0x19] = game ? 1 : 0;
    strncpy((char*)(crt_header + 0x20), name, sizeof(crt_header) - 0x20 - 1);

    if (fwrite(crt_header, sizeof(crt_header), 1, fd) < 1) {
        fclose(fd);
        return NULL;
    }

    return fd;
}

/* create v1.1 header with sub type */
FILE *crt_create_v11(const char *filename, int type, int subtype, int exrom, int game, const char *name)
{
    uint8_t crt_header[0x40];
    FILE *fd;

    if (filename == NULL) {
        return NULL;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return NULL;
    }

    memset(&crt_header, 0, sizeof(crt_header));
    memcpy(crt_header, CRT_HEADER_C64, 16); /* FIXME */
    util_dword_to_be_buf(&crt_header[0x10], sizeof(crt_header));
    util_word_to_be_buf(&crt_header[0x14], 0x0101); /* version */
    util_word_to_be_buf(&crt_header[0x16], (uint16_t)type);
    crt_header[0x18] = exrom ? 1 : 0;
    crt_header[0x19] = game ? 1 : 0;
    crt_header[0x1a] = subtype;
    strncpy((char*)(crt_header + 0x20), name, sizeof(crt_header) - 0x20 - 1);

    if (fwrite(crt_header, sizeof(crt_header), 1, fd) < 1) {
        fclose(fd);
        return NULL;
    }

    return fd;
}
