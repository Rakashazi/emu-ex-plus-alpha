/*
 * c1541.c - Stand-alone disk image maintenance program.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Jouko Valta <jopi@zombie.oulu.fi>
 *  Gerhard Wesp <gwesp@cosy.sbg.ac.at>
 *  Daniel Sladic <sladic@eecg.toronto.edu>
 *  Ricardo Ferreira <storm@esoterica.pt>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * Patches by
 *  Olaf Seibert <rhialto@mbfys.kun.nl>
 *  Dirk Schnorpfeil <D.Schnorpfeil@web.de> (GEOS stuff)
 *
 * Zipcode implementation based on `zip2disk' by
 *  Paul David Doherty <h0142kdd@rz.hu-berlin.de>
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
#include "diskimage.h"
#include "diskimage/fsimage.h"
#include "diskcontents.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "cbmdos.h"
#include "cbmimage.h"
#include "charset.h"
#include "cmdline.h"
#include "diskimage.h"
#include "fileio.h"
#include "gcr.h"
#include "info.h"
#include "imagecontents.h"
#include "ioutil.h"
#include "lib.h"
#include "log.h"
#include "network.h"
#include "serial.h"
#include "snapshot.h"
#include "tape.h"
#include "types.h"
#include "util.h"
#include "vdrive-bam.h"
#include "vdrive-command.h"
#include "vdrive-dir.h"
#include "vdrive-iec.h"
#include "vdrive.h"
#include "vice-event.h"
#include "zipcode.h"
#include "p64.h"

/* #define DEBUG_DRIVE */

#define MAXARG          256
#define MAXDRIVE        1

#define C1541_VERSION_MAJOR     4
#define C1541_VERSION_MINOR     0

const char machine_name[] = "C1541";

/* Global clock counter.  */
CLOCK clk = 0L;

static vdrive_t *drives[4] = { NULL, NULL, NULL, NULL };
static unsigned int p00save[4] = { 0, 0, 0, 0 };

static int drive_number = 0;

static int interactive_mode = 0;

/* Local functions.  */
static int attach_cmd(int nargs, char **args);
static int block_cmd(int nargs, char **args);
static int check_drive(int dev, int mode);
static int copy_cmd(int nargs, char **args);
static int delete_cmd(int nargs, char **args);
static int extract_cmd(int nargs, char **args);
static int format_cmd(int nargs, char **args);
static int help_cmd(int nargs, char **args);
static int info_cmd(int nargs, char **args);
static int list_cmd(int nargs, char **args);
static int name_cmd(int nargs, char **args);
static int p00save_cmd(int nargs, char **args);
static int quit_cmd(int nargs, char **args);
static int raw_cmd(int nargs, char **args); /* @ */
static int read_cmd(int nargs, char **args);
static int rename_cmd(int nargs, char **args);
static int show_cmd(int nargs, char **args);
static int tape_cmd(int nargs, char **args);
static int unit_cmd(int nargs, char **args);
static int unlynx_cmd(int nargs, char **args);
static int validate_cmd(int nargs, char **args);
static int verbose_cmd(int nargs, char **args);
static int write_cmd(int nargs, char **args);
static int zcreate_cmd(int nargs, char **args);

static int open_image(int dev, char *name, int create, int disktype);

int internal_read_geos_file(int unit, FILE* outf, char* src_name_ascii);
static int read_geos_cmd(int nargs, char **args);
static int fix_ts(int unit, unsigned int trk, unsigned int sec,
                  unsigned int next_trk, unsigned int next_sec,
                  unsigned int blk_offset);
static int internal_write_geos_file(int unit, FILE* f);
static int write_geos_cmd(int nargs, char **args);
static int extract_geos_cmd(int nargs, char **args);

int rom1540_loaded = 0;
int rom1541_loaded = 0;
int rom1541ii_loaded = 0;
int rom1571_loaded = 0;
int rom1581_loaded = 0;
int rom2000_loaded = 0;
int rom4000_loaded = 0;
int rom2031_loaded = 0;
int rom1001_loaded = 0;
int rom2040_loaded = 0;

BYTE *drive_rom1540;
BYTE *drive_rom1541;
BYTE *drive_rom1541ii;
BYTE *drive_rom1571;
BYTE *drive_rom1581;
BYTE *drive_rom2000;
BYTE *drive_rom4000;
BYTE *drive_rom2031;
BYTE *drive_rom1001;
BYTE *drive_rom2040;

/* ------------------------------------------------------------------------- */

/* dummy functions */
int cmdline_register_options(const cmdline_option_t *c)
{
    return 0;
}

int network_connected(void)
{
    return 0;
}

int network_get_mode(void)
{
    return NETWORK_IDLE;
}

void network_event_record(unsigned int type, void *data, unsigned int size)
{
}

void event_record_in_list(event_list_state_t *list, unsigned int type,
                          void *data, unsigned int size)
{
}

void ui_error(const char *format, ...)
{
}

/* ------------------------------------------------------------------------- */

struct command {
    const char *name;
    const char *syntax;
    const char *description;
    unsigned int min_args, max_args;
    int (*func)(int nargs, char **args);
};
typedef struct command command_t;

const command_t command_list[] = {
    { "@",
      "@ [<command>]",
      "Execute specified CBM DOS command and print the current status of the\n"
      "drive.  If no <command> is specified, just print the status.",
      0, 1, raw_cmd },
    { "?",
      "? [<command>]",
      "Explain specified command.  If no command is specified, list available\n"      "ones.",
      0, 1, help_cmd },
    { "attach",
      "attach <diskimage> [<unit>]",
      "Attach <diskimage> to <unit> (default unit is 8).",
      1, 2,
      attach_cmd },
    { "block",
      "block <track> <sector> <disp> [<drive>]",
      "Show specified disk block in hex form.",
      3, 4, block_cmd },
    { "copy",
      "copy <source1> [<source2> ... <sourceN>] <destination>",
      "Copy `source1' ... `sourceN' into destination.  If N > 1, `destination'\n"
      "must be a simple drive specifier (@n:).",
      2, MAXARG, copy_cmd },
    { "delete",
      "delete <file1> [<file2> ... <fileN>]",
      "Delete the specified files.",
      1, MAXARG,
      delete_cmd },
    { "dir",
      "dir [<pattern>]",
      "List files matching <pattern> (default is all files).",
      0, 1,
      list_cmd },
    { "exit",
      "exit",
      "Exit (same as `quit').",
      0, 0, quit_cmd },
    { "extract",
      "extract [<unit>]",
      "Extract all the files to the file system.",
      0, 1, extract_cmd },
    { "format",
      "format <diskname,id> [<type> <imagename>] [<unit>]",
      "If <unit> is specified, format the disk in unit <unit>.\n"
      "If <type> and <imagename> are specified, create a new image named\n"
      "<imagename>, attach it to unit 8 and format it.  <type> is a disk image\n"
      "type, and must be either `x64', `d64' (both VC1541/2031), `g64' (VC1541/2031,\n"
      "but in GCR coding, `d67' (2040 DOS1), `d71' (VC1571), `d81' (VC1581), \n"
      "`d80' (CBM8050) or `d82' (CBM8250). Otherwise, format the disk in \n"
      "the current unit, if any.",
      1, 4,
      format_cmd },
    { "geosread",
      "geosread <source> [<destination>]",
      "Read GEOS <source> from the disk image and copy it as a Convert file into \n"
      "<destination> in the file system.  If <destination> is not specified, copy \n"
      "it into a file with the same name as <source>.",
      1, 2, read_geos_cmd },
    { "geoswrite",
      "geoswrite <source>",
      "Write GOES Convert file <source> from the file system on a disk image.",
      1, 1, write_geos_cmd },
    { "geosextract",
      "geosextract <source>",
      "Extract all the files to the file system and GEOS Convert them.",
      0, 1, extract_geos_cmd },
    { "help",
      "help [<command>]",
      "Explain specified command.  If no command is specified, list available\n"      "ones.",
      0, 1, help_cmd },
    { "info",
      "info [<unit>]",
      "Display information about unit <unit> (if unspecified, use the current\n"      "one).",
      0, 1, info_cmd },
    { "list",
      "list [<pattern>]",
      "List files matching <pattern> (default is all files).",
      0, 1,
      list_cmd },
    { "name",
      "name <diskname>[,<id>] <unit>",
      "Change image name.",
      1, 2, name_cmd },
    { "p00save",
      "p00save <enable> [<unit>]",
      "Save P00 files to the file system.",
      1, 2, p00save_cmd },
    { "quit",
      "quit",
      "Exit (same as `exit').",
      0, 0, quit_cmd },
    { "read",
      "read <source> [<destination>]",
      "Read <source> from the disk image and copy it into <destination> in\n"
      "the file system.  If <destination> is not specified, copy it into a\n"
      "file with the same name as <source>.",
      1, 2, read_cmd },
    { "rename",
      "rename <oldname> <newname>",
      "Rename <oldname> into <newname>.  The files must be on the same drive.",
      2, 2, rename_cmd },
    { "show",
      "show [copying | warranty]",
      "Show conditions for redistributing copies of C1541 (`copying') or the\n"
      "various kinds of warranty you do not have with C1541 (`warranty').",
      1, 1, show_cmd },
    { "tape",
      "tape <t64name> [<file1> ... <fileN>]",
      "Extract files from a T64 image into the current drive.",
      1, MAXARG, tape_cmd },
    { "unit",
      "unit <number>",
      "Make unit <number> the current unit.",
      1, 1, unit_cmd },
    { "unlynx",
      "unlynx <lynxname> [<unit>]",
      "Extract the specified Lynx image file into the specified unit (default\n"
      "is the current unit).",
      1, 2, unlynx_cmd },
    { "validate",
      "validate [<unit>]",
      "Validate the disk in unit <unit>.  If <unit> is not specified, validate\n"
      "the disk in the current unit.",
      0, 1, validate_cmd },
    { "verbose",
      "verbose",
      "Enable verbose output.",
      0, 0, verbose_cmd },
    { "write",
      "write <source> [<destination>]",
      "Write <source> from the file system into <destination> on a disk image.", 1, 2, write_cmd },
    { "zcreate",
      "zcreate <d64name> <zipname> [<label,id>]",
      "Create a D64 disk image out of a set of four Zipcoded files named\n"
      "`1!<zipname>', `2!<zipname>', `3!<zipname>' and `4!<zipname>'.",
      2, 3, zcreate_cmd },
    { NULL, NULL, NULL, 0, 0, NULL }
};

/* ------------------------------------------------------------------------- */

#ifndef HAVE_READLINE

static char *read_line(const char *prompt)
{
    static char line[1024];

    line[sizeof(line) - 1] = 0; /* Make sure there is a 0 at the end of the string */

    fputs(prompt, stdout);
    fflush(stdout);
    return fgets(line, sizeof(line) - 1, stdin);
}

#else

# include "editline.h"

static char *read_line(const char *prompt)
{
    static char *line;

    free(line);
    line = readline(prompt);
    if (line != 0 && *line != 0) {
        add_history(line);
    }
    return line;
}

#endif

static int split_args(const char *line, int *nargs, char **args)
{
    const char *s;
    char *d;
    char tmp[256];
    int begin_of_arg, in_quote;

    *nargs = 0;

    in_quote = 0;
    d = tmp;
    begin_of_arg = 1;

    for (s = line;; s++) {
        switch (*s) {
            case '"':
                begin_of_arg = 0;
                in_quote = !in_quote;
                continue;
            case '\\':
                begin_of_arg = 0;
                *(d++) = *(++s);
                continue;
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case 0:
                if (*s == 0 && in_quote) {
                    fprintf(stderr, "Unbalanced quotes.\n");
                    return -1;
                }
                if (!in_quote && !begin_of_arg) {
                    if (*nargs == MAXARG) {
                        fprintf(stderr, "Too many arguments.\n");
                        return -1;
                    } else {
                        size_t len;

                        len = d - tmp;
                        if (args[*nargs] != NULL) {
                            args[*nargs] = lib_realloc(args[*nargs], len + 1);
                        } else {
                            args[*nargs] = lib_malloc(len + 1);
                        }
                        memcpy(args[*nargs], tmp, len);
                        args[*nargs][len] = 0;
                        begin_of_arg = 1;
                        (*nargs)++;
                        d = tmp;
                    }
                }
                if (*s == 0) {
                    return 0;
                }
                if (!(*s == ' ' && in_quote)) {
                    break;
                }
            default:
                begin_of_arg = 0;
                *(d++) = *s;
        }
    }

    return 0;
}

static int arg_to_int(const char *arg, int *return_value)
{
    char *tailptr;
    int counter = 0;

    *return_value = (int)strtol(arg, &tailptr, 10);

    if (ioutil_errno(IOUTIL_ERRNO_ERANGE)) {
        return -1;
    }

    /* Only whitespace is allowed after the last valid character.  */
    if (!util_check_null_string(tailptr)) {
        while (isspace((int)tailptr[counter])) {
            counter++;
        }
        tailptr += counter;
        if (*tailptr != 0) {
            return -1;
        }
    }
    return 0;
}

static void print_error_message(int errval)
{
    if (errval < 0) {
        switch (errval) {
            case FD_OK:
                break;
            case FD_NOTREADY:
                fprintf(stderr, "Drive not ready.\n");
                break;
            case FD_CHANGED:
                fprintf(stderr, "Image file has changed on disk.\n");
                break;
            case FD_NOTRD:
                fprintf(stderr, "Cannot read file.\n");
                break;
            case FD_NOTWRT:
                fprintf(stderr, "Cannot write file.\n");
                break;
            case FD_WRTERR:
                fprintf(stderr, "Floppy write failed.\n");
                break;
            case FD_RDERR:
                fprintf(stderr, "Floppy read failed.\n");
                break;
            case FD_INCOMP:
                fprintf(stderr, "Incompatible DOS version.\n");
                break;
            case FD_BADIMAGE:
                fprintf(stderr, "Invalid image.\n");    /* Disk or tape */
                break;
            case FD_BADNAME:
                fprintf(stderr, "Invalid filename.\n");
                break;
            case FD_BADVAL:
                fprintf(stderr, "Illegal value.\n");
                break;
            case FD_BADDEV:
                fprintf(stderr, "Illegal device number.\n");
                break;
            case FD_BAD_TS:
                fprintf(stderr, "Inaccessible Track or Sector.\n");
                break;
            default:
                fprintf(stderr, "Unknown error.\n");
        }
    }
}

#define LOOKUP_NOTFOUND         -1
#define LOOKUP_AMBIGUOUS        -2
#define LOOKUP_SUCCESSFUL(n)    ((n) >= 0)
static int lookup_command(const char *cmd)
{
    size_t cmd_len;
    int match;
    int i;

    match = LOOKUP_NOTFOUND;
    cmd_len = strlen(cmd);

    for (i = 0; command_list[i].name != NULL; i++) {
        size_t len;

        len = strlen(command_list[i].name);
        if (len < cmd_len) {
            continue;
        }

        if (memcmp(command_list[i].name, cmd, cmd_len) == 0) {
            if (match != -1) {
                return LOOKUP_AMBIGUOUS;
            }
            match = i;
            if (len == cmd_len) {
                break;          /* Exact match.  */
            }
        }
    }

    return match;
}

static int lookup_and_execute_command(int nargs, char **args)
{
    int match;

    match = lookup_command(args[0]);

    if (LOOKUP_SUCCESSFUL(match)) {
        const command_t *cp;

        cp = &command_list[match];
        if (nargs - 1 < (int)(cp->min_args)
            || nargs - 1 > (int)(cp->max_args)) {
            fprintf(stderr, "Wrong number of arguments.\n");
            fprintf(stderr, "Syntax: %s\n", cp->syntax);
            return -1;
        } else {
            int retval;

            retval = command_list[match].func(nargs, args);
            print_error_message(retval);
            if (retval == FD_OK) {
                return 0;
            } else {
                return -1;
            }
        }
    } else {
        if (match == LOOKUP_AMBIGUOUS) {
            fprintf(stderr,
                    "Command `%s' is ambiguous.  Try `help'.\n", args[0]);
        } else {
            fprintf(stderr,
                    "Command `%s' unrecognized.  Try `help'.\n", args[0]);
        }
        return -1;
    }
}

static char *extract_unit_from_file_name(const char *name,
                                         unsigned int *unit_return)
{
    if (name[0] == '@' && name[2] == ':'
        && (name[1] == '8' || name[1] == '9')) {
        *unit_return = (unsigned int)(name[1] - '8');
        return (char *)name + 3;
    } else {
        return NULL;
    }
}

static int is_valid_cbm_file_name(const char *name)
{
    /* Notice that ':' is the same on PETSCII and ASCII.  */
    return strchr(name, ':') == NULL;
}

/* ------------------------------------------------------------------------- */

static int open_disk_image(vdrive_t *vdrive, const char *name,
                           unsigned int unit)
{
    disk_image_t *image;

    image = disk_image_create();

    if (archdep_file_is_blockdev(name)) {
        image->device = DISK_IMAGE_DEVICE_RAW;
        serial_device_type_set(SERIAL_DEVICE_RAW, unit);
        serial_realdevice_disable();
    } else {
        if (archdep_file_is_chardev(name)) {
            image->device = DISK_IMAGE_DEVICE_REAL;
            serial_device_type_set(SERIAL_DEVICE_REAL, unit);
            serial_realdevice_enable();
        } else {
            image->device = DISK_IMAGE_DEVICE_FS;
            serial_device_type_set(SERIAL_DEVICE_FS, unit);
            serial_realdevice_disable();
        }
    }

    disk_image_media_create(image);

    image->gcr = NULL;
    image->p64 = lib_calloc(1, sizeof(TP64Image));
    P64ImageCreate((PP64Image)image->p64);
    image->read_only = 0;

    disk_image_name_set(image, lib_stralloc(name));

    if (disk_image_open(image) < 0) {
        P64ImageDestroy((PP64Image)image->p64);
        lib_free(image->p64);
        disk_image_media_destroy(image);
        disk_image_destroy(image);
        fprintf(stderr, "Cannot open file `%s'.\n", name);
        return -1;
    }

    vdrive_device_setup(vdrive, unit);
    vdrive->image = image;
    vdrive_attach_image(image, unit, vdrive);
    return 0;
}

static void close_disk_image(vdrive_t *vdrive, int unit)
{
    disk_image_t *image;

    image = vdrive->image;

    if (image != NULL) {
        vdrive_detach_image(image, unit, vdrive);
        P64ImageDestroy((PP64Image)image->p64);
        lib_free(image->p64);
        if (image->device == DISK_IMAGE_DEVICE_REAL) {
            serial_realdevice_disable();
        }
        disk_image_close(image);
        disk_image_media_destroy(image);
        disk_image_destroy(image);
        vdrive->image = NULL;
    }
}

/* Open image or create a new one.  If the file exists, it must have valid
   header.  */
static int open_image(int dev, char *name, int create, int disktype)
{
    if (dev < 0 || dev > MAXDRIVE) {
        return -1;
    }

    if (create) {
        if (cbmimage_create_image(name, disktype) < 0) {
            printf("Cannot create disk image.\n");
            return -1;
        }
    }

    if (open_disk_image(drives[dev], name, dev + 8) < 0) {
        printf("Cannot open disk image.\n");
        return -1;
    }
    return 0;
}

static int check_drive(int dev, int flags)
{
    vdrive_t *vdrive;

    dev &= 7;
    if (dev < 0 || dev > 3) {
        return FD_BADDEV;
    }

    vdrive = drives[dev & 3];

    if (flags != CHK_NUM && (vdrive == NULL || vdrive->image == NULL)) {
        return FD_NOTREADY;
    }

    return FD_OK;
}

/* ------------------------------------------------------------------------- */

/* Here are the commands.  */

/* Note: The double ASCII/PETSCII copies of file names we keep in some
   functions are needed because we want to print the names of the files being
   copied in ASCII and we don't trust `charset_petconvstring()' to be
   reliable to get the original value back when we convert ASCII -> PETSCII
   and then PETSCII -> ASCII again.  */

static int attach_cmd(int nargs, char **args)
{
    int dev = 0;

    switch (nargs) {
        case 2:
            /* attach <image> */
            dev = drive_number;
            break;
        case 3:
            /* attach <image> <unit> */
            if (arg_to_int(args[2], &dev) < 0) {
                return FD_BADDEV;
            }
            break;
    }

    if (check_drive(dev, CHK_NUM) < 0) {
        return FD_BADDEV;
    }

    open_disk_image(drives[dev & 3], args[1], (dev & 3) + 8);
    return FD_OK;
}

static int block_cmd(int nargs, char **args)
{
    int drive, disp;
    int track, sector;
    vdrive_t *vdrive;
    BYTE *buf, str[20], sector_data[256];
    int cnt;

    /* block <track> <sector> <disp> [<drive>] show disk blocks in hex form */
    if (arg_to_int(args[1], &track) < 0 || arg_to_int(args[2], &sector) < 0) {
        return FD_BAD_TS;
    }
    if (arg_to_int(args[3], &disp) < 0) {
        return FD_BADVAL;
    }

    if (nargs == 5) {
        if (arg_to_int(args[4], &drive) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(drive, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        drive -= 8;
    } else {
        drive = drive_number;
    }

    if (check_drive(drive, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    vdrive = drives[drive & 3];

    if (disk_image_check_sector(vdrive->image, track, sector) < 0) {
        return FD_BAD_TS;
    }

    /* Read one block */
    if (vdrive_read_sector(vdrive, sector_data, track, sector)
        != 0) {
        fprintf(stderr, "Cannot read track %i sector %i.", track, sector);
        return FD_RDERR;
    }

    buf = sector_data;

    /* Show block */

    printf("<%2d: %2d %2d>\n", drive, track, sector);
    str[16] = 0;
    for (; disp < 256; ) {
        printf("> %02X ", disp & 255);
        for (cnt = 0; cnt < 16; cnt++, disp++) {
            printf(" %02X", buf[disp & 255]);
            str[cnt] = (buf[disp & 255] < ' ' ?
                        '.' : charset_p_toascii(buf[disp & 255], 0));
        }
        printf("  ;%s\n", str);
    }
    return FD_OK;
}

static int copy_cmd(int nargs, char **args)
{
    char *p;
    char *dest_name_ascii, *dest_name_petscii;
    unsigned int dest_unit, src_unit;
    int i;

    p = extract_unit_from_file_name(args[nargs - 1], &dest_unit);
    if (p == NULL) {
        if (nargs > 3) {
            fprintf(stderr,
                    "The destination must be a drive if multiple sources are specified.\n");
            return FD_OK;           /* FIXME */
        }
        dest_name_ascii = lib_stralloc(args[nargs - 1]);
        dest_name_petscii = lib_stralloc(dest_name_ascii);
        charset_petconvstring((BYTE *)dest_name_petscii, 0);
        dest_unit = drive_number;
    } else {
        if (*p != 0) {
            if (nargs > 3) {
                fprintf(stderr,
                        "The destination must be a drive if multiple sources are specified.\n");
                return FD_OK;           /* FIXME */
            }
            dest_name_ascii = lib_stralloc(p);
            dest_name_petscii = lib_stralloc(dest_name_ascii);
            charset_petconvstring((BYTE *)dest_name_petscii, 0);
        } else {
            dest_name_ascii = dest_name_petscii = NULL;
        }
    }

    if (dest_name_ascii != NULL && !is_valid_cbm_file_name(dest_name_ascii)) {
        fprintf(stderr,
                "`%s' is not a valid CBM DOS file name.\n", dest_name_ascii);
        return FD_OK;               /* FIXME */
    }

    if (check_drive(dest_unit, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    for (i = 1; i < nargs - 1; i++) {
        char *src_name_ascii, *src_name_petscii;

        p = extract_unit_from_file_name(args[i], &src_unit);

        if (p == NULL) {
            src_name_ascii = lib_stralloc(args[i]);
            src_unit = drive_number;
        } else {
            if (check_drive(src_unit, CHK_RDY) < 0) {
                return FD_NOTREADY;
            }
            src_name_ascii = lib_stralloc(p);
        }

        if (!is_valid_cbm_file_name(src_name_ascii)) {
            fprintf(stderr, "`%s' is not a valid CBM DOS file name: ignored.\n", src_name_ascii);
            lib_free(src_name_ascii);
            continue;
        }

        src_name_petscii = lib_stralloc(src_name_ascii);
        charset_petconvstring((BYTE *)src_name_petscii, 0);

        if (vdrive_iec_open(drives[src_unit], (BYTE *)src_name_petscii,
                            (int)strlen(src_name_petscii), 0, NULL)) {
            fprintf(stderr, "Cannot read `%s'.\n", src_name_ascii);
            if (dest_name_ascii != NULL) {
                lib_free(dest_name_ascii);
                lib_free(dest_name_petscii);
            }

            lib_free(src_name_ascii);
            lib_free(src_name_petscii);
            return FD_RDERR;
        }

        if (dest_name_ascii != NULL) {
            if (vdrive_iec_open(drives[dest_unit], (BYTE *)dest_name_petscii,
                                (int)strlen(dest_name_petscii), 1, NULL)) {
                fprintf(stderr, "Cannot write `%s'.\n", dest_name_petscii);
                vdrive_iec_close(drives[src_unit], 0);
                lib_free(dest_name_ascii);
                lib_free(dest_name_petscii);
                lib_free(src_name_ascii);
                lib_free(src_name_petscii);
                return FD_OK;
            }
        } else {
            if (vdrive_iec_open(drives[dest_unit], (BYTE *)src_name_petscii,
                                (int)strlen(src_name_petscii), 1, NULL)) {
                fprintf(stderr, "Cannot write `%s'.\n", src_name_petscii);
                vdrive_iec_close(drives[src_unit], 0);
                lib_free(src_name_ascii);
                lib_free(src_name_petscii);
                return FD_OK;
            }
        }

        printf("Copying `%s'...\n", args[i]); /* FIXME */

        {
            BYTE c;
            int status = 0;

            do {
                status = vdrive_iec_read(drives[src_unit], ((BYTE *)&c), 0);
                if (vdrive_iec_write(drives[dest_unit], ((BYTE)(c)), 1)) {
                    fprintf(stderr, "No space on image ?\n");
                    break;
                }
            } while (status == SERIAL_OK);
        }

        vdrive_iec_close(drives[src_unit], 0);
        vdrive_iec_close(drives[dest_unit], 1);

        lib_free(src_name_ascii);
        lib_free(src_name_petscii);
    }

    lib_free(dest_name_ascii);
    lib_free(dest_name_petscii);
    return FD_OK;
}

static int delete_cmd(int nargs, char **args)
{
    int i = 1, status;

    if (check_drive(drive_number, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    for (i = 1; i < nargs; i++) {
        unsigned int dnr;
        char *p, *name;
        char *command;

        p = extract_unit_from_file_name(args[i], &dnr);

        if (p == NULL) {
            dnr = drive_number;
            name = args[i];
        } else {
            name = p;
        }

        if (!is_valid_cbm_file_name(name)) {
            fprintf(stderr,
                    "`%s' is not a valid CBM DOS file name: ignored.\n", name);
            continue;
        }

        command = util_concat("s:", name, NULL);
        charset_petconvstring((BYTE *)command, 0);

        printf("Deleting `%s' on unit %d.\n", name, dnr + 8);

        status = vdrive_command_execute(drives[dnr], (BYTE *)command,
                                        (unsigned int)strlen(command));

        lib_free(command);

        printf("ERRORCODE %i\n", status);
    }

    return FD_OK;
}

static void unix_filename(char *p)
{
    while (*p) {
        if (*p == '/') {
            *p = '_';
        }
        p++;
    }
}

/* Extract all files <gwesp@cosy.sbg.ac.at>.  */
/* FIXME: This does not work with non-standard file names.  */

static int extract_cmd_common(int nargs, char **args, int geos)
{
    int dnr = 0, track, sector;
    vdrive_t *floppy;
    BYTE *buf, *str;
    int err;
    int channel = 2;

    if (nargs == 2) {
        if (arg_to_int(args[1], &dnr) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(dnr, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        dnr -= 8;
    }

    err = check_drive(dnr, CHK_RDY);

    if (err < 0) {
        return err;
    }

    floppy = drives[dnr & 3];

    if (vdrive_iec_open(floppy, (const BYTE *)"#", 1, channel, NULL)) {
        fprintf(stderr, "Cannot open buffer #%d in unit %d.\n", channel,
                dnr + 8);
        return FD_RDERR;
    }

    track = floppy->Dir_Track;
    sector = floppy->Dir_Sector;

    while (1) {
        int i, res;

        str = (BYTE *)lib_msprintf("B-R:%d 0 %d %d", channel, track, sector);
        res = vdrive_command_execute(floppy, str, (unsigned int)strlen((char *)str));

        lib_free(str);

        if (res) {
            return FD_RDERR;
        }

        buf = floppy->buffers[channel].buffer;

        for (i = 0; i < 256; i += 32) {
            BYTE file_type = buf[i + SLOT_TYPE_OFFSET];

            if (((file_type & 7) == CBMDOS_FT_SEQ
                 || (file_type & 7) == CBMDOS_FT_PRG
                 || (file_type & 7) == CBMDOS_FT_USR)
                && (file_type & CBMDOS_FT_CLOSED)) {
                int len;
                BYTE *file_name = buf + i + SLOT_NAME_OFFSET;
                BYTE c, name[17], cbm_name[17];
                FILE *fd;
                int status = 0;

                memset(name, 0, 17);
                memset(cbm_name, 0, 17);
                for (len = 0; len < 16; len++) {
                    if (file_name[len] == 0xa0) {
                        break;
                    } else {
                        name[len] = file_name[len];
                        cbm_name[len] = file_name[len];
                    }
                }

                charset_petconvstring((BYTE *)name, 1);
                printf("%s\n", name);
                unix_filename((char *)name); /* For now, convert '/' to '_'. */
                if (vdrive_iec_open(floppy, cbm_name, len, 0, NULL)) {
                    fprintf(stderr,
                            "Cannot open `%s' on unit %d.\n", name, dnr + 8);
                    continue;
                }
                fd = fopen((char *)name, MODE_WRITE);
                if (fd == NULL) {
                    fprintf(stderr, "Cannot create file `%s': %s.",
                            name, strerror(errno));
                    vdrive_iec_close(floppy, 0);
                    continue;
                }
                if (geos) {
                    status = internal_read_geos_file(dnr, fd, (char *)name);
                } else {
                    do {
                        status = vdrive_iec_read(floppy, &c, 0);
                        fputc(c, fd);
                    } while (status == SERIAL_OK);
                }

                vdrive_iec_close(floppy, 0);

                if (fclose(fd)) {
                    return FD_RDERR;
                }
            }
        }
        if (buf[0] && buf[1]) {
            track = buf[0];
            sector = buf[1];
        } else {
            break;
        }
    }
    vdrive_iec_close(floppy, channel);
    return FD_OK;
}

static int extract_cmd(int nargs, char **args)
{
    return extract_cmd_common(nargs, args, 0);
}
static int extract_geos_cmd(int nargs, char **args)
{
    return extract_cmd_common(nargs, args, 1);
}

static int format_cmd(int nargs, char **args)
{
    char *command;
    int disk_type;
    int unit = -1;

    switch (nargs) {
        case 2:
            /* format <diskname,id> */
            unit = drive_number;
            break;
        case 3:
            /* format <diskname,id> <unit> */
            /* Format the disk image in unit <unit>.  */
            if (arg_to_int(args[2], &unit) >= 0
                && check_drive(unit, CHK_NUM) >= 0) {
                /* It's a valid unit number.  */
                unit -= 8;
            } else {
                return FD_BADDEV;
            }
            break;
        case 4:
        case 5:
            /* format <diskname,id> <type> <imagename> */
            /* Create a new image.  */
            /* FIXME: I want a unit number here too.  */
            *args[2] = util_tolower(*args[2]);
            if (strcmp(args[2], "d64") == 0) {
                disk_type = DISK_IMAGE_TYPE_D64;
            } else if (strcmp(args[2], "d67") == 0) {
                disk_type = DISK_IMAGE_TYPE_D67;
            } else if (strcmp(args[2], "d71") == 0) {
                disk_type = DISK_IMAGE_TYPE_D71;
            } else if (strcmp(args[2], "d81") == 0) {
                disk_type = DISK_IMAGE_TYPE_D81;
            } else if (strcmp(args[2], "d80") == 0) {
                disk_type = DISK_IMAGE_TYPE_D80;
            } else if (strcmp(args[2], "d82") == 0) {
                disk_type = DISK_IMAGE_TYPE_D82;
            } else if (strcmp(args[2], "g64") == 0) {
                disk_type = DISK_IMAGE_TYPE_G64;
            } else if (strcmp(args[2], "x64") == 0) {
                disk_type = DISK_IMAGE_TYPE_X64;
            } else if (strcmp(args[2], "d1m") == 0) {
                disk_type = DISK_IMAGE_TYPE_D1M;
            } else if (strcmp(args[2], "d2m") == 0) {
                disk_type = DISK_IMAGE_TYPE_D2M;
            } else if (strcmp(args[2], "d4m") == 0) {
                disk_type = DISK_IMAGE_TYPE_D4M;
            } else {
                return FD_BADVAL;
            }
            if (nargs > 4) {
                arg_to_int(args[4], &unit);
                unit -= 8;
            } else {
                unit = 0;
            }
            if (open_image(unit, args[3], 1, disk_type) < 0) {
                return FD_BADIMAGE;
            }
            break;
        default:
            /* Shouldn't happen.  */
            return FD_BADVAL;
    }
    printf("Unit: %i\n", unit);
    if (!strchr(args[1], ',')) {
        fprintf(stderr, "There must be ID on the name.\n");
        return FD_OK;
    }

    if (check_drive(unit, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    command = util_concat("n:", args[1], NULL);
    charset_petconvstring((BYTE *)command, 0);

    printf("Formatting in unit %d...\n", unit + 8);
    vdrive_command_execute(drives[unit], (BYTE *)command, (unsigned int)strlen(command));

    lib_free(command);
    return FD_OK;
}

static int help_cmd(int nargs, char **args)
{
    if (nargs == 1) {
        int i;

        printf("Available commands are:\n");
        for (i = 0; command_list[i].name != NULL; i++) {
            printf("  %s\n", command_list[i].syntax);
        }
    } else {
        int match;

        match = lookup_command(args[1]);
        switch (match) {
            case LOOKUP_AMBIGUOUS:
                fprintf(stderr, "Command `%s' is ambiguous.\n", args[1]);
                break;
            case LOOKUP_NOTFOUND:
                fprintf(stderr, "Unknown command `%s'.\n", args[1]);
                break;
            default:
                if (LOOKUP_SUCCESSFUL(match)) {
                    printf("Syntax: %s\n%s\n",
                           command_list[match].syntax,
                           command_list[match].description);
                }
        }
    }

    return FD_OK;
}

static int info_cmd(int nargs, char **args)
{
    vdrive_t *vdrive;
    const char *format_name;
    int dnr;

    if (nargs == 2) {
        int unit;

        if (arg_to_int(args[1], &unit) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(unit, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        dnr = unit - 8;
    } else {
        dnr = drive_number;
    }

    if (check_drive(dnr, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    vdrive = drives[dnr];

    switch (vdrive->image_format) {
        case VDRIVE_IMAGE_FORMAT_1541:
            format_name = "1541";
            break;
        case VDRIVE_IMAGE_FORMAT_1571:
            format_name = "1571";
            break;
        case VDRIVE_IMAGE_FORMAT_1581:
            format_name = "1581";
            break;
        case VDRIVE_IMAGE_FORMAT_8050:
            format_name = "8050";
            break;
        case VDRIVE_IMAGE_FORMAT_8250:
            format_name = "8250";
            break;
        case VDRIVE_IMAGE_FORMAT_2040:
            format_name = "2040";
            break;
        case VDRIVE_IMAGE_FORMAT_4000:
            format_name = "4000";
            break;
        default:
            return FD_NOTREADY;
    }

    printf("Description: %s\n", "None.");
    printf("Disk Format: %s.\n", format_name);
/*printf("Sides\t   : %d.\n", hdr.sides);*/
    printf("Tracks\t   : %d.\n", vdrive->image->tracks);
    if (vdrive->image->device == DISK_IMAGE_DEVICE_FS) {
        printf(((vdrive->image->media.fsimage)->error_info.map)
               ? "Error Block present.\n" : "No Error Block.\n");
    }
    printf("Write protect: %s.\n", vdrive->image->read_only ? "On" : "Off");

    return FD_OK;
}

static int list_match_pattern(char *pat, char *str)
{
    int n;

    if (*str == '"') {
        str++;
    }

    if ((*str == 0) && (*pat != 0)) {
        return 0;
    }

    n = strlen(str);
    while (n) {
        n--;
        if (str[n] == '"') {
            str[n] = 0;
            break;
        }
    }

    while (*str) {
        if (*pat == '*') {
            return 1;
        } else if ((*pat != '?') && (*pat != *str)) {
            return 0;
        }
        str++; pat++;
    }
    if ((*pat != 0) && (*pat != '*')) {
        return 0;
    }
    return 1;
}

/*
    FIXME: diskcontents_read internally opens/closes the disk image, including
           a complete reset of internal vdrive variables. this makes things like
           changing sub partitions and sub directories inside images impossible
           from the c1541 shell.
*/
static int list_cmd(int nargs, char **args)
{
    char *pattern, *name;
    image_contents_t *listing;
    unsigned int dnr;
    vdrive_t *vdrive;

    if (nargs > 1) {
        /* list <pattern> */
        pattern = extract_unit_from_file_name(args[1], &dnr);
        if (pattern == NULL) {
            dnr = drive_number;
        } else if (*pattern == 0) {
            pattern = NULL;
        }
    } else {
        /* list */
        pattern = NULL;
        dnr = drive_number;
    }

    if (check_drive(dnr, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    vdrive = drives[dnr & 3];
    name = disk_image_name_get(vdrive->image);

    listing = diskcontents_read(name, dnr + 8);

    if (listing != NULL) {
        char *string = image_contents_to_string(listing, 1);
        image_contents_file_list_t *element = listing->file_list;

        printf("%s\n", string);
        lib_free(string);
        if (element == NULL) {
            printf("Empty image\n");
        } else {
            do {
                string = image_contents_filename_to_string(element, 1);
                if ((pattern == NULL) || list_match_pattern(pattern, string)) {
                    lib_free(string);
                    string = image_contents_file_to_string(element, 1);
                    printf("%s\n", string);
                }
                lib_free(string);
            } while ((element = element->next) != NULL);
        }
        if (listing->blocks_free >= 0) {
            printf("%d blocks free.\n", listing->blocks_free);
        }
    }

    return FD_OK;
}

static int name_cmd(int nargs, char **args)
{
    char *id;
    char *name;
    BYTE *dst;
    int i;
    int unit;
    vdrive_t *vdrive;

    if (nargs > 2) {
        if (arg_to_int(args[2], &unit) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(unit, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        unit -= 8;
    } else {
        unit = drive_number;
    }

    if (check_drive(unit, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    vdrive = drives[unit];
    vdrive_bam_read_bam(vdrive);
    name = args[1];
    charset_petconvstring((BYTE *)name, 0);
    id = strrchr(args[1], ',');
    if (id) {
        *id++ = '\0';
    }

    dst = &vdrive->bam[vdrive->bam_name];
    for (i = 0; i < 16; i++) {
        *dst++ = *name ? *name++ : 0xa0;
    }

    if (id) {
        dst = &vdrive->bam[vdrive->bam_id];
        for (i = 0; i < 5 && *id; i++) {
            *dst++ = *id++;
        }
    }

    vdrive_bam_write_bam(vdrive);
    return FD_OK;
}

static int quit_cmd(int nargs, char **args)
{
    int i;

    for (i = 0; i <= MAXDRIVE; i++) {
        close_disk_image(drives[i], i + 8);
    }

    exit(0);
    return 0;   /* OSF1 cc complains */
}

static int verbose_cmd(int nargs, char **args)
{
    return log_set_verbose(1);
}

static int read_cmd(int nargs, char **args)
{
    char *src_name_petscii, *src_name_ascii;
    char *dest_name_ascii;
    char *actual_name;
    char *p;
    unsigned int dnr;
    FILE *outf = NULL;
    fileio_info_t *finfo = NULL;
    unsigned int format = FILEIO_FORMAT_RAW;
    BYTE c;
    int status = 0;

    p = extract_unit_from_file_name(args[1], &dnr);
    if (p == NULL) {
        dnr = drive_number;
    }

    if (check_drive(dnr, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    if (p00save[dnr]) {
        format = FILEIO_FORMAT_P00;
    }

    if (p == NULL) {
        src_name_ascii = lib_stralloc(args[1]);
    } else {
        src_name_ascii = lib_stralloc(p);
    }

    if (!is_valid_cbm_file_name(src_name_ascii)) {
        fprintf(stderr, "`%s' is not a valid CBM DOS file name.\n",
                src_name_ascii);
        lib_free(src_name_ascii);
        return FD_OK;               /* FIXME */
    }

    src_name_petscii = lib_stralloc(src_name_ascii);
    charset_petconvstring((BYTE *)src_name_petscii, 0);

    if (vdrive_iec_open(drives[dnr], (BYTE *)src_name_petscii,
                        (int)strlen(src_name_petscii), 0, NULL)) {
        fprintf(stderr,
                "Cannot read `%s' on unit %d.\n", src_name_ascii, dnr + 8);
        lib_free(src_name_ascii);
        lib_free(src_name_petscii);
        return FD_BADNAME;
    }

    /* Get real filename from the disk file.  Slot must be defined by
       vdrive_iec_open().  */
    actual_name = lib_malloc(17);  /* FIXME: Should be a #define.  */
    memcpy(actual_name, drives[dnr]->buffers[0].slot + SLOT_NAME_OFFSET, 16);
    actual_name[16] = 0;

    if (nargs == 3) {
        if (strcmp(args[2], "-") == 0) {
            dest_name_ascii = NULL;      /* stdout */
        } else {
            char *open_petscii_name;

            dest_name_ascii = args[2];
            open_petscii_name = lib_stralloc(dest_name_ascii);
            charset_petconvstring((BYTE *)open_petscii_name, 0);
            finfo = fileio_open(open_petscii_name, NULL, format,
                                FILEIO_COMMAND_WRITE, FILEIO_TYPE_PRG);
            lib_free(open_petscii_name);
        }
    } else {
        size_t l;

        dest_name_ascii = actual_name;
        vdrive_dir_no_a0_pads((BYTE *)dest_name_ascii, 16);
        l = strlen(dest_name_ascii) - 1;
        while (dest_name_ascii[l] == ' ') {
            dest_name_ascii[l] = 0;
            l--;
        }

        finfo = fileio_open(dest_name_ascii, NULL, format,
                            FILEIO_COMMAND_WRITE, FILEIO_TYPE_PRG);
    }

    if (dest_name_ascii == NULL) {
        outf = stdout;
    } else {
        if (finfo == NULL) {
            fprintf(stderr, "Cannot create output file `%s': %s.\n",
                    dest_name_ascii, strerror(errno));
            vdrive_iec_close(drives[dnr], 0);
            lib_free(src_name_petscii);
            lib_free(src_name_ascii);
            lib_free(actual_name);
            return FD_NOTWRT;
        }
    }                           /* stdout */

    printf("Reading file `%s' from unit %d.\n", src_name_ascii, dnr + 8);

    do {
        status = vdrive_iec_read(drives[dnr], &c, 0);
        if (dest_name_ascii == NULL) {
            fputc(c, outf);
        } else {
            fileio_write(finfo, &c, 1);
        }
    } while (status == SERIAL_OK);

    if (dest_name_ascii != NULL) {
        fileio_close(finfo);
    }

    vdrive_iec_close(drives[dnr], 0);

    lib_free(src_name_petscii);
    lib_free(src_name_ascii);
    lib_free(actual_name);

    return FD_OK;
}

#define SLOT_GEOS_FILE_STRUC  23  /* Offset to geos file structure byte */
#define SLOT_GEOS_FILE_TYPE   24  /* Offset to geos file type
          */

/* Geos file structure sequential (no vlir) */
#define GEOS_FILE_STRUC_SEQ   0
/* Geos file structure VLIR (index block, max. 127 chains) */
#define GEOS_FILE_STRUC_VLIR  1

/* Author:      DiSc
 * Date:        2000-07-28
 *
 * companion to geos_read_cmd.
 * Expects an opened file in drives[unit]->buffers[0].slot
 */
int internal_read_geos_file(int unit, FILE* outf, char* src_name_ascii)
{
    int n;

    /* Get TS of info block */
    BYTE infoTrk = drives[unit]->buffers[0].slot[SLOT_SIDE_TRACK];
    BYTE infoSec = drives[unit]->buffers[0].slot[SLOT_SIDE_SECTOR];

    /* Get TS of first data block or vlir block
       (depends on the geos file type) */
    BYTE firstTrk = drives[unit]->buffers[0].slot[SLOT_FIRST_TRACK];
    BYTE firstSec = drives[unit]->buffers[0].slot[SLOT_FIRST_SECTOR];

    /* get geos file structure and geos file type */
    BYTE geosFileStruc = drives[unit]->buffers[0].slot[SLOT_GEOS_FILE_STRUC];
    /*BYTE geosFileType = drives[unit]->buffers[0].slot[SLOT_GEOS_FILE_TYPE];*/

    BYTE infoBlock[256];
    BYTE vlirBlock[256];
    BYTE block[256];
    BYTE vlirTrans[256];

    int aktTrk, aktSec, vlirIdx, NoOfBlocks, NoOfChains, BytesInLastSector;

    /* the first block in a cvt file is the directory entry padded with
       zeros */
    for (n = 2; n < 32; n++) {
        BYTE c = drives[unit]->buffers[0].slot[n];
        fputc(c, outf);
    }
    /* signature */
    fprintf(outf, "%s formatted GEOS file V1.0", (geosFileStruc == GEOS_FILE_STRUC_SEQ) ? "SEQ" : "PRG");
    /* pad with zeros */
    for (n = 0x3a; n < 0xfe; n++) {
        fputc(0, outf);
    }

    /* read info block */
    if (vdrive_read_sector(drives[unit], infoBlock, infoTrk, infoSec) != 0) {
        fprintf(stderr,
                "Cannot read input file info block `%s': %s.\n",
                src_name_ascii, strerror(errno));
        return FD_RDERR;
    }
    /* the next block is the info sector
     * write info block
     */
    for (n = 2; n < 256; n++) {
        fputc(infoBlock[n], outf);
    }

    /* read first data block or vlir block */
    if (vdrive_read_sector(drives[unit], vlirBlock, firstTrk, firstSec) != 0) {
        fprintf(stderr,
                "Cannot read input file data `%s': %s.\n",
                src_name_ascii, strerror(errno));
        return FD_RDERR;
    }

    if (geosFileStruc == GEOS_FILE_STRUC_SEQ) {
#ifdef DEBUG_DRIVE
        log_debug("DEBUG: GEOS_FILE_STRUC_SEQ (%d:%d)", infoTrk, infoSec);
#endif
        /* sequential file contained in cvt file
         * since vlir block is the first data block simply put it to
         * disk
         */

        for (n = 2; n < 256; n++) {
            fputc(vlirBlock[n], outf);
        }

        /* the rest is like standard cbm file TS-Chains.
           Put them to disk */

        aktTrk = vlirBlock[0];
        aktSec = vlirBlock[1];
        while (aktTrk != 0) {
            if (vdrive_read_sector(drives[unit], block, aktTrk, aktSec) != 0) {
                fprintf(stderr,
                        "Cannot read input file data block `%s': %s.\n",
                        src_name_ascii, strerror(errno));
                return FD_RDERR;
            }
            aktTrk = block[0];
            aktSec = block[1];
            BytesInLastSector = aktTrk != 0 ? 256 : aktSec + 1;
            for (n = 2; n < BytesInLastSector; n++) {
                fputc(block[n], outf);
            }
        }
    } else if (geosFileStruc == GEOS_FILE_STRUC_VLIR) {
#ifdef DEBUG_DRIVE
        log_debug("DEBUG: GEOS_FILE_STRUC_VLIR (%d:%d)", infoTrk, infoSec);
#endif
        /* The vlir block in cvt files is a conversion of the vlir
         * block on cbm disks.
         * Every non empty or non existent TS pointer is replaced with
         * (NoOfBlocks in Record, Bytes in last Record block).
         * Therefore the vlirBlock
         * is translated to vlirTrans according this rule.
         *
         * Copy vlir block to vlirTrans
         */
        for (n = 2; n < 256; n++) {
            vlirTrans[n] = vlirBlock[n];
        }

#ifdef DEBUG_DRIVE
        log_debug("DEBUG: VLIR scan record chains");
#endif

        /* Replace the TS-chain-origins with NoOfBlocks/BytesInLastSector */

        vlirIdx = 2;
        aktTrk = vlirBlock[vlirIdx];
        aktSec = vlirBlock[vlirIdx + 1];
        NoOfBlocks = 0;
        NoOfChains = 0;
        BytesInLastSector = 255;
        while (aktTrk != 0 && vlirIdx <= 254) {
            if (aktTrk != 0) { /* Record exists and is not empty */
#ifdef DEBUG_DRIVE
                log_debug("DEBUG: VLIR IDX %d", vlirIdx);
#endif
                NoOfChains++;
                while (aktTrk != 0) {
                    /* Read the chain and collect No Of Blocks */
#ifdef DEBUG_DRIVE
                    log_debug("DEBUG: VLIR BLOCK (%d:%d)", aktTrk, aktSec);
#endif

                    if (vdrive_read_sector(drives[unit], block, aktTrk, aktSec) != 0) {
                        fprintf(stderr,
                                "Cannot read input file data block `%s': %s.\n",
                                src_name_ascii, strerror(errno));
                        return FD_RDERR;
                    }

                    /* Read TS for next sector*/

                    aktTrk = block[0];
                    aktSec = block[1];

                    if (aktTrk != 0) { /* Next Sector exists */
                        NoOfBlocks++;
                    } else { /* Current was last sector of chain */
                        NoOfBlocks++;
                        BytesInLastSector = aktSec;
                    }
                }
            }
            if (NoOfBlocks != 0) {
                /* Entries for empty or non existing entries
                 * in vlir block are the same in cvt and vlir files */
                vlirTrans[vlirIdx] = NoOfBlocks;
                vlirTrans[vlirIdx + 1] = BytesInLastSector;
            }

            /* Prepare for next loop iteration */

            vlirIdx += 2;
            if (vlirIdx <= 254) {
                aktTrk = vlirBlock[vlirIdx];
                aktSec = vlirBlock[vlirIdx + 1];
            }
            NoOfBlocks = 0;
            BytesInLastSector = 255;
        }

        /* output transformed vlir block */

        for (n = 2; n < 256; n++) {
            fputc(vlirTrans[n], outf);
        }

#ifdef DEBUG_DRIVE
        log_debug("DEBUG: VLIR output record chains");
#endif
        /* now output the record chains
           (leave the TS-Pointers since they are usesless now) */

        vlirIdx = 2;
        aktTrk = vlirBlock[vlirIdx];
        aktSec = vlirBlock[vlirIdx + 1];
        while (aktTrk != 0 && vlirIdx <= 254) {
            if (aktTrk != 0) {
#ifdef DEBUG_DRIVE
                log_debug("DEBUG: VLIR IDX %d", vlirIdx);
#endif
                NoOfChains--;
                /* Record exists */
                while (aktTrk != 0) {
#ifdef DEBUG_DRIVE
                    log_debug("DEBUG: VLIR BLOCK (%d:%d)", aktTrk, aktSec);
#endif
                    if (vdrive_read_sector(drives[unit], block, aktTrk, aktSec) != 0) {
                        fprintf(stderr,
                                "Cannot read input file data block `%s': %s.\n",
                                src_name_ascii, strerror(errno));
                        return FD_RDERR;
                    }
                    aktTrk = block[0];
                    aktSec = block[1];
                    BytesInLastSector = ((NoOfChains != 0) || (aktTrk != 0)) ? 256 : aktSec + 1;
                    for (n = 2; n < BytesInLastSector; n++) {
                        fputc(block[n], outf);
                    }
                }
            }
            vlirIdx += 2;
            if (vlirIdx <= 254) {
                aktTrk = vlirBlock[vlirIdx];
                aktSec = vlirBlock[vlirIdx + 1];
            }
        }
    } else {
        fprintf(stderr, "Unknown GEOS-File structure\n");
        return FD_RDERR;
    }
    return FD_OK;
}

/* Author:      DiSc
 * Date:        2000-07-28
 * Reads a geos file from the diskimage and writes it to a convert file
 * This code was copied from the write_cmd function.
 */
static int read_geos_cmd(int nargs, char **args)
{
    char *src_name_petscii, *src_name_ascii;
    char *dest_name_ascii;
    char *actual_name;
    char *p;
    unsigned int unit;
    FILE *outf;
    int err_code;

    p = extract_unit_from_file_name(args[1], &unit);
    if (p == NULL) {
        unit = drive_number;
    }

    if (check_drive(unit, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    if (p == NULL) {
        src_name_ascii = lib_stralloc(args[1]);
    } else {
        src_name_ascii = lib_stralloc(p);
    }

    if (!is_valid_cbm_file_name(src_name_ascii)) {
        fprintf(stderr,
                "`%s' is not a valid CBM DOS file name.\n", src_name_ascii);
        lib_free(src_name_ascii);
        return FD_OK;               /* FIXME */
    }

    src_name_petscii = lib_stralloc(src_name_ascii);
    charset_petconvstring((BYTE *)src_name_petscii, 0);

    if (vdrive_iec_open(drives[unit], (BYTE *)src_name_petscii,
                        (int)strlen(src_name_petscii), 0, NULL)) {
        fprintf(stderr,
                "Cannot read `%s' on unit %d.\n", src_name_ascii, unit + 8);
        lib_free(src_name_ascii);
        lib_free(src_name_petscii);
        return FD_BADNAME;
    }

    /* Get real filename from the disk file.
       Slot must be defined by vdrive_iec_open().  */
    actual_name = lib_malloc(17);  /* FIXME: Should be a #define.  */
    memcpy(actual_name, drives[unit]->buffers[0].slot + SLOT_NAME_OFFSET, 16);
    actual_name[16] = 0;

    if (nargs == 3) {
        dest_name_ascii = args[2];
    } else {
        int l;

        dest_name_ascii = actual_name;
        vdrive_dir_no_a0_pads((BYTE *) dest_name_ascii, 16);
        l = (int)strlen(dest_name_ascii) - 1;
        while (dest_name_ascii[l] == ' ') {
            dest_name_ascii[l] = 0;
            l--;
        }
        charset_petconvstring((BYTE *)dest_name_ascii, 1);
    }

    outf = fopen(dest_name_ascii, MODE_WRITE);
    if (outf == NULL) {
        fprintf(stderr,
                "Cannot create output file `%s': %s.\n",
                dest_name_ascii, strerror(errno));
        vdrive_iec_close(drives[unit], 0);
        lib_free(src_name_petscii);
        lib_free(src_name_ascii);
        lib_free(actual_name);
        return FD_NOTWRT;
    }

    printf("Reading file `%s' from unit %d.\n", src_name_ascii, unit + 8);

    err_code = internal_read_geos_file(unit, outf, src_name_ascii);

    fclose(outf);
    vdrive_iec_close(drives[unit], 0);

    lib_free(src_name_petscii);
    lib_free(src_name_ascii);
    lib_free(actual_name);

    return err_code;
}

/* Author: DiSc
 * Date:   2000-07-28
 * Put the NTS-Chain on a block. Can also be a vlir block if blk_offset is > 1
 */
static int fix_ts(int unit, unsigned int trk, unsigned int sec,
                  unsigned int next_trk, unsigned int next_sec,
                  unsigned int blk_offset)
{
    BYTE block[256];
    if (vdrive_read_sector(drives[unit], block, trk, sec) == 0) {
        block[blk_offset] = next_trk;
        block[blk_offset + 1] = next_sec;
        if (vdrive_write_sector(drives[unit], block, trk, sec) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Author:      DiSc
 * Date:        2000-07-28
 * Companion to write_geos_cmd. Expects an file opened for writing in
 * drives[unit]->buffers[1].slot
 */
static int internal_write_geos_file(int unit, FILE* f)
{
    BYTE dirBlock[256];
    BYTE infoBlock[256];
    BYTE vlirBlock[256];
    BYTE block[256];
    unsigned int vlirTrk, vlirSec;
    unsigned int aktTrk, aktSec;
    unsigned int lastTrk, lastSec;
    BYTE geosFileStruc;
    int c = 0;
    int n;
    int bContinue;
    int numBlks, bytesInLastBlock;

    /* First block of cvt file is the directory entry, rest padded with zeros */

    for (n = 2; n < 256; n++) {
        c = fgetc(f);
        dirBlock[n] = c;
    }

    /* copy to the already created slot */

    for (n = 2; n < 32; n++) {
        drives[unit]->buffers[1].slot[n] = dirBlock[n];
    }

    geosFileStruc = drives[unit]->buffers[1].slot[SLOT_GEOS_FILE_STRUC];

    /* next is the geos info block */

    infoBlock[0] = 0;
    infoBlock[1] = 0xFF;
    for (n = 2; n < 256; n++) {
        c = fgetc(f);
        infoBlock[n] = c;
    }

    /* put it on disk */

    if (vdrive_bam_alloc_first_free_sector(drives[unit], &vlirTrk, &vlirSec) < 0) {
        fprintf(stderr, "Disk full\n");
        return FD_WRTERR;
    }
    if (vdrive_write_sector(drives[unit], infoBlock, vlirTrk, vlirSec) != 0) {
        fprintf(stderr, "Disk full\n");
        return FD_WRTERR;
    }

    /* and put the blk/sec in the dir entry */

    drives[unit]->buffers[1].slot[SLOT_SIDE_TRACK] = vlirTrk;
    drives[unit]->buffers[1].slot[SLOT_SIDE_SECTOR] = vlirSec;

    /* now read the first data block. if its a vlir-file its the vlir index
     * block
     * else its already a data block */
    for (n = 2; n < 256; n++) {
        c = fgetc(f);
        vlirBlock[n] = c;
    }

    /* the vlir index block always has a NTS-chain of (0, FF) */

    if (geosFileStruc == GEOS_FILE_STRUC_VLIR) {
        vlirBlock[0] = 0;
        vlirBlock[1] = 0xFF;
    }

    /* write the block */

    if (vdrive_bam_alloc_next_free_sector(drives[unit], &vlirTrk, &vlirSec) < 0) {
        fprintf(stderr, "Disk full\n");
        return FD_WRTERR;
    }

    if (vdrive_write_sector(drives[unit], vlirBlock, vlirTrk, vlirSec) != 0) {
        fprintf(stderr, "Disk full\n");
        return FD_WRTERR;
    }

    /* put the TS in the dir entry */

    drives[unit]->buffers[1].slot[SLOT_FIRST_TRACK] = vlirTrk;
    drives[unit]->buffers[1].slot[SLOT_FIRST_SECTOR] = vlirSec;

    if (geosFileStruc == GEOS_FILE_STRUC_SEQ) {
#ifdef DEBUG_DRIVE
        log_debug("DEBUG: GEOS_FILE_STRUC_SEQ (%d:%d)", vlirTrk, vlirSec);
#endif
        /* normal seq file (rest like standard files) */
        lastTrk = vlirTrk;
        lastSec = vlirSec;
        aktTrk = vlirTrk;
        aktSec = vlirSec;
        bContinue = (vlirBlock[0] != 0);
        while (bContinue) {
            block[0] = 0;
            block[1] = 255;

            /* read next block */

            for (n = 2; n < 256; n++) {
                c = fgetc(f);
                if (c == EOF) {
                    block[0] = 0;
                    block[1] = n - 1;
                    while (n < 256) {
                        block[n++] = 0x00;
                    }
                    bContinue = 0;
                    break;
                }
                block[n] = c;
            }

            /* allocate it */

            if (vdrive_bam_alloc_next_free_sector(drives[unit], &aktTrk, &aktSec) < 0) {
                fprintf(stderr, "Disk full\n");
                return FD_WRTERR;
            }

            /* write it to disk */

            if (vdrive_write_sector(drives[unit], block, aktTrk, aktSec) != 0) {
                fprintf(stderr, "Disk full\n");
                return FD_WRTERR;
            }

            /* put the TS of the current block to the predecessor block */

            if (!fix_ts(unit, lastTrk, lastSec, aktTrk, aktSec, 0)) {
                fprintf(stderr, "Internal error.\n");
                return FD_WRTERR;
            }
            lastTrk = aktTrk;
            lastSec = aktSec;
        }
    } else if (geosFileStruc == GEOS_FILE_STRUC_VLIR) {
#ifdef DEBUG_DRIVE
        log_debug("DEBUG: GEOS_FILE_STRUC_VLIR (%d:%d)", vlirTrk, vlirSec);
#endif
        /* in a cvt file containing a vlir file the vlir block contains
         * a pair (NoOfBlocksForChain, BytesInLastBlock + 2) for every vlir
         * record that exists. Non-existing record have a (0, 0) pair,
         * empty records a (0, 255) pair. There can be empty or non
         * existing records between data records.
         */

        int vlirIdx = 2;
        while (vlirIdx <= 254) {
            if (vlirBlock[vlirIdx] != 0) {
#ifdef DEBUG_DRIVE
                log_debug("DEBUG: VLIR IDX %d (%d:%d)", vlirIdx, vlirTrk, vlirSec);
#endif
                lastTrk = vlirTrk;
                lastSec = vlirSec;
                aktTrk = vlirTrk;
                aktSec = vlirSec;
                numBlks = vlirBlock[vlirIdx];
                /* How many blocks in record */
                bytesInLastBlock = vlirBlock[vlirIdx + 1];
                /* Bytes in last blocks */
                while (numBlks > 0) {
                    /* read next block */

                    block[0] = 0;
                    block[1] = 0xFF;
                    for (n = 2; n < 256; n++) {
                        c = fgetc(f);
                        if (c == EOF) {
                            if (numBlks > 1) {
                                fprintf(stderr, "unexpected EOF encountered\n");
                                return FD_RDERR;
                            }
                            while (n < 256) {
                                block[n++] = 0x00;
                            }
                            break;
                        } else {
                            block[n] = c;
                        }
                    }
                    if (numBlks == 1) { /* last block */
                        block[0] = 0;
                        block[1] = bytesInLastBlock;
                    }

                    /* allocate it */

                    if (vdrive_bam_alloc_next_free_sector(drives[unit], &aktTrk, &aktSec) < 0) {
                        fprintf(stderr, "Disk full\n");
                        return FD_WRTERR;
                    }

                    /* write it to disk */
#ifdef DEBUG_DRIVE
                    log_debug("DEBUG: VLIR BLOCK (%d:%d)", aktTrk, aktSec);
#endif

                    if (vdrive_write_sector(drives[unit], block, aktTrk, aktSec) != 0) {
                        fprintf(stderr, "Disk full\n");
                        return FD_WRTERR;
                    }
                    /*
                     * write the TS to the predecessor block or if this is
                     * the first block of a vlir record write it to the vlir
                     * index block at the correct offset.
                     */
                    if (!fix_ts(unit, lastTrk, lastSec, aktTrk, aktSec,
                                lastTrk == vlirTrk && lastSec == vlirSec
                                ? vlirIdx : 0)) {
                        fprintf(stderr, "Internal error.\n");
                        return FD_WRTERR;
                    }
                    lastTrk = aktTrk;
                    lastSec = aktSec;
                    numBlks--;
                }
            }
            vlirIdx += 2;
        }
    }
    return FD_OK;
}


/* Author:      DiSc
 * Date:        2000-07-28
 * Write a geos .cvt file to the diskimage
 * This code was copied from the write_cmd function.
 */
static int write_geos_cmd(int nargs, char **args)
{
    int unit, erg;
    char *dest_name_ascii, *dest_name_petscii;
    FILE *f;
    BYTE* e;
    char *slashp;
    vdrive_dir_context_t dir;

    /* geoswrite <source> */
    dest_name_ascii = NULL;
    unit = drive_number;

    if (check_drive(unit, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    f = fopen(args[1], MODE_READ);
    if (f == NULL) {
        fprintf(stderr,
                "Cannot read file `%s': %s.\n", args[1], strerror(errno));
        return FD_NOTRD;
    }

    /* User did not specify a destination name...  Let's try to make an
       educated guess at what she expects.

       Convert file have a directory and info block which contains the
       resulting filename. Because i am using the vdrive functions to create
       a directory entry lets get a temporary name out of the source file for
       initial creating of the USR file.
     */

    slashp = strrchr(args[1], '/');
    if (slashp == NULL) {
        dest_name_ascii = lib_stralloc(args[1]);
    } else {
        dest_name_ascii = lib_stralloc(slashp + 1);
    }
    dest_name_petscii = lib_stralloc(dest_name_ascii);
    charset_petconvstring((BYTE *)dest_name_petscii, 0);

    if (vdrive_iec_open(drives[unit], (BYTE *)dest_name_petscii,
                        (int)strlen(dest_name_petscii), 1, NULL)) {
        fprintf(stderr, "Cannot open `%s' for writing on image.\n",
                dest_name_ascii);
        fclose(f);
        return FD_WRTERR;
    }

    /* the following function reuses the fresh created dir entry ... */
    erg = internal_write_geos_file(unit, f);
    fclose(f);

    /* Start: copied from vdrive_iec_close
     * The bam and directory entry must be copied to the disk. the code
     * from the vdrive routines does that thing.
     */
    vdrive_dir_find_first_slot(drives[unit], dest_name_petscii,
                               (int)strlen(dest_name_petscii), 0, &dir);
    e = vdrive_dir_find_next_slot(&dir);

    if (!e) {
        drives[unit]->buffers[1].mode = BUFFER_NOT_IN_USE;
        lib_free(drives[unit]->buffers[1].buffer);
        drives[unit]->buffers[1].buffer = NULL;

        vdrive_command_set_error(drives[unit], CBMDOS_IPE_DISK_FULL, 0, 0);
        return SERIAL_ERROR;
    }
    /* The buffer MUST be mark as closed to avoid the vdrive functions
     * to add an empty block at the end of the file that was allocated
     * when the file was created!!!!
     */
    drives[unit]->buffers[1].slot[SLOT_TYPE_OFFSET] |= 0x80; /* Closed */

    memcpy(&dir.buffer[dir.slot * 32 + 2],
           drives[unit]->buffers[1].slot + 2,
           30);

#ifdef DEBUG_DRIVE
    log_debug("DEBUG: closing, write DIR slot (%d %d) and BAM.", dir.track, dir.sector);
#endif
    vdrive_write_sector(drives[unit], dir.buffer, dir.track, dir.sector);
    vdrive_bam_write_bam(drives[unit]);
    drives[unit]->buffers[1].mode = BUFFER_NOT_IN_USE;
    lib_free((char *)drives[unit]->buffers[1].buffer);
    drives[unit]->buffers[1].buffer = NULL;

    /* End: copied from vdrive_iec_close */

    lib_free(dest_name_ascii);
    lib_free(dest_name_petscii);

    return erg;
}

static int rename_cmd(int nargs, char **args)
{
    char *src_name, *dest_name;
    unsigned int src_unit, dest_unit;
    char *command;
    char *p;

    p = extract_unit_from_file_name(args[1], &src_unit);
    if (p == NULL) {
        src_unit = drive_number;
        src_name = lib_stralloc(args[1]);
    } else {
        src_name = lib_stralloc(p);
    }

    p = extract_unit_from_file_name(args[2], &dest_unit);
    if (p == NULL) {
        dest_unit = drive_number;
        dest_name = lib_stralloc(args[2]);
    } else {
        dest_name = lib_stralloc(p);
    }

    if (dest_unit != src_unit) {
        fprintf(stderr, "Source and destination must be on the same unit.\n");
        lib_free(src_name);
        lib_free(dest_name);
        return FD_OK;               /* FIXME */
    }

    if (check_drive(dest_unit, CHK_RDY) < 0) {
        lib_free(src_name);
        lib_free(dest_name);
        return FD_NOTREADY;
    }

    if (!is_valid_cbm_file_name(src_name)) {
        fprintf(stderr, "`%s' is not a valid CBM DOS file name.\n", src_name);
        lib_free(src_name);
        lib_free(dest_name);
        return FD_OK;               /* FIXME */
    }

    if (!is_valid_cbm_file_name(dest_name)) {
        fprintf(stderr, "`%s' is not a valid CBM DOS file name.\n", dest_name);
        lib_free(src_name);
        lib_free(dest_name);
        return FD_OK;               /* FIXME */
    }

    printf("Renaming `%s' to `%s'\n", src_name, dest_name);

    command = util_concat("r:", dest_name, "=", src_name, NULL);
    charset_petconvstring((BYTE *)command, 0);

    vdrive_command_execute(drives[dest_unit],
                           (BYTE *)command, (unsigned int)strlen(command));

    lib_free(command);
    lib_free(dest_name);
    lib_free(src_name);

    return FD_OK;
}

static int show_cmd(int nargs, char **args)
{
    if (strcasecmp(args[1], "copying") == 0) {
        printf("%s", info_license_text);
    } else if (strcasecmp(args[1], "warranty") == 0) {
        printf("%s", info_warranty_text);
    } else {
        fprintf(stderr, "Use either `show copying' or `show warranty'.\n");
        return FD_OK;           /* FIXME? */
    }

    return FD_OK;
}

/* Copy files from a  tape image.  */
static int tape_cmd(int nargs, char **args)
{
    tape_image_t *tape_image;
    vdrive_t *drive;
    int count;

    if (check_drive(drive_number, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    drive = drives[drive_number];

    tape_image = tape_internal_open_tape_image(args[1], 1);

    if (tape_image == NULL) {
        fprintf(stderr, "Cannot read tape file `%s'.\n", args[1]);
        return FD_BADNAME;
    }

    for (count = 0; tape_seek_to_next_file(tape_image, 0) >= 0; ) {
        tape_file_record_t *rec;

        rec = tape_get_current_file_record(tape_image);

        if (rec->type) {
            char *dest_name_ascii;
            char *dest_name_petscii;
            BYTE *buf;
            size_t name_len;
            WORD file_size;
            int i, retval;

            /* Ignore traling spaces and 0xa0's.  */
            name_len = strlen((char *)(rec->name));
            while (name_len > 0 && (rec->name[name_len - 1] == 0xa0
                                    || rec->name[name_len - 1] == 0x20)) {
                name_len--;
            }

            dest_name_petscii = lib_calloc(1, name_len + 1);
            memcpy(dest_name_petscii, rec->name, name_len);

            dest_name_ascii = lib_calloc(1, name_len + 1);
            memcpy(dest_name_ascii, dest_name_petscii, name_len);
            charset_petconvstring((BYTE *)dest_name_ascii, 1);

            if (nargs > 2) {
                int i, found;

                for (i = 2, found = 0; i < nargs; i++) {
                    if (name_len == strlen(args[i])
                        && memcmp(args[i], dest_name_ascii, name_len) == 0) {
                        found = 1;
                        break;
                    }
                }

                if (!found) {
                    continue;
                }
            }

            if (rec->type == 1 || rec->type == 3) {
                if (vdrive_iec_open(drive, (BYTE *)dest_name_petscii,
                                    (int)name_len, 1, NULL)) {
                    fprintf(stderr,
                            "Cannot open `%s' for writing on drive %d.\n",
                            dest_name_ascii, drive_number + 8);
                    lib_free(dest_name_petscii);
                    lib_free(dest_name_ascii);
                    continue;
                }

                fprintf(stderr, "Writing `%s' ($%04X - $%04X) to drive %d.\n",
                        dest_name_ascii, rec->start_addr, rec->end_addr,
                        drive_number + 8);

                vdrive_iec_write(drive, ((BYTE)(rec->start_addr & 0xff)), 1);
                vdrive_iec_write(drive, ((BYTE)(rec->start_addr >> 8)), 1);

                file_size = rec->end_addr - rec->start_addr;

                buf = lib_calloc((size_t)file_size, 1);

                retval = tape_read(tape_image, buf, file_size);

                if (retval < 0 || retval != (int) file_size) {
                    fprintf(stderr,
                            "Unexpected end of tape: file may be truncated.\n");
                }

                for (i = 0; i < file_size; i++) {
                    if (vdrive_iec_write(drives[drive_number],
                                         ((BYTE)(buf[i])), 1)) {
                        tape_internal_close_tape_image(tape_image);
                        lib_free(dest_name_petscii);
                        lib_free(dest_name_ascii);
                        lib_free(buf);
                        return FD_WRTERR;
                    }
                }

                lib_free(buf);
            } else if (rec->type == 4) {
                BYTE b;
                char *dest_name_plustype;
                dest_name_plustype = util_concat(dest_name_petscii, ",S,W", NULL);
                retval = vdrive_iec_open(drive, (BYTE *)dest_name_plustype,
                                         (int)name_len + 4, 2, NULL);
                lib_free(dest_name_plustype);

                if (retval) {
                    fprintf(stderr,
                            "Cannot open `%s' for writing on drive %d.\n",
                            dest_name_ascii, drive_number + 8);
                    lib_free(dest_name_petscii);
                    lib_free(dest_name_ascii);
                    continue;
                }

                fprintf(stderr, "Writing SEQ file `%s' to drive %d.\n",
                        dest_name_ascii, drive_number + 8);

                do {
                    retval = tape_read(tape_image, &b, 1);

                    if (retval < 0) {
                        fprintf(stderr,
                                "Unexpected end of tape: file may be truncated.\n");
                        break;
                    } else if (vdrive_iec_write(drives[drive_number], b, 2)) {
                        tape_internal_close_tape_image(tape_image);
                        lib_free(dest_name_petscii);
                        lib_free(dest_name_ascii);
                        return FD_WRTERR;
                    }
                } while (retval == 1);
            }

            vdrive_iec_close(drive, 1);
            lib_free(dest_name_petscii);
            lib_free(dest_name_ascii);
            count++;
        }
    }

    tape_internal_close_tape_image(tape_image);

    printf("\n%d files copied.\n", count);

    return FD_OK;
}

static int unit_cmd(int nargs, char **args)
{
    int dev;

    if (arg_to_int(args[1], &dev) < 0 || check_drive(dev, CHK_NUM) < 0) {
        return FD_BADDEV;
    }

    drive_number = dev & 3;
    return FD_OK;
}

static int unlynx_loop(FILE *f, FILE *f2, vdrive_t *vdrive, long dentries)
{
    long lbsize, bsize;
    char cname[20], ftype;
    BYTE val;
    int cnt;
    unsigned int len;
    char buff[256];
    cbmdos_cmd_parse_t cmd_parse;

    while (dentries != 0) {
        int filetype, rc;

        /* Read CBM filename */
        cnt = 0;
        while (1) {
            if (fread(&val, 1, 1, f) != 1) {
                return FD_RDERR;
            }
            if (val != 13 && cnt < 20 - 1) {
                cname[cnt++] = val;
            } else {
                break;
            }
        }
        cname[cnt] = 0;

        /* Read the block size */
        cnt = 0;
        while (1) {
            if (fread(&val, 1, 1, f) != 1) {
                return FD_RDERR;
            }
            if (val != 13) {
                buff[cnt++] = val;
            } else {
                break;
            }
        }
        buff[cnt] = 0;

        if (util_string_to_long(buff, NULL, 10, &bsize) < 0) {
            fprintf(stderr, "Invalid Lynx file.\n");
            return FD_RDERR;
        }
        /* Get the file type (P[RG], S[EQ], R[EL], U[SR]) */
        ftype = fgetc(f);
        fgetc(f);

        switch (ftype) {
            case 'D':
                filetype = CBMDOS_FT_DEL;
                break;
            case 'P':
                filetype = CBMDOS_FT_PRG;
                break;
            case 'S':
                filetype = CBMDOS_FT_SEQ;
                break;
            case 'U':
                filetype = CBMDOS_FT_USR;
                break;
            case 'R':
                fprintf(stderr, "REL not supported.\n");
                return FD_RDERR;
            default:
                filetype = CBMDOS_FT_PRG;
        }
        /* Get the byte size of the last block +1 */
        cnt = 0;
        while (1) {
            if (fread(&val, 1, 1, f) != 1) {
                return FD_RDERR;
            }
            if (val != 13) {
                buff[cnt++] = val;
            } else {
                break;
            }
        }
        buff[cnt] = 0;

        if (util_string_to_long(buff, NULL, 10, &lbsize) < 0) {
            fprintf(stderr, "Invalid Lynx file.\n");
            return FD_RDERR;
        }
        /* Calculate byte size of file */
        cnt = (bsize - 1) * 254 + lbsize - 1;

        printf("Writing file '%s' to image.\n", cname);

        cmd_parse.parsecmd = lib_stralloc(cname);
        cmd_parse.secondary = 1;
        cmd_parse.parselength = (unsigned int)strlen(cname);
        cmd_parse.readmode = CBMDOS_FAM_WRITE;
        cmd_parse.filetype = filetype;

        rc = vdrive_iec_open(vdrive, NULL, 0, 1, &cmd_parse);

        if (rc != SERIAL_OK) {
            fprintf(stderr, "Error writing file %s.\n", cname);
            break;
        }

        while (cnt != 0) {
            if (fread(&val, 1, 1, f2) != 1) {
                return FD_RDERR;
            }
            if (vdrive_iec_write(vdrive, val, 1)) {
                fprintf(stderr, "No space on image ?\n");
                break;
            }
            cnt--;
        }
        vdrive_iec_close(vdrive, 1);

        /* Adjust for the last block */
        if (lbsize < 255) {
            len = 254 + 1 - lbsize;
            if (fread(buff, 1, len, f2) != len) {
                return FD_RDERR;
            }
        }
        dentries--;
    }

    return FD_OK;
}

static int unlynx_cmd(int nargs, char **args)
{
    vdrive_t *vdrive;
    FILE *f, *f2;
    int dev, cnt;
    long dentries, dirsize;
    BYTE val;
    char buff[256];
    int rc;

    if (nargs < 3) {
        dev = drive_number;
    } else {
        if (arg_to_int(args[2], &dev) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(dev, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        dev -= 8;
    }

    if (check_drive(dev, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    vdrive = drives[dev & 3];

    f = fopen(args[1], MODE_READ);

    if (f == NULL) {
        fprintf(stderr, "Cannot open `%s' for reading.\n", args[1]);
        return FD_NOTRD;
    }

    /* Look for the 0, 0, 0 sign of the end of BASIC.  */
    cnt = 0;
    while (1) {
        if (fread(&val, 1, 1, f) != 1) {
            return FD_RDERR;
        }
        if (val == 0) {
            cnt++;
        } else {
            cnt = 0;
        }
        if (cnt == 3) {
            break;
        }
    }

    /* Bypass the 1st return in the file */
    fgetc(f);

    /* Get the directory block size */
    cnt = 0;
    while (1) {
        if (fread(&val, 1, 1, f) != 1) {
            return FD_RDERR;
        }
        if (val != 13) {
            buff[cnt++] = val;
        } else {
            break;
        }
    }

    buff[cnt] = 0;

    if (util_string_to_long(buff, NULL, 10, &dirsize) < 0 || dirsize <= 0) {
        fprintf(stderr, "Invalid Lynx file.\n");
        fclose(f);
        return FD_RDERR;
    }

    /* Get the number of dir entries */
    cnt = 0;
    while (1) {
        if (fread(&val, 1, 1, f) != 1) {
            return FD_RDERR;
        }
        if (val != 13 && cnt < 256 - 1) {
            buff[cnt++] = val;
        } else {
            break;
        }
    }

    buff[cnt] = 0;

    if (util_string_to_long(buff, NULL, 10, &dentries) < 0 || dentries <= 0) {
        fprintf(stderr, "Invalid Lynx file.\n");
        fclose(f);
        return FD_RDERR;
    }

    /* Open the file for reading of the chained data */
    f2 = fopen(args[1], MODE_READ);

    if (f2 == NULL) {
        fprintf(stderr, "Cannot open `%s' for reading.\n", args[1]);
        fclose(f);
        return FD_NOTRD;
    }

    fseek(f2, (dirsize * 254), SEEK_SET);

    rc = unlynx_loop(f, f2, vdrive, dentries);

    fclose(f);
    fclose(f2);

    return rc;
}

static int validate_cmd(int nargs, char **args)
{
    int dnr;

    switch (nargs) {
        case 1:
            /* validate */
            dnr = drive_number;
            break;
        case 2:
            {
                int unit;

                /* validate <unit> */
                if (arg_to_int(args[1], &unit) < 0) {
                    return FD_BADDEV;
                }
                dnr = unit - 8;
                break;
            }
        default:
            return FD_BADVAL;
    }

    if (check_drive(dnr, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    printf("Validating in unit %d...\n", dnr + 8);
    vdrive_command_validate(drives[dnr]);

    return FD_OK;
}

static int write_cmd(int nargs, char **args)
{
    unsigned int dnr;
    char *dest_name;
    unsigned int dest_len;
    char *p;
    fileio_info_t *finfo;

    if (nargs == 3) {
        /* write <source> <dest> */
        p = extract_unit_from_file_name(args[2], &dnr);
        if (p == NULL) {
            dnr = drive_number;
            dest_name = lib_stralloc(args[2]);
        } else {
            if (*p != 0) {
                dest_name = lib_stralloc(args[2]);
            } else {
                dest_name = NULL;
            }
        }
        if (dest_name != NULL) {
            charset_petconvstring((BYTE *)dest_name, 0);
        }
    } else {
        /* write <source> */
        dest_name = NULL;
        dnr = drive_number;
    }

    if (check_drive(dnr, CHK_RDY) < 0) {
        return FD_NOTREADY;
    }

    finfo = fileio_open(args[1], NULL, FILEIO_FORMAT_RAW | FILEIO_FORMAT_P00,
                        FILEIO_COMMAND_READ | FILEIO_COMMAND_FSNAME,
                        FILEIO_TYPE_PRG);

    if (finfo == NULL) {
        fprintf(stderr, "Cannot read file `%s': %s.\n", args[1],
                strerror(errno));
        return FD_NOTRD;
    }

    if (dest_name == NULL) {
        dest_name = lib_stralloc((char *)(finfo->name));
        dest_len = finfo->length;
    } else {
        dest_len = (unsigned int)strlen(dest_name);
    }

    if (vdrive_iec_open(drives[dnr], (BYTE *)dest_name, (int)dest_len, 1, NULL)) {
        fprintf(stderr, "Cannot open `%s' for writing on image.\n",
                finfo->name);
        fileio_close(finfo);
        lib_free(dest_name);
        return FD_WRTERR;
    }

    if (dest_name == (char *)finfo->name) {
        printf("Writing file `%s' to unit %d.\n", finfo->name, dnr + 8);
    } else {
        printf("Writing file `%s' as `%s' to unit %d.\n", finfo->name,
               dest_name, dnr + 8);
    }

    while (1) {
        BYTE c;

        if (fileio_read(finfo, &c, 1) != 1) {
            break;
        }

        if (vdrive_iec_write(drives[dnr], c, 1)) {
            fprintf(stderr, "No space on image?\n");
            break;
        }
    }

    fileio_close(finfo);
    vdrive_iec_close(drives[dnr], 1);

    lib_free(dest_name);

    return FD_OK;
}

static int zcreate_cmd(int nargs, char **args)
{
    vdrive_t *vdrive = drives[drive_number];
    FILE *fsfd = NULL;
    unsigned int track, sector;
    unsigned int count;
    char *p, *fname, *dirname, *oname;
    int singlefilemode = 0, err;
    BYTE sector_data[256];

    /* Open image or create a new one.  If the file exists, it must have
       valid header.  */
    if (open_image(drive_number, args[1], 1, DISK_IMAGE_TYPE_D64) < 0) {
        return FD_BADIMAGE;
    }

    fname = lib_malloc((size_t)ioutil_maxpathlen());
    dirname = lib_malloc((size_t)ioutil_maxpathlen());

    p = strrchr(args[2], FSDEV_DIR_SEP_CHR);
    if (p == NULL) {
        /* ignore '[0-4]!' if found */
        if (args[2][0] >= '1' && args[2][0] <= '4' && args[2][1] == '!') {
            strcpy(fname + 2, args[2] + 2);
        } else {
            strcpy(fname + 2, args[2]);
        }
        fname[0] = '0';
        fname[1] = '!';
        strcpy(dirname, "");
    } else {
        size_t len_path;

        len_path = (size_t)(p - args[2]);
        if (len_path == strlen(args[2]) - 1) {
            /* FIXME: Close image?  */
            lib_free(fname);
            lib_free(dirname);
            return FD_RDERR;
        }
        strncpy(dirname, args[2], len_path + 1);
        dirname[len_path + 1] = '\0';

        /* ignore '[0-4]!' if found */
        if (args[2][len_path + 1] >= '1' && args[2][len_path + 1] <= '4'
            && args[2][len_path + 1 + 1] == '!') {
            strcpy(fname + 2, &(args[2][len_path + 1]) + 2);
        } else {
            strcpy(fname + 2, &(args[2][len_path + 1]));
        }
        fname[0] = '0';
        fname[1] = '!';
    }

    oname = lib_malloc((size_t)ioutil_maxpathlen());

    printf("Copying blocks to image.\n");

    for (track = 1; track <= 35; track++) {
        if (singlefilemode || track == 1) {
            if (track == 1) {
                /* For now we disable one-file more, because it is not detected
                   correctly.  */
                strcpy(oname, dirname);
                strcat(oname, fname + 2);
                fsfd = fopen(oname, MODE_READ);
                if (fsfd != NULL) {
                    printf("Reading zipfile on one-file mode\n");
                    singlefilemode = 1;
                    fseek(fsfd, 4, SEEK_SET);
                }
            } else if (track == 9 || track == 17 || track == 26) {
                fseek(fsfd, 2, SEEK_CUR);
            }
        }
        if (!singlefilemode) {
            switch (track) {
                case 1:
                case 9:
                case 17:
                case 26:
                    fname[0]++;
                    if (fsfd != NULL) {
                        fclose(fsfd);
                    }
                    strcpy(oname, dirname);
                    strcat(oname, fname);
                    if ((fsfd = fopen(oname, MODE_READ)) == NULL) {
                        fprintf(stderr, "Cannot open `%s'.\n", fname);
                        lib_free(fname);
                        lib_free(dirname);
                        lib_free(oname);
                        return FD_NOTRD;
                    }
                    fseek(fsfd, (track == 1) ? 4 : 2, SEEK_SET);
                    break;
            }
        }
        for (count = 0;
             count < disk_image_sector_per_track(DISK_IMAGE_TYPE_D64, track);
             count++) {
            err = zipcode_read_sector(fsfd, track, (int *)&sector,
                                      (char *)sector_data);
            if (err) {
                fclose(fsfd);
                lib_free(fname);
                lib_free(dirname);
                lib_free(oname);
                return FD_BADIMAGE;
            }
            /* Write one block */
            if (vdrive_write_sector(vdrive, sector_data, track, sector) < 0) {
                fclose(fsfd);
                lib_free(fname);
                lib_free(dirname);
                lib_free(oname);
                return FD_RDERR;
            }
        }
    }

    fclose(fsfd);

    vdrive_command_execute(vdrive, (BYTE *)"I", 1);

    lib_free(fname);
    lib_free(dirname);
    lib_free(oname);

    return FD_OK;
}

static int raw_cmd(int nargs, char **args)
{
    vdrive_t *vdrive = drives[drive_number];

    if (vdrive == NULL || vdrive->buffers[15].buffer == NULL) {
        return FD_NOTREADY;
    }

    /* Write to the command channel.  */
    if (nargs >= 2) {
        char *command = lib_stralloc(args[1]);

        charset_petconvstring((BYTE *)command, 0);
        vdrive_command_execute(vdrive, (BYTE *)command, (unsigned int)strlen(command));
        lib_free(command);
    }

    /* Print the error now.  */
    puts((char *)vdrive->buffers[15].buffer);
    return FD_OK;
}

/* ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    char *args[MAXARG];
    int nargs;
    int i;
    int retval;

    archdep_init(&argc, argv);

    /* This causes all the logging messages from the various VICE modules to
       appear on stdout.  */
    log_init_with_fd(stdout);

    serial_iec_bus_init();

    for (i = 0; i < MAXARG; i++) {
        args[i] = NULL;
    }
    nargs = 0;

    drives[0] = lib_calloc(1, sizeof(vdrive_t));
    drives[1] = lib_calloc(1, sizeof(vdrive_t));

    retval = 0;

    /* The first arguments without leading `-' are interpreted as disk images
       to attach.  */
    for (i = 1; i < argc && *argv[i] != '-'; i++) {
        if (i - 1 > MAXDRIVE) {
            fprintf(stderr, "Ignoring disk image `%s'.\n", argv[i]);
        } else {
            open_disk_image(drives[i - 1], argv[i], i - 1 + 8);
        }
    }

    if (i == argc) {
        char *line;
        char *buf = NULL;

        /* Interactive mode.  */
        interactive_mode = 1;
        printf("C1541 Version %d.%02d.\n",
               C1541_VERSION_MAJOR, C1541_VERSION_MINOR);
        printf("Copyright 1995-2015 The VICE Development Team.\n"
               "C1541 is free software, covered by the GNU General Public License,"
               " and you are\n"
               "welcome to change it and/or distribute copies of it under certain"
               " conditions.\n"
               "Type `show copying' to see the conditions.\n"
               "There is absolutely no warranty for C1541.  Type `show warranty'"
               " for details.\n");

        while (1) {
            lib_free(buf);
            buf = lib_msprintf("c1541 #%d> ", drive_number | 8);
            line = read_line(buf);

            if (line == NULL) {
                putchar('\n');
                fflush(stdout), fflush(stderr);
                break;
            }

            if (*line == '!') {
                retval = system(line + 1);
                printf("Exit code: %d.\n", retval);
            } else {
                split_args(line, &nargs, args);
                if (nargs > 0) {
                    lookup_and_execute_command(nargs, args);
                }
            }
        }
    } else {
        while (i < argc) {
            args[0] = argv[i] + 1;
            nargs = 1;
            i++;
            for (; i < argc && *argv[i] != '-'; i++) {
                args[nargs++] = argv[i];
            }
            if (lookup_and_execute_command(nargs, args) < 0) {
                retval = 1;
                break;
            }
        }
    }

    for (i = 0; i <= MAXDRIVE; i++) {
        if (drives[i]) {
            close_disk_image(drives[i], i + 8);
        }
    }

    return retval;
}

static int p00save_cmd(int nargs, char **args)
{
    int dnr = 0, enable;

    arg_to_int(args[1], &enable);

    if (nargs == 3) {
        if (arg_to_int(args[2], &dnr) < 0) {
            return FD_BADDEV;
        }
        if (check_drive(dnr, CHK_NUM) < 0) {
            return FD_BADDEV;
        }
        dnr -= 8;
    }

    p00save[dnr] = (unsigned int)enable;
    return FD_OK;
}

/* ------------------------------------------------------------------------- */
/* FIXME: Can we get rid of this stuff?  */

void enable_text(void)
{
}

void disable_text(void)
{
}

int machine_bus_device_attach(unsigned int device, const char *name,
                              int (*getf)(vdrive_t *, BYTE *, unsigned int,
                                          struct cbmdos_cmd_parse_s *),
                              int (*putf)(vdrive_t *, BYTE, unsigned int),
                              int (*openf)(vdrive_t *, const char *, int,
                                           unsigned int),
                              int (*closef)(vdrive_t *, unsigned int),
                              void (*flushf)(vdrive_t *, unsigned int),
                              void (*listenf)(vdrive_t *, unsigned int))
{
    return 0;
}

struct vdrive_s *file_system_get_vdrive(unsigned int unit)
{
    if (unit < 8 || unit > 11) {
        printf("Wrong unit for vdrive");
        return NULL;
    }

    return drives[unit - 8];
}

snapshot_module_t *snapshot_module_create(snapshot_t *s,
                                          const char *name,
                                          BYTE major_version,
                                          BYTE minor_version)
{
    return NULL;
}

snapshot_module_t *snapshot_module_open(snapshot_t *s,
                                        const char *name,
                                        BYTE *major_version_return,
                                        BYTE *minor_version_return)
{
    return NULL;
}

int snapshot_module_close(snapshot_module_t *m)
{
    return 0;
}

void archdep_ui_init(int argc, char *argv[])
{
}

void ui_error_string(const char *text)
{
}

void vsync_suspend_speed_eval(void)
{
}

int machine_drive_rom_read(unsigned int type, WORD addr, BYTE *data)
{
    return -1;
}

struct image_contents_s *machine_diskcontents_bus_read(unsigned int unit)
{
    return diskcontents_iec_read(unit);
}

int machine_bus_lib_directory(unsigned int unit, const char *pattern,
                              BYTE **buf)
{
    return serial_iec_lib_directory(unit, pattern, buf);
}

int machine_bus_lib_read_sector(unsigned int unit, unsigned int track,
                                unsigned int sector, BYTE *buf)
{
    return serial_iec_lib_read_sector(unit, track, sector, buf);
}

int machine_bus_lib_write_sector(unsigned int unit, unsigned int track,
                                 unsigned int sector, BYTE *buf)
{
    return serial_iec_lib_write_sector(unit, track, sector, buf);
}

unsigned int machine_bus_device_type_get(unsigned int unit)
{
    return serial_device_type_get(unit);
}

void machine_drive_flush(void)
{
}

const char *machine_get_name(void)
{
    return machine_name;
}
