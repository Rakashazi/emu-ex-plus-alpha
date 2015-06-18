/*
 * crt.c - CRT image handling.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
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
#include "resources.h"
#include "types.h"
#include "c64cart.h"

#define CARTRIDGE_INCLUDE_PRIVATE_API
#include "actionreplay.h"
#include "actionreplay2.h"
#include "actionreplay3.h"
#include "actionreplay4.h"
#include "atomicpower.h"
#include "c64-generic.h"
#include "c64tpi.h"
#include "comal80.h"
#include "capture.h"
#include "delaep256.h"
#include "delaep64.h"
#include "delaep7x8.h"
#include "diashowmaker.h"
#include "dinamic.h"
#include "easyflash.h"
#include "epyxfastload.h"
#include "exos.h"
#include "expert.h"
#include "final.h"
#include "finalplus.h"
#include "final3.h"
#include "formel64.h"
#include "freezeframe.h"
#include "freezemachine.h"
#include "funplay.h"
#include "gamekiller.h"
#include "gs.h"
#include "ide64.h"
#include "isepic.h"
#include "kcs.h"
#include "kingsoft.h"
#include "mach5.h"
#include "magicdesk.h"
#include "magicformel.h"
#include "magicvoice.h"
#include "mikroass.h"
#include "mmc64.h"
#include "mmcreplay.h"
#include "ocean.h"
#include "pagefox.h"
#include "prophet64.h"
#include "retroreplay.h"
#include "rexep256.h"
#include "rexutility.h"
#include "rgcd.h"
#include "ross.h"
#include "silverrock128.h"
#include "simonsbasic.h"
#include "stardos.h"
#include "stb.h"
#include "snapshot64.h"
#include "supergames.h"
#include "supersnapshot4.h"
#include "supersnapshot.h"
#include "superexplode5.h"
#include "warpspeed.h"
#include "westermann.h"
#include "zaxxon.h"
#include "util.h"
#undef CARTRIDGE_INCLUDE_PRIVATE_API

/* #define DEBUGCRT */

#ifdef DEBUGCRT
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

/*
 * CRT image "strings".
 */
const char CRT_HEADER[] = "C64 CARTRIDGE   ";
static const char CHIP_HEADER[] = "CHIP";

/*
    Open a crt file and read header, return NULL on fault, fd otherwise
*/
static FILE *crt_open(const char *filename, crt_header_t *header)
{
    BYTE crt_header[0x40];
    DWORD skip;
    FILE *fd;

    fd = fopen(filename, MODE_READ);

    if (fd == NULL) {
        return NULL;
    }

    do {
        if (fread(crt_header, sizeof(crt_header), 1, fd) < 1) {
            DBG(("CRT: could not read header\n"));
            break;
        }

        if (memcmp(crt_header, CRT_HEADER, 16)) {
            DBG(("CRT: header invalid\n"));
            break;
        }

        skip = util_be_buf_to_dword(&crt_header[0x10]);

        if (skip < sizeof(crt_header)) {
            break; /* invalid header size */
        }
        skip -= sizeof(crt_header); /* without header */

        header->version = util_be_buf_to_word(&crt_header[0x14]);
        header->type = util_be_buf_to_word(&crt_header[0x16]);
        header->exrom = crt_header[0x18];
        header->game = crt_header[0x19];
        memset(header->name, 0, sizeof(header->name));
        strncpy(header->name, (char*)&crt_header[0x20], sizeof(header->name) - 1);

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
    crt_header_t header;
    FILE *fd;

    fd = crt_open(filename, &header);

    if (fd == NULL) {
        return -1;
    }

    fclose(fd);

    return header.type;
}

/*
    Read and pharse chip header, return -1 on fault
*/
int crt_read_chip_header(crt_chip_header_t *header, FILE *fd)
{
    BYTE chipheader[0x10];

    if (fread(chipheader, sizeof(chipheader), 1, fd) < 1) {
        return -1; /* couldn't read header */
    }
    if (memcmp(chipheader, CHIP_HEADER, 4)) {
        return -1; /* invalid header signature */
    }

    header->skip = util_be_buf_to_dword(&chipheader[4]);

    if (header->skip < sizeof(chipheader)) {
        return -1; /* invalid packet size */
    }
    header->skip -= sizeof(chipheader); /* without header */

    header->size = util_be_buf_to_word(&chipheader[14]);
    if (header->size > header->skip) {
        return -1; /* rom bigger then total size?! */
    }
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
int crt_read_chip(BYTE *rawcart, int offset, crt_chip_header_t *chip, FILE *fd)
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
int crt_write_chip(BYTE *data, crt_chip_header_t *header, FILE *fd)
{
    BYTE chipheader[0x10];

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
    BYTE crt_header[0x40];
    FILE *fd;

    if (filename == NULL) {
        return NULL;
    }

    fd = fopen(filename, MODE_WRITE);

    if (fd == NULL) {
        return NULL;
    }

    memset(&crt_header, 0, sizeof(crt_header));
    memcpy(crt_header, CRT_HEADER, 16);
    util_dword_to_be_buf(&crt_header[0x10], sizeof(crt_header));
    util_word_to_be_buf(&crt_header[0x14], 0x100); /* version */
    util_word_to_be_buf(&crt_header[0x16], (WORD)type);
    crt_header[0x18] = exrom ? 1 : 0;
    crt_header[0x19] = game ? 1 : 0;
    strncpy((char*)&crt_header[0x20], name, sizeof(crt_header) - 0x20);

    if (fwrite(crt_header, sizeof(crt_header), 1, fd) < 1) {
        fclose(fd);
        return NULL;
    }

    return fd;
}

/*
    returns -1 on error, else a positive CRT ID

    FIXME: to simplify this function a little bit, all subfunctions should
           also return the respective CRT ID on success
*/
int crt_attach(const char *filename, BYTE *rawcart)
{
    crt_header_t header;
    int rc, new_crttype;
    FILE *fd;

    DBG(("crt_attach: %s\n", filename));

    fd = crt_open(filename, &header);

    if (fd == NULL) {
        return -1;
    }

    new_crttype = header.type;
    if (new_crttype & 0x8000) {
        /* handle our negative test IDs */
        new_crttype -= 0x10000;
    }
    DBG(("crt_attach ID: %d\n", new_crttype));

/*  cart should always be detached. there is no reason for doing fancy checks
    here, and it will cause problems incase a cart MUST be detached before
    attaching another, or even itself. (eg for initialization reasons)

    most obvious reason: attaching a different ROM (software) for the same
    cartridge (hardware) */

    cartridge_detach_image(new_crttype);

    switch (new_crttype) {
        case CARTRIDGE_CRT:
            rc = generic_crt_attach(fd, rawcart);
            if (rc != CARTRIDGE_NONE) {
                new_crttype = rc;
            }
            break;
        case CARTRIDGE_ACTION_REPLAY:
            rc = actionreplay_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ACTION_REPLAY2:
            rc = actionreplay2_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ACTION_REPLAY3:
            rc = actionreplay3_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ACTION_REPLAY4:
            rc = actionreplay4_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ATOMIC_POWER:
            rc = atomicpower_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_CAPTURE:
            rc = capture_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_COMAL80:
            rc = comal80_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_DELA_EP256:
            rc = delaep256_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_DELA_EP64:
            rc = delaep64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_DELA_EP7x8:
            rc = delaep7x8_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_DIASHOW_MAKER:
            rc = dsm_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_DINAMIC:
            rc = dinamic_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_EASYFLASH:
            rc = easyflash_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_EPYX_FASTLOAD:
            rc = epyxfastload_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_EXOS:
            rc = exos_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_EXPERT:
            rc = expert_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_FINAL_I:
            rc = final_v1_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FINAL_III:
            rc = final_v3_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FINAL_PLUS:
            rc = final_plus_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FORMEL64:
            rc = formel64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FREEZE_FRAME:
            rc = freezeframe_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FREEZE_MACHINE:
            rc = freezemachine_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_FUNPLAY:
            rc = funplay_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_GAME_KILLER:
            rc = gamekiller_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_GS:
            rc = gs_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_IDE64:
            rc = ide64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_IEEE488:
            rc = tpi_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ISEPIC:
            rc = isepic_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_KCS_POWER:
            rc = kcs_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_KINGSOFT:
            rc = kingsoft_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MACH5:
            rc = mach5_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MAGIC_DESK:
            rc = magicdesk_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MAGIC_FORMEL:
            rc = magicformel_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MAGIC_VOICE:
            rc = magicvoice_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MIKRO_ASSEMBLER:
            rc = mikroass_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MMC64:
            rc = mmc64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_MMC_REPLAY:
            rc = mmcreplay_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_OCEAN:
            rc = ocean_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_P64:
            rc = p64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_PAGEFOX:
            rc = pagefox_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_RETRO_REPLAY:
            rc = retroreplay_crt_attach(fd, rawcart, filename);
            break;
        case CARTRIDGE_REX_EP256:
            rc = rexep256_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_REX:
            rc = rex_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_RGCD:
            rc = rgcd_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ROSS:
            rc = ross_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SILVERROCK_128:
            rc = silverrock128_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SIMONS_BASIC:
            rc = simon_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_STARDOS:
            rc = stardos_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SNAPSHOT64:
            rc = snapshot64_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_STRUCTURED_BASIC:
            rc = stb_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SUPER_GAMES:
            rc = supergames_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SUPER_SNAPSHOT:
            rc = supersnapshot_v4_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SUPER_SNAPSHOT_V5:
            rc = supersnapshot_v5_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_SUPER_EXPLODE_V5:
            rc = se5_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_WARPSPEED:
            rc = warpspeed_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_WESTERMANN:
            rc = westermann_crt_attach(fd, rawcart);
            break;
        case CARTRIDGE_ZAXXON:
            rc = zaxxon_crt_attach(fd, rawcart);
            break;
        default:
            archdep_startup_log_error("unknown CRT ID: %d\n", new_crttype);
            rc = -1;
            break;
    }

    fclose(fd);

    if (rc == -1) {
        DBG(("crt_attach error (%d)\n", rc));
        return -1;
    }
    DBG(("crt_attach return ID: %d\n", new_crttype));
    return new_crttype;
}
