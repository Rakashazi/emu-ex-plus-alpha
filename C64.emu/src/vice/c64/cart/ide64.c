/*
 * ide64.c - Cartridge handling, IDE64 cart.
 *
 * Written by
 *  Kajtar Zsolt <soci@c64.rulez.org>
 *
 * Real-Time-Clock patches by
 *  Greg King <greg.king4@verizon.net>
 *
 * Shortbus and clockport emulation by
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

/* required for off_t on some platforms */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* VAC++ has off_t in sys/stat.h */
#ifdef __IBMC__
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alarm.h"
#include "archdep.h"
#include "ata.h"
#define CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "c64cartsystem.h"
#undef CARTRIDGE_INCLUDE_SLOTMAIN_API
#include "cartio.h"
#include "cartridge.h"
#include "clockport.h"
#include "cmdline.h"
#include "crt.h"
#include "ds1202_1302.h"
#include "export.h"
#include "ide64.h"
#include "log.h"
#include "lib.h"
#include "machine.h"
#include "maincpu.h"
#include "monitor.h"
#include "resources.h"
#include "shortbus.h"
#include "snapshot.h"
#include "types.h"
#include "util.h"
#include "vicesocket.h"
#include "vicii-phi1.h"

#ifdef IDE64_DEBUG
#define debug(x) log_debug(x)
#else
#define debug(x)
#endif

#ifndef HAVE_FSEEKO
#define fseeko(a, b, c) fseek(a, b, c)
#define ftello(a) ftell(a)
#endif

#define LATENCY_TIMER 4000

static int ide64_enabled = 0;

/* Current IDE64 bank */
static int current_bank;

/* Current memory config */
static int current_cfg;

/* ds1302 context */
static rtc_ds1202_1302_t *ds1302_context = NULL;

/*  */
static uint8_t kill_port;

static struct drive_s {
    ata_drive_t *drv;
    char *filename;
    ata_drive_geometry_t settings;
    int autodetect_size;
    ata_drive_type_t type;
    ata_drive_geometry_t detected;
    int update_needed;
} drives[4];

static int idrive = 0;

/* communication latch */
static uint16_t out_d030, in_d030, idebus;

#ifdef HAVE_NETWORK
static struct alarm_s *usb_alarm;
static char *settings_usbserver_address = NULL;
static vice_network_socket_t * usbserver_socket = NULL;
static vice_network_socket_t * usbserver_asocket = NULL;
static int settings_usbserver;
static uint8_t ft245_rx[256], ft245_tx[128];
static int ft245_rxp, ft245_rxl, ft245_txp;
#else
#define usbserver_activate(a) {}
#endif

static int settings_version;
static int ide64_rtc_save;

/* Current clockport device */
static int clockport_device_id = CLOCKPORT_DEVICE_NONE;
static clockport_device_t *clockport_device = NULL;

static const char STRING_IDE64_CLOCKPORT[] = CARTRIDGE_NAME_IDE64 " Clockport";

static char *clockport_device_names = NULL;

/* ---------------------------------------------------------------------*/

/* some prototypes are needed */
static void ide64_idebus_store(uint16_t addr, uint8_t value);
static uint8_t ide64_idebus_read(uint16_t addr);
static uint8_t ide64_idebus_peek(uint16_t addr);
static int ide64_idebus_dump(void);
static void ide64_io_store(uint16_t addr, uint8_t value);
static uint8_t ide64_io_read(uint16_t addr);
static uint8_t ide64_io_peek(uint16_t addr);
static int ide64_io_dump(void);
static void ide64_ft245_store(uint16_t addr, uint8_t value);
static uint8_t ide64_ft245_read(uint16_t addr);
static uint8_t ide64_ft245_peek(uint16_t addr);
static void ide64_ds1302_store(uint16_t addr, uint8_t value);
static uint8_t ide64_ds1302_read(uint16_t addr);
static uint8_t ide64_ds1302_peek(uint16_t addr);
static void ide64_romio_store(uint16_t addr, uint8_t value);
static uint8_t ide64_romio_read(uint16_t addr);
static uint8_t ide64_romio_peek(uint16_t addr);
static uint8_t ide64_clockport_read(uint16_t io_address);
static uint8_t ide64_clockport_peek(uint16_t io_address);
static void ide64_clockport_store(uint16_t io_address, uint8_t byte);
static int ide64_clockport_dump(void);
static int ide64_rtc_dump(void);

static io_source_t ide64_idebus_device = {
    CARTRIDGE_NAME_IDE64 " IDE", /* name of the device */
    IO_DETACH_CART,              /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,       /* does not use a resource for detach */
    0xde20, 0xde2f, 0x0f,        /* range for the device, regs:$de20-$de2f */
    0,                           /* read validity is determined by the device upon a read */
    ide64_idebus_store,          /* store function */
    NULL,                        /* NO poke function */
    ide64_idebus_read,           /* read function */
    ide64_idebus_peek,           /* peek function */
    ide64_idebus_dump,           /* device state information dump function */
    CARTRIDGE_IDE64,             /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0                            /* insertion order, gets filled in by the registration function */
};

static io_source_t ide64_io_device = {
    CARTRIDGE_NAME_IDE64 " I/O", /* name of the device */
    IO_DETACH_CART,              /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,       /* does not use a resource for detach */
    0xde30, 0xde37, 0x07,        /* range for the device, regs:$de30-$de37 */
    0,                           /* read validity is determined by the device upon a read */
    ide64_io_store,              /* store function */
    NULL,                        /* NO poke function */
    ide64_io_read,               /* read function */
    ide64_io_peek,               /* peek function */
    ide64_io_dump,               /* device state information dump function */
    CARTRIDGE_IDE64,             /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0                            /* insertion order, gets filled in by the registration function */
};

static io_source_t ide64_ft245_device = {
    CARTRIDGE_NAME_IDE64 " FT245", /* name of the device */
    IO_DETACH_CART,                /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,         /* does not use a resource for detach */
    0xde5d, 0xde5e, 0x01,          /* range for the device, regs:$de5d-$de5e */
    0,                             /* read validity is determined by the device upon a read */
    ide64_ft245_store,             /* store function */
    NULL,                          /* NO poke function */
    ide64_ft245_read,              /* read function */
    ide64_ft245_peek,              /* peek function */
    NULL,                          /* TODO: device state information dump function */
    CARTRIDGE_IDE64,               /* cartridge ID */
    IO_PRIO_NORMAL,                /* normal priority, device read needs to be checked for collisions */
    0                              /* insertion order, gets filled in by the registration function */
};

static io_source_t ide64_ds1302_device = {
    CARTRIDGE_NAME_IDE64 " DS1302", /* name of the device */
    IO_DETACH_CART,                 /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,          /* does not use a resource for detach */
    0xde5f, 0xde5f, 0x00,           /* range for the device, reg:$de5f */
    0,                              /* read validity is determined by the device upon a read */
    ide64_ds1302_store,             /* store function */
    NULL,                           /* NO poke function */
    ide64_ds1302_read,              /* read function */
    ide64_ds1302_peek,              /* peek function */
    ide64_rtc_dump,                 /* device state information dump function */
    CARTRIDGE_IDE64,                /* cartridge ID */
    IO_PRIO_NORMAL,                 /* normal priority, device read needs to be checked for collisions */
    0                               /* insertion order, gets filled in by the registration function */
};

static io_source_t ide64_rom_device = {
    CARTRIDGE_NAME_IDE64 " ROM", /* name of the device */
    IO_DETACH_CART,              /* use cartridge ID to detach the device when involved in a read-collision */
    IO_DETACH_NO_RESOURCE,       /* does not use a resource for detach */
    0xde60, 0xdeff, 0xff,        /* range for the device, regs:$de60-$deff */
    0,                           /* read validity is determined by the device upon a read */
    ide64_romio_store,           /* store function */
    NULL,                        /* NO poke function */
    ide64_romio_read,            /* read function */
    ide64_romio_peek,            /* peek function */
    NULL,                        /* TODO: device state information dump function */
    CARTRIDGE_IDE64,             /* cartridge ID */
    IO_PRIO_NORMAL,              /* normal priority, device read needs to be checked for collisions */
    0                            /* insertion order, gets filled in by the registration function */
};

static io_source_t ide64_clockport_device = {
    CARTRIDGE_NAME_IDE64 "Clockport", /* name of the device */
    IO_DETACH_RESOURCE,               /* use resource to detach the device when involved in a read-collision */
    "IDE64ClockPort",                 /* resource to set to '0' */
    0xde00, 0xde0f, 0x0f,             /* range for the device, regs:$de00-$de0f */
    0,                                /* read validity is determined by the device upon a read */
    ide64_clockport_store,            /* store function */
    NULL,                             /* NO poke function */
    ide64_clockport_read,             /* read function */
    ide64_clockport_peek,             /* peek function */
    ide64_clockport_dump,             /* device state information dump function */
    CARTRIDGE_IDE64,                  /* cartridge ID */
    IO_PRIO_NORMAL,                   /* normal priority, device read needs to be checked for collisions */
    0                                 /* insertion order, gets filled in by the registration function */
};

static io_source_list_t *ide64_idebus_list_item = NULL;
static io_source_list_t *ide64_io_list_item = NULL;
static io_source_list_t *ide64_ft245_list_item = NULL;
static io_source_list_t *ide64_ds1302_list_item = NULL;
static io_source_list_t *ide64_rom_list_item = NULL;
static io_source_list_t *ide64_clockport_list_item = NULL;

static const export_resource_t export_res[6] = {
    {CARTRIDGE_NAME_IDE64 " IDE", 1, 1, &ide64_idebus_device, NULL, CARTRIDGE_IDE64},
    {CARTRIDGE_NAME_IDE64 " I/O", 1, 1, &ide64_io_device, NULL, CARTRIDGE_IDE64},
    {CARTRIDGE_NAME_IDE64 " FT245", 1, 1, &ide64_ft245_device, NULL, CARTRIDGE_IDE64},
    {CARTRIDGE_NAME_IDE64 " DS1302", 1, 1, &ide64_ds1302_device, NULL, CARTRIDGE_IDE64},
    {CARTRIDGE_NAME_IDE64 " ROM", 1, 1, &ide64_rom_device, NULL, CARTRIDGE_IDE64},
    {CARTRIDGE_NAME_IDE64 " ClockPort", 1, 1, &ide64_clockport_device, NULL, CARTRIDGE_IDE64},
};

static int clockport_activate(void)
{
    if (!ide64_enabled) {
        return 0;
    }

    if (clockport_device) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device = clockport_open_device(clockport_device_id, STRING_IDE64_CLOCKPORT);
    if (!clockport_device) {
        return -1;
    }
    return 0;
}

static int clockport_deactivate(void)
{
    if (!ide64_enabled) {
        return 0;
    }

    if (clockport_device_id == CLOCKPORT_DEVICE_NONE) {
        return 0;
    }

    clockport_device->close(clockport_device);
    clockport_device = NULL;

    return 0;
}

static int ide64_register(void)
{
    int i;

    if (ide64_rom_list_item) {
        return 0;
    }

    for (i = 0; i < 6; i++) {
        if (settings_version < IDE64_VERSION_4_1 && i == 2) {
            continue;
        }
        if (settings_version < IDE64_VERSION_4_1 && i == 5) {
            continue;
        }
        if (export_add(&export_res[i]) < 0) {
            return -1;
        }
    }

    ide64_idebus_list_item = io_source_register(&ide64_idebus_device);
    ide64_io_list_item = io_source_register(&ide64_io_device);
    if (settings_version >= IDE64_VERSION_4_1) {
        ide64_ft245_list_item = io_source_register(&ide64_ft245_device);
    }
    ide64_ds1302_list_item = io_source_register(&ide64_ds1302_device);
    ide64_rom_list_item = io_source_register(&ide64_rom_device);
    if (settings_version >= IDE64_VERSION_4_1) {
        ide64_clockport_list_item = io_source_register(&ide64_clockport_device);
    }

    if (clockport_activate() < 0) {
        return -1;
    }

    ide64_enabled = 1;

    return 0;
}

static void ide64_unregister(void)
{
    int i;

    if (!ide64_rom_list_item) {
        return;
    }

    for (i = 0; i < 6; i++) {
        if (settings_version < IDE64_VERSION_4_1 && i == 2) {
            continue;
        }
        if (settings_version < IDE64_VERSION_4_1 && i == 5) {
            continue;
        }
        export_remove(&export_res[i]);
    }

    io_source_unregister(ide64_idebus_list_item);
    io_source_unregister(ide64_io_list_item);
    if (ide64_ft245_list_item) {
        io_source_unregister(ide64_ft245_list_item);
    }
    io_source_unregister(ide64_ds1302_list_item);
    io_source_unregister(ide64_rom_list_item);
    if (ide64_clockport_list_item) {
        io_source_unregister(ide64_clockport_list_item);
    }
    ide64_idebus_list_item = NULL;
    ide64_io_list_item = NULL;
    ide64_ft245_list_item = NULL;
    ide64_ds1302_list_item = NULL;
    ide64_rom_list_item = NULL;
    ide64_clockport_list_item = NULL;

    clockport_deactivate();

    ide64_enabled = 0;
}

static void detect_ide64_image(struct drive_s *drive)
{
    FILE *file;
    unsigned char header[24];
    size_t res;
    char *ext;
    ata_drive_geometry_t *geometry = &drive->detected;

    if (!ide64_rom_list_item) {
        drive->type = ATA_DRIVE_NONE;
        return;
    }
    debug("IDE64 detect");
    geometry->cylinders = drive->settings.cylinders;
    geometry->heads = drive->settings.heads;
    geometry->sectors = drive->settings.sectors;
    geometry->size = geometry->cylinders * geometry->heads * geometry->sectors;

    if (!drive->filename) {
        drive->type = ATA_DRIVE_NONE;
        return;
    }

    res = strlen(drive->filename);

    if (!res) {
        drive->type = ATA_DRIVE_NONE;
        return;
    }

    drive->type = ATA_DRIVE_CF;
    ext = util_get_extension(drive->filename);
    if (ext) {
        if (!strcasecmp(ext, "cfa")) {
            drive->type = ATA_DRIVE_CF;
        } else if (!strcasecmp(ext, "hdd")) {
            drive->type = ATA_DRIVE_HDD;
        } else if (!strcasecmp(ext, "fdd")) {
            drive->type = ATA_DRIVE_FDD;
        } else if (!strcasecmp(ext, "iso")) {
            drive->type = ATA_DRIVE_CD;
        }
    }

    file = fopen(drive->filename, MODE_READ);

    if (!file) {
        return;
    }

    if (drive->autodetect_size) {
        if (fread(header, 1, 24, file) < 24) {
            memset(&header, 0, sizeof(header));
        }

        if (memcmp(header, "C64-IDE V", 9) == 0) { /* old filesystem always CHS */
            geometry->cylinders = util_be_buf_to_word(&header[0x10]) + 1;
            geometry->heads = (header[0x12] & 0x0f) + 1;
            geometry->sectors = header[0x13];
            geometry->size = geometry->cylinders * geometry->heads * geometry->sectors;
        } else if (memcmp(header + 8, "C64 CFS V", 9) == 0) {
            if (header[0x04] & 0x40) { /* LBA */
                geometry->cylinders = 0;
                geometry->heads = 0;
                geometry->sectors = 0;
                geometry->size = util_be_buf_to_dword(&header[0x04]) & 0x0fffffff;
            } else { /* CHS */
                geometry->cylinders = util_be_buf_to_word(&header[0x05]) + 1;
                geometry->heads = (header[0x04] & 0x0f) + 1;
                geometry->sectors = header[0x07];
                geometry->size = geometry->cylinders * geometry->heads * geometry->sectors;
            }
        } else {
            off_t size = 0;
            if (fseeko(file, 0, SEEK_END) == 0) {
                size = ftello(file);
                if (size < 0) {
                    size = 0;
                }
            }
            geometry->cylinders = 0;
            geometry->heads = 0;
            geometry->sectors = 0;
            geometry->size = (int)(size / ((drive->type == ATA_DRIVE_CD) ? 2048 : 512));
        }
    }

    fclose(file);
    return;
}

static int set_ide64_image_file(const char *name, void *param)
{
    struct drive_s *drive = &drives[vice_ptr_to_int(param)];

    util_string_set(&drive->filename, name);

    if (drive->drv) {
        detect_ide64_image(drive);
        drive->update_needed = ata_image_change(drive->drv, drive->filename, drive->type, drive->detected);
    }
    return 0;
}

static int set_cylinders(int cylinders, void *param)
{
    struct drive_s *drive = &drives[vice_ptr_to_int(param)];

    if (cylinders > 65535 || cylinders < 1) {
        return -1;
    }

    drive->settings.cylinders = cylinders;
    if (drive->drv) {
        drive->update_needed = ata_image_change(drive->drv, drive->filename, drive->type, drive->detected);
    }
    return 0;
}

static int set_heads(int heads, void *param)
{
    struct drive_s *drive = &drives[vice_ptr_to_int(param)];

    if (heads > 16 || heads < 1) {
        return -1;
    }
    drive->settings.heads = heads;
    if (drive->drv) {
        drive->update_needed = ata_image_change(drive->drv, drive->filename, drive->type, drive->detected);
    }
    return 0;
}

static int set_sectors(int sectors, void *param)
{
    struct drive_s *drive = &drives[vice_ptr_to_int(param)];

    if (sectors > 63 || sectors < 1) {
        return -1;
    }
    drive->settings.sectors = sectors;
    if (drive->drv) {
        drive->update_needed = ata_image_change(drive->drv, drive->filename, drive->type, drive->detected);
    }
    return 0;
}

static int set_autodetect_size(int autodetect_size, void *param)
{
    struct drive_s *drive = &drives[vice_ptr_to_int(param)];

    drive->autodetect_size = autodetect_size ? 1 : 0;
    if (drive->drv) {
        detect_ide64_image(drive);
        drive->update_needed = ata_image_change(drive->drv, drive->filename, drive->type, drive->detected);
    }
    return 0;
}

#ifdef HAVE_NETWORK
static void usbserver_activate(int);
#endif

static int ide64_set_rtc_save(int val, void *param)
{
    ide64_rtc_save = val ? 1 : 0;

    return 0;
}

static int set_version(int value, void *param)
{
    int val;

    switch (value) {
        default:
            val = IDE64_VERSION_3;
            break;
        case 1:
            val = IDE64_VERSION_4_1;
            break;
        case 2:
            val = IDE64_VERSION_4_2;
            break;
    }

    if (!ide64_rom_list_item) {
        settings_version = val;
        return 0;
    }
    if (settings_version != val) {
        ide64_unregister();
        settings_version = val;
        if (ide64_register() < 0) {
            return -1;
        }
        usbserver_activate(settings_usbserver);
        machine_trigger_reset(MACHINE_RESET_MODE_HARD);
    }
    return 0;
}

#ifdef HAVE_NETWORK
static void usb_send(void);

static void usb_alarm_handler(CLOCK offset, void *data)
{
    usb_send();
}

static void usbserver_activate(int mode)
{
    vice_network_socket_address_t * server_addr = NULL;

    ft245_rxp = ft245_rxl = ft245_txp = 0;

    if (settings_version < IDE64_VERSION_4_1) {
        mode = 0;
    }

    if (usbserver_asocket) {
        vice_network_socket_close(usbserver_asocket);
        usbserver_asocket = NULL;
    }

    if (usbserver_socket) {
        vice_network_socket_close(usbserver_socket);
        usbserver_socket = NULL;
    }

    if (!mode) {
        if (usb_alarm) {
            alarm_destroy(usb_alarm);
            usb_alarm = NULL;
        }
        return;
    }

    if (!usb_alarm) {
        usb_alarm = alarm_new(maincpu_alarm_context, "IDE64USBAlarm", usb_alarm_handler, NULL);
        if (!usb_alarm) {
            return;
        }
    }

    if (!settings_usbserver_address) {
        return;
    }

    server_addr = vice_network_address_generate(settings_usbserver_address, 0);
    if (!server_addr) {
        return;
    }

    usbserver_socket = vice_network_server(server_addr);
    vice_network_address_close(server_addr);
}

static int set_usbserver(int value, void *param)
{
    int val = value ? 1 : 0;

    if (settings_usbserver != val && ide64_rom_list_item) {
        usbserver_activate(val);
    }
    settings_usbserver = val;
    return 0;
}

static int set_usbserver_address(const char *name, void *param)
{
    if (settings_usbserver_address != NULL && name != NULL
        && strcmp(name, settings_usbserver_address) == 0) {
        return 0;
    }

    util_string_set(&settings_usbserver_address, name);
    if (usbserver_socket && ide64_rom_list_item) {
        usbserver_activate(settings_usbserver);
    }
    return 0;
}
#endif

static int set_ide64_clockport_device(int val, void *param)
{
    if (val == clockport_device_id) {
        return 0;
    }

    if (!ide64_enabled) {
        clockport_device_id = val;
        return 0;
    }

    if (clockport_device_id != CLOCKPORT_DEVICE_NONE) {
        clockport_device->close(clockport_device);
        clockport_device_id = CLOCKPORT_DEVICE_NONE;
        clockport_device = NULL;
    }

    if (val != CLOCKPORT_DEVICE_NONE) {
        clockport_device = clockport_open_device(val, STRING_IDE64_CLOCKPORT);
        if (!clockport_device) {
            return -1;
        }
        clockport_device_id = val;
    }
    return 0;
}

static const resource_string_t resources_string[] = {
    { "IDE64Image1", "ide.cfa", RES_EVENT_NO, NULL,
      &drives[0].filename, set_ide64_image_file, (void *)0 },
    { "IDE64Image2", "", RES_EVENT_NO, NULL,
      &drives[1].filename, set_ide64_image_file, (void *)1 },
    { "IDE64Image3", "", RES_EVENT_NO, NULL,
      &drives[2].filename, set_ide64_image_file, (void *)2 },
    { "IDE64Image4", "", RES_EVENT_NO, NULL,
      &drives[3].filename, set_ide64_image_file, (void *)3 },
#ifdef HAVE_NETWORK
    { "IDE64USBServerAddress", "ip4://127.0.0.1:64245", RES_EVENT_NO, NULL,
      &settings_usbserver_address, set_usbserver_address, NULL },
#endif
    RESOURCE_STRING_LIST_END
};

static const resource_int_t resources_int[] = {
    { "IDE64Cylinders1", 256,
      RES_EVENT_NO, NULL,
      (int *)&drives[0].settings.cylinders, set_cylinders, (void *)0 },
    { "IDE64Cylinders2", 256,
      RES_EVENT_NO, NULL,
      (int *)&drives[1].settings.cylinders, set_cylinders, (void *)1 },
    { "IDE64Cylinders3", 256,
      RES_EVENT_NO, NULL,
      (int *)&drives[2].settings.cylinders, set_cylinders, (void *)2 },
    { "IDE64Cylinders4", 256,
      RES_EVENT_NO, NULL,
      (int *)&drives[3].settings.cylinders, set_cylinders, (void *)3 },
    { "IDE64Heads1", 4,
      RES_EVENT_NO, NULL,
      (int *)&drives[0].settings.heads, set_heads, (void *)0 },
    { "IDE64Heads2", 4,
      RES_EVENT_NO, NULL,
      (int *)&drives[1].settings.heads, set_heads, (void *)1 },
    { "IDE64Heads3", 4,
      RES_EVENT_NO, NULL,
      (int *)&drives[2].settings.heads, set_heads, (void *)2 },
    { "IDE64Heads4", 4,
      RES_EVENT_NO, NULL,
      (int *)&drives[3].settings.heads, set_heads, (void *)3 },
    { "IDE64Sectors1", 16,
      RES_EVENT_NO, NULL,
      (int *)&drives[0].settings.sectors, set_sectors, (void *)0 },
    { "IDE64Sectors2", 16,
      RES_EVENT_NO, NULL,
      (int *)&drives[1].settings.sectors, set_sectors, (void *)1 },
    { "IDE64Sectors3", 16,
      RES_EVENT_NO, NULL,
      (int *)&drives[2].settings.sectors, set_sectors, (void *)2 },
    { "IDE64Sectors4", 16,
      RES_EVENT_NO, NULL,
      (int *)&drives[3].settings.sectors, set_sectors, (void *)3 },
    { "IDE64AutodetectSize1", 1,
      RES_EVENT_NO, NULL,
      (int *)&drives[0].autodetect_size, set_autodetect_size, (void *)0 },
    { "IDE64AutodetectSize2", 1,
      RES_EVENT_NO, NULL,
      (int *)&drives[1].autodetect_size, set_autodetect_size, (void *)1 },
    { "IDE64AutodetectSize3", 1,
      RES_EVENT_NO, NULL,
      (int *)&drives[2].autodetect_size, set_autodetect_size, (void *)2 },
    { "IDE64AutodetectSize4", 1,
      RES_EVENT_NO, NULL,
      (int *)&drives[3].autodetect_size, set_autodetect_size, (void *)3 },
    { "IDE64version", 1,
      RES_EVENT_NO, NULL,
      (int *)&settings_version, set_version, NULL },
#ifdef HAVE_NETWORK
    { "IDE64USBServer", 0,
      RES_EVENT_NO, NULL,
      &settings_usbserver, set_usbserver, NULL },
#endif
    { "IDE64RTCSave", 0,
      RES_EVENT_NO, NULL,
      &ide64_rtc_save, ide64_set_rtc_save, NULL },
    { "IDE64ClockPort", 0, RES_EVENT_NO, NULL,
      &clockport_device_id, set_ide64_clockport_device, NULL },
    RESOURCE_INT_LIST_END
};

int ide64_resources_init(void)
{
    int i;

    debug("IDE64 resource init");
    for (i = 0; i < 4; i++) {
        drives[i].drv = NULL;
        drives[i].filename = NULL;
    }
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }
    if (resources_register_int(resources_int) < 0) {
        return -1;
    }
    if (shortbus_resources_init() < 0) {
        return -1;
    }

    return 0;
}

int ide64_resources_shutdown(void)
{
    int i;

    debug("IDE64 resource shutdown");

    for (i = 0; i < 4; i++) {
        if (drives[i].filename) {
            lib_free(drives[i].filename);
        }
        drives[i].filename = NULL;
    }

#ifdef HAVE_NETWORK
    if (settings_usbserver_address) {
        lib_free(settings_usbserver_address);
        settings_usbserver_address = NULL;
    }
#endif

    shortbus_resources_shutdown();

    lib_free(clockport_device_names);
    clockport_device_names = NULL;

    return 0;
}

static const cmdline_option_t cmdline_options[] =
{
    { "-IDE64image1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Image1", NULL,
      "<Name>", "Specify name of IDE64 image file" },
    { "-IDE64image2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Image2", NULL,
      "<Name>", "Specify name of IDE64 image file" },
    { "-IDE64image3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Image3", NULL,
      "<Name>", "Specify name of IDE64 image file" },
    { "-IDE64image4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Image4", NULL,
      "<Name>", "Specify name of IDE64 image file" },
    { "-IDE64cyl1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Cylinders1", NULL,
      "<value>", "Set number of cylinders for the IDE64 emulation. (1..65535)" },
    { "-IDE64cyl2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Cylinders2", NULL,
      "<value>", "Set number of cylinders for the IDE64 emulation. (1..65535)" },
    { "-IDE64cyl3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Cylinders3", NULL,
      "<value>", "Set number of cylinders for the IDE64 emulation. (1..65535)" },
    { "-IDE64cyl4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Cylinders4", NULL,
      "<value>", "Set number of cylinders for the IDE64 emulation. (1..65535)" },
    { "-IDE64hds1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Heads1", NULL,
      "<value>", "Set number of heads for the IDE64 emulation. (1..16)" },
    { "-IDE64hds2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Heads2", NULL,
      "<value>", "Set number of heads for the IDE64 emulation. (1..16)" },
    { "-IDE64hds3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Heads3", NULL,
      "<value>", "Set number of heads for the IDE64 emulation. (1..16)" },
    { "-IDE64hds4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Heads4", NULL,
      "<value>", "Set number of heads for the IDE64 emulation. (1..16)" },
    { "-IDE64sec1", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Sectors1", NULL,
      "<value>", "Set number of sectors for the IDE64 emulation. (1..63)" },
    { "-IDE64sec2", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Sectors2", NULL,
      "<value>", "Set number of sectors for the IDE64 emulation. (1..63)" },
    { "-IDE64sec3", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Sectors3", NULL,
      "<value>", "Set number of sectors for the IDE64 emulation. (1..63)" },
    { "-IDE64sec4", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Sectors4", NULL,
      "<value>", "Set number of sectors for the IDE64 emulation. (1..63)" },
    { "-IDE64autosize1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize1", (void *)1,
      NULL, "Autodetect image size" },
    { "+IDE64autosize1", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize1", (void *)0,
      NULL, "Do not autodetect geometry of formatted images" },
    { "-IDE64autosize2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize2", (void *)1,
      NULL, "Autodetect image size" },
    { "+IDE64autosize2", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize2", (void *)0,
      NULL, "Do not autodetect geometry of formatted images" },
    { "-IDE64autosize3", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize3", (void *)1,
      NULL, "Autodetect image size" },
    { "+IDE64autosize3", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize3", (void *)0,
      NULL, "Do not autodetect geometry of formatted images" },
    { "-IDE64autosize4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize4", (void *)1,
      NULL, "Autodetect image size" },
    { "+IDE64autosize4", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64AutodetectSize4", (void *)0,
      NULL, "Do not autodetect geometry of formatted images" },
    { "-IDE64version", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64Version", NULL,
      "<value>", "IDE64 cartridge version" },
#ifdef HAVE_NETWORK
    { "-IDE64USB", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64USBServer", (resource_value_t)1,
      NULL, "Enable IDE64 USB server" },
    { "+IDE64USB", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64USBServer", (resource_value_t)0,
      NULL, "Disable IDE64 USB server" },
    { "-IDE64USBAddress", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64USBServerAddress", NULL,
      "<Name>", "IDE64 USB server address" },
#endif
    { "-IDE64rtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64RTCSave", (void *)1,
      NULL, "Enable saving of IDE64 RTC data when changed." },
    { "+IDE64rtcsave", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "IDE64RTCSave", (void *)0,
      NULL, "Disable saving of IDE64 RTC data when changed." },
    CMDLINE_LIST_END
};

static cmdline_option_t clockport_cmdline_options[] =
{
    { "-ide64clockportdevice", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "IDE64ClockPort", NULL,
      "<device>", NULL },
    CMDLINE_LIST_END
};

int ide64_cmdline_options_init(void)
{
    int i;
    char *tmp;
    char number[10];

    if (shortbus_cmdline_options_init() < 0) {
        return -1;
    }

    if (cmdline_register_options(cmdline_options) < 0) {
        return -1;
    }

    sprintf(number, "%d", clockport_supported_devices[0].id);

    clockport_device_names = util_concat("Clockport device. (", number, ": ", clockport_supported_devices[0].name, NULL);

    for (i = 1; clockport_supported_devices[i].name; ++i) {
        tmp = clockport_device_names;
        sprintf(number, "%d", clockport_supported_devices[i].id);
        clockport_device_names = util_concat(tmp, ", ", number, ": ", clockport_supported_devices[i].name, NULL);
        lib_free(tmp);
    }
    tmp = clockport_device_names;
    clockport_device_names = util_concat(tmp, ")", NULL);
    lib_free(tmp);
    clockport_cmdline_options[0].description = clockport_device_names;

    return cmdline_register_options(clockport_cmdline_options);
}

void ide64_reset(void)
{
    shortbus_reset();
    if (ide64_enabled && clockport_device) {
        clockport_device->reset(clockport_device->device_context);
    }
}

static uint8_t ide64_idebus_read(uint16_t addr)
{
    in_d030 = ata_register_read(drives[idrive ^ 1].drv, addr, idebus);
    in_d030 = ata_register_read(drives[idrive].drv, addr, in_d030);
    if (settings_version >= IDE64_VERSION_4_1) {
        idebus = (in_d030 & 0xff00) | vicii_read_phi1();
        ide64_idebus_device.io_source_valid = 1;
        return in_d030 & 0xff;
    } else {
        idebus = in_d030;
    }
    ide64_idebus_device.io_source_valid = 0;
    return 0;
}

static uint8_t ide64_idebus_peek(uint16_t addr)
{
    if (settings_version >= IDE64_VERSION_4_1) {
        return ata_register_peek(drives[idrive].drv, addr) | ata_register_peek(drives[idrive ^ 1].drv, addr);
    }
    return 0;
}

static void ide64_idebus_store(uint16_t addr, uint8_t value)
{
    switch (addr) {
        case 8:
        case 9:
            idrive = (addr & 1) << 1;
        /* fall through */
        default:
            if (settings_version >= IDE64_VERSION_4_1) {
                out_d030 = value | (out_d030 & 0xff00);
            }
            ata_register_store(drives[idrive].drv, addr, out_d030);
            ata_register_store(drives[idrive ^ 1].drv, addr, out_d030);
            idebus = out_d030;
            return;
    }
}

static uint8_t ide64_io_read(uint16_t addr)
{
    ide64_io_device.io_source_valid = 1;

    switch (addr) {
        case 0:
            if (settings_version >= IDE64_VERSION_4_1) {
                break;
            }
            return (uint8_t)in_d030;
        case 1:
            return in_d030 >> 8;
        case 2:
            switch (settings_version) {
            case IDE64_VERSION_3:
                return 0x10 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            case IDE64_VERSION_4_1:
                return 0x20 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            case IDE64_VERSION_4_2:
                return 0x80 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            }
    }
    ide64_io_device.io_source_valid = 0;
    return 0;
}

static uint8_t ide64_io_peek(uint16_t addr)
{
    switch (addr) {
        case 0:
            if (settings_version >= IDE64_VERSION_4_1) {
                break;
            }
            return (uint8_t)in_d030;
        case 1:
            return in_d030 >> 8;
        case 2:
            switch (settings_version) {
            case IDE64_VERSION_3:
                return 0x10 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            case IDE64_VERSION_4_1:
                return 0x20 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            case IDE64_VERSION_4_2:
                return 0x80 | (current_bank << 2) | (((current_cfg & 1) ^ 1) << 1) | (current_cfg >> 1);
            }
    }
    return 0;
}

static void ide64_io_store(uint16_t addr, uint8_t value)
{
    switch (addr) {
        case 0:
            if (settings_version < IDE64_VERSION_4_1) {
                out_d030 = (out_d030 & 0xff00) | value;
            }
            return;
        case 1:
            out_d030 = (out_d030 & 0x00ff) | (value << 8);
            return;
        case 2:
        case 3:
        case 4:
        case 5:
            if (settings_version < IDE64_VERSION_4_1 && current_bank != ((addr ^ 2) & 3)) {
                current_bank = (addr ^ 2) & 3;
                cart_config_changed_slotmain(0, (uint8_t)(current_cfg | (current_bank << CMODE_BANK_SHIFT)), CMODE_READ | CMODE_PHI2_RAM);
            }
            return;
    }
}

#ifdef HAVE_NETWORK
static void usb_receive(void)
{
    if (ft245_rxp >= ft245_rxl && usbserver_socket) {
        int reconnect = 2;
        if (!usbserver_asocket && vice_network_select_poll_one(usbserver_socket)) {
            usbserver_asocket = vice_network_accept(usbserver_socket);
        }
        while (usbserver_asocket && vice_network_select_poll_one(usbserver_asocket) && reconnect--) {
            int r;
            r = vice_network_receive(usbserver_asocket, ft245_rx, sizeof(ft245_rx), 0);
            if (r <= 0) {
                vice_network_socket_close(usbserver_asocket);
                if (vice_network_select_poll_one(usbserver_socket)) {
                    usbserver_asocket = vice_network_accept(usbserver_socket);
                } else {
                    usbserver_asocket = NULL;
                }
                if (ft245_txp)  {
                    alarm_unset(usb_alarm);
                    ft245_txp = 0; /* transmit buffer_lost */
                }
            } else {
                ft245_rxp = 0;
                ft245_rxl = r;
                break;
            }
        }
    }
}

static void usb_send(void)
{
    if (ft245_txp && usbserver_socket) {
        int reconnect = 2;
        if (!usbserver_asocket && vice_network_select_poll_one(usbserver_socket)) {
            usbserver_asocket = vice_network_accept(usbserver_socket);
        }
        while (usbserver_asocket && reconnect--) {
            int r = vice_network_send(usbserver_asocket, ft245_tx, ft245_txp, 0);
            if (r > 0 && vice_network_send(usbserver_asocket, ft245_tx, 0, 0) < 0) {
                r = -1;
            }
            if (r <= 0) {
                vice_network_socket_close(usbserver_asocket);
                if (vice_network_select_poll_one(usbserver_socket)) {
                    usbserver_asocket = vice_network_accept(usbserver_socket);
                } else {
                    usbserver_asocket = NULL;
                }
                ft245_rxp = ft245_rxl = 0; /* receive buffer lost */
                if (!reconnect) {
                    ft245_txp = 0; /* nowhere to transmit */
                }
            } else {
                if (r < ft245_txp) {
                    memmove(ft245_tx, ft245_tx + r, ft245_txp - r);
                }
                ft245_txp -= r;
                break;
            }
        }
    }
    if (ft245_txp) {
        alarm_set(usb_alarm, maincpu_clk + LATENCY_TIMER);
    } else {
        alarm_unset(usb_alarm);
    }
}
#endif

static uint8_t ide64_ft245_read(uint16_t addr)
{
    if (settings_version >= IDE64_VERSION_4_1) {
        switch (addr ^ 1) {
            case 0:
#ifdef HAVE_NETWORK
                if (ft245_rxp >= ft245_rxl) {
                    usb_receive();
                }
                if (ft245_rxp < ft245_rxl) {
                    ide64_ft245_device.io_source_valid = 1;
                    return ft245_rx[ft245_rxp++];
                }
#endif
                break;
            case 1:
                ide64_ft245_device.io_source_valid = 1;
#ifdef HAVE_NETWORK
                if (ft245_rxp >= ft245_rxl) {
                    usb_receive();
                }
                if (ft245_txp >= sizeof(ft245_tx)) {
                    usb_send();
                }
                return ((ft245_rxp < ft245_rxl && usbserver_asocket) ? 0x00 : 0x40) | (ft245_txp < sizeof(ft245_tx) && usbserver_asocket ? 0x00 : 0x80);
#else
                return 0xc0;
#endif
        }
    }
    ide64_ft245_device.io_source_valid = 0;
    return 0;
}

static uint8_t ide64_ft245_peek(uint16_t addr)
{
    if (settings_version >= IDE64_VERSION_4_1) {
        switch (addr ^ 1) {
            case 0:
#ifdef HAVE_NETWORK
                if (ft245_rxp >= ft245_rxl) {
                    usb_receive();
                }
                if (ft245_rxp < ft245_rxl) {
                    return ft245_rx[ft245_rxp];
                }
#endif
                return 0xff;
            case 1:
#ifdef HAVE_NETWORK
                if (ft245_rxp >= ft245_rxl) {
                    usb_receive();
                }
                if (ft245_txp >= sizeof(ft245_tx)) {
                    usb_send();
                }
                return (ft245_rxp < ft245_rxl) ? 0x00 : (usbserver_asocket ? 0x40 : 0xc0);
#else
                return 0xc0;
#endif
        }
    }
    return 0;
}

static void ide64_ft245_store(uint16_t addr, uint8_t value)
{
    if (settings_version >= IDE64_VERSION_4_1) {
        switch (addr ^ 1) {
            case 0:
#ifdef HAVE_NETWORK
                if (ft245_txp < sizeof(ft245_tx)) {
                    if (!ft245_txp && usb_alarm) {
                        alarm_set(usb_alarm, maincpu_clk + LATENCY_TIMER);
                    }
                    ft245_tx[ft245_txp++] = value;
                }
                if (ft245_txp >= sizeof(ft245_tx)) {
                    usb_send();
                }
#endif
                break;
            case 1:
                break;
        }
    }
    return;
}

static uint8_t ide64_ds1302_read(uint16_t addr)
{
    int i;

    if (kill_port & 1) {
        ide64_ds1302_device.io_source_valid = 0;
        return 0;
    }

    i = vicii_read_phi1() & ~1;
    ds1202_1302_set_lines(ds1302_context, kill_port & 2, 0u, 1u);
    i |= ds1202_1302_read_data_line(ds1302_context);
    ds1202_1302_set_lines(ds1302_context, kill_port & 2, 1u, 1u);

    ide64_ds1302_device.io_source_valid = 1;
    return i;
}

static uint8_t ide64_ds1302_peek(uint16_t addr)
{
    return 0;
}

static void ide64_ds1302_store(uint16_t addr, uint8_t value)
{
    if (kill_port & 1) {
        return;
    }
    ds1202_1302_set_lines(ds1302_context, kill_port & 2u, 0u, 1u);
    ds1202_1302_set_lines(ds1302_context, kill_port & 2u, 1u, value & 1u);
    return;
}

static uint8_t ide64_clockport_read(uint16_t address)
{
    /* read from clockport device */
    if (clockport_device) {
        ide64_clockport_device.io_source_valid = 1;
        return clockport_device->read(address, &ide64_clockport_device.io_source_valid, clockport_device->device_context);
    }
    /* read open clock port */
    ide64_clockport_device.io_source_valid = 0;
    return 0;
}

static uint8_t ide64_clockport_peek(uint16_t address)
{
    /* read from clockport device */
    if (clockport_device) {
        return clockport_device->peek(address, clockport_device->device_context);
    }
    return 0;
}

static void ide64_clockport_store(uint16_t address, uint8_t byte)
{
    /* write to clockport device */
    if (clockport_device) {
        clockport_device->store(address, byte, clockport_device->device_context);
    }
}

static int ide64_clockport_dump(void)
{
    if (clockport_device) {
        clockport_device->dump(clockport_device->device_context);
    }
    return 0;
}

static uint8_t ide64_romio_read(uint16_t addr)
{
    if (kill_port & 1) {
        ide64_rom_device.io_source_valid = 0;
        return 0;
    }

    ide64_rom_device.io_source_valid = 1;
    return roml_banks[addr | 0x1e00 | (current_bank << 14)];
}

static uint8_t ide64_romio_peek(uint16_t addr)
{
    if (kill_port & 1) {
        return 0;
    }
    return roml_banks[addr | 0x1e00 | (current_bank << 14)];
}

static void ide64_romio_store(uint16_t addr, uint8_t value)
{
    if (kill_port & 1) {
        return;
    }

    switch (addr) {
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
            if (settings_version >= IDE64_VERSION_4_1 && current_bank != (addr & 7)) {
                current_bank = addr & 7;
                break;
            }
            return;
        case 0x68:
        case 0x69:
        case 0x6a:
        case 0x6b:
        case 0x6c:
        case 0x6d:
        case 0x6e:
        case 0x6f:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7a:
        case 0x7b:
        case 0x7c:
        case 0x7d:
        case 0x7e:
        case 0x7f:
            if (settings_version >= IDE64_VERSION_4_2 && current_bank != (addr & 31)) {
                current_bank = addr & 31;
                break;
            }
            return;
        case 0xfb:
            kill_port = value;
            ds1202_1302_set_lines(ds1302_context, kill_port & 2u, 1u, 1u);
            if ((kill_port & 1) == 0) {
                return;
            }
        /* fall through */
        case 0xfc:
        case 0xfd:
        case 0xfe:
        case 0xff:
            if (current_cfg != ((addr ^ 1) & 3)) {
                current_cfg = (addr ^ 1) & 3;
                break;
            }
            return;
        default:
            return;
    }
    cart_config_changed_slotmain(0, (uint8_t)(current_cfg | (current_bank << CMODE_BANK_SHIFT)), CMODE_READ | CMODE_PHI2_RAM);
}

uint8_t ide64_rom_read(uint16_t addr)
{
    return roml_banks[(addr & 0x3fff) | (roml_bank << 14)];
}

void ide64_rom_store(uint16_t addr, uint8_t value)
{
}

uint8_t ide64_ram_read(uint16_t addr)
{
    return export_ram0[addr & 0x7fff];
}

void ide64_ram_store(uint16_t addr, uint8_t value)
{
    export_ram0[addr & 0x7fff] = value;
}

void ide64_mmu_translate(unsigned int addr, uint8_t **base, int *start, int *limit)
{
    switch (addr & 0xf000) {
        case 0xf000:
        case 0xe000:
            *base = &roml_banks[roml_bank << 14] - 0xc000;
            *start = 0xe000;
            *limit = 0xfffd;
            break;
        case 0xc000:
            *base = export_ram0 - 0x8000;
            *start = 0xc000;
            *limit = 0xcffd;
            break;
        case 0xb000:
        case 0xa000:
            *base = &roml_banks[roml_bank << 14] - 0x8000;
            *start = 0xa000;
            *limit = 0xbffd;
            break;
        case 0x9000:
        case 0x8000:
            *base = &roml_banks[roml_bank << 14] - 0x8000;
            *start = 0x8000;
            *limit = 0x9ffd;
            break;
        case 0x7000:
        case 0x6000:
        case 0x5000:
        case 0x4000:
        case 0x3000:
        case 0x2000:
        case 0x1000:
            *base = export_ram0;
            *start = 0x1000;
            *limit = 0x7ffd;
            break;
        default:
            *base = NULL;
            *start = 0;
            *limit = 0;
    }
}

void ide64_config_init(void)
{
    int i;
    struct drive_s *drive;

    debug("IDE64 init");
    cart_config_changed_slotmain(0, 0, CMODE_READ | CMODE_PHI2_RAM);
    current_bank = 0;
    current_cfg = 0;
    kill_port = 0;
    idrive = 0;

    for (i = 0; i < 4; i++) {
        drive = &drives[i];
        ata_update_timing(drive->drv, (CLOCK)machine_get_cycles_per_second());
        if (drive->update_needed) {
            drive->update_needed = 0;
            detect_ide64_image(drive);
            ata_image_attach(drive->drv, drive->filename, drive->type, drive->detected);
            memset(export_ram0, 0, 0x8000);
        }
    }
}

void ide64_config_setup(uint8_t *rawcart)
{
    debug("IDE64 setup");
    memcpy(roml_banks, rawcart, 0x80000);
    memset(export_ram0, 0, 0x8000);
}

void ide64_detach(void)
{
    int i;

    if (ds1302_context) {
        ds1202_1302_destroy(ds1302_context, ide64_rtc_save);
        ds1302_context = NULL;
    }


    for (i = 0; i < 4; i++) {
        if (drives[i].drv) {
            ata_image_detach(drives[i].drv);
            ata_shutdown(drives[i].drv);
            drives[i].drv = NULL;
        }
    }

    usbserver_activate(0);

    ide64_unregister();

    shortbus_unregister();

    debug("IDE64 detached");
}

static int ide64_common_attach(uint8_t *rawcart, int detect)
{
    int i;

    idebus = 0;
    ds1302_context = ds1202_1302_init("IDE64", 1302);

    if (detect) {
        for (i = 0x1e60; i < 0x1efd; i++) {
            if (rawcart[i] == 0x8d && ((rawcart[i + 1] - 2) & 0xfc) == 0x30 && rawcart[i + 2] == 0xde) {
                settings_version = IDE64_VERSION_3; /* V3 emulation required */
                break;
            }
            if (rawcart[i] == 0x8d && (rawcart[i + 1] & 0xf8) == 0x60 && rawcart[i + 2] == 0xde) {
                settings_version = IDE64_VERSION_4_1; /* V4 emulation required */
                break;
            }
        }
    }

    for (i = 0; i < 4; i++) {
        if (!drives[i].drv) {
            drives[i].drv = ata_init(i);
        }
        drives[i].update_needed = 1;
    }

    usbserver_activate(settings_usbserver);

    debug("IDE64 attached");

    shortbus_register();

    return ide64_register();
}

int ide64_bin_attach(const char *filename, uint8_t *rawcart)
{
    size_t len;
    FILE *fd;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return -1;
    }
    len = util_file_length(fd);
    fclose(fd);

    /* we accept 64k, 128k and full 512k images */
    if (len == 0x10000 || len == 0x20000 || len == 0x80000) {
        if (util_file_load(filename, rawcart, len, UTIL_FILE_LOAD_SKIP_ADDRESS) < 0) {
            return -1;
        }
    }

    return ide64_common_attach(rawcart, 1);
}

int ide64_crt_attach(FILE *fd, uint8_t *rawcart)
{
    crt_chip_header_t chip;
    int i;

    for (i = 0; i <= 31; i++) {
        if (crt_read_chip_header(&chip, fd)) {
            if (i == 4 || i == 8) {
                break;
            }
            return -1;
        }

        if (chip.start != 0x8000 || chip.size != 0x4000) {
            return -1;
        }

        if (chip.bank > 31) {
            return -1;
        }

        if (crt_read_chip(rawcart, chip.bank << 14, &chip, fd)) {
            return -1;
        }
    }

    return ide64_common_attach(rawcart, 1);
}

static int ide64_idebus_dump(void)
{
    if (ata_register_dump(drives[idrive].drv) == 0) {
        return 0;
    }
    return ata_register_dump(drives[idrive ^ 1].drv);
}

static int ide64_io_dump(void)
{
    static const char * const configs[4] = {
        "8k", "16k", "stnd", "open"
    };
    mon_out("Version: %d, Mode: %s, ", settings_version >= IDE64_VERSION_4_1 ? 4 : 3, (kill_port & 1) ? "Disabled" : "Enabled");
    mon_out("ROM bank: %d, Config: %s, Interface: %d\n", current_bank, configs[current_cfg], idrive >> 1);
    return 0;
}

static int ide64_rtc_dump(void)
{
    return ds1202_1302_dump(ds1302_context);
}

/* ---------------------------------------------------------------------*/
/*    snapshot support functions                                             */

/* CARTIDE snapshot module format:

   type  | name      | description
   -----------------------------
   WORD  | version   | IDE64 version
   ARRAY | ROML      | 65536 (3.x) / 131072 (4.1) / 524288 (4.2) BYTES of ROML data
   ARRAY | RAM       | 32768 BYTES of RAM data
   DWORD | bank      | current bank
   DWORD | config    | current config
   DWORD | kill port | kill port flag
   DWORD | idrive    | idrive
   WORD  | in d030   | input state of $d030 register
   WORD  | out d030  | output state of $d030 register
 */

static const char snap_module_name[] = "CARTIDE";
#define SNAP_MAJOR   0
#define SNAP_MINOR   0

int ide64_snapshot_write_module(snapshot_t *s)
{
    snapshot_module_t *m;
    int i;

    for (i = 0; i < 4; i++) {
        if (drives[i].drv) {
            if (ata_snapshot_write_module(drives[i].drv, s)) {
                return -1;
            }
        }
    }

    m = snapshot_module_create(s, snap_module_name, SNAP_MAJOR, SNAP_MINOR);

    if (m == NULL) {
        return -1;
    }

    SMW_DW(m, settings_version);
    switch (settings_version) {
        case IDE64_VERSION_3:
            SMW_BA(m, roml_banks, 0x10000);
            break;
        case IDE64_VERSION_4_1:
            SMW_BA(m, roml_banks, 0x20000);
            break;
        case IDE64_VERSION_4_2:
            SMW_BA(m, roml_banks, 0x80000);
            break;
    }
    SMW_BA(m, export_ram0, 0x8000);
    SMW_DW(m, current_bank);
    SMW_DW(m, current_cfg);
    SMW_B(m, kill_port);
    SMW_DW(m, idrive);
    SMW_W(m, in_d030);
    SMW_W(m, out_d030);

    snapshot_module_close(m);

    if (shortbus_write_snapshot_module(s) < 0) {
        return -1;
    }

    return ds1202_1302_write_snapshot(ds1302_context, s);
}

int ide64_snapshot_read_module(snapshot_t *s)
{
    uint8_t vmajor, vminor;
    snapshot_module_t *m;
    int i;

    for (i = 0; i < 4; i++) {
        if (!drives[i].drv) {
            drives[i].drv = ata_init(i);
            detect_ide64_image(&drives[i]);
            ata_image_attach(drives[i].drv, drives[i].filename, drives[i].type, drives[i].detected);
        }
        if (ata_snapshot_read_module(drives[i].drv, s)) {
            return -1;
        }
    }

    m = snapshot_module_open(s, snap_module_name, &vmajor, &vminor);

    if (m == NULL) {
        return -1;
    }

    /* Do not accept versions higher than current */
    if (snapshot_version_is_bigger(vmajor, vminor, SNAP_MAJOR, SNAP_MINOR)) {
        snapshot_set_error(SNAPSHOT_MODULE_HIGHER_VERSION);
        snapshot_module_close(m);
        return -1;
    }

    ide64_unregister();

    if (SMR_DW_INT(m, (int *)&settings_version) < 0) {
        goto fail;
    }

    switch (settings_version) {
        default:
            settings_version = IDE64_VERSION_3;
            break;
        case 1:
            settings_version = IDE64_VERSION_4_1;
            break;
        case 2:
            settings_version = IDE64_VERSION_4_2;
            break;
    }
    ide64_register();
    switch (settings_version) {
        case IDE64_VERSION_3:
            if (SMR_BA(m, roml_banks, 0x10000) < 0) {
                goto fail;
            }
            break;
        case IDE64_VERSION_4_1:
            if (SMR_BA(m, roml_banks, 0x20000) < 0) {
                goto fail;
            }
            break;
        case IDE64_VERSION_4_2:
            if (SMR_BA(m, roml_banks, 0x80000) < 0) {
                goto fail;
            }
            break;
    }

    if (0
        || SMR_BA(m, export_ram0, 0x8000) < 0
        || SMR_DW_INT(m, &current_bank) < 0
        || SMR_DW_INT(m, &current_cfg) < 0
        || SMR_B(m, &kill_port) < 0
        || SMR_DW_INT(m, &idrive) < 0
        || SMR_W(m, &in_d030) < 0
        || SMR_W(m, &out_d030) < 0) {
        goto fail;
    }

    switch (settings_version) {
        case IDE64_VERSION_3:
            current_bank &= 3;
            break;
        case IDE64_VERSION_4_1:
            current_bank &= 7;
            break;
        case IDE64_VERSION_4_2:
            current_bank &= 31;
            break;
    }

    current_cfg &= 3;

    if (idrive) {
        idrive = 2;
    }

    snapshot_module_close(m);

    if (ide64_common_attach(roml_banks, 0) < 0) {
        return -1;
    }

    if (shortbus_read_snapshot_module(s) < 0) {
        return -1;
    }
    return ds1202_1302_read_snapshot(ds1302_context, s);

fail:
    snapshot_module_close(m);
    return -1;
}
