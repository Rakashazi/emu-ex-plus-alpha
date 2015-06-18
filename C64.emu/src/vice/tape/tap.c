/*
 * tap.c - TAP file support.
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  David Hansel <david@hansels.net>
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
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "datasette.h"
#include "lib.h"
#include "tap.h"
#include "tape.h"
#include "types.h"
#include "util.h"
#include "zfile.h"

#define TAP_DEBUG 0

#define TAP_PULSE_SHORT(x) \
    ((x) >= tap_pulse_short_min && (x) <= tap_pulse_short_max)
#define TAP_PULSE_MIDDLE(x) \
    ((x) >= tap_pulse_middle_min && (x) <= tap_pulse_middle_max)
#define TAP_PULSE_LONG(x) \
    ((x) >= tap_pulse_long_min && (x) <= tap_pulse_long_max)

#define TAP_PULSE_TT_SHORT(x) \
    ((x) >= tap_pulse_tt_short_min && (x) <= tap_pulse_tt_short_max)
#define TAP_PULSE_TT_LONG(x) \
    ((x) >= tap_pulse_tt_long_min && (x) <= tap_pulse_tt_long_max)


#define MAX_ERRORS 30

#define PILOT_TYPE_ANY -1
#define PILOT_TYPE_CBM 0
#define PILOT_TYPE_TT  1

/* Default values.  Call tap_init() to change. */
static int tap_pulse_short_min = 0x24;
static int tap_pulse_short_max = 0x36;
static int tap_pulse_middle_min = 0x37;
static int tap_pulse_middle_max = 0x49;
static int tap_pulse_long_min = 0x4a;
static int tap_pulse_long_max = 0x64;

static int tap_pulse_tt_short_min = 0x0a;
static int tap_pulse_tt_short_max = 0x22;
static int tap_pulse_tt_long_min = 0x23;
static int tap_pulse_tt_long_max = 0x36;


static int tap_header_read(tap_t *tap, FILE *fd)
{
    BYTE buf[TAP_HDR_SIZE];

    if (fread(buf, TAP_HDR_SIZE, 1, fd) != 1) {
        return -1;
    }

    if (strncmp("C64-TAPE-RAW", (char *)&buf[TAP_HDR_MAGIC_OFFSET], 12)
        && strncmp("C16-TAPE-RAW", (char *)&buf[TAP_HDR_MAGIC_OFFSET], 12)) {
        return -1;
    }

    tap->version = buf[TAP_HDR_VERSION];
    tap->system = buf[TAP_HDR_SYSTEM];

    memcpy(tap->name, &buf[TAP_HDR_MAGIC_OFFSET], 12);

    return 0;
}

static tap_t *tap_new(void)
{
    tap_t *tap;

    tap = lib_calloc(1, sizeof(tap_t));

    tap->file_name = NULL;
    tap->counter = 0;
    tap->current_file_seek_position = 0;
    tap->mode = DATASETTE_CONTROL_STOP;
    tap->offset = TAP_HDR_SIZE;
    tap->has_changed = 0;
    tap->current_file_number = -1;
    tap->current_file_data = NULL;
    tap->current_file_size = 0;

    return tap;
}

tap_t *tap_open(const char *name, unsigned int *read_only)
{
    FILE *fd;
    tap_t *new;

    fd = NULL;

    if (*read_only == 0) {
        fd = zfile_fopen(name, MODE_READ_WRITE);
    }

    if (fd == NULL) {
        fd = zfile_fopen(name, MODE_READ);
        if (fd == NULL) {
            return NULL;
        }
        *read_only = 1;
    } else {
        *read_only = 0;
    }

    new = tap_new();

    if (tap_header_read(new, fd) < 0) {
        zfile_fclose(fd);
        lib_free(new);
        return NULL;
    }

    new->fd = fd;
    new->read_only = *read_only;

    new->size = (int)util_file_length(fd) - TAP_HDR_SIZE;

    if (new->size < 3) {
        zfile_fclose(new->fd);
        lib_free(new);
        return NULL;
    }

    new->file_name = lib_stralloc(name);
    new->tap_file_record = lib_calloc(1, sizeof(tape_file_record_t));
    new->current_file_number = -1;
    new->current_file_data = NULL;
    new->current_file_size = 0;

    return new;
}

int tap_close(tap_t *tap)
{
    int retval;

    if (tap->fd != NULL) {
        if (tap->has_changed) {
            BYTE buf[4];
            util_dword_to_le_buf(buf, tap->size);
            util_fpwrite(tap->fd, buf, 4, TAP_HDR_LEN);
        }
        retval = zfile_fclose(tap->fd);
        tap->fd = NULL;
    } else {
        retval = 0;
    }

    lib_free(tap->current_file_data);
    lib_free(tap->file_name);
    lib_free(tap->tap_file_record);
    lib_free(tap);

    return retval;
}

/* ------------------------------------------------------------------------- */

int tap_create(const char *name)
{
    FILE *fd;
    BYTE block[256];

    memset(block, 0, sizeof(block));

    fd = fopen(name, MODE_WRITE);

    if (fd == NULL) {
        return -1;
    }

    /* create an empty tap */
    strcpy((char *)&block[TAP_HDR_MAGIC_OFFSET], "C64-TAPE-RAW");

    block[TAP_HDR_VERSION] = 1;

    util_dword_to_le_buf(&block[TAP_HDR_LEN], 4);

    if (fwrite(block, 24, 1, fd) < 1) {
        fclose(fd);
        return -1;
    }

    fclose(fd);

    return 0;
}


/* ------------------------------------------------------------------------- */

static int tap_find_pilot(tap_t *tap, int type);

inline static int tap_get_pulse(tap_t *tap, int *pos_advance)
{
    BYTE data;
    DWORD pulse_length = 0;
    size_t res;

    *pos_advance = 0;
    res = fread(&data, 1, 1, tap->fd);

    if (res == 0) {
        return -1;
    }

    *pos_advance += (int)res;

    if (data == 0) {
        if (tap->version == 0) {
            pulse_length = 256;
        } else if ((tap->version == 1) || (tap->version == 2)) {
            BYTE size[3];
            res = fread(size, 3, 1, tap->fd);
            if (res == 0) {
                return -1;
            }
            *pos_advance += 3;
            pulse_length = ((size[2] << 16) | (size[1] << 8) | size[0]) >> 3;
        }
    } else {
        pulse_length = data;
    }

    /*  Handle Halfwave format for C16 tapes */
    if (tap->version == 2) {
        DWORD pulse_length2;

        res = fread(&data, 1, 1, tap->fd);

        if (res == 0) {
            return -1;
        }
        *pos_advance += (int)res;
        if (data == 0) {
            BYTE size[3];
            res = fread(size, 3, 1, tap->fd);
            if (res == 0) {
                return -1;
            }
            *pos_advance += 3;
            pulse_length2 = ((size[2] << 16) | (size[1] << 8) | size[0]) >> 3;
        } else {
            pulse_length2 = data;
        }

        /*  This should do for the time being */
        pulse_length += pulse_length2;
    }

#if TAP_DEBUG > 2
    if (TAP_PULSE_SHORT(data)) {
        log_debug("s");
    } else if (TAP_PULSE_MIDDLE(data)) {
        log_debug("m");
    } else if (TAP_PULSE_LONG(data)) {
        log_debug("l");
    }
#endif

    return (int)pulse_length;
}

/* ------------------------------------------------------------------------- */


inline static int tap_cbm_read_bit(tap_t *tap)
{
    int pulse1, pulse2;
    int pos_advance;

    pulse1 = tap_get_pulse(tap, &pos_advance);
    if (pulse1 < 0) {
        return -1;
    }
    pulse2 = tap_get_pulse(tap, &pos_advance);
    if (pulse2 < 0) {
        return -1;
    }

    /* when reading a bit, treat L as M */
    if (TAP_PULSE_SHORT(pulse1) && (TAP_PULSE_MIDDLE(pulse2) || TAP_PULSE_LONG(pulse2))) {
        return 0; /* S-M => 0 */
    } else if ((TAP_PULSE_MIDDLE(pulse1) || TAP_PULSE_LONG(pulse1)) && TAP_PULSE_SHORT(pulse2)) {
        return 1; /* M-S => 1 */
    } else {
        return -2; /* either M-M or S-S or out-of-range => read error */
    }
}


static int tap_cbm_read_byte(tap_t *tap)
{
    int i, data, parity;
    BYTE read;
    int pos_advance;

    /* check for L pulse (start-of-byte) */
    data = tap_get_pulse(tap, &pos_advance);
    if (data < 0 || !TAP_PULSE_LONG(data)) {
        return -1;
    }

    /* expect either M (L-M: start-of-byte) or S (L-S: end-of-data-block) */
    data = tap_get_pulse(tap, &pos_advance);
    if (data < 0) {
        return -1; /* end of tape */
    } else if (TAP_PULSE_SHORT(data)) {
        return -3; /* end-of-data-block */
    } else if (TAP_PULSE_LONG(data)) {
        return -2; /* found L-L sequence within a block: read error */
    }
    /* read eight bits */
    read = 0;
    parity = 1;
    for (i = 0; i < 8; i++) {
        read >>= 1;
        data = tap_cbm_read_bit(tap);
        if (data < 0) {
            return data;
        } else if (data > 0) {
            read |= 0x80;
        }

        parity ^= data;
    }

    /* read and check parity bit */
    data = tap_cbm_read_bit(tap);
    if (data < 0) {
        return data;
    }
    if (data != parity) {
        return -2;
    }

    return read;
}

static int tap_cbm_skip_pilot(tap_t *tap)
{
    int data, errors;
    long fpos, counter;
    long fpos2;
    long current_filepos;
    int pos_advance;

    errors = 0;
    counter = 0;
    current_filepos = ftell(tap->fd);
    while (1) {
        /*  Save file position */
        fpos = current_filepos;
        data = tap_get_pulse(tap, &pos_advance);
        current_filepos += pos_advance;
        fpos2 = current_filepos;
        if (TAP_PULSE_LONG(data)) {
            /* found an L pulse, try to read a byte */
            fseek(tap->fd, fpos, SEEK_SET);
            current_filepos = fpos;
            data = tap_cbm_read_byte(tap);
            if (data == -1) {
                /* end-of-tape */
                return -1;
            } else if (data < 0) {
                /* byte read failed. Give up after trying 50 times */
                if (++errors > 50) {
                    return 0;
                }

                /* Start over after the L pulse */
                fseek(tap->fd, fpos2, SEEK_SET);
                current_filepos = fpos2;
                counter = 0;
            } else {
                /* success.  Go back to start of byte and return */
                fseek(tap->fd, fpos, SEEK_SET);
                current_filepos = fpos;
                return 0;
            }
        } else if (data < 0) {
            return -1; /* end-of-tape */
        } else {
            ++counter;
            if (!TAP_PULSE_SHORT(data)) {
                return 0;
            }
        } 
    }

    return 0;
}


#if TAP_DEBUG>0 
void tap_cbm_print_error(int ret)
{
    switch (ret) {
        case 0:
            log_debug("OK\n");
            break;
        case -1:
            log_debug("ERROR (tape end)\n");
            break;
        case -2:
            log_debug("ERROR (sync read)\n");
            break;
/*      case -3:
            log_debug("ERROR (short block)\n");
            break;
 */
        case -4:
            log_debug("ERROR (long block)\n");
            break;
        case -5:
            log_debug("ERROR (too many single errors)\n");
            break;
        case -6:
            log_debug("ERROR (double read error)\n");
            break;
        case -7:
            log_debug("ERROR (checksum error)\n");
            break;
        default:
            log_debug("ERROR (%i?)\n", ret);
    }
}
#endif

static int tap_cbm_read_block_once(tap_t *tap, int *pass, BYTE *buffer, int *size, int *error_buf, int *error_count)
{
    int count, data, ecount, found_pass;

    /* return values:
        0 : ok
       -1 : tape end reached
       -2 : error reading sync
       (-3 : short block) short blocks are okay, size will be adapted (iAN CooG, AndrerasM)
       -4 : long block
       -5 : too many single errors
       -6 : unrecoverable read error */

    /* skip pilot */
    if (tap_cbm_skip_pilot(tap) < 0) {
        return -1;
    }

    /* check countdown (sync) */
    found_pass = -1;
    for (count = 9; count > 0; count--) {
        data = tap_cbm_read_byte(tap);
#if TAP_DEBUG > 1
        if (data >= 0) {
            log_debug("<%02x> ", data);
        }
#endif
        if (data == -1) {
            return -1; /* end-of-tape */
        } else {
            if (count != (data & 0x7f)) {
                return -2; /* sync read error */
            }
            if (found_pass < 0) {
                found_pass = data & 0x80 ? 1 : 2;
            } else if ((found_pass == 1 && !(data & 0x80)) || (found_pass == 2 && (data & 0x80))) {
                return -2; /* sync read error */
            }
        }
    }

    /* we found a sync countdown. Adjust 'pass' to reflect the pass indicated
       by the countdown */
    *pass = found_pass;
    if (*pass == 1) {
        *error_count = 0;
    }

    /* read data */
    count = 0;
    ecount = 0;
    while (1) {
        data = tap_cbm_read_byte(tap);

        if (data == -1) {
            return -1; /* end-of-tape */
        } else if (data == -2) {
#if TAP_DEBUG > 1
            log_debug(" ##");
#endif
            if (*pass == 1) {
                /* single error in first pass => store position in error_buf and go on */
                if (*error_count < MAX_ERRORS) {
                    error_buf[(*error_count)++] = count;
                } else {
                    return -5; /* too many single errors */
                }
            } else if (*pass == 2) {
                /* single error in second pass => check if this byte was also in error on
                   first pass.  If so then fail, otherwise go on. */
                while (ecount < *error_count && error_buf[ecount] < count) {
                    ecount++;
                }
                if ((*error_count < 0) || (error_buf[ecount] == count)) {
                    return -6; /* error: double read error  */
                }
            }

            count++;
        } else if (data == -3) {
            /* found L-S sequence (end-of-block) */
            *size = count; /* set size */
            return 0;
        } else {
            /* byte ok */
            if (count < *size) {
#if TAP_DEBUG > 1
                log_debug(" %02x", (BYTE) data);
#endif
                buffer[count++] = (BYTE) data;
                if ((tap->system == 2) && (count == *size)) {
                    /*  On the C16 a block is finished with 1 Medium pulse
                        followed by 450 Small pulse
                        Return here, because the byte read expects a Long pulse
                        and that would position
                        us on the next block which is plain wrong */
                    return 0;
                }
            } else {
                return -4; /* error: long block */
            }
        }
    }
}


/* NOTE: parameter "size" must equal expected block size + 1 (for parity byte) */
static int tap_cbm_read_block(tap_t *tap, BYTE *buffer, int size)
{
    int i, ret, pass, error_count, error_buf[MAX_ERRORS];

#if TAP_DEBUG > 0
    log_debug("\nTAP_CBM_READ_BLOCK(size %i): ", size);
#endif

    ret = -1;
    error_count = -1;
    for (pass = 1; pass <= 2; pass++) {
        /* try to read data.  If tap_cbm_read_block_once() finds a sync countdown
           it will reset 'pass' to the value indicated by the countdown. */
        ret = tap_cbm_read_block_once(tap, &pass, buffer, &size, error_buf, &error_count);

#if TAP_DEBUG > 0
        log_debug(" PASS%i:%i/%i ", pass, ret, error_count);
#endif

        if ((ret >= 0) && (error_count == 0)) {
            int parity;

            /* block read ok */
            if (pass == 1) {
                /* The countdown indicated that this was the first pass.
                   Skip over next pilot so next call to tap_read_block
                   won't find the repeat */
                if (tap_find_pilot(tap, PILOT_TYPE_CBM) < 0) {
                    ret = -1;
                }
                if (tap_cbm_skip_pilot(tap) < 0) {
                    ret = -1;
                }
            }

            /* Test checksum:
               EXORing all bytes (including checksum byte) must result in 0 */
            parity = 0;
            for (i = 0; i < size; i++) {
                parity ^= buffer[i];
            }
            if (parity != 0) {
                ret = -7;
            }

            /* exit */
            break;
        } else if (ret == -1) {
            /* reached the end of the tape => exit */
            break;
        } else if (ret < 0) {
            /* we could not read the block at all. */
            error_count = -1;
        }

        if (pass < 2) {
            /* we need to try again.  Find next pilot before repeating */
            ret = tap_find_pilot(tap, PILOT_TYPE_CBM);
            if (ret < 0) {
                break;
            }
        }
    }

#if TAP_DEBUG > 0
    tap_cbm_print_error(ret);
#endif

    return ret;
}

static int tap_cbm_read_header(tap_t *tap)
{
    int ret;
    BYTE buffer[255];

    /* read header data */
    ret = tap_cbm_read_block(tap, buffer, tap->system == 2 ? 193 : 255);
    if (ret < 0) {
        return ret;
    }

    /* first byte of header must represent a valid file type (1, 3 or 4) */
    if (buffer[0] != 1 && buffer[0] != 3 && buffer[0] != 4) {
        return -2;
    }

    /* extract info */
    tap->tap_file_record->type = buffer[0];
    tap->tap_file_record->encoding = TAPE_ENCODING_CBM;
    tap->tap_file_record->start_addr = (WORD)(buffer[1] + buffer[2] * 256);
    tap->tap_file_record->end_addr = (WORD)(buffer[3] + buffer[4] * 256);
    memcpy(tap->tap_file_record->name, buffer + 5, 16);

    return 0;
}

static int tap_cbm_read_file_prg(tap_t *tap)
{
    int size, ret;
    size = tap->tap_file_record->end_addr - tap->tap_file_record->start_addr;

    if (size < 0) {
        return -1;
    } else {
        tap->current_file_size = size;
        tap->current_file_data = lib_malloc(tap->current_file_size + 1);

        /* find next pilot */
        ret = tap_find_pilot(tap, PILOT_TYPE_CBM);
        if (ret < 0) {
            return ret;
        }

        return tap_cbm_read_block(tap, tap->current_file_data, (int)tap->current_file_size + 1);
    }
}


static int tap_cbm_read_file_seq(tap_t *tap)
{
    int ret;
    BYTE buffer[193];

    while (1) {
        /* find next pilot */
        ret = tap_find_pilot(tap, PILOT_TYPE_CBM);
        if (ret < 0) {
            /* no more pilot found => end of data */
            break;
        }

        /* read next data block */
        ret = tap_cbm_read_block(tap, buffer, 193);
        if (ret < 0 || buffer[0] != 2) {
            /* next block is not a data continuation block => end of data */
            break;
        }

        /* add received data minus the first byte (continuation marker)
           and last byte (checksum) */
        tap->current_file_size += 191;
        tap->current_file_data = lib_realloc(tap->current_file_data, tap->current_file_size);
        memcpy(tap->current_file_data + tap->current_file_size - 191, buffer + 1, 191);
    }

    return 0;
}


static int tap_cbm_read_file(tap_t *tap)
{
    int res;

    /* read header */
    res = tap_cbm_read_header(tap);
    if (res >= 0) {
        /* read file of type determined by header */
        switch (tap->tap_file_record->type) {
            case 1:
            case 3:
                res = tap_cbm_read_file_prg(tap);
                break;
            case 4:
                res = tap_cbm_read_file_seq(tap);
                break;
            default:
                res = -1;
        }
    }

    return res;
}


static int tap_cbm_skip_file(tap_t *tap)
{
    /* skip header pilot */
    if (tap_cbm_skip_pilot(tap) < 0) {
        return -1;
    }

    /* find and skip header repeat pilot */
    if (tap_find_pilot(tap, PILOT_TYPE_CBM) < 0) {
        return -1;
    }
    if (tap_cbm_skip_pilot(tap) < 0) {
        return -1;
    }

    if (tap->tap_file_record->type == 4) {
        /* sequential file.  must read each block to find last one */
        BYTE buffer[193];
        long fpos;
        int ret;

        while (1) {
            fpos = ftell(tap->fd);

            /* find next pilot */
            ret = tap_find_pilot(tap, PILOT_TYPE_CBM);
            if (ret < 0) {
                /* no more pilot found => end of data */
                fseek(tap->fd, fpos, SEEK_SET);
                break;
            }

            /* read next data block */
            ret = tap_cbm_read_block(tap, buffer, 193);
            if (ret < 1 || buffer[0] != 2) {
                /* next block is not a data continuation block => end of data */
                fseek(tap->fd, fpos, SEEK_SET);
                break;
            }
        }
    } else {
        /* program file has only one block. find and skip data pilot */
        if (tap_find_pilot(tap, PILOT_TYPE_CBM) < 0) {
            return -1;
        }
        if (tap_cbm_skip_pilot(tap) < 0) {
            return -1;
        }

        /* find and skip data repeat pilot */
        if (tap_find_pilot(tap, PILOT_TYPE_CBM) < 0) {
            return -1;
        }
        if (tap_cbm_skip_pilot(tap) < 0) {
            return -1;
        }
    }

    return 0;
}


/* ------------------------------------------------------------------------- */

#define TT_BLOCK_TYPE_HEADER 1
#define TT_BLOCK_TYPE_DATA   0


/* FIXME:  It would be better to decide whether a pulse is S or L by
   checking if it's shorter or longer than a threshold value computed when
   reading the pilot (instead of using fixed thresholds) */
static int tap_tt_read_byte(tap_t *tap)
{
    int pulse, i;
    BYTE read;
    int pos_advance;

    read = 0;

    /* turbo-tape encodes a byte as a sequence of 8 short or long pulses,
       short pulse=0,  long pulse=1.  MSB comes first */
    for (i = 0; i < 8; i++) {
        pulse = tap_get_pulse(tap, &pos_advance);
        if (pulse < 0) {
            return -1;
        }

        read <<= 1;
        if (TAP_PULSE_TT_LONG(pulse)) {
            read |= 0x01;
        } else if (!TAP_PULSE_TT_SHORT(pulse)) {
            return -2;
        }
    }

    return read;
}

static int tap_tt_skip_pilot(tap_t *tap)
{
    int data;

#if TAP_DEBUG > 1
    log_debug("\nTAP_TT_SKIP_PILOT(0x%X", ftell(tap->fd));
#endif

    /* turbo-tape pilot is just repeats of value 0x02 */
    do {
        data = tap_tt_read_byte(tap);
        if (data < 0) {
            return data;
        }

        if (data != 2) {
            /* value != 0x02, we found the end of the pilot.  Go back
               so byte can be read again */
            fseek(tap->fd, -8, SEEK_CUR);
        }
    } while (data == 2);

#if TAP_DEBUG > 1
    log_debug("-0x%X) ", ftell(tap->fd));
#endif

    return 0;
}


#if TAP_DEBUG > 0
static void tap_tt_print_error(int ret)
{
    switch (ret) {
        case -1:
            log_debug("ERROR (tape end)\n");
            break;
        case -2:
            log_debug("ERROR (sync read)\n");
            break;
        case -3:
            log_debug("ERROR (block type)\n");
            break;
        case -4:
            log_debug("ERROR (data)\n");
            break;
        case -5:
            log_debug("ERROR (checksum)\n");
            break;
        case -6:
            log_debug("ERROR (pilot)\n");
            break;

        default:
            log_debug("OK (%i bytes read)\n", ret);
    }
}
#endif

static int tap_tt_read_block(tap_t *tap, int type, BYTE *buffer, unsigned int size)
{
    int data;
    unsigned int count;

#if TAP_DEBUG > 1
    log_debug("TAP_TT_READ_BLOCK(%u) ", size);
#endif

    /* skip pilot */
    data = tap_tt_skip_pilot(tap);
    if (data == -1) {
        return -1; /* end-of-tape */
    } else if (data < 0) {
        return -6; /* pilot read error */
    }
    /* check countdown */
    for (count = 9; count > 0; count--) {
        data = tap_tt_read_byte(tap);
#if TAP_DEBUG > 1
        if (data >= 0) {
            log_debug("<%02x> ", data);
        }
#endif
        if (data == -1) {
            return -1; /* end-of-tape */
        } else if (data != (int) count) {
            return -2; /* sync read error */
        }
    }

    /* read and check type (0x01 = header block, 0x00 = data block) */
    data = tap_tt_read_byte(tap);
    if (data == -1) {
        return data;
    }
#if TAP_DEBUG > 1
    if (data >= 0) {
        log_debug("<%02x> ", data);
    }
#endif
    if (((type == TT_BLOCK_TYPE_DATA) && (data != 0)) ||
        ((type == TT_BLOCK_TYPE_HEADER) && (data != 1) && (data != 2))) {
        return -3; /* block type error */
    }
#if TAP_DEBUG > 1
    if (buffer == NULL) {
        log_debug("(.....) ");
    }
#endif

    /* read data */
    for (count = 0; count < size; count++) {
        data = tap_tt_read_byte(tap);
        if (data == -1) {
            return -1;             /* end-of-tape */
        }
        if (data < 0) {
            return -4;             /* data error */
        }
#if TAP_DEBUG > 1
        if (data >= 0 && buffer != NULL) {
            log_debug("%02x ", data);
        }
#endif

        if (buffer != NULL) {
            buffer[count] = (BYTE) data;
        }
    }

    if (type == TT_BLOCK_TYPE_DATA) {
        /* data block has checksum */
        data = tap_tt_read_byte(tap);
        if (data == -1) {
            return -1;             /* end-of-tape */
        }
        if (data < 0) {
            return -4;             /* data error */
        }
#if TAP_DEBUG > 1
        if (data >= 0) {
            log_debug("[%02x] ", data);
        }
#endif
        if (buffer != NULL) {
            for (count = 0; count < size; count++) {
                data ^= buffer[count];
            }
            if (data != 0) {
                return -5;          /* checksum error */
            }
        }
    }

    return size;
}


static int tap_tt_read_header(tap_t *tap)
{
    BYTE buffer[193];
    int res;

    /* read header block */
    res = tap_tt_read_block(tap, TT_BLOCK_TYPE_HEADER, buffer, 193);
#if TAP_DEBUG > 0
    tap_tt_print_error(res);
#endif
    if (res < 0) {
        return res;
    }

    /* extract info */
    tap->tap_file_record->type = 1;
    tap->tap_file_record->encoding = TAPE_ENCODING_TURBOTAPE;
    tap->tap_file_record->start_addr = (WORD)(buffer[0] + buffer[1] * 256);
    tap->tap_file_record->end_addr = (WORD)(buffer[2] + buffer[3] * 256);
    memcpy(tap->tap_file_record->name, buffer + 5, 16);

    return 0;
}


static int tap_tt_read_file(tap_t *tap)
{
    int ret;

    /* read header */
    ret = tap_tt_read_header(tap);
    if (ret < 0) {
        return ret;
    }

    tap->current_file_size = tap->tap_file_record->end_addr - tap->tap_file_record->start_addr + 1;
    tap->current_file_data = lib_malloc(tap->current_file_size);

    /* read data */
    return tap_tt_read_block(tap, TT_BLOCK_TYPE_DATA,
                             tap->current_file_data,
                             (unsigned int)tap->current_file_size);
}


static int tap_tt_skip_file(tap_t *tap)
{
    int res;
    BYTE buffer[193];

    /* read header */
    res = tap_tt_read_block(tap, TT_BLOCK_TYPE_HEADER, buffer, 193);
    if (res >= 0) {
        /* skip data */
        res = tap_tt_read_block(tap, TT_BLOCK_TYPE_DATA, NULL,
                                (buffer[2] + buffer[3] * 256) -
                                (buffer[0] + buffer[1] * 256) + 1);
    }

    return res;
}


/* ------------------------------------------------------------------------- */

#define PILOT_MIN_LENGTH_TT   200
#define PILOT_MIN_LENGTH_CBM   32

static int tap_determine_pilot_type(tap_t *tap)
{
    int res;

    /* assuming we are located on a pilot, try to find out which type it is */
    if (tap->system == 2) {
        return PILOT_TYPE_CBM;
    }
    res = tap_tt_read_byte(tap);
    if (res == 2) {
        return PILOT_TYPE_TT;
    } else {
        return PILOT_TYPE_CBM;
    }
}


static int tap_find_pilot(tap_t *tap, int type)
{
    int i, countCBM, countTT, startCBM, startTT, minCBM;
    int count;
    int data[256];
    long pos[257];
    BYTE buffer[256];

    /* when looking for any pilot type, require CBM pilot to be longer
       than when specifically looking for CBM pilot.  A TurboTape L pulse
       (encoding a '1') is pretty close to a CBM S pulse.  So a sequence
       of '1' bits in a TT encoded file can be misinterpreted as a CBM
       pilot.  This will of course be rejected when trying to read the CBM
       header but that takes time.  By looking for a longer pilot we
       can significantly cut down on the time it takes to scan through a TAP
       file */
    minCBM = (type == PILOT_TYPE_ANY) ? 1000 : PILOT_MIN_LENGTH_CBM;

    startCBM = ftell(tap->fd);
    startTT = startCBM;
    countCBM = 0;
    countTT = 0;

#if TAP_DEBUG > 0
    log_debug(" TAP_FIND_PILOT");
#endif

    while ((countCBM < minCBM) && (countTT < PILOT_MIN_LENGTH_TT * 8)) {
/*        count = fread(&data, 1, 256, tap->fd); */
        int startpos = ftell(tap->fd);
        int readlen = (int)fread(buffer, 1, 256, tap->fd);
        DWORD pulse_length = 0;
        int j = 0;
        int needed;
        int res;
        for (i = 0; i < readlen; ) {
            pos[j] = startpos + i;
            if (buffer[i] == 0) {
                if (tap->version == 0) {
                    pulse_length = 256;
                    i++;
                } else if ((tap->version == 1) || (tap->version == 2)) {
                    int still_in_buffer = readlen - (i + 1);
                    needed = 3 - still_in_buffer;
                    if (needed <= 0) {
                        pulse_length = ((buffer[i + 3] << 16) | (buffer[i + 2] << 8) | buffer[i + 1]) >> 3;
                        i += 4;
                    } else {
                        /* There is not enough in the buffer
                           Read some more */
                        memcpy(buffer, buffer + i + 1, still_in_buffer);
                        res = (int)fread(buffer + still_in_buffer, 1, needed, tap->fd);
                        i = readlen;
                        if (res == 0) {
                            continue;
                        }
                        pulse_length = ((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]) >> 3;
                    }
                }
            } else {
                pulse_length = buffer[i];
                i++;
            }
            data[j] = pulse_length;

            if (tap->version == 2) {
                DWORD pulse_length2;
                /*  Read one more byte if run out of buffer */
                if (i == readlen) {
                    readlen = (int)fread(buffer, 1, 1, tap->fd);
                    if (readlen == 0) {
                        continue;
                    }
                    i = 0;
                }
                if (buffer[i] == 0) {
                    int still_in_buffer = readlen - (i + 1);
                    needed = 3 - still_in_buffer;
                    if (needed <= 0) {
                        pulse_length2 = ((buffer[i + 3] << 16) | (buffer[i + 2] << 8) | buffer[i + 1]) >> 3;
                        i += 4;
                    } else {
                        /* There is not enough in the buffer
                           Read some more */
                        memcpy(buffer, buffer + i + 1, still_in_buffer);
                        res = (int)fread(buffer + still_in_buffer, 1, needed, tap->fd);
                        i = readlen;
                        if (res == 0) {
                            continue;
                        }
                        pulse_length2 = ((buffer[2] << 16) | (buffer[1] << 8) | buffer[0]) >> 3;
                    }
                } else {
                    pulse_length2 = buffer[i];
                    i++;
                }
                data[j] += pulse_length2;
            }
            j++;
        }
        count = j;
        pos[j] = ftell(tap->fd);

/*        for (i = 0, count = 0; i < 256; i++, count++) {
            pos[i] = ftell(tap->fd);
            data[i] = tap_get_pulse(tap);
            if (data[i] < 0) break;
        }
        pos[i] = ftell(tap->fd);*/
        if (count < 1) {
            return -1;
        }

        for (i = 0; (i < count) && (countCBM < minCBM) && (countTT < PILOT_MIN_LENGTH_TT * 8); i++) {
            if (type == PILOT_TYPE_ANY || type == PILOT_TYPE_CBM) {
                /* cbm pilot is at least PILOT_MIN_LENGTH_CBM consecutive short pulses */
                if (TAP_PULSE_SHORT(data[i])) {
                    countCBM++;
                } else {
                    startCBM = pos[i + 1];
                    countCBM = 0;
                }

/*                  { startCBM+=countCBM+1; countCBM = 0; } */
            }

            if (type == PILOT_TYPE_ANY || type == PILOT_TYPE_TT) {
                /* TurboTape pilot is PILOT_MIN_LENGTH_TT or more repeats of the value 0x02.
                   Accept any long bit sequence of 1000000010000000100...
                   Trust that reading the header will fail if we detect a wrong
                   sequence (in that case we come back here) */
                if ((countTT & 7) == 0) {
                    if (TAP_PULSE_TT_LONG(data[i])) {
                        countTT++;
                    } else {
                        startTT = pos[i + 1];
                        countTT = 0;
                    }
/*                      { startTT+=countTT+1; countTT = 0; } */
                } else {
                    if (TAP_PULSE_TT_SHORT(data[i])) {
                        countTT++;
                    } else if (TAP_PULSE_TT_LONG(data[i])) {
                        startTT = pos[i];
                        countTT = 1;
                    }
/*                      { startTT+=countTT; countTT = 1; } */
                    else {
                        startTT = pos[i + 1];
                        countTT = 0;
                    }
/*                      { startTT+=countTT+1; countTT = 0; } */
                }
            }
        }
    }

#if TAP_DEBUG > 0
    if (countTT >= PILOT_MIN_LENGTH_TT * 8) {
        log_debug(" found TT pilot(0x%X)", startTT + 2);
    } else {
        log_debug(" found CBM pilot(0x%X)", startCBM);
    }
#endif

    if (countTT >= PILOT_MIN_LENGTH_TT * 8) {
        /* startTT points to a '1' bit which we assume to be part of the
           value 00000010.  Skip over the 1 and following 0 so we start
           at the beginning of a 00000010 sequence */
        fseek(tap->fd, startTT + 2, SEEK_SET);
        return 1;
    } else {
        fseek(tap->fd, startCBM, SEEK_SET);
        return 0;
    }
}


static int tap_find_header(tap_t *tap)
{
    int res, type;
    long fpos;

    while (1) {
        /* find next pilot */
        type = tap_find_pilot(tap, PILOT_TYPE_ANY);
        if (type < 0) {
            /* reached the end of the tape */
            return -1;
        }

        /* store current position in TAP file */
        fpos = ftell(tap->fd);

        /* try to read a header */
        if (type == PILOT_TYPE_CBM) {
            res = tap_cbm_read_header(tap);
            if (res < 0) {
                int pos_advance;
                fseek(tap->fd, fpos, SEEK_SET);
                while (TAP_PULSE_SHORT(tap_get_pulse(tap, &pos_advance))) {
                }
            }
        } else if (type == PILOT_TYPE_TT) {
            res = tap_tt_read_header(tap);
            if (res < 0) {
                fseek(tap->fd, fpos, SEEK_SET);
                tap_tt_skip_pilot(tap);
            }
        } else {
            res = -2;
        }

        if (res == 0) {
            if (tap->tap_file_record->type == 5) {
                /* found end-of-tape marker */
                return -1;
            }

            /* success.  Rewind to start of header and return. */
            fseek(tap->fd, fpos, SEEK_SET);
            tap->current_file_seek_position = fpos;
            return type;
        }
    }
}


static int tap_read_file(tap_t *tap)
{
    int ret;
    long fpos;

#if TAP_DEBUG > 0
    log_debug("\nTAP_READ_FILE(START)\n");
#endif

    /* store current position in TAP file */
    fpos = ftell(tap->fd);

    /* clear old file data */
    tap->current_file_size = 0;
    lib_free(tap->current_file_data);
    tap->current_file_data = NULL;

    ret = tap_determine_pilot_type(tap);
    if (ret < 0) {
    } else if (ret == PILOT_TYPE_CBM) {
        ret = tap_cbm_read_file(tap);
    } else if (ret == PILOT_TYPE_TT) {
        ret = tap_tt_read_file(tap);
    } else {
        ret = -2;
    }

    if (ret < 0) {
        /* we failed to read the file.  Set size=1 and data=NULL to
           permanently indicate error condition */
        tap->current_file_size = 1;
        lib_free(tap->current_file_data);
        tap->current_file_data = NULL;
    }

    /* go back to previous position in TAP file */
    fseek(tap->fd, fpos, SEEK_SET);

#if TAP_DEBUG > 0
    log_debug("\nTAP_READ_FILE(END%i)\n", ret);
#endif


    return ret;
}


static int tap_skip_file(tap_t *tap)
{
    int ret;

#if TAP_DEBUG > 0
    log_debug("\nTAP_SKIP_FILE(START)\n");
#endif

    /* clear old file data */
    tap->current_file_size = 0;
    lib_free(tap->current_file_data);
    tap->current_file_data = NULL;

    ret = tap_determine_pilot_type(tap);
    if (ret < 0) {
        ret = -1;
    } else if (ret == PILOT_TYPE_CBM) {
        ret = tap_cbm_skip_file(tap);
    } else if (ret == PILOT_TYPE_TT) {
        ret = tap_tt_skip_file(tap);
    } else {
        ret = -2;
    }

#if TAP_DEBUG > 0
    log_debug("\nTAP_SKIP_FILE(END%i)\n", ret);
#endif

    return ret;
}

/* ------------------------------------------------------------------------- */

tape_file_record_t *tap_get_current_file_record(tap_t *tap)
{
    return tap->tap_file_record;
}

int tap_seek_start(tap_t *tap)
{
    /* clear old file data */
    tap->current_file_size = 0;
    lib_free(tap->current_file_data);
    tap->current_file_data = NULL;

    tap->current_file_number = -1;
    tap->current_file_seek_position = 0;
    fseek(tap->fd, tap->offset, SEEK_SET);
    return 0;
}

int tap_seek_to_file(tap_t *tap, unsigned int file_number)
{
    tap_seek_start(tap);
    while ((int) file_number > tap->current_file_number) {
        if (tap_seek_to_next_file(tap, 0) < 0) {
            return -1;
        }
    }

    return 0;
}

int tap_seek_to_next_file(tap_t *tap, unsigned int allow_rewind)
{
    if (tap == NULL) {
        return -1;
    }

    /* clear old file content buffer */
    tap->current_file_size = 0;
    lib_free(tap->current_file_data);
    tap->current_file_data = NULL;

    /* skip over current and find NEXT pilot
       (only if not at beginning of tape) */
    if (tap->current_file_number >= 0) {
        tap_skip_file(tap);
    }

    if (tap_find_header(tap) < 0) {
        if (allow_rewind) {
            tap_seek_start(tap);
            if (tap_find_header(tap) < 0) {
                return -1;
            }
        } else {
            return -1;
        }
    }

    tap->current_file_number++;
    return 0;
}

int tap_read(tap_t *tap, BYTE *buf, size_t size)
{
    if (tap->current_file_data == NULL) {
        /* no file data yet */
        if (tap->current_file_size > 0) {
            return -1; /* data==NULL and size>0 indicates read error */
        } else {
            /* if at beginning of TAP file, seek to first file */
            if (tap->current_file_number < 0) {
                if (tap_seek_to_next_file(tap, 0) < 0) {
                    return -1;
                }
            }

            if (tap_read_file(tap) < 0) {
                return -1; /* reading the file failed */
            } else {
                tap->current_file_data_pos = 0;
            }
        }
    }

    if (tap->current_file_data_pos < tap->current_file_size) {
        if (size > tap->current_file_size - tap->current_file_data_pos) {
            size = tap->current_file_size - tap->current_file_data_pos;
        }

        memcpy(buf, tap->current_file_data + tap->current_file_data_pos, size);
        tap->current_file_data_pos += size;

        return (int) size;
    }

    return 0;
}


void tap_get_header(tap_t *tap, BYTE *name)
{
    memcpy(name, tap->name, 12);
}


void tap_init(const tape_init_t *init)
{
    tap_pulse_short_min = init->pulse_short_min / 8;
    tap_pulse_short_max = init->pulse_short_max / 8;
    tap_pulse_middle_min = init->pulse_middle_min / 8;
    tap_pulse_middle_max = init->pulse_middle_max / 8;
    tap_pulse_long_min = init->pulse_long_min / 8;
    tap_pulse_long_max = init->pulse_long_max / 8;
}
