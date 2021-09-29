/*
 * pet-cmdline-options.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Andre Fachat <fachat@physik.tu-chemnitz.de>
 *  Ettore Perazzoli <ettore@comm2000.it>
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

#include "cmdline.h"
#include "machine.h"
#include "pet-cmdline-options.h"
#include "pet.h"
#include "pets.h"
#include "resources.h"

static const cmdline_option_t cmdline_options[] =
{
    { "-pal", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_PAL,
      NULL, "Use PAL sync factor" },
    { "-ntsc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MachineVideoStandard", (void *)MACHINE_SYNC_NTSC,
      NULL, "Use NTSC sync factor" },
    { "-model", CALL_FUNCTION, CMDLINE_ATTRIB_NEED_ARGS,
      pet_set_model, NULL, NULL, NULL,
      "<modelnumber>", "Specify PET model to emulate. (2001/3008/3016/3032/3032B/4016/4032/4032B/8032/8096/8296/SuperPET)" },
    { "-kernal", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "KernalName", NULL,
      "<Name>", "Specify name of Kernal ROM image" },
    { "-basic", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "BasicName", NULL,
      "<Name>", "Specify name of BASIC ROM image" },
    { "-editor", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "EditorName", NULL,
      "<Name>", "Specify name of Editor ROM image" },
    { "-chargen", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "ChargenName", NULL,
      "<Name>", "Specify name of character generator ROM image" },
    { "-rom9", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RomModule9Name", NULL,
      "<Name>", "Specify 4KiB extension ROM name at $9***" },
    { "-romA", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RomModuleAName", NULL,
      "<Name>", "Specify 4KiB extension ROM name at $A***" },
    { "-romB", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RomModuleBName", NULL,
      "<Name>", "Specify 4KiB extension ROM name at $B***" },
    { "-petram9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram9", (void *)1,
      NULL, "Enable PET8296 4KiB RAM mapping at $9***" },
    { "+petram9", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Ram9", (void *)0,
      NULL, "Disable PET8296 4KiB RAM mapping at $9***" },
    { "-petramA", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RamA", (void *)1,
      NULL, "Enable PET8296 4KiB RAM mapping at $A***" },
    { "+petramA", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "RamA", (void *)0,
      NULL, "Disable PET8296 4KiB RAM mapping at $A***" },
    { "-superpet", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SuperPET", (void *)1,
      NULL, "Enable SuperPET I/O" },
    { "+superpet", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "SuperPET", (void *)0,
      NULL, "Disable SuperPET I/O" },
    { "-basic1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Basic1", (void *)1,
      NULL, "Enable ROM 1 Kernal patches" },
    { "+basic1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Basic1", (void *)0,
      NULL, "Disable ROM 1 Kernal patches" },
    { "-basic1char", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Basic1Chars", (void *)1,
      NULL, "Switch upper/lower case charset" },
    { "+basic1char", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Basic1Chars", (void *)0,
      NULL, "Do not switch upper/lower case charset" },
    { "-eoiblank", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "EoiBlank", (void *)1,
      NULL, "EOI blanks screen" },
    { "+eoiblank", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "EoiBlank", (void *)0,
      NULL, "EOI does not blank screen" },
    { "-cpu6502", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_6502,
      NULL, "Set SuperPET CPU switch to '6502'" },
    { "-cpu6809", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_6809,
      NULL, "Set SuperPET CPU switch to '6809'" },
    { "-cpuprog", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "CPUswitch", (void *)SUPERPET_CPU_PROG,
      NULL, "Set SuperPET CPU switch to 'Prog'" },
    { "-6809romA", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomAName", NULL,
      "<Name>", "Specify 4KiB to 24KiB ROM file name at $A000 for 6809" },
    { "-6809romB", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomBName", NULL,
      "<Name>", "Specify 4KiB to 20KiB ROM file name at $B000 for 6809" },
    { "-6809romC", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomCName", NULL,
      "<Name>", "Specify 4KiB to 16KiB ROM file name at $C000 for 6809" },
    { "-6809romD", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomDName", NULL,
      "<Name>", "Specify 4KiB to 12KiB ROM file name at $D000 for 6809" },
    { "-6809romE", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomEName", NULL,
      "<Name>", "Specify 2KiB or 8KiB ROM file name at $E000 for 6809" },
    { "-6809romF", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "H6809RomFName", NULL,
      "<Name>", "Specify 4KiB ROM file name at $F000 for 6809" },
    { "-colour-rgbi", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PETColour", (void *)PET_COLOUR_TYPE_RGBI,
      NULL, "RGBI colour extension to PET 4032" },
    { "-colour-analog", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "PETColour", (void *)PET_COLOUR_TYPE_ANALOG,
      NULL, "Analog colour extension to PET 4032" },
    { "-colour-analog-bg", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "PETColourBG", NULL,
      "<Colour 0-255>", "Analog colour background on PET 4032" },
    { "-ramsize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "RamSize", NULL,
      "<size in KiB>", "PET RAM size (4/8/16/32/96/128)" },
    { "-iosize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IOSize", NULL,
      "<size>", "PET I/O size (256/2048)" },
    { "-crtc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Crtc", (void *)1,
      NULL, "Enable CRTC" },
    { "+crtc", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "Crtc", (void *)0,
      NULL, "Disable CRTC" },
    { "-videosize", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "VideoSize", NULL,
      "<size>", "Set video size (0: Automatic, 40: 40 Columns, 80: 80 Columns)" },
    CMDLINE_LIST_END
};

int pet_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}
