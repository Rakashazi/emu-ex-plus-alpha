/*
 * petmodel.c - PET model detection and setting.
 *
 * Written by
 *  groepaz <groepaz@gmx.net>
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

#include "machine.h"
#include "pet-resources.h"
#include "petmem.h"
#include "petmodel.h"
#include "pets.h"
#include "resources.h"
#include "uiapi.h"
#include "vsync.h"

/* #define DEBUGPET */

#ifdef DEBUGPET
#define DBG(x)  printf x
#else
#define DBG(x)
#endif

#define PET_CHARGEN_NAME        "chargen"
#define SUPERPET_CHARGEN_NAME   "characters.901640-01.bin"

int pet_init_ok = 0; /* set to 1 in pet.c */

/* ------------------------------------------------------------------------- */

/*
 * table with Model information
 */
struct pet_table_s {
    const char *model;
    petinfo_t info;
};
typedef struct pet_table_s pet_table_t;

static pet_table_t pet_table[] = {
    { "2001",
      { RAM_8K, IO_2048, NO_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        PATCH_2K_KERNAL, PATCH_2K_CHARGEN, EOI_BLANKS, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL1NAME, PET_EDITOR1G40NAME, PET_BASIC1NAME,
        NULL, NULL, NULL } },
    { "3008",
      { RAM_8K, IO_2048, NO_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3016",
      { RAM_16K, IO_2048, NO_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3032",
      { RAM_32K, IO_2048, NO_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2G40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "3032B",
      { RAM_32K, IO_2048, NO_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL2NAME, PET_EDITOR2B40NAME, PET_BASIC2NAME,
        NULL, NULL, NULL } },
    { "4016",
      { RAM_16K, IO_256, HAS_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4G40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "4032",
      { RAM_32K, IO_256, HAS_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_GRAPHICS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4G40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "4032B",
      { RAM_32K, IO_256, HAS_CRTC, COLS_40, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B40NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8032",
      { RAM_32K, IO_256, HAS_CRTC, COLS_80, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8096",
      { RAM_96K, IO_256, HAS_CRTC, COLS_80, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "8296",
      { RAM_128K, IO_256, HAS_CRTC, COLS_80, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, NORMAL_IO,
        PET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL } },
    { "SuperPET",
      { RAM_32K, IO_2048, HAS_CRTC, COLS_80, NO_RAM_9, NO_RAM_A, KBD_TYPE_BUSINESS_US,
        NO_KERNAL_PATCH, NO_CHARGEN_PATCH, NO_EOI, SUPERPET_IO,
        SUPERPET_CHARGEN_NAME, PET_KERNAL4NAME, PET_EDITOR4B80NAME, PET_BASIC4NAME,
        NULL, NULL, NULL,
        { "waterloo-a000.901898-01.bin",
          "waterloo-b000.901898-02.bin",
          "waterloo-c000.901898-03.bin",
          "waterloo-d000.901898-04.bin",
          "waterloo-e000.901897-01.bin",
          "waterloo-f000.901898-05.bin" }} },
    { NULL }
};

static int petmem_get_conf_info(petinfo_t *pi)
{
    int ktype;

    if ((resources_get_int("RamSize", &pi->ramSize) < 0)
        || (resources_get_int("IOSize", &pi->IOSize) < 0)
        || (resources_get_int("Crtc", &pi->crtc) < 0)
        || (resources_get_int("Ram9", &pi->ramsel9) < 0)
        || (resources_get_int("RamA", &pi->ramselA) < 0)
        || (resources_get_int("EoiBlank", &pi->eoiblank) < 0)
        || (resources_get_int("SuperPET", &pi->superpet) < 0)
        || (resources_get_int("KeyboardType", &ktype) < 0)) {
        return -1;
    }

    pi->video = petmem_get_screen_columns();
    pi->kbd_type = ktype;
    return 0;
}

int petmem_set_conf_info(petinfo_t *pi)
{
    resources_set_int("RamSize", pi->ramSize);
    resources_set_int("IOSize", pi->IOSize);
    resources_set_int("Crtc", pi->crtc);
    resources_set_int("VideoSize", pi->video);
    resources_set_int("Ram9", pi->ramsel9);
    resources_set_int("RamA", pi->ramselA);
    resources_set_int("EoiBlank", pi->eoiblank);
    resources_set_int("SuperPET", pi->superpet);
    resources_set_int("KeyboardType", pi->kbd_type);
    return 0;
}

static int pet_set_model_info(petinfo_t *pi)
{
    /* set hardware config */
    petmem_set_conf_info(pi);

    if (pi->pet2k) {    /* set resource only when necessary */
        resources_set_int("Basic1", pi->pet2k);
    }
    resources_set_int("Basic1Chars", pi->pet2kchar);

    resources_set_string("ChargenName", pi->chargenName);
    resources_set_string("KernalName", pi->kernalName);
    resources_set_string("BasicName", pi->basicName);
    resources_set_string("EditorName", pi->editorName);

    /* allow additional ROMs to survive a model switch. */
    if (pi->mem9name) {
        resources_set_string("RomModule9Name", pi->mem9name);
    }
    if (pi->memAname) {
        resources_set_string("RomModuleAName", pi->memAname);
    }
    if (pi->memBname) {
        resources_set_string("RomModuleBName", pi->memBname);
    }
    if (pi->superpet) {
        int i;

        for (i = 0; i < NUM_6809_ROMS; i++) {
            if (pi->h6809romName[i]) {
                resources_set_string_sprintf("H6809Rom%cName", pi->h6809romName[i], 'A' + i);
            }
        }
    }
    return 0;
}

/* FIXME: this one should only be used by commandline */
int pet_set_model(const char *model_name, void *extra)
{
    int i;

    i = 0;
    while (pet_table[i].model) {
        if (!strcmp(pet_table[i].model, model_name)) {
            petmodel_set(i);
            ui_update_menus();
            return 0;
        }
        i++;
    }

    return -1;
}

static int petmodel_get_temp(petinfo_t *pi)
{
    int i;

    for (i = 0; i < PETMODEL_NUM; ++i) {
        if ((pet_table[i].info.ramSize == pi->ramSize)
            && (pet_table[i].info.IOSize == pi->IOSize)
            && (pet_table[i].info.crtc == pi->crtc)
            && (pet_table[i].info.video == pi->video)
            && (pet_table[i].info.eoiblank == pi->eoiblank)
            && (pet_table[i].info.superpet == pi->superpet)
            && (pet_table[i].info.kbd_type == pi->kbd_type)) {
            if ((pet_table[i].info.ramsel9 != pi->ramsel9)
                && (i != PETMODEL_8296)) {
                continue;
            }
            if ((pet_table[i].info.ramselA != pi->ramselA)
                && (i != PETMODEL_8296)) {
                continue;
            }
            return i;
        }
    }

    return PETMODEL_UNKNOWN;
}

int petmodel_get(void)
{
    petinfo_t pi;

    if (petmem_get_conf_info(&pi) < 0) {
        return -1;
    }
    DBG(("ramSize: %d IOSize: %d crtc: %d video: %d eoiblank: %d superpet: %d kbd_type: %d\n",
           pi.ramSize, pi.IOSize, pi.crtc, pi.video, pi.eoiblank, pi.superpet, pi.kbd_type));

    return petmodel_get_temp(&pi);
}

void petmodel_set(int model)
{
    if (model >= PETMODEL_NUM) {
        return;
    }

    petres.video = -1; /* force reinitialization in pet-resources.c:set_video, see bug #3496413 */
    pet_set_model_info(&pet_table[model].info);

    /* we have to wait until we have done enough initialization */
    if (pet_init_ok) {
        /* mem_load(); - not needed as resources now load */
        vsync_suspend_speed_eval();
        machine_trigger_reset(MACHINE_RESET_MODE_SOFT);
    }
}
