/*
 * cbmdos.h - Common CBM DOS routines.
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

#ifndef VICE_CBMDOS_H
#define VICE_CBMDOS_H

#include "types.h"

/* CBM DOS Input Processor Error Codes.  */
#define CBMDOS_IPE_OK                      0
#define CBMDOS_IPE_DELETED                 1
#define CBMDOS_IPE_SEL_PARTN               2   /* 1581 */
#define CBMDOS_IPE_UNIMPL                  3
#define CBMDOS_IPE_MEMORY_READ             4

#define CBMDOS_IPE_READ_ERROR_BNF          20
#define CBMDOS_IPE_READ_ERROR_SYNC         21
#define CBMDOS_IPE_READ_ERROR_DATA         22
#define CBMDOS_IPE_READ_ERROR_CHK          23
#define CBMDOS_IPE_READ_ERROR_GCR          24
#define CBMDOS_IPE_WRITE_ERROR_VER         25
#define CBMDOS_IPE_WRITE_PROTECT_ON        26
#define CBMDOS_IPE_READ_ERROR_BCHK         27
#define CBMDOS_IPE_WRITE_ERROR_BIG         28
#define CBMDOS_IPE_DISK_ID_MISMATCH        29
#define CBMDOS_IPE_SYNTAX                  30
#define CBMDOS_IPE_INVAL                   31
#define CBMDOS_IPE_LONG_LINE               32
#define CBMDOS_IPE_BAD_NAME                33
#define CBMDOS_IPE_NO_NAME                 34
#define CBMDOS_IPE_PATH_NOT_FOUND          39

#define CBMDOS_IPE_NO_RECORD               50
#define CBMDOS_IPE_OVERFLOW                51
#define CBMDOS_IPE_TOOLARGE                52  /* 1581 */

#define CBMDOS_IPE_NOT_WRITE               60
#define CBMDOS_IPE_NOT_OPEN                61
#define CBMDOS_IPE_NOT_FOUND               62
#define CBMDOS_IPE_FILE_EXISTS             63
#define CBMDOS_IPE_BAD_TYPE                64
#define CBMDOS_IPE_NO_BLOCK                65
#define CBMDOS_IPE_ILLEGAL_TRACK_OR_SECTOR 66

#define CBMDOS_IPE_NO_CHANNEL              70
#define CBMDOS_IPE_DISK_FULL               72
#define CBMDOS_IPE_DOS_VERSION             73
#define CBMDOS_IPE_NOT_READY               74
#define CBMDOS_IPE_BAD_PARTN               77  /* 1581 */

#define CBMDOS_IPE_NOT_EMPTY               80  /* dir to remove not empty */
#define CBMDOS_IPE_PERMISSION              81  /* permission denied */

/* CBM DOS File Types */
#define CBMDOS_FT_DEL         0
#define CBMDOS_FT_SEQ         1
#define CBMDOS_FT_PRG         2
#define CBMDOS_FT_USR         3
#define CBMDOS_FT_REL         4
#define CBMDOS_FT_CBM         5       /* 1581 partition */
#define CBMDOS_FT_DIR         6
#define CBMDOS_FT_REPLACEMENT 0x20
#define CBMDOS_FT_LOCKED      0x40
#define CBMDOS_FT_CLOSED      0x80

/* CBM DOS Access Control Methods */
#define CBMDOS_FAM_READ   0
#define CBMDOS_FAM_WRITE  1
#define CBMDOS_FAM_APPEND 2
#define CBMDOS_FAM_EOF    4

/* CBM DOS directory definitions.  */
#define CBMDOS_SLOT_NAME_LENGTH 16

/* fdc error codes to return to drive CPU */
enum fdc_err_e {
    CBMDOS_FDC_ERR_OK      = 1,
    CBMDOS_FDC_ERR_HEADER  = 2,
    CBMDOS_FDC_ERR_SYNC    = 3,
    CBMDOS_FDC_ERR_NOBLOCK = 4,
    CBMDOS_FDC_ERR_DCHECK  = 5,
    CBMDOS_FDC_ERR_VERIFY  = 7,
    CBMDOS_FDC_ERR_WPROT   = 8,
    CBMDOS_FDC_ERR_HCHECK  = 9,
    CBMDOS_FDC_ERR_BLENGTH = 10,
    CBMDOS_FDC_ERR_ID      = 11,
    CBMDOS_FDC_ERR_FSPEED  = 12,
    CBMDOS_FDC_ERR_DRIVE   = 15,
    CBMDOS_FDC_ERR_DECODE  = 16
};
typedef enum fdc_err_e fdc_err_t;

struct cbmdos_cmd_parse_s {
    const BYTE *cmd; /* input: full dos-command string */
    unsigned int cmdlength; /* input */
    char *parsecmd; /* output: parsed command */
    unsigned int secondary; /* input */
    unsigned int parselength; /* output */
    unsigned int readmode; /* output */
    unsigned int filetype; /* output */
    unsigned int recordlength; /* output */
    unsigned int drive; /* output: drive number */
};
typedef struct cbmdos_cmd_parse_s cbmdos_cmd_parse_t;


extern const char *cbmdos_errortext(unsigned int code);
extern const char *cbmdos_filetype_get(unsigned int filetype);

extern unsigned int cbmdos_parse_wildcard_check(const char *name, unsigned int len);
extern unsigned int cbmdos_parse_wildcard_compare(const BYTE *name1, const BYTE *name2);
extern BYTE *cbmdos_dir_slot_create(const char *name, unsigned int len);

extern unsigned int cbmdos_command_parse(cbmdos_cmd_parse_t *cmd_parse);

#endif
