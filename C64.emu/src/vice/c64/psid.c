/*
 * psid.c - PSID file handling.
 *
 * Written by
 *  Dag Lem <resid@nimrod.no>
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
#include "c64mem.h"
#include "charset.h"
#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "psid.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "vsidui.h"
#include "vsync.h"
#include "zfile.h"

static int mus_load_file(const char* filename, int ispsid);

static log_t vlog = LOG_ERR;

typedef struct psid_s {
    /* PSID data */
    BYTE is_rsid;
    WORD version;
    WORD data_offset;
    WORD load_addr;
    WORD init_addr;
    WORD play_addr;
    WORD songs;
    WORD start_song;
    DWORD speed;
    /* psid v3 allows all 32 bytes to be used with no zero termination */
    BYTE name[32 + 1];
    BYTE author[32 + 1];
    BYTE copyright[32 + 1];
    WORD flags;
    BYTE start_page;
    BYTE max_pages;
    WORD reserved;
    WORD data_size;
    BYTE data[65536];

    /* Non-PSID data */
    DWORD frames_played;
    WORD load_last_addr;
} psid_t;

#define PSID_V1_DATA_OFFSET 0x76
#define PSID_V2_DATA_OFFSET 0x7c

int psid_ui_set_tune(int tune, void *param);

static psid_t* psid = NULL;
static int psid_tune = 0;       /* currently selected tune, 0: default 1: first, 2: second, etc */
static int keepenv = 0;

static int firstfile = 0;
static int psid_tune_cmdline = 0;

static int set_keepenv(int val, void *param)
{
    keepenv = val ? 1 : 0;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "PSIDKeepEnv", 0, RES_EVENT_NO, NULL,
      &keepenv, set_keepenv, NULL },
    { "PSIDTune", 0, RES_EVENT_NO, NULL,
      &psid_tune, psid_ui_set_tune, NULL },
    { NULL }
};

int psid_resources_init(void)
{
    return resources_register_int(resources_int);
}

static int cmdline_keepenv(const char *param, void *extra_param)
{
    keepenv = 1;
    return 0;
}

static int cmdline_psid_tune(const char *param, void *extra_param)
{
    psid_tune_cmdline = atoi(param);
    if (psid_tune_cmdline < 0) {
        psid_tune_cmdline = 0;
    }
    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    /* The Video Standard options are copied from the machine files. */
    { "-pal", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_PAL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PAL_SYNC_FACTOR,
      NULL, NULL },
    { "-ntsc", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_NTSC,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-ntscold", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_NTSCOLD,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_OLD_NTSC_SYNC_FACTOR,
      NULL, NULL },
    { "-paln", SET_RESOURCE, 0,
      NULL, NULL, "MachineVideoStandard", (resource_value_t)MACHINE_SYNC_PALN,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_USE_PALN_SYNC_FACTOR,
      NULL, NULL },
    { "-keepenv", CALL_FUNCTION, 0,
      cmdline_keepenv, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_OVERWRITE_PSID_SETTINGS,
      NULL, NULL },
    { "-tune", CALL_FUNCTION, 1,
      cmdline_psid_tune, NULL, NULL, NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NUMBER, IDCLS_SPECIFY_PSID_TUNE_NUMBER,
      NULL, NULL },
    { NULL }
};

int psid_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static WORD psid_extract_word(BYTE** buf)
{
    WORD word = (*buf)[0] << 8 | (*buf)[1];
    *buf += 2;
    return word;
}

int psid_load_file(const char* filename)
{
    FILE* f;
    BYTE buf[PSID_V2_DATA_OFFSET + 2];
    BYTE* ptr = buf;
    unsigned int length;

    /* HACK: the selected tune number is handled by the "PSIDtune" resource, which
     *       is actually saved in the ini file, and thus loaded and restored at
     *       startup. however, we do not want that. instead we want the default
     *       tune of the respective .sid file to be used, or the explicit tune
     *       number given on commandline (if any).
     */
    if (!firstfile) {
        if (psid_tune_cmdline) {
            psid_tune = psid_tune_cmdline;
        } else {
            psid_tune = 0;
        }
        firstfile = 1;
    }

    if (vlog == LOG_ERR) {
        vlog = log_open("Vsid");
    }

    if (!(f = zfile_fopen(filename, MODE_READ))) {
        return -1;
    }

    lib_free(psid);
    psid = lib_calloc(sizeof(psid_t), 1);

    if (fread(ptr, 1, 6, f) != 6 || (memcmp(ptr, "PSID", 4) != 0 && memcmp(ptr, "RSID", 4) != 0)) {
        goto fail;
    }
    psid->is_rsid = (ptr[0] == 'R');

    ptr += 4;
    psid->version = psid_extract_word(&ptr);

    if (psid->version < 1 || psid->version > 4) {
        log_error(vlog, "Unknown PSID version number: %d.", (int)psid->version);
        goto fail;
    }
    log_message(vlog, "PSID version number: %d.", (int)psid->version);

    length = (unsigned int)((psid->version == 1 ? PSID_V1_DATA_OFFSET : PSID_V2_DATA_OFFSET) - 6);

    if (fread(ptr, 1, length, f) != length) {
        log_error(vlog, "Reading PSID header.");
        goto fail;
    }

    psid->data_offset = psid_extract_word(&ptr);
    psid->load_addr = psid_extract_word(&ptr);
    psid->init_addr = psid_extract_word(&ptr);
    psid->play_addr = psid_extract_word(&ptr);
    psid->songs = psid_extract_word(&ptr);
    psid->start_song = psid_extract_word(&ptr);
    psid->speed = psid_extract_word(&ptr) << 16;
    psid->speed |= psid_extract_word(&ptr);
    psid->frames_played = 0;
    memcpy(psid->name, ptr, 32);
    psid->name[32] = '\0';
    ptr += 32;
    memcpy(psid->author, ptr, 32);
    psid->author[32] = '\0';
    ptr += 32;
    memcpy(psid->copyright, ptr, 32);
    psid->copyright[32] = '\0';
    ptr += 32;
    if (psid->version >= 2) {
        psid->flags = psid_extract_word(&ptr);
        psid->start_page = *ptr++;
        psid->max_pages = *ptr++;
        psid->reserved = psid_extract_word(&ptr);
    } else {
        psid->flags = 0;
        psid->start_page = 0;
        psid->max_pages = 0;
        psid->reserved = 0;
    }

    if ((psid->start_song < 1) || (psid->start_song > psid->songs)) {
        log_error(vlog, "Default tune out of range (%d of %d ?), using 1 instead.", psid->start_song, psid->songs);
        psid->start_song = 1;
    }

    /* Check for SIDPLAYER MUS files. */
    if (psid->flags & 0x01) {
        zfile_fclose(f);
        return mus_load_file(filename, 1);
    }

    /* Zero load address => the load address is stored in the
       first two bytes of the binary C64 data. */
    if (psid->load_addr == 0) {
        if (fread(ptr, 1, 2, f) != 2) {
            log_error(vlog, "Reading PSID load address.");
            goto fail;
        }
        psid->load_addr = ptr[0] | ptr[1] << 8;
    }

    /* Zero init address => use load address. */
    if (psid->init_addr == 0) {
        psid->init_addr = psid->load_addr;
    }

    /* Read binary C64 data. */
    psid->data_size = (WORD)fread(psid->data, 1, sizeof(psid->data), f);
    psid->load_last_addr = (psid->load_addr + psid->data_size - 1);

    if (ferror(f)) {
        log_error(vlog, "Reading PSID data.");
        goto fail;
    }

    if (!feof(f)) {
        log_error(vlog, "More than 64K PSID data.");
        goto fail;
    }

    /* Relocation setup. */
    if (psid->start_page == 0x00) {
        /* Start and end pages. */
        int startp = psid->load_addr >> 8;
        int endp = psid->load_last_addr >> 8;

        /* Used memory ranges. */
        unsigned int used[] = {
            0x00,
            0x03,
            0xa0,
            0xbf,
            0xd0,
            0xff,
            0x00,
            0x00
        };        /* calculated below */

        unsigned int pages[256];
        unsigned int last_page = 0;
        unsigned int i, page, tmp;

        log_message(vlog, "No PSID freepages set, recalculating...");

        /* finish initialization */
        used[6] = startp; used[7] = endp;

        /* Mark used pages in table. */
        memset(pages, 0, sizeof(pages));
        for (i = 0; i < sizeof(used) / sizeof(*used); i += 2) {
            for (page = used[i]; page <= used[i + 1]; page++) {
                pages[page] = 1;
            }
        }

        /* Find largest free range. */
        psid->max_pages = 0x00;
        for (page = 0; page < sizeof(pages) / sizeof(*pages); page++) {
            if (!pages[page]) {
                continue;
            }
            tmp = page - last_page;
            if (tmp > psid->max_pages) {
                psid->start_page = last_page;
                psid->max_pages = tmp;
            }
            last_page = page + 1;
        }

        if (psid->max_pages == 0x00) {
            psid->start_page = 0xff;
        }
    }

    if (psid->start_page == 0xff || psid->max_pages < 2) {
        log_error(vlog, "No space for driver.");
        goto fail;
    }

    zfile_fclose(f);

    return 0;

fail:
    zfile_fclose(f);
    lib_free(psid);
    psid = NULL;

    /* if the file wasnt in PSID format, try MUS/STR */
    if (memcmp(ptr, "PSID", 4) != 0 && memcmp(ptr, "RSID", 4) != 0) {
        return mus_load_file(filename, 0);
    }

    return -1;
}

void psid_shutdown(void)
{
    lib_free(psid);
    psid = NULL;
}

/* Use CBM80 vector to start PSID driver. This is a simple method to
   transfer control to the PSID driver while running in a pure C64
   environment. */
static int psid_set_cbm80(WORD vec, WORD addr)
{
    unsigned int i;
    BYTE cbm80[] = { 0x00, 0x00, 0x00, 0x00, 0xc3, 0xc2, 0xcd, 0x38, 0x30 };

    cbm80[0] = vec & 0xff;
    cbm80[1] = vec >> 8;

    for (i = 0; i < sizeof(cbm80); i++) {
        /* make backup of original content at 0x8000 */
        ram_store((WORD)(addr + i), ram_read((WORD)(0x8000 + i)));
        /* copy header */
        ram_store((WORD)(0x8000 + i), cbm80[i]);
    }

    return i;
}

void psid_init_tune(int install_driver_hook)
{
    int start_song = psid_tune;
    int sync, sid_model;
    int i;
    WORD reloc_addr;
    WORD addr;
    int speedbit;
    char* irq;
    char irq_str[20];
    const char csidflag[4][8] = { "UNKNOWN", "6581", "8580", "ANY"};

    if (!psid) {
        return;
    }

    psid->frames_played = 0;

    reloc_addr = psid->start_page << 8;

    log_message(vlog, "Driver=$%04X, Image=$%04X-$%04X, Init=$%04X, Play=$%04X", reloc_addr, psid->load_addr, psid->load_addr + psid->data_size - 1, psid->init_addr, psid->play_addr);

    /* PAL/NTSC. */
    resources_get_int("MachineVideoStandard", &sync);

    /* MOS6581/MOS8580. */
    resources_get_int("SidModel", &sid_model);

    /* Check tune number. */
    /* printf("start_song: %d psid->start_song %d\n", start_song, psid->start_song); */

    if (start_song == 0) {
        start_song = psid->start_song;
    } else if (start_song < 1 || start_song > psid->songs) {
        log_warning(vlog, "Tune out of range.");
        start_song = psid->start_song;
    }

    /* Check for PlaySID specific file. */
    if (psid->flags & 0x02 && !psid->is_rsid) {
        log_warning(vlog, "Image is PlaySID specific - trying anyway.");
    }

    /* Check tune speed. */
    speedbit = 1;
    for (i = 1; i < start_song && i < 32; i++) {
        speedbit <<= 1;
    }

    irq = psid->speed & speedbit ? "CIA 1" : "VICII";

    if (psid->play_addr) {
        strcpy(irq_str, irq);
    } else {
        sprintf(irq_str, "custom (%s ?)", irq);
    }

    if (console_mode) {
        log_message(vlog, "   Title: %s", (char *) psid->name);
        log_message(vlog, "  Author: %s", (char *) psid->author);
        log_message(vlog, "Released: %s", (char *) psid->copyright);
        log_message(vlog, "Using %s sync", sync == MACHINE_SYNC_PAL ? "PAL" : "NTSC");
        log_message(vlog, "SID model: %s", csidflag[(psid->flags >> 4) & 3]);
        log_message(vlog, "Using %s interrupt", irq_str);
        log_message(vlog, "Playing tune %d out of %d (default=%d)", start_song, psid->songs, psid->start_song);
    } else {
        if (machine_class == VICE_MACHINE_VSID) {
            char * driver_info_text;
            driver_info_text = lib_msprintf("Driver=$%04X, Image=$%04X-$%04X, Init=$%04X, Play=$%04X", reloc_addr, psid->load_addr,
                                            psid->load_addr + psid->data_size - 1, psid->init_addr, psid->play_addr);
            vsid_ui_setdrv(driver_info_text);
            lib_free(driver_info_text);
        }
        vsid_ui_display_name((char *)(psid->name));
        vsid_ui_display_author((char *)(psid->author));
        vsid_ui_display_copyright((char *)(psid->copyright));

        vsid_ui_display_sync(sync);
        vsid_ui_display_sid_model(sid_model);
        vsid_ui_display_irqtype(irq_str);
        vsid_ui_display_tune_nr(start_song);
        vsid_ui_set_default_tune(psid->start_song);
        vsid_ui_display_nr_of_tunes(psid->songs);
        vsid_ui_display_time(0);
    }

    /* Store parameters for PSID player. */
    if (install_driver_hook) {
        /* Skip JMP. */
        addr = reloc_addr + 3 + 9;

        /* CBM80 reset vector. */
        addr += psid_set_cbm80((WORD)(reloc_addr + 9), addr);

        ram_store(addr, (BYTE)(start_song));
    }

    /* put song number into address 780/1/2 (A/X/Y) for use by BASIC tunes */
    ram_store(780, (BYTE)(start_song - 1));
    ram_store(781, (BYTE)(start_song - 1));
    ram_store(782, (BYTE)(start_song - 1));
    /* force flag in c64 memory, many sids reads it and must be set AFTER the sid flag is read */
    ram_store((WORD)(0x02a6), (BYTE)(sync == MACHINE_SYNC_NTSC ? 0 : 1));
}

int psid_basic_rsid_to_autostart(WORD *address, BYTE **data, WORD *length) {
    if (psid && psid->is_rsid && psid->flags & 0x02) {
        *address = psid->load_addr;
        *data = psid->data;
        *length = psid->data_size;
        return 1;
    }
    return 0;
}

/* called from machine_play_psid */
void psid_set_tune(int tune)
{
    if (tune == -1) {
        psid_tune = 0;
        lib_free(psid);
        psid = NULL;
    } else {
        psid_tune = tune;
    }
}

/* used for setting the PSIDtune resource */
int psid_ui_set_tune(int tune, void *param)
{
    psid_tune = (int)(tune == -1) ? 0 : (int)tune;

    psid_set_tune(psid_tune);
    vsync_suspend_speed_eval();
    machine_trigger_reset(MACHINE_RESET_MODE_SOFT);

    return 0;
}

int psid_tunes(int* default_tune)
{
    *default_tune = psid ? psid->start_song : 0;

    return psid ? psid->songs : 0;
}

void psid_init_driver(void)
{
    BYTE psid_driver[] = {
#include "psiddrv.h"
    };
    char *psid_reloc = (char *)psid_driver;
    int psid_size;

    WORD reloc_addr;
    WORD addr;
    int i;
    int sync;
    int sid2loc, sid3loc;

    if (!psid) {
        return;
    }

    /* C64 PAL/NTSC flag. */
    resources_get_int("MachineVideoStandard", &sync);
    if (!keepenv) {
        switch ((psid->flags >> 2) & 0x03) {
            case 0x01:
                sync = MACHINE_SYNC_PAL;
                resources_set_int("MachineVideoStandard", sync);
                break;
            case 0x02:
                sync = MACHINE_SYNC_NTSC;
                resources_set_int("MachineVideoStandard", sync);
                break;
            default:
                /* Keep settings (00 = unknown, 11 = any) */
                break;
        }
    }

    /* Stereo SID specification support from Wilfred Bos.
     * Top byte of reserved holds the middle nybbles of
     * the 2nd chip address. */
    resources_set_int("SidStereo", 0);
    if (psid->version >= 3) {
        sid2loc = 0xd000 | ((psid->reserved >> 4) & 0x0ff0);
        log_message(vlog, "2nd SID at $%04x", sid2loc);
        if (((sid2loc >= 0xd420 && sid2loc < 0xd800) || sid2loc >= 0xde00)
            && (sid2loc & 0x10) == 0) {
            resources_set_int("SidStereo", 1);
            resources_set_int("SidStereoAddressStart", sid2loc);
        }
        sid3loc = 0xd000 | ((psid->reserved << 4) & 0x0ff0);
        if (sid3loc != 0xd000) {
            log_message(vlog, "3rd SID at $%04x", sid3loc);
            if (((sid3loc >= 0xd420 && sid3loc < 0xd800) || sid3loc >= 0xde00)
                && (sid3loc & 0x10) == 0) {
                resources_set_int("SidStereo", 2);
                resources_set_int("SidTripleAddressStart", sid3loc);
            }
        }
    }

    /* MOS6581/MOS8580 flag. */
    if (!keepenv) {
        switch ((psid->flags >> 4) & 0x03) {
            case 0x01:
                resources_set_int("SidModel", 0);
                break;
            case 0x02:
                resources_set_int("SidModel", 1);
                break;
            default:
                /* Keep settings (00 = unknown, 11 = any) */
                break;
        }
        /* FIXME: second chip model is ignored,
         * but it is stored at (flags >> 6) & 3. */
    }

    /* Clear low memory to minimize the damage of PSIDs doing bad reads. */
    for (addr = 0; addr < 0x0800; addr++) {
        ram_store(addr, (BYTE)0x00);
    }

    /* Relocation of C64 PSID driver code. */
    reloc_addr = psid->start_page << 8;
    psid_size = sizeof(psid_driver);
    log_message(vlog, "PSID free pages: $%04x-$%04x", reloc_addr, (reloc_addr + (psid->max_pages << 8)) -1);

    if (!reloc65((char **)&psid_reloc, &psid_size, reloc_addr)) {
        log_error(vlog, "Relocation.");
        psid_set_tune(-1);
        return;
    }

    for (i = 0; i < psid_size; i++) {
        ram_store((WORD)(reloc_addr + i), psid_reloc[i]);
    }

    /* Store binary C64 data. */
    for (i = 0; i < psid->data_size; i++) {
        ram_store((WORD)(psid->load_addr + i), psid->data[i]);
    }

    /* Skip JMP and CBM80 reset vector. */
    addr = reloc_addr + 3 + 9 + 9;

    /* Store parameters for PSID player. */
    ram_store(addr++, (BYTE)(0));
    ram_store(addr++, (BYTE)(psid->songs));
    ram_store(addr++, (BYTE)(psid->load_addr & 0xff));
    ram_store(addr++, (BYTE)(psid->load_addr >> 8));
    ram_store(addr++, (BYTE)(psid->init_addr & 0xff));
    ram_store(addr++, (BYTE)(psid->init_addr >> 8));
    ram_store(addr++, (BYTE)(psid->play_addr & 0xff));
    ram_store(addr++, (BYTE)(psid->play_addr >> 8));
    ram_store(addr++, (BYTE)(psid->speed & 0xff));
    ram_store(addr++, (BYTE)((psid->speed >> 8) & 0xff));
    ram_store(addr++, (BYTE)((psid->speed >> 16) & 0xff));
    ram_store(addr++, (BYTE)(psid->speed >> 24));
    ram_store(addr++, (BYTE)((int)sync == MACHINE_SYNC_PAL ? 1 : 0));
    ram_store(addr++, (BYTE)(psid->load_last_addr & 0xff));
    ram_store(addr++, (BYTE)(psid->load_last_addr >> 8));
}

unsigned int psid_increment_frames(void)
{
    if (!psid) {
        return 0;
    }

    (psid->frames_played)++;

    return (unsigned int)(psid->frames_played);
}

/******************************************************************************
 * compute sidplayer (.mus/.str) support
 *
 * to minimize code duplication and to simplify the integration with the rest
 * of the code, the sidplayer data is simply converted into PSID like format 
 * at load time. heavily inspired by the respective code in libsidplay2.
 ******************************************************************************/

#define MUS_HLT_CMD      0x014F

#define MUS_IMAGE_START  0x0900
#define MUS_DATA_ADDR    0x0900
#define MUS_DATA2_ADDR   0x6900
#define MUS_DATA_MAXLEN  0x6000

#define MUS_DRIVER_ADDR  0xe000
#define MUS_DRIVER2_ADDR 0xf000

#define MUS_SID1_BASE_ADDR   0xd400
#define MUS_SID2_BASE_ADDR   0xd500

#include "musdrv.h"

static void mus_install(void)
{
    WORD dest;
    /* Install MUS player #1. */
    dest = ((mus_driver[1] << 8) | mus_driver[0]) - MUS_IMAGE_START;
    memcpy(psid->data + dest, mus_driver + 2, sizeof(mus_driver) - 2);
    /* Point player #1 to data #1. */
    psid->data[dest + 0xc6e] = MUS_DATA_ADDR & 0xFF;
    psid->data[dest + 0xc70] = MUS_DATA_ADDR >> 8;

    /* Install MUS player #2. It doesnt hurt to do it also for mono tunes. */
    dest = ((mus_stereo_driver[1] << 8) | mus_stereo_driver[0]) - MUS_IMAGE_START;
    memcpy(psid->data + dest, mus_stereo_driver + 2, sizeof(mus_stereo_driver) - 2);
    /* Point player #2 to data #2. */
    psid->data[dest + 0xc6e] = MUS_DATA2_ADDR & 0xFF;
    psid->data[dest + 0xc70] = MUS_DATA2_ADDR >> 8;
}

static int mus_check(const unsigned char *buf)
{
    unsigned int voice1Index, voice2Index, voice3Index;

    /* Skip 3x length entry. */
    voice1Index = ((buf[1] << 8) | buf[0]) + (3 * 2);
    voice2Index = voice1Index + ((buf[3] << 8) | buf[2]);
    voice3Index = voice2Index + ((buf[5] << 8) | buf[4]);
    return (((buf[voice1Index - 2] << 8) | buf[voice1Index - 1]) == MUS_HLT_CMD)
        && (((buf[voice2Index - 2] << 8) | buf[voice2Index - 1]) == MUS_HLT_CMD)
        && (((buf[voice3Index - 2] << 8) | buf[voice3Index - 1]) == MUS_HLT_CMD);
}

/* check for graphic characters */
static int ispetchar(unsigned char c)
{
    if ((c >= 32) && (c <= 93)) {
        return 1;
    }
    return 0;
}

static const unsigned char *copystring(unsigned char *d, const unsigned char *s)
{
    unsigned char *end = d + 32;
    int n = 0;
    /* skip leading spaces and special characters */
    while (((*s == 0x20) || !ispetchar(*s)) && (*s != 0x00)) {
        ++s;
    }
    /* copy until end of line, omit special characters */
    while ((*s != 0x0d) && (*s != 0x00)) {
        if (ispetchar(*s)) {
            *d++ = *s;
            n = (*s == 0x20);
        } else {
            /* for special chars, insert one space */
            if (!n) {
                *d++ = 0x20;
                n = 1;
            }
        }
        /* if max len of destination is reached, skip until end of source */
        if (d == end) {
            while ((*s != 0x0d) && (*s != 0x00)) {
                ++s;
            }
            break;
        }
        ++s;
    }
    *d = 0;
    charset_petconvstring(d, 1);
    return s + 1;
}

static void mus_extract_credits(const unsigned char *buf, int datalen)
{
    const unsigned char *end;
    int n;

    /* get offset behind note data */
    n = ((buf[1] << 8) | buf[0]) + (3 * 2);
    n += ((buf[3] << 8) | buf[2]);
    n += ((buf[5] << 8) | buf[4]);

    end = buf + datalen;
    buf += n;

    psid->name[0] = psid->author[0] = psid->copyright[0] = 0;

    if (buf < end) {
        buf = copystring(psid->name, buf);
    }
    if (buf < end) {
        buf = copystring(psid->author, buf);
    }
    if (buf < end) {
        buf = copystring(psid->copyright, buf);
    }
}

static int mus_load_file(const char* filename, int ispsid)
{
    char *strname;
    FILE *f;
    int n, stereo = 0;
    int mus_datalen;

    if (!(f = zfile_fopen(filename, MODE_READ))) {
        return -1;
    }

    if (!ispsid) {
        lib_free(psid);
        psid = lib_calloc(sizeof(psid_t), 1);
    }

    /* skip header & original load address */
    fseek(f, psid->data_offset + (psid->load_addr ? 0 : 2), SEEK_SET);
    /* read .mus data */
    mus_datalen = fread(psid->data, 1, 0xffff - MUS_DATA_MAXLEN, f);

    if (!mus_check(psid->data)) {
        log_error(vlog, "not a valid .mus file.");
        goto fail;
    }
    zfile_fclose(f);

    /* read additional stereo (.str) data if available */
    /* FIXME: the psid file format specification does not tell how to handle
              stereo sidplayer tunes when they are in psid format */
    if (!ispsid) {
        strname = lib_stralloc(filename);
        n = strlen(strname) - 4;
        strcpy(strname + n, ".str");

        if ((f = zfile_fopen(strname, MODE_READ))) {
            fseek(f, 2, SEEK_SET); /* skip original load address */
            if (fread(psid->data + (MUS_DATA2_ADDR - MUS_IMAGE_START), 1, 0xffff - MUS_DATA_MAXLEN, f) < (3 * 2)) {
                goto fail;
            }
            zfile_fclose(f);
            stereo = 1;
        }
        lib_free(strname);
        /* only extract credits if this is NOT a psid file */
        mus_extract_credits(psid->data, mus_datalen);
    }

    mus_install();

    psid->version = 3;  /* v3 so we get stereo support */
    psid->flags = 2 << 2; /* NTSC */
    psid->start_page = 0x04;
    psid->max_pages = (MUS_DATA_ADDR - 0x0400) >> 8;

    psid->load_addr = MUS_IMAGE_START;
    psid->data_size = 0x10000 - MUS_IMAGE_START;

    psid->songs = 1;
    psid->start_song = 1;
    psid->speed = 1;    /* play at 60Hz */

    if (stereo) {
        /* Player #1 + #2. */
        psid->reserved = (MUS_SID2_BASE_ADDR << 4) & 0xff00; /* second SID at 0xd500 */
        psid->init_addr = 0xfc90;
        psid->play_addr = 0xfc96;
    } else {
        /* Player #1. */
        psid->init_addr = 0xec60;
        psid->play_addr = 0xec80;
    }

    return 0;

fail:
    zfile_fclose(f);
    lib_free(psid);
    psid = NULL;
    return -1;
}
