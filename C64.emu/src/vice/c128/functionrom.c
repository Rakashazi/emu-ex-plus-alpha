/*
 * functionrom.c
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
#include "bq4830y.h"
#include "c128mem.h"
#include "cmdline.h"
#include "functionrom.h"
#include "lib.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "util.h"


#define INTERNAL_FUNCTION_ROM_SIZE 0x8000
#define EXTERNAL_FUNCTION_ROM_SIZE 0x8000

/* Flag: Do we enable the internal function ROM?  */
static int internal_function_rom_enabled;

/* Name of the internal function ROM.  */
static char *internal_function_rom_name = NULL;

/* Image of the internal function ROM.  */
BYTE int_function_rom[INTERNAL_FUNCTION_ROM_SIZE];

/* Flag: Do we enable the external function ROM?  */
/* New meaning: 0=no, 1=yes (rom), 2=yes (ram), 3=yes (rtc) */
static int external_function_rom_enabled;

/* Name of the external function ROM.  */
static char *external_function_rom_name = NULL;

/* Image of the external function ROM.  */
BYTE ext_function_rom[EXTERNAL_FUNCTION_ROM_SIZE];

/* Flag: Do we save RTC info when changed? */
static int internal_function_rtc_save;
static int external_function_rtc_save;

/* Some prototypes are needed */
static int functionrom_load_internal(void);
static int functionrom_load_external(void);

/* bq4830y context */
static rtc_bq4830y_t *rtc1_context = NULL;
static rtc_bq4830y_t *rtc2_context = NULL;

static int set_internal_function_rom_enabled(int val, void *param)
{
    if (internal_function_rom_enabled == val) {
        return 0;
    }

    switch (val) {
        case INT_FUNCTION_NONE:
        case INT_FUNCTION_ROM:
        case INT_FUNCTION_RAM:
        case INT_FUNCTION_RTC:
            break;
        default:
            return -1;
    }

    if (internal_function_rom_enabled == INT_FUNCTION_RTC && rtc1_context) {
        bq4830y_destroy(rtc1_context, internal_function_rtc_save);
        rtc1_context = NULL;
    }

    internal_function_rom_enabled = val;

    switch (val) {
        case INT_FUNCTION_RTC:
            rtc1_context = bq4830y_init("IFR");
        case INT_FUNCTION_RAM:
            memset(int_function_rom, 0, sizeof(int_function_rom));
            break;
        default:
            return functionrom_load_internal();
            break;
    }
    return 0;
}

static int set_internal_function_rtc_save(int val, void *param)
{
    internal_function_rtc_save = val ? 1 : 0;

    return 0;
}

static int set_internal_function_rom_name(const char *val, void *param)
{
    if (util_string_set(&internal_function_rom_name, val)) {
        return 0;
    }
    return functionrom_load_internal();
}

static int set_external_function_rom_enabled(int val, void *param)
{
    if (external_function_rom_enabled == val) {
        return 0;
    }

    switch (val) {
        case EXT_FUNCTION_NONE:
        case EXT_FUNCTION_ROM:
        case EXT_FUNCTION_RAM:
        case EXT_FUNCTION_RTC:
            break;
        default:
            return -1;
    }

    if (external_function_rom_enabled == EXT_FUNCTION_RTC && rtc2_context) {
        bq4830y_destroy(rtc2_context, external_function_rtc_save);
        rtc2_context = NULL;
    }

    external_function_rom_enabled = val;

    switch (val) {
        case EXT_FUNCTION_RTC:
            rtc2_context = bq4830y_init("EFR");
        case EXT_FUNCTION_RAM:
            memset(ext_function_rom, 0, sizeof(ext_function_rom));
            break;
        default:
            return functionrom_load_external();
            break;
    }
    return 0;
}

static int set_external_function_rtc_save(int val, void *param)
{
    external_function_rtc_save = val ? 1 : 0;

    return 0;
}

static int set_external_function_rom_name(const char *val, void *param)
{
    if (util_string_set(&external_function_rom_name, val)) {
        return 0;
    }
    return functionrom_load_external();
}

static const resource_string_t resources_string[] =
{
    { "InternalFunctionName", "", RES_EVENT_NO, NULL,
      &internal_function_rom_name,
      set_internal_function_rom_name, NULL },
    { "ExternalFunctionName", "", RES_EVENT_NO, NULL,
      &external_function_rom_name,
      set_external_function_rom_name, NULL },
    { NULL }
};

static const resource_int_t resources_int[] =
{
    { "InternalFunctionROM", INT_FUNCTION_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &internal_function_rom_enabled,
      set_internal_function_rom_enabled, NULL },
    { "ExternalFunctionROM", EXT_FUNCTION_NONE, RES_EVENT_STRICT, (resource_value_t)0,
      &external_function_rom_enabled,
      set_external_function_rom_enabled, NULL },
    { "InternalFunctionROMRTCSave", 0, RES_EVENT_NO, NULL,
      &internal_function_rtc_save,
      set_internal_function_rtc_save, NULL },
    { "ExternalFunctionROMRTCSave", 0, RES_EVENT_NO, NULL,
      &external_function_rtc_save,
      set_external_function_rtc_save, NULL },
    { NULL }
};

int functionrom_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    return resources_register_int(resources_int);
}

void functionrom_resources_shutdown(void)
{
    if (rtc1_context) {
        bq4830y_destroy(rtc1_context, internal_function_rtc_save);
        rtc1_context = NULL;
    }
    if (rtc2_context) {
        bq4830y_destroy(rtc2_context, external_function_rtc_save);
        rtc2_context = NULL;
    }
    lib_free(internal_function_rom_name);
    lib_free(external_function_rom_name);
}

static const cmdline_option_t cmdline_options[] = {
    { "-intfrom", SET_RESOURCE, 1,
      NULL, NULL, "InternalFunctionName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_INT_FUNC_ROM_NAME,
      NULL, NULL },
    { "-extfrom", SET_RESOURCE, 1,
      NULL, NULL, "ExternalFunctionName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_SPECIFY_EXT_FUNC_ROM_NAME,
      NULL, NULL },
    { "-intfunc", SET_RESOURCE, 1,
      NULL, NULL, "InternalFunctionROM", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_ENABLE_INT_FUNC_ROM,
      NULL, NULL },
    { "-extfunc", SET_RESOURCE, 1,
      NULL, NULL, "ExternalFunctionROM", NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_P_TYPE, IDCLS_ENABLE_EXT_FUNC_ROM,
      NULL, NULL },
    { "-intfuncrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "InternalFunctionROMRTCSave", (resource_value_t)1,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_INT_FUNC_RTC_SAVE,
      NULL, NULL },
    { "+intfuncrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "InternalFunctionROMRTCSave", (resource_value_t)0,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_INT_FUNC_RTC_SAVE,
      NULL, NULL },
    { "-extfuncrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "ExternalFunctionROMRTCSave", (resource_value_t)1,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_EXT_FUNC_RTC_SAVE,
      NULL, NULL },
    { "+extfuncrtcsave", SET_RESOURCE, 0,
      NULL, NULL, "ExternalFunctionROMRTCSave", (resource_value_t)0,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_EXT_FUNC_RTC_SAVE,
      NULL, NULL },
    { NULL }
};

int functionrom_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

static int functionrom_load_internal(void)
{
    if (internal_function_rom_enabled == INT_FUNCTION_ROM) {
        if (util_check_null_string(internal_function_rom_name)) {
            return 0;
        }

        if (util_file_load(internal_function_rom_name, int_function_rom,
                           INTERNAL_FUNCTION_ROM_SIZE,
                           UTIL_FILE_LOAD_SKIP_ADDRESS | UTIL_FILE_LOAD_FILL) < 0) {
            return -1;
        }
    } else if (internal_function_rom_enabled == INT_FUNCTION_NONE) {
        memset(int_function_rom, 0, sizeof(int_function_rom));
    }

    return 0;
}

static int functionrom_load_external(void)
{
    if (external_function_rom_enabled) {
        if (util_check_null_string(external_function_rom_name)) {
            return 0;
        }

        if (util_file_load(external_function_rom_name, ext_function_rom,
                           EXTERNAL_FUNCTION_ROM_SIZE,
                           UTIL_FILE_LOAD_SKIP_ADDRESS | UTIL_FILE_LOAD_FILL) < 0) {
            return -1;
        }
    } else {
        memset(ext_function_rom, 0, sizeof(ext_function_rom));
    }

    return 0;
}

BYTE internal_function_rom_read(WORD addr)
{
    if (internal_function_rom_enabled == INT_FUNCTION_RTC) {
        return bq4830y_read(rtc1_context, (WORD)(addr & 0x7fff));
    }
    return int_function_rom[addr & (INTERNAL_FUNCTION_ROM_SIZE - 1)];
}

void internal_function_rom_store(WORD addr, BYTE value)
{
    if (internal_function_rom_enabled == INT_FUNCTION_RTC) {
        bq4830y_store(rtc1_context, (WORD)(addr & 0x7fff), value);
        ram_store(addr, value);
    } else if (internal_function_rom_enabled == INT_FUNCTION_RAM) {
        int_function_rom[addr & (INTERNAL_FUNCTION_ROM_SIZE - 1)] = value;
        ram_store(addr, value);
    } else {
        ram_store(addr, value);
    }
}

void internal_function_top_shared_store(WORD addr, BYTE value)
{
    if (internal_function_rom_enabled == INT_FUNCTION_RTC) {
        bq4830y_store(rtc1_context, (WORD)(addr & 0x7fff), value);
        top_shared_store(addr, value);
    } else if (internal_function_rom_enabled == INT_FUNCTION_RAM) {
        int_function_rom[addr & (INTERNAL_FUNCTION_ROM_SIZE - 1)] = value;
        top_shared_store(addr, value);
    } else {
        top_shared_store(addr, value);
    }
}

BYTE external_function_rom_read(WORD addr)
{
    if (external_function_rom_enabled == EXT_FUNCTION_RTC) {
        return bq4830y_read(rtc2_context, (WORD)(addr & 0x7fff));
    }
    return ext_function_rom[addr & (EXTERNAL_FUNCTION_ROM_SIZE - 1)];
}

void external_function_rom_store(WORD addr, BYTE value)
{
    if (external_function_rom_enabled == EXT_FUNCTION_RTC) {
        bq4830y_store(rtc2_context, (WORD)(addr & 0x7fff), value);
        ram_store(addr, value);
    } else if (external_function_rom_enabled == EXT_FUNCTION_RAM) {
        ext_function_rom[addr & (EXTERNAL_FUNCTION_ROM_SIZE - 1)] = value;
        ram_store(addr, value);
    } else {
        ram_store(addr, value);
    }
}

void external_function_top_shared_store(WORD addr, BYTE value)
{
    if (external_function_rom_enabled == INT_FUNCTION_RTC) {
        bq4830y_store(rtc2_context, (WORD)(addr & 0x7fff), value);
        top_shared_store(addr, value);
    } else if (external_function_rom_enabled == INT_FUNCTION_RAM) {
        ext_function_rom[addr & (INTERNAL_FUNCTION_ROM_SIZE - 1)] = value;
        top_shared_store(addr, value);
    } else {
        top_shared_store(addr, value);
    }
}
