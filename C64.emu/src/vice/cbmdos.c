/*
 * cbmdos.c - Common CBM DOS routines.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Parser plus by
 *  Roberto Muscedere <rmusced@uwindsor.ca>
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

#include "cbmdos.h"
#include "lib.h"
#include "log.h"

/* #define DEBUG_CBMDOS */

typedef struct cbmdos_errortext_s {
    unsigned int nr;
    const char *text;
} cbmdos_errortext_t;

static const cbmdos_errortext_t cbmdos_error_messages[] =
{
    {  0, " OK" },
    {  1, "FILES SCRATCHED" },
    {  2, "SELECTED PARTITION" },           /* 1581 */
    {  3, "UNIMPLEMENTED" },
    { 20, "READ ERROR" },
    { 21, "READ ERROR" },
    { 22, "READ ERROR" },
    { 23, "READ ERROR" },
    { 24, "READ ERROR" },
    { 25, "WRITE ERROR" },
    { 26, "WRITE PROTECT ON" },
    { 27, "READ ERROR" },
    { 28, "WRITE ERROR" },
    { 29, "DISK ID MISMATCH" },
    { 30, "SYNTAX ERROR" },
    { 31, "SYNTAX ERROR" },
    { 32, "SYNTAX ERROR" },
    { 33, "SYNTAX ERROR" },
    { 34, "SYNTAX ERROR" },
    { 39, "FILE NOT FOUND" }, /* 2000/4000 */
    { 50, "RECORD NOT PRESENT" },
    { 51, "OVERFLOW IN RECORD" },
    { 52, "FILE TOO LARGE" },   /* 1581 */
    { 60, "WRITE FILE OPEN" },
    { 61, "FILE NOT OPEN" },
    { 62, "FILE NOT FOUND" },
    { 63, "FILE EXISTS" },
    { 64, "FILE TYPE MISMATCH" },
    { 65, "NO BLOCK" },
    { 66, "ILLEGAL TRACK OR SECTOR" },
    { 67, "ILLEGAL SYSTEM T OR S" }, /* on 1581 it is still ILLEGAL TRACK OR SECTOR */
    { 70, "NO CHANNEL" },
    { 71, "DIRECTORY ERROR" },
    { 72, "DISK FULL" },
    { 73, "VIRTUAL DRIVE EMULATION V3.5" }, /* The program version */
    { 74, "DRIVE NOT READY" },
    { 75, "FORMAT ERROR" },
    { 77, "SELECTED PARTITION ILLEGAL" },   /* 1581 */
    { 80, "DIRECTORY NOT EMPTY" },
    { 81, "PERMISSION DENIED" },
    { 255, NULL }
};

/* types 0 - 6 are regular CBM file types. real drives return random garbage
 * from the ROM when type 7 appears in the directoy
 */
static const char * const cbmdos_ft[] = {
    "DEL", "SEQ", "PRG", "USR", "REL", "CBM", "DIR", "???"
};

const char *cbmdos_errortext(unsigned int code)
{
    unsigned int count = 0;

    while (cbmdos_error_messages[count].nr != 255 && cbmdos_error_messages[count].nr != code) {
        count++;
    }

    if (cbmdos_error_messages[count].nr != 255) {
        return cbmdos_error_messages[count].text;
    }

    return "UNKNOWN ERROR NUMBER";
}

const char *cbmdos_filetype_get(unsigned int filetype)
{
    return cbmdos_ft[filetype];
}

unsigned int cbmdos_parse_wildcard_check(const char *name, unsigned int len)
{
    unsigned int index;

    for (index = 0; index < len; index++) {
        if (name[index] == '*' || name[index] == '?') {
            return 1;
        }
    }
    return 0;
}

unsigned int cbmdos_parse_wildcard_compare(const uint8_t *name1, const uint8_t *name2)
{
    unsigned int index;

    for (index = 0; index < CBMDOS_SLOT_NAME_LENGTH; index++) {
        switch (name1[index]) {
            case '*':
                return 1; /* rest is not interesting, it's a match */
            case '?':
                if (name2[index] == 0xa0) {
                    return 0; /* wildcard, but the other is too short */
                }
                break;
            case 0xa0: /* This one ends, let's see if the other as well */
                return (name2[index] == 0xa0);
            default:
                if (name1[index] != name2[index]) {
                    return 0; /* does not match */
                }
        }
    }

    return 1; /* matched completely */
}

uint8_t *cbmdos_dir_slot_create(const char *name, unsigned int len)
{
    uint8_t *slot;

    if (len > CBMDOS_SLOT_NAME_LENGTH) {
        len = CBMDOS_SLOT_NAME_LENGTH;
    }

    slot = lib_malloc(CBMDOS_SLOT_NAME_LENGTH);
    memset(slot, 0xa0, CBMDOS_SLOT_NAME_LENGTH);

    memcpy(slot, name, (size_t)len);

    return slot;
}

/* Parse file-name `parsecmd', type, and read/write mode from the given string
   `cmd' with `cmdlength'. '@' on write must be checked elsewhere. */

unsigned int cbmdos_command_parse(cbmdos_cmd_parse_t *cmd_parse)
{
    const uint8_t *p;
    char *parsecmd, *c;
    int cmdlen;

#ifdef DEBUG_CBMDOS
    log_debug("CBMDOS parse cmd: '%s' cmdlen: %d", cmd_parse->cmd, cmd_parse->cmdlength);
#endif

    cmd_parse->atsign = 0;
    cmd_parse->parsecmd = NULL;
    cmd_parse->recordlength = 0;
    cmd_parse->readmode = (cmd_parse->secondary == 1)
                          ? CBMDOS_FAM_WRITE : CBMDOS_FAM_READ;

    if (cmd_parse->cmd == NULL || cmd_parse->cmdlength == 0) {
        return CBMDOS_IPE_NO_NAME;
    }

    p = cmd_parse->cmd;
    if (*p == '$') {
        /* Directory listings are special - see $da55 in the 1541 for reference*/
        if (cmd_parse->cmdlength > 1) {
            p++;
            if ((*p == '0') || (*p == '1')) {
                /* A single 0/1 digit is a drive number, skip it */
                cmd_parse->drive = *p - '0';
                p++;
            }
            /* skip colon */
            if (*p == ':') {
                if (*++p == '\0') {
                    /* "Nothing" after a colon is actually an empty pattern.
                     * Make it match nothing -- count the NUL byte.
                     */
                    ++cmd_parse->cmdlength;
                }
            }
            /* everything from here is the pattern */
        } else {
            /* Just a single $, set pointer to NUL byte */
            ++p;
        }
    } else {
        p = memchr(cmd_parse->cmd, ':', cmd_parse->cmdlength);
        if (p) {
            if (p == cmd_parse->cmd) {
                /* first char is a colon, just skip it */
            } else {
                if ((p[-1] == '0') || (p[-1] == '1')) {
                    /* A single 0/1 digit before the colon is a drive number */
                    cmd_parse->drive = p[-1] - '0';
                }
            }
            p++;
            /* everything after the colon is the pattern */
        } else {
            /* no colon found, entire input is the pattern */
            p = cmd_parse->cmd;
        }
    }

#ifdef DEBUG_CBMDOS
    log_debug("CBMDOS parse pattern: '%s' drive:%d", p, cmd_parse->drive);
#endif

    cmdlen = cmd_parse->cmdlength - (int)(p - cmd_parse->cmd);
    cmd_parse->parselength = 0;

    /* Temporary hack.  */
    cmd_parse->parsecmd = lib_calloc(1, cmdlen + 2);

    /* append rest of command string until first comma */
    parsecmd = cmd_parse->parsecmd;

    while (*p != ',' && cmdlen-- > 0) {
        (cmd_parse->parselength)++;
        *(parsecmd++) = *(p++);
    }

#ifdef DEBUG_CBMDOS
    log_debug("CBMDOS parsed cmd: '%s'", cmd_parse->parsecmd);
#endif

    /* Preset the file-type if the LOAD/SAVE secondary addresses are used. */
    cmd_parse->filetype = (cmd_parse->secondary < 2) ? CBMDOS_FT_PRG : 0;

    /*
     * Change type or mode?
     */
    while (cmdlen > 0) {
        cmdlen--;
        p++;

        if (cmdlen == 0) {
            return CBMDOS_IPE_INVAL;
        }

        switch (*p) {
            case 'S':
                cmd_parse->filetype = CBMDOS_FT_SEQ;
                break;
            case 'P':
                cmd_parse->filetype = CBMDOS_FT_PRG;
                break;
            case 'U':
                cmd_parse->filetype = CBMDOS_FT_USR;
                break;
            case 'L'|0x80:
            case 'L':                   /* L,(#record length)  max 254 */
                /*
                 * Allow extra text between L and the comma,
                 * like with other file types.
                 */
                {
                    uint8_t *comma = memchr(p+1, ',', cmdlen);
                    if (comma && p + cmdlen > comma + 1) {
                        cmd_parse->recordlength = comma[1]; /* Changing RL causes error */

#ifdef DEBUG_CBMDOS
                        log_debug("L recordlength=%d", cmd_parse->recordlength);
#endif
                        /* Don't allow REL file record lengths less than 2 or
                           greater than 254.  The 1541/71/81 lets you create a
                           REL file of record length 0, but it locks up the CPU
                           on the drive - nice. */
                        if (cmd_parse->recordlength < 2 || cmd_parse->recordlength > 254) {
                            return CBMDOS_IPE_OVERFLOW;
                        }
                        /* skip the rest */
                        cmdlen = 0;
                    }
                }
                cmd_parse->filetype = CBMDOS_FT_REL;
                break;
            case 'R':
                cmd_parse->readmode = CBMDOS_FAM_READ;
                break;
            case 'W':
                cmd_parse->readmode = CBMDOS_FAM_WRITE;
                break;
            case 'A':
                cmd_parse->readmode = CBMDOS_FAM_APPEND;
                break;
            default:
                if (cmd_parse->readmode != CBMDOS_FAM_READ
                    && cmd_parse->readmode != CBMDOS_FAM_WRITE) {
                    return CBMDOS_IPE_INVAL;
                }
        }

        c = (char *)memchr(p, ',', cmdlen);
        if (c) {
            cmdlen -= (int)(c - (const char *)p);
            p = (uint8_t *)c;
        } else {
            cmdlen = 0;
        }
    }

    /* Override read mode if secondary is 0 or 1.  */
    if (cmd_parse->secondary == 0) {
        cmd_parse->readmode = CBMDOS_FAM_READ;
    }
    if (cmd_parse->secondary == 1) {
        cmd_parse->readmode = CBMDOS_FAM_WRITE;
    }

    return CBMDOS_IPE_OK;
}

unsigned int cbmdos_command_parse_plus(cbmdos_cmd_parse_plus_t *cmd_parse)
{
    const uint8_t *p, *limit, *p1, *p2;
    uint8_t temp[256];
    int special = 0;
    int i, templength = 0;

#ifdef DEBUG_CBMDOS
    log_debug("CBMDOS parse plus cmd: '%s' cmdlen: %d", cmd_parse->full, cmd_parse->fulllength);
#endif

    cmd_parse->command = NULL;
    cmd_parse->commandlength = 0;
    cmd_parse->abbrv = NULL;
    cmd_parse->abbrvlength = 0;
    cmd_parse->path = NULL;
    cmd_parse->pathlength = 0;
    cmd_parse->file = NULL;
    cmd_parse->filelength = 0;
    cmd_parse->more = NULL;
    cmd_parse->morelength = 0;
    cmd_parse->recordlength = 0;
    cmd_parse->filetype = 0;
    cmd_parse->readmode = (cmd_parse->secondary == 1)
                          ? CBMDOS_FAM_WRITE : CBMDOS_FAM_READ;
    cmd_parse->colon = 0;
    /* drive is not reset here */

    if (cmd_parse->full == NULL || cmd_parse->fulllength == 0) {
        return CBMDOS_IPE_NO_NAME;
    }

    p = cmd_parse->full;
    limit = p + cmd_parse->fulllength;

    /* in file mode */
    if ((cmd_parse->mode == 0 || cmd_parse->mode == 2) && p < limit) {
        if (cmd_parse->mode == 0) {
            cmd_parse->drive = 0;
        }
        /* look for a ':' to separate the unit/part-path from name */
        p2 = memchr(p, ':', limit - p);
        /* if a colon is found, flag it */
        if (p2) {
            cmd_parse->colon = 1;
        }
        /* check for special commands without : */
        if (*p == '$' || *p == '#') {
            special = 1;
        }
        /* check for special commands with : */
        if (p2 && *p == '@') {
            special = 1;
        }
        if (p2 || special) {
            /* check for anything before unit/partition number (@,&), but not a '/' */
            if (*p != '/' && (*p < '0' || *p > '9')) {
                p1 = p;
                /* wait for numbers to appear */
                if (special) {
                    /* compensate for CMD $*=P and $*=T syntax */
                    if (*p == '$' && (p + 2) < limit && *(p + 1) == '='
                        && (*(p + 2) == 'P' || *(p + 2) == 'T') ) {
                        p += 2;
                    } else if (!p2 && *p == '$' && (p + 1) < limit
                        && *(p + 1) >= '0' && *(p + 1) <= '9') {
                        /* compensate for just $n */
                        p2 = limit;
                    }
                    p++;
                } else {
                    while (p < limit && (*p < '0' || *p > '9')) {
                        p++;
                    }
                }
                cmd_parse->commandlength = (unsigned int)(p - p1);
                cmd_parse->command = lib_calloc(1, cmd_parse->commandlength + 1);
                memcpy(cmd_parse->command, p1, cmd_parse->commandlength);
                cmd_parse->command[cmd_parse->commandlength] = 0;
            }
            /* if in special mode, and no unit/path is provided, anything after first character is the filename */
            if (p2) {
                /* skip any more spaces */
                while (p < limit && *p == ' ') {
                    p++;
                }
                /* get unit/part number if not at ':' yet and next value is a digit*/
                if (p < p2 && (*p >= '0' || *p <= '9')) {
                    cmd_parse->drive = 0;
                    while (p < p2 && (*p >= '0' && *p <= '9')) {
                        /* unit/part should never be more than 256, if so, skip them */
                        i = cmd_parse->drive * 10 + (*p - '0');
                        if (i < 256) {
                            cmd_parse->drive = i;
                        }
                        p++;
                    }
                }
                /* skip any more spaces */
                while (p < limit && *p == ' ') {
                    p++;
                }
/* TODO: CMD limits paths by having them begin and end with a '/' */
                /* get path if not at ':' yet and next value is not a ':'; just use p2 limit here*/
                if (p < p2) {
                    p1 = p;
                    p = p2;
                    cmd_parse->pathlength = (unsigned int)(p - p1);
                    cmd_parse->path = lib_calloc(1, cmd_parse->pathlength + 1);
                    memcpy(cmd_parse->path, p1, cmd_parse->pathlength);
                    cmd_parse->path[cmd_parse->pathlength] = 0;
                    p = p2;
                }
            }
        }
        /* skip first colon, others are allowed in the file name */
        if (*p == ':') {
            p++;
        }
        /* file to follow */
        p1 = p;
        p = p2 = memchr(p, ',', limit - p);
        /* find end of input or first ',' */
        /* compensate for CMD $*=P, and $*=T, syntax (coma) */
        if (p2 && cmd_parse->command && cmd_parse->command[0]=='$') {
            p = NULL;
        }
        if (!p) {
            p = limit;
        }
        cmd_parse->filelength = (unsigned int)(p - p1);
        cmd_parse->file = lib_calloc(1, cmd_parse->filelength + 1);
        memcpy(cmd_parse->file, p1, cmd_parse->filelength);
        cmd_parse->file[cmd_parse->filelength] = 0;

        /* Preset the file-type if the LOAD/SAVE secondary addresses are used. */
        cmd_parse->filetype = (cmd_parse->secondary < 2) ? CBMDOS_FT_PRG : 0;

        if (p2) {
        /* type and mode next */
            while (p < limit) {
                p++;
                switch (*p) {
                    case 'S':
                        cmd_parse->filetype = CBMDOS_FT_SEQ;
                        break;
                    case 'P':
                        cmd_parse->filetype = CBMDOS_FT_PRG;
                        break;
                    case 'U':
                        cmd_parse->filetype = CBMDOS_FT_USR;
                        break;
                    case 'L'|0x80:
                    case 'L':                   /* L,(#record length)  max 254 */
                        if (p+2 < limit && p[1] == ',') {
                            cmd_parse->recordlength = p[2]; /* Changing RL causes error */
                            /* Don't allow REL file record lengths less than 2 or
                               greater than 254.  The 1541/71/81 lets you create a
                               REL file of record length 0, but it locks up the CPU
                               on the drive - nice. */
                            if (cmd_parse->recordlength < 2 || cmd_parse->recordlength > 254) {
                                return CBMDOS_IPE_OVERFLOW;
                            }
                            /* skip the REL length */
                            p += 2;
                            cmd_parse->filetype = CBMDOS_FT_REL;
                        } else {
                            return CBMDOS_IPE_OVERFLOW;
                        }             
                        break;
                    case 'R':
                        cmd_parse->readmode = CBMDOS_FAM_READ;
                        break;
                    case 'W':
                        cmd_parse->readmode = CBMDOS_FAM_WRITE;
                        break;
                    case 'A':
                        cmd_parse->readmode = CBMDOS_FAM_APPEND;
                        break;
                    default:
                        return CBMDOS_IPE_INVAL;
                }
                p++;
                /* skip extra characters after first ','; ",sequential,write" is allowed for example */
                p = memchr(p, ',', limit - p);
                if (!p) {
                    break;
                }
            }
            /* Override read mode if secondary is 0 or 1.  */
            if (cmd_parse->secondary == 0) {
                cmd_parse->readmode = CBMDOS_FAM_READ;
            }
            if (cmd_parse->secondary == 1) {
                cmd_parse->readmode = CBMDOS_FAM_WRITE;
            }
        }
    } else 
    /* in standard (flexible) command mode; too hard to merge everything
        - too many special cases */
    if (cmd_parse->mode == 1 && p < limit) {
        cmd_parse->drive = 0;
        /* any command beginning with P, or M do not require any parsing,
            copy all information into command */
        /* only process U1, U2, UA, UB */
        special = 0;
        if (*p == 'U' || *p == 'M') {
            special++;
            if (p + 1 < limit) {
                if (p[0] == 'U'
                    && (p[1] == '1' || p[1] == 'A' || p[1] == '2' || p[1] == 'B')) {
                    special = 0;
                }
                if (p[0] == 'M' && p[1] == 'D') {
                    special = 0;
                }
            } else {
                return CBMDOS_IPE_INVAL;
            }
        }
        if (special || *p == 'P') {
            cmd_parse->commandlength = cmd_parse->fulllength;
            cmd_parse->command = lib_calloc(1, cmd_parse->commandlength);
            memcpy(cmd_parse->command, cmd_parse->full, cmd_parse->commandlength);
            return CBMDOS_IPE_OK;
        }
        /* look for a ':' to separate the unit/part-path from name */
        p2 = memchr(p, ':', limit - p);
        /* if a colon is found, flag it */
        if (p2) {
            cmd_parse->colon = 1;
        }

        special = (*p == 'U');
        /* check for command before unit/partition number (I,V,C,etc) */
        if (*p < '0' || *p > '9') {
            p1 = p;
            /* compensate for "BLOCK-ALLOCATE", for example, which will
                become "B-A" , but not for UA, etc*/
            while (p < limit) {
                /* alpha */
                temp[templength++] = *p;
                p++;
                if(p >= limit) {
                    break;
                }
                /* if first character of command is not alpha, then it is only 1 character */
                if(*p1 < 'A') {
                    break;
                }
                /* if command is CP or C(shift P) */
                if (*p1 == 'C') {
                    if (*p == 'P' || *p == 'D') {
                        /* for CP, partition # is the file name */
                        /* for CD, "<-" is the file name, or the unit and part
                            go in the right place */
                        p++;
                        break;
                    } else if (*p == ('P' | 0x80)) {
                        /* for C{shift}P, copy the command */
                        cmd_parse->commandlength = 3;
                        cmd_parse->command = lib_calloc(1, cmd_parse->commandlength);
                        memcpy(cmd_parse->command, cmd_parse->full, cmd_parse->commandlength);
                        return CBMDOS_IPE_OK;
                    }
                } else if (*p1 == 'M' && *p == 'D') {
                    /* MD */
                    p++;
                    break;
                }
                /* wait for non-alpha to appear or next character after U */
                if (!special) {
                    while (p < limit
                        && ((*p >= 'A' && *p <= 'Z')
                            || (*p >= ('A' | 0x80) && *p <= ('Z' | 0x80)))) {
                        p++;
                    }
                }
                if (p >= limit) {
                    break;
                }
                /* get out the moment we see a delimiter */
                if (*p == ':' || *p == ' ' || *p == 29 ) {
                    break;
                }
                if (!special) {
                    if (*p == '/' || (*p >= '0' && *p <= '9')) {
                        break;
                    }
                }
                /* non-alpha */
                temp[templength++] = *p;
                p++;
                if (p >= limit) {
                    break;
                }
                /* get out the moment we see a delimiter */
                if (*p == ':' || *p == ' ' || *p == 29 ) {
                    break;
                }
                if (!special) {
                    if (*p == '/' || (*p >= '0' && *p <= '9')) {
                        break;
                    }
                }
            }

            cmd_parse->commandlength = (unsigned int)(p - p1);
            cmd_parse->command = lib_calloc(1, cmd_parse->commandlength + 1);
            memcpy(cmd_parse->command, p1, cmd_parse->commandlength);
            cmd_parse->command[cmd_parse->commandlength] = 0;
            cmd_parse->abbrvlength = templength;
            cmd_parse->abbrv = lib_calloc(1, cmd_parse->abbrvlength + 1);
            memcpy(cmd_parse->abbrv, temp, cmd_parse->abbrvlength);
            cmd_parse->abbrv[cmd_parse->abbrvlength] = 0;
        }

        if (p >= limit) {
            return CBMDOS_IPE_OK;
        }

        /* skip any spaces or cursor-lefts */
        while (p < limit && (*p == ' ' || *p == 29) ) {
            p++;
        }

        /* get unit/part number if not at ':' yet and next value is a digit*/
        if (p < p2 && (*p >= '0' || *p <= '9')) {
            while (p < p2 && (*p >= '0' && *p <= '9')) {
                /* unit/part should never be more than 256, if so, skip them */
                i = cmd_parse->drive * 10 + (*p - '0');
                if (i < 256) {
                    cmd_parse->drive = i;
                }
                p++;
            }
        }

        /* skip any more spaces */
        while (p < limit && *p ==' ') {
            p++;
        }

        /* get path if not at ':' yet and next value is not a ':'; just use p2 limit here*/
        if (p < p2) {
            p1 = p;
            p = p2;
            cmd_parse->pathlength = (unsigned int)(p - p1);
            cmd_parse->path = lib_calloc(1, cmd_parse->pathlength + 1);
            memcpy(cmd_parse->path, p1, cmd_parse->pathlength);
            cmd_parse->path[cmd_parse->pathlength] = 0;
            p = p2;
        }

        /* skip first colon, others are allowed in the file name */
        if (*p == ':') {
            p++;
        }
        /* file to follow */
        p1 = p;
        p = memchr(p1, '=', limit - p1);
        p2 = memchr(p1, ',', limit - p1);
        if (p && p2 && (p2 < p)) {
            p = p2;
        } else if (!p && p2) {
            p = p2;
        }
        /* find end of input or first ',' */
        if (!p) {
            p = limit;
        }
        cmd_parse->filelength = (unsigned int)(p - p1);
        cmd_parse->file = lib_calloc(1, cmd_parse->filelength + 1);
        memcpy(cmd_parse->file, p1, cmd_parse->filelength);
        cmd_parse->file[cmd_parse->filelength] = 0;

        if (p < limit) {
            cmd_parse->morelength = (unsigned int)(limit - p);
            cmd_parse->more = lib_calloc(1, cmd_parse->morelength + 1);
            memcpy(cmd_parse->more, p, cmd_parse->morelength);
            cmd_parse->more[cmd_parse->morelength] = 0;
        }

    }

    return CBMDOS_IPE_OK;
}
