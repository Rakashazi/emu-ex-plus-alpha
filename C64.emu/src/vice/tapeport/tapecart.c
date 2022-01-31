/** \file   tapecart.c
 * \brief   tapecart emulation
 *
 * \author  Ingo Korb <ingo@akana.de>
 *
 * based on sense-dongle.c by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 */

/*
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

/*
 * Note: This is a reimplementation of the original tapecart firmware.
 *
 * If you just want to understand how a tapecart works, refer to
 * the original programmer's reference:
 *   https://github.com/ikorb/tapecart/blob/master/doc/ProgRef.md
 *
 *
 * Warning: Messy jungle of event-triggered callback functions ahead.
 *
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "alarm.h"
#include "archdep.h"
#include "cmdline.h"
#include "crc32.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "resources.h"
#include "snapshot.h"
#include "tapeport.h"

#include "tapecart.h"


#define TCRT_MAGIC_LEN  13


static const char idstring[] = "TAPECART V1.0 W25QFLASH";


static int tapecart_enabled       = 0;
static int tapecart_update_tcrt   = 0;
static int tapecart_optimize_tcrt = 0;
static int tapecart_loglevel      = 0;

/* ------------------------------------------------------------------------- */

#undef max
#undef min
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

/* tapecart major modes */
typedef enum {
    MODE_UNINITIALIZED,
    MODE_STREAM,
    MODE_FASTLOAD,
    MODE_COMMAND,
    MODE_REINIT
} tapecart_mode_t;

typedef clock_t (*wait_handler_t)(void);

/* a few forward declarations */
static void    tapecart_shutdown(void);
static void    tapecart_store_motor(int port, int state);
static void    tapecart_store_write(int port, int state);
static void    tapecart_store_sense(int port, int state);
static int     tapecart_enable(int port, int val);

static int     tapecart_write_snapshot(int port, struct snapshot_s *s, int write_image);
static int     tapecart_read_snapshot(int port, struct snapshot_s *s);

static void    tapecart_pulse_alarm_handler(CLOCK offset, void *unused);
static void    tapecart_logic_alarm_handler(CLOCK offset, void *unused);

static void    tapecart_set_mode(tapecart_mode_t mode);
static clock_t fasttx_byte_advance(void);
static clock_t cmdmode_receive_command(void);

static int     load_tcrt(const char *filename, tapecart_memory_t *tcmem);
static void    update_tcrt(void);

#define VICE_MACHINE_MASK (VICE_MACHINE_C64|VICE_MACHINE_C64SC|VICE_MACHINE_C128)

static tapeport_device_t tapecart_device = {
    "tapecart",                   /* device name */
    TAPEPORT_DEVICE_TYPE_STORAGE, /* device is a 'storage' type device */
    VICE_MACHINE_MASK,            /* device works on x64/x64sc/x128 machines */
    TAPEPORT_PORT_1_MASK,         /* device only works on port 1 */
    tapecart_enable,              /* device enable function */
    NULL,                         /* NO device specific hard reset function */
    tapecart_shutdown,            /* device shutdown function */
    tapecart_store_motor,         /* set motor line function */
    tapecart_store_write,         /* set write line function */
    tapecart_store_sense,         /* set sense line function */
    NULL,                         /* NO set read line function */
    tapecart_write_snapshot,      /* device snapshot write function */
    tapecart_read_snapshot        /* device snapshot read function */
};

static alarm_t *tapecart_logic_alarm;
static alarm_t *tapecart_pulse_alarm;
static log_t    tapecart_log = LOG_ERR;

/* current physical state of tape port lines */
static int motor_state;
static int write_state;
static int sense_state;

typedef enum {
    SIGNAL_MOTOR,
    SIGNAL_WRITE,
    SIGNAL_SENSE
} tapesignal_t;

/* ------------------------------------------------------------------------- */
/*  tapecart-internal definitions and variables                              */
/* ------------------------------------------------------------------------- */

/* nominal TAP pulse lengths */
#define CBMPULSE_SHORT  0x30
#define CBMPULSE_MEDIUM 0x42
#define CBMPULSE_LONG   0x56
#define PULSE_CYCLES    8

/* trigger values for mode changes */
#define SREG_MAGIC_VALUE_FASTLOAD  0xca65
#define SREG_MAGIC_VALUE_COMMAND   0xfce2

/* flash parameters */
#define FLASH_PAGE_SIZE            256
#define FLASH_ERASE_SIZE           4096

/* first five bytes of the header block */
static const uint8_t tape_header[] = {
    0x03,       /* load to absolute address */
    0x02, 0x03, /* start address            */
    0x04, 0x03  /* end address + 1          */
};

/* two-byte autostart address plus checksum */
static const uint8_t tape_startvector[] = {
    0x51,       /* start address of loader */
    0x03,
    0x51 ^ 0x03 /* checksum */
};

/** stream mode variables **/

/* pulse buffer */
typedef struct {
    unsigned char length;
    unsigned char repetitions;
} pulse_t;

static pulse_t        current_pulse;
static unsigned int   pulse_read_index, pulse_count;
static unsigned int   sense_pause_ticks;
static unsigned short tapecart_shiftreg;

/** fastload mode variables **/

/* state list for 2-bit transmission */
typedef enum { /* order matters! */
    FASTTX_INIT,
    FASTTX_WAIT_WRITE_HIGH,
    FASTTX_BITS_54,
    FASTTX_BITS_76,
    FASTTX_BITS_10,
    FASTTX_BITS_32,
    FASTTX_HOLD_DONE,
    FASTTX_WAIT_WRITE_LOW,
    FASTTX_BUSY_DELAY
} fasttx_state_t;

static fasttx_state_t      fasttx_state;

/** shared between fastload mode and command mode */
static wait_handler_t  transfer_complete;
/* 256 bytes needed for dir_lookup */
static uint8_t         transfer_buffer[max(256, FLASH_PAGE_SIZE)];
static uint8_t        *transfer_ptr;
static uint8_t         transfer_byte;
static unsigned int    transfer_bit, transfer_remaining;



/** command mode variables **/

typedef enum {
  CMD_EXIT = 0,
  CMD_READ_DEVICEINFO,
  CMD_READ_DEVICESIZES,
  CMD_READ_CAPABILITIES,

  CMD_READ_FLASH  = 0x10,
  CMD_READ_FLASH_FAST,
  CMD_WRITE_FLASH,
  CMD_WRITE_FLASH_FAST, /* reserved, but not implemented */
  CMD_ERASE_FLASH_64K,
  CMD_ERASE_FLASH_BLOCK,
  CMD_CRC32_FLASH,

  CMD_READ_LOADER = 0x20,
  CMD_READ_LOADINFO,
  CMD_WRITE_LOADER,
  CMD_WRITE_LOADINFO,

  CMD_LED_OFF = 0x30,
  CMD_LED_ON,
  CMD_READ_DEBUGFLAGS,
  CMD_WRITE_DEBUGFLAGS,

  CMD_DIR_SETPARAMS = 0x40,
  CMD_DIR_LOOKUP,
} command_t;

typedef enum {
    RX1BIT_RECEIVE,
    RX1BIT_WAIT_WRITE_LOW,
    RX1BIT_WAIT_SENSE_HIGH,
    RX1BIT_WAIT_SENSE_LOW,
    RX1BIT_DELAY,
    RX1BIT_PROCESSING
} rx1bit_state_t;
static rx1bit_state_t rx1bit_state;

typedef enum {
    TX1BIT_PREPARE_SEND,
    TX1BIT_SEND,
    TX1BIT_WAIT_WRITE_LOW,
    TX1BIT_WAIT_WRITE_HIGH,
    TX1BIT_PROCESSING
} tx1bit_state_t;
static tx1bit_state_t tx1bit_state;

static uint8_t tapecart_debugflags[2];
static unsigned int dir_base, dir_entries, dir_name_len, dir_data_len;
static unsigned int flash_address, total_length, block_length;
static clock_t flash_erase_64k_time, flash_erase_page_time;
static clock_t flash_page_write_time;

/** common data **/

/* semi-memory-intensive data that is only allocated when tapecart is enabled */
typedef struct tapecart_buffers_s {
    pulse_t       pulse_buffer[8600]; /* note: measured length is 8585 pulses */
    unsigned char data[6 + 65535];    /* additional 3x2 byte header */
} tapecart_buffers_t;

/* callback system for waiting for specific signal levels */
typedef enum {
    WAIT_NONE,
    WAIT_WRITE_LOW,
    WAIT_WRITE_HIGH,
    WAIT_SENSE_LOW,
    WAIT_SENSE_HIGH
} statewait_t;

static statewait_t    wait_for_signal;
static wait_handler_t wait_handler;

static char               *tcrt_filename;
static tapecart_buffers_t *tapecart_buffers;
static tapecart_memory_t  *tapecart_memory;
static tapecart_mode_t     tapecart_mode  = MODE_UNINITIALIZED;
static tapecart_mode_t     requested_mode = MODE_STREAM;

static wait_handler_t      alarm_trigger_callback;


/* ------------------------------------------------------------------------- */

/* workaround for inverted tapeport functions to keep my sanity */
static void set_sense(int value)
{
    tapeport_set_tape_sense(!value, TAPEPORT_PORT_1);
}

static void set_write(int value)
{
    tapeport_set_write_in(!value, TAPEPORT_PORT_1);
}

/* ---------------------------------------------------------------------*/
/*  command line and resource stuff                                     */
/* ---------------------------------------------------------------------*/

static void clear_memory(tapecart_memory_t *memory)
{
    memset(memory, 0xff, sizeof(tapecart_memory_t));
    memory->changed = 0;
}

static int tapecart_enable(int port, int value)
{
    int val = !!value;

    /* don't do anything if the state stays the same */
    if (tapecart_enabled == val) {
        return 0;
    }

    if (val) {
        /* allocate the tapecart's internal memory */
        tapecart_memory = lib_malloc(sizeof(tapecart_memory_t));
        if (tapecart_memory == NULL) {
            return -1;
        }
        clear_memory(tapecart_memory);

        /* allocate buffers */
        tapecart_buffers = lib_calloc(1, sizeof(tapecart_buffers_t));
        if (tapecart_buffers == NULL) {
            return -1;
        }

        /* enable logging */
        tapecart_log = log_open("tapecart");
        if (tapecart_log == LOG_ERR) {
            return -1;
        }

        /* calculate delays, using W25Q16 datasheet "typical" times */
        flash_page_write_time = machine_get_cycles_per_second() * 700 / 1000000;
        flash_erase_64k_time  = machine_get_cycles_per_second() * 180 / 1000;
        flash_erase_page_time = machine_get_cycles_per_second() *  60 / 1000;

        /* allocate alarms */
        if (tapecart_logic_alarm == NULL) {
          tapecart_logic_alarm = alarm_new(maincpu_alarm_context,
                                           "tapecart_logic",
                                           tapecart_logic_alarm_handler, NULL);
          tapecart_pulse_alarm = alarm_new(maincpu_alarm_context,
                                           "tapecart_pulse",
                                           tapecart_pulse_alarm_handler, NULL);
        }

        /* if a TCRT name is already available, load it */
        if (tcrt_filename != NULL && *tcrt_filename != 0) {
            load_tcrt(tcrt_filename, tapecart_memory);
        }

    } else {
        if (tapecart_logic_alarm != NULL) {
            alarm_unset(tapecart_logic_alarm);
            alarm_unset(tapecart_pulse_alarm);
        }

        set_sense(1);

        lib_free(tapecart_memory);
        tapecart_memory = NULL;

        lib_free(tapecart_buffers);
        tapecart_buffers = NULL;

        if (tapecart_log != LOG_ERR) {
            log_close(tapecart_log);
        }
    }

    tapecart_enabled = val;
    return 0;
}

static int set_tapecart_update_tcrt(int value, void *unused_param)
{
    tapecart_update_tcrt = !!value;
    return 0;
}

static int set_tapecart_optimize_tcrt(int value, void *unused_param)
{
    tapecart_optimize_tcrt = !!value;
    return 0;
}

static int set_tapecart_loglevel(int value, void *unused_param)
{
    tapecart_loglevel = value;
    return 0;
}

static const resource_int_t resources_int[] = {
    { "TapecartUpdateTCRT", 1, RES_EVENT_STRICT, (resource_value_t)0,
      &tapecart_update_tcrt, set_tapecart_update_tcrt, NULL },
    { "TapecartOptimizeTCRT", 1, RES_EVENT_STRICT, (resource_value_t)0,
      &tapecart_optimize_tcrt, set_tapecart_optimize_tcrt, NULL },
    { "TapecartLogLevel", 0, RES_EVENT_NO, (resource_value_t)0,
      &tapecart_loglevel, set_tapecart_loglevel, NULL },
    RESOURCE_INT_LIST_END
};

static const resource_string_t resources_string[] = {
    { "TapecartTCRTFilename", "", RES_EVENT_NO, NULL,
      &tcrt_filename, tapecart_attach_tcrt, NULL },
    RESOURCE_STRING_LIST_END
};

int tapecart_resources_init(int amount)
{
    if (tapeport_device_register(TAPEPORT_DEVICE_TAPECART, &tapecart_device) < 0) {
        return -1;
    }

    if (resources_register_int(resources_int) < 0) {
        return -1;
    }

    return resources_register_string(resources_string);
}

static const cmdline_option_t cmdline_options[] =
{
    { "-tcrt", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "TapecartTCRTFilename", NULL,
      "<Name>", "Attach TCRT tapecart image" },
    { "-tapecartupdatetcrt", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapecartUpdateTCRT", (resource_value_t)1,
      NULL, "Enable updating tapecart .tcrt image" },
    { "+tapecartupdatetcrt", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapecartUpdateTCRT", (resource_value_t)0,
      NULL, "Disable updating tapecart .tcrt image" },
    { "-tapecartoptimizetcrt", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapecartOptimizeTCRT", (resource_value_t)1,
      NULL, "Enable tapecart .tcrt image optimization on write" },
    { "+tapecartoptimizetcrt", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "TapecartOptimizeTCRT", (resource_value_t)0,
      NULL, "Disable tapecart .tcrt image optimization on write" },
    { "-tapecartloglevel", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "TapecartLogLevel", NULL,
      NULL, "Set tapecart log verbosity" },
    CMDLINE_LIST_END
};

int tapecart_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}


/* ---------------------------------------------------------------------*/
/*  endian-independent buffer access functions                          */
/* ---------------------------------------------------------------------*/

static uint32_t get_u32_le(const uint8_t *buf)
{
    uint32_t val = *buf++;

    val |= (uint32_t)(*buf++) << 8;
    val |= (uint32_t)(*buf++) << 16;
    val |= (uint32_t)(*buf  ) << 24;
    return val;
}

static uint32_t get_u24_le(const uint8_t *buffer)
{
    return buffer[0] | (buffer[1] << 8) | (buffer[2] << 16);
}

static uint16_t get_u16_le(const uint8_t *buffer)
{
    return buffer[0] | (buffer[1] << 8);
}

static uint8_t *put_u32_le(uint8_t *buf, uint32_t value)
{
    *buf++ =  value        & 0xff;
    *buf++ = (value >>  8) & 0xff;
    *buf++ = (value >> 16) & 0xff;
    *buf++ = (value >> 24) & 0xff;
    return buf;
}

static uint8_t *put_u24_le(uint8_t *buf, uint32_t value)
{
    *buf++ =  value        & 0xff;
    *buf++ = (value >>  8) & 0xff;
    *buf++ = (value >> 16) & 0xff;
    return buf;
}

static uint8_t *put_u16_le(uint8_t *buf, uint16_t value)
{
    *buf++ =  value        & 0xff;
    *buf++ = (value >>  8) & 0xff;
    return buf;
}


/* ---------------------------------------------------------------------*/
/*  stream mode functions                                               */
/* ---------------------------------------------------------------------*/

static unsigned int needed_space = 0;

static int check_pulsebuffer_size(void)
{
    if (pulse_count >= sizeof(tapecart_buffers->pulse_buffer)/
        sizeof(tapecart_buffers->pulse_buffer[0])) {
        log_message(tapecart_log, "Pulse buffer overflow, need %u more",
                    ++needed_space);
        return 1;
    }

    return 0;
}

static void pulse_store(const unsigned char pulselen)
{
    if (check_pulsebuffer_size()) {
        return;
    }

    tapecart_buffers->pulse_buffer[pulse_count].length      = pulselen;
    tapecart_buffers->pulse_buffer[pulse_count].repetitions = 1;
    pulse_count++;
}

static void pulse_store_sync(unsigned int length)
{
    while (length > 0) {
        if (check_pulsebuffer_size()) {
            return;
        }

        tapecart_buffers->pulse_buffer[pulse_count].length = CBMPULSE_SHORT;

        if (length > 255) {
            tapecart_buffers->pulse_buffer[pulse_count].repetitions = 255;
        } else {
            tapecart_buffers->pulse_buffer[pulse_count].repetitions = length;
        }

        length -= tapecart_buffers->pulse_buffer[pulse_count].repetitions;
        pulse_count++;
    }
}

static void pulse_store_bit(const unsigned int index, const unsigned char bit)
{
    if (bit) {
        pulse_store(CBMPULSE_MEDIUM);
        pulse_store(CBMPULSE_SHORT);
    } else {
        pulse_store(CBMPULSE_SHORT);
        pulse_store(CBMPULSE_MEDIUM);
    }
}

static void pulse_store_byte(unsigned char byte)
{
    int parity = 1;
    int i;

    pulse_store(CBMPULSE_LONG);
    pulse_store(CBMPULSE_MEDIUM);

    for (i = 0; i < 8; i++) {
        pulse_store_bit(i, byte & 1);

        if (byte & 1) {
            parity = !parity;
        }

        byte >>= 1;
    }

    pulse_store_bit(8, parity);
}

static void construct_header(void)
{
    unsigned int i;
    unsigned char checksum = 0;

    for (i = 0; i < 5; i++) {
        checksum ^= tape_header[i];
        pulse_store_byte(tape_header[i]);
    }

    for (i = 0; i < 16; i++) {
        checksum ^= tapecart_memory->filename[i];
        pulse_store_byte(tapecart_memory->filename[i]);
    }

    for (i = 0; i < TAPECART_LOADER_SIZE; i++) {
        checksum ^= tapecart_memory->loader[i];
        pulse_store_byte(tapecart_memory->loader[i]);
    }

    pulse_store_byte(checksum);
}

static void construct_startvector(void)
{
    unsigned int i;

    for (i = 0; i < sizeof(tape_startvector)/sizeof(tape_startvector[0]); i++) {
        pulse_store_byte(tape_startvector[i]);
    }
}

static void construct_countdown(int repeat)
{
    unsigned int i;

    for (i = 9; i > 0; i--) {
        if (repeat) {
            pulse_store_byte(i);
        } else {
            pulse_store_byte(i | 0x80);
        }
    }
}

static void construct_datablock(void (*datafunc)(void))
{
    unsigned int repeat;

    for (repeat = 0; repeat < 2; repeat++) {
        construct_countdown(repeat);

        datafunc();

        /* end marker */
        pulse_store(CBMPULSE_LONG);
        pulse_store(CBMPULSE_SHORT);

        /* block gap */
        pulse_store_sync(60);
    }
}

static void construct_pulsestream(void)
{
    pulse_count               = 0;
    pulse_read_index          = 0;
    current_pulse.length      = 0;
    current_pulse.repetitions = 0;

    pulse_store_sync(1500);
    construct_datablock(construct_header);

    pulse_store_sync(1500);
    construct_datablock(construct_startvector);

    pulse_store_sync(100);
}

static int get_next_pulse(void)
{
    /* check for end of stream, force immediate end in non-stream mode */
    if (requested_mode != MODE_STREAM ||
        (current_pulse.repetitions == 0 && pulse_read_index >= pulse_count)) {
        pulse_read_index = 0;
        current_pulse.repetitions = 0;
        return -1;
    }

    if (!current_pulse.repetitions) {
        current_pulse = tapecart_buffers->pulse_buffer[pulse_read_index++];
    }
    current_pulse.repetitions--;

    return current_pulse.length * PULSE_CYCLES;
}

/* ---------------------------------------------------------------------*/
/*  fastload mode functions                                             */
/* ---------------------------------------------------------------------*/

/* common fasttx for fastload mode and READ_FLASH_FAST */
static clock_t fasttx_nibble_advance(void)
{
    /* FIXME: This assumes that 1 cycle == 1 microsecond */

    fasttx_state++; /* advance state */

    switch (fasttx_state) {
    case FASTTX_WAIT_WRITE_HIGH:
        wait_handler    = fasttx_nibble_advance;
        wait_for_signal = WAIT_WRITE_HIGH;
        transfer_byte   = *transfer_ptr++;
        transfer_remaining--;

        /* clear busy indication */
        if (tapecart_mode == MODE_FASTLOAD) {
            set_sense(0);
        } else {
            set_sense(1);
        }
        return 0; /* no alarm */

    case FASTTX_BITS_54:
        set_sense(transfer_byte & 0x20);
        set_write(transfer_byte & 0x10);
        alarm_trigger_callback = fasttx_nibble_advance;
        return 9;

    case FASTTX_BITS_76:
        set_sense(transfer_byte & 0x80);
        set_write(transfer_byte & 0x40);
        return 9;

    case FASTTX_BITS_10:
        set_sense(transfer_byte & 2);
        set_write(transfer_byte & 1);
        return 9;

    case FASTTX_BITS_32:
        set_sense(transfer_byte & 8);
        set_write(transfer_byte & 4);
        return 10;

    case FASTTX_HOLD_DONE:
        set_sense(1);
        set_write(1);
        return 1;

    case FASTTX_WAIT_WRITE_LOW:
        wait_handler    = fasttx_nibble_advance;
        wait_for_signal = WAIT_WRITE_LOW;
        return 0;

    case FASTTX_BUSY_DELAY:
        /* set busy indication */
        if (tapecart_mode == MODE_FASTLOAD) {
            set_sense(1);
        } else {
            set_sense(0);
        }

        alarm_trigger_callback = fasttx_byte_advance;
        return 1; /* random guess, not sure how much time it takes in reality */

    default:
        log_error(tapecart_log, "In fasttx_advance with unhandled state %u",
                  fasttx_state - 1);
        return 0;
    }
}

static clock_t fasttx_byte_advance(void)
{
    if (transfer_remaining) {
        fasttx_state = FASTTX_INIT;
        return fasttx_nibble_advance();
    } else {
        /* at end of buffer */
        return transfer_complete();
    }
}

static clock_t fastload_complete(void)
{
    tapecart_set_mode(MODE_STREAM);
    return 0;
}

static clock_t fastload_postdelay(void)
{
    /* 200ms delay after fastload */
    alarm_trigger_callback = fastload_complete;
    return machine_get_cycles_per_second() / 5;
}

static clock_t transmit_fast(clock_t delay,
                             uint8_t *buffer, unsigned int length,
                             wait_handler_t complete_callback)
{
    transfer_remaining     = length;
    transfer_ptr           = buffer;
    fasttx_state           = FASTTX_INIT;
    alarm_trigger_callback = fasttx_nibble_advance;
    transfer_complete      = complete_callback;
    return delay;
}

static clock_t fastload_mode_init(void)
{
    unsigned char *ptr = tapecart_buffers->data;
    unsigned short loadaddr, endaddr;

    /* retrieve/calculate actual start/end addresses */
    loadaddr = tapecart_memory->flash[tapecart_memory->data_offset    ] |
              (tapecart_memory->flash[tapecart_memory->data_offset + 1] << 8);
    endaddr = loadaddr + tapecart_memory->data_length - 2;

    ptr = put_u16_le(ptr, tapecart_memory->call_address);
    ptr = put_u16_le(ptr, endaddr);
    ptr = put_u16_le(ptr, loadaddr);

    /* copy the actual data to load */
    memcpy(ptr,
           tapecart_memory->flash + tapecart_memory->data_offset + 2,
           tapecart_memory->data_length - 2);

    /* transfer data after 100ms to give the C64 time to turn off the motor */
    return transmit_fast(machine_get_cycles_per_second() / 10,
                         tapecart_buffers->data,
                         6 + tapecart_memory->data_length - 2,
                         fastload_postdelay);
}


/* ---------------------------------------------------------------------*/
/*  command mode functions                                              */
/* ---------------------------------------------------------------------*/

static int validate_flashaddress(unsigned int address, unsigned int length)
{
    return (address < TAPECART_FLASH_SIZE) &&
        (address + length <= TAPECART_FLASH_SIZE);
}

static clock_t receive_1bit_callback(void)
{
    switch (rx1bit_state) {
        case RX1BIT_RECEIVE:
            *transfer_ptr = (*transfer_ptr << 1) | !!sense_state;

            if (++transfer_bit == 8) {
                transfer_ptr++;
                transfer_remaining--;
                transfer_bit = 0;
                wait_for_signal = WAIT_WRITE_LOW;
                rx1bit_state = RX1BIT_WAIT_WRITE_LOW;
            } else {
                wait_for_signal = WAIT_WRITE_HIGH;
            }
            break;

        case RX1BIT_WAIT_WRITE_LOW:
            /* waiting only triggers on changes, so check that sense is low */
            if (!sense_state) {
                wait_for_signal = WAIT_SENSE_HIGH;
                rx1bit_state = RX1BIT_WAIT_SENSE_HIGH;
                break;
            }

            /* sense is already high: */
            /* fall through */
        case RX1BIT_WAIT_SENSE_HIGH:
            wait_for_signal = WAIT_SENSE_LOW;
            rx1bit_state = RX1BIT_WAIT_SENSE_LOW;
            break;

        case RX1BIT_WAIT_SENSE_LOW:
            rx1bit_state = RX1BIT_DELAY;
            alarm_trigger_callback = receive_1bit_callback;
            return machine_get_cycles_per_second() / 100000; /* 10 microseconds */

        case RX1BIT_DELAY:
            set_sense(0);
            rx1bit_state = RX1BIT_PROCESSING;
            return 5; /* simulate a bit of processing delay */

        case RX1BIT_PROCESSING:
            if (transfer_remaining > 0) {
                wait_for_signal = WAIT_WRITE_HIGH;
                rx1bit_state    = RX1BIT_RECEIVE;
                set_sense(1);
            } else {
                return transfer_complete();
            }
            break;
    }

    return 0;
}

static clock_t receive_1bit_delayed(void)
{
    set_sense(1);
    wait_for_signal = WAIT_WRITE_HIGH;
    return 0;
}

static clock_t receive_1bit(clock_t delay,
                            uint8_t *buffer, unsigned int length,
                            wait_handler_t complete_callback)
{
    if (length == 0) {
        log_warning(tapecart_log,
                    "WARNING: attempted to do 1-bit-receive with length 0");
        return complete_callback();
    }

    transfer_ptr       = buffer;
    transfer_remaining = length;
    transfer_bit       = 0;
    transfer_complete  = complete_callback;

    wait_handler = receive_1bit_callback;
    rx1bit_state = RX1BIT_RECEIVE;

    if (delay > 0) {
        alarm_trigger_callback = receive_1bit_delayed;
        return delay;
    } else {
        set_sense(1);
        wait_for_signal = WAIT_WRITE_HIGH;
        return 0;
    }
}

static clock_t transmit_1bit_callback(void)
{
    switch (tx1bit_state) {
        case TX1BIT_PREPARE_SEND:
            wait_for_signal = WAIT_WRITE_LOW;
            tx1bit_state = TX1BIT_SEND;
            transfer_byte = *transfer_ptr;
            break;

        case TX1BIT_SEND:
            set_sense(transfer_byte & 0x80);
            transfer_byte <<= 1;

            if (++transfer_bit == 8) {
                transfer_ptr++;
                transfer_remaining--;
                transfer_bit = 0;
                wait_for_signal = WAIT_WRITE_HIGH;
                tx1bit_state = TX1BIT_WAIT_WRITE_HIGH;
            } else {
                wait_for_signal = WAIT_WRITE_LOW;
            }
            break;

        case TX1BIT_WAIT_WRITE_HIGH:
            tx1bit_state = TX1BIT_WAIT_WRITE_LOW;
            wait_for_signal = WAIT_WRITE_LOW;
            break;

        case TX1BIT_WAIT_WRITE_LOW:
            set_sense(0);
            tx1bit_state = TX1BIT_PROCESSING;
            alarm_trigger_callback = transmit_1bit_callback;
            return 5; /* simulate a bit of processing delay */

        case TX1BIT_PROCESSING:
            if (transfer_remaining > 0) {
                wait_for_signal = WAIT_WRITE_HIGH;
                tx1bit_state    = TX1BIT_PREPARE_SEND;
                set_sense(1);
            } else {
                return transfer_complete();
            }
            break;
    }

    return 0;
}

static clock_t transmit_1bit_delayed(void)
{
    set_sense(1);
    wait_for_signal = WAIT_WRITE_HIGH;
    return 0;
}

static clock_t transmit_1bit(clock_t delay,
                             uint8_t *buffer, unsigned int length,
                             wait_handler_t complete_callback)
{
    transfer_ptr       = buffer;
    transfer_remaining = length;
    transfer_bit       = 0;
    transfer_complete  = complete_callback;

    wait_handler    = transmit_1bit_callback;
    tx1bit_state    = TX1BIT_PREPARE_SEND;

    if (delay > 0) {
        alarm_trigger_callback = transmit_1bit_delayed;
        return delay;
    } else {
        set_sense(1);
        wait_for_signal = WAIT_WRITE_HIGH;
        return 0;
    }
}

/*
 * Callbacks for command implementations
 */

static clock_t cmd_read_flash(void)
{
    unsigned int address = get_u24_le(transfer_buffer);
    unsigned int length  = get_u16_le(transfer_buffer + 3);

    if (!validate_flashaddress(address, length)) {
        log_message(tapecart_log,
                    "read attempt beyond end of flash memory: address 0x%X length 0x%04x",
                    address, length);
        address = 0;
    }

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log,
                "reading %u byte from flash address 0x%X",
                length, address);
    }

    /* pre-transmit delay is pretty much handwaved */
    return transmit_1bit(4, tapecart_memory->flash + address, length, cmdmode_receive_command);
}

static clock_t cmd_read_flash_fast(void)
{
    unsigned int address = get_u24_le(transfer_buffer);
    unsigned int length  = get_u16_le(transfer_buffer + 3);

    if (!validate_flashaddress(address, length)) {
        log_message(tapecart_log,
                    "read attempt beyond end of flash memory: address 0x%X length 0x%04x",
                    address, length);
        address = 0;
    }

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log, "reading %u byte from flash address 0x%X",
                    length, address);
    }

    /* note: 1 cycle delay before start is probably inaccurate */
    return transmit_fast(1, tapecart_memory->flash + address, length,
                         cmdmode_receive_command);
}

static clock_t cmdmode_write_flash_page(void)
{
    int non_erased_write = 0;
    unsigned int i = 0;

    /* accept writes only to bytes that are erased */
    /* (flash datasheets no not specify what happens with non-erased bytes) */
    for (i = 0; i < block_length; i++) {
        if (tapecart_memory->flash[flash_address + i] == 0xff) {
            tapecart_memory->flash[flash_address + i] = transfer_buffer[i];
            tapecart_memory->changed = 1;
        } else if (tapecart_loglevel > 0 && !non_erased_write) {
            log_message(tapecart_log,
                        "detected write to non-erased address at 0x%X",
                        flash_address + i);
            non_erased_write = 1;
        }
    }

    flash_address += block_length;
    total_length  -= block_length;

    if (total_length > 0) {
        block_length = min(FLASH_PAGE_SIZE, total_length);
        return receive_1bit(flash_page_write_time, transfer_buffer,
                            block_length, cmdmode_write_flash_page);

    } else {
        alarm_trigger_callback = cmdmode_receive_command;
        return flash_page_write_time;
    }
}

static clock_t cmd_write_flash(void)
{
    flash_address = get_u24_le(transfer_buffer);
    total_length  = get_u16_le(transfer_buffer + 3);

    if (!validate_flashaddress(flash_address, total_length)) {
        log_message(tapecart_log,
                    "write attempt beyond end of flash memory: address 0x%X length 0x%04x",
                    flash_address, total_length);
        flash_address = 0;
    }

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log, "writing %u byte to flash address 0x%X",
                    total_length, flash_address);
    }

    if (flash_address & (FLASH_PAGE_SIZE - 1)) {
        /* start is unaligned */
        block_length = FLASH_PAGE_SIZE - (flash_address & (FLASH_PAGE_SIZE - 1));
    } else {
        block_length = FLASH_PAGE_SIZE;
    }
    block_length = min(block_length, total_length);

    return receive_1bit(flash_page_write_time, transfer_buffer, block_length,
                        cmdmode_write_flash_page);
}

static clock_t cmd_erase_flash_64k(void)
{
    unsigned int address = get_u24_le(transfer_buffer);

    if (!validate_flashaddress(address, 0)) {
        log_message(tapecart_log,
                    "erase attempt beyond end of flash memory: address 0x%X",
                    address);
    } else {
        address = address & ~0xffff;

        if (tapecart_loglevel > 1) {
            log_message(tapecart_log,
                        "erasing 64KiB starting at flash address 0x%X", address);
        }

        memset(tapecart_memory->flash + address, 0xff, 65536);
        tapecart_memory->changed = 1;
    }

    /* simulate typical erase time of SPI flash */
    alarm_trigger_callback = cmdmode_receive_command;
    return flash_erase_64k_time;
}

static clock_t cmd_erase_flash_block(void)
{
    unsigned int address = get_u24_le(transfer_buffer);

    if (!validate_flashaddress(address, 0)) {
        log_message(tapecart_log,
                    "erase attempt beyond end of flash memory: address 0x%X",
                    address);
    } else {
        address = address & ~(FLASH_ERASE_SIZE - 1);

        if (tapecart_loglevel > 1) {
            log_message(tapecart_log,
                        "erasing %d bytes starting at flash address 0x%X",
                        FLASH_ERASE_SIZE, address);
        }

        memset(tapecart_memory->flash + address, 0xff, FLASH_ERASE_SIZE);
        tapecart_memory->changed = 1;
    }

    /* simulate typical erase time of SPI flash */
    alarm_trigger_callback = cmdmode_receive_command;
    return flash_erase_page_time;
}

static clock_t cmd_crc32_flash(void)
{
    unsigned int address = get_u24_le(transfer_buffer);
    unsigned int length  = get_u24_le(transfer_buffer + 3);
    uint32_t crc;

    if (!validate_flashaddress(address, length)) {
        log_message(tapecart_log,
                    "CRC32 attempt beyond end of flash memory: address 0x%X length 0x%X",
                    address, length);
        address = 0;
        length  = 1;
    }

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log,
                    "calculating CRC from flash address 0x%X length %u",
                    address, length);
    }

    crc = crc32_buf((char *)(tapecart_memory->flash + address), length);
    put_u32_le(transfer_buffer, crc & 0xffffffffU);

    /* very handwaved processing time estimate */
    return transmit_1bit(4 * length, transfer_buffer, 4,
                         cmdmode_receive_command);
}

static clock_t cmd_write_loadinfo(void)
{
    tapecart_memory->data_offset  = get_u16_le(transfer_buffer);
    tapecart_memory->data_length  = get_u16_le(transfer_buffer + 2);
    tapecart_memory->call_address = get_u16_le(transfer_buffer + 4);
    memcpy(tapecart_memory->filename, transfer_buffer + 6,
           TAPECART_FILENAME_SIZE);
    tapecart_memory->changed = 0;

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log,
                    "write_loadinfo data address 0x%04x data length %d call_address 0x%04x (filename omitted)",
                    tapecart_memory->data_offset,
                    tapecart_memory->data_length,
                    tapecart_memory->call_address);
    }

    return cmdmode_receive_command();
}

static clock_t cmd_dir_setparams(void)
{
    dir_base     = get_u24_le(transfer_buffer);
    dir_entries  = get_u16_le(transfer_buffer + 3);
    dir_name_len = transfer_buffer[5];
    dir_data_len = transfer_buffer[6];

    if (dir_name_len > 16)
        dir_name_len = 16;

    if (!validate_flashaddress(dir_base,
                               dir_entries * (dir_name_len + dir_data_len))) {
        log_message(tapecart_log,
                    "directory search would fall off end of flash: "
                    "base 0x%X namelen %u datalen %u",
                    dir_base, dir_name_len, dir_data_len);
        dir_base    = 0;
        dir_entries = 1;
    }

    if (tapecart_loglevel > 1) {
        log_message(tapecart_log,
                    "dir_setparams base 0x%X entries %u name length %u data length %u",
                    dir_base, dir_entries, dir_name_len, dir_data_len);
    }

    return cmdmode_receive_command();
}

static clock_t cmd_dir_lookup(void)
{
    unsigned int i;
    uint8_t *ptr;
    clock_t processing_delay; /* note: very rough guess */

    ptr = tapecart_memory->flash + dir_base;
    for (i = 0; i < dir_entries; i++) {
        if (!memcmp(transfer_buffer, ptr, dir_name_len)) {
            /* name part matches */
            transfer_buffer[0] = 0;
            memcpy(transfer_buffer + 1, ptr + dir_name_len, dir_data_len);

            if (tapecart_loglevel > 1) {
                log_message(tapecart_log,
                            "successful dir lookup at entry %u", i);
            }

            processing_delay = (dir_name_len + dir_data_len) * (i+1);
            return transmit_1bit(processing_delay, transfer_buffer,
                                 1 + dir_data_len, cmdmode_receive_command);
        }

        ptr += dir_name_len + dir_data_len;
    }

    /* no matching entry found */
    if (tapecart_loglevel > 1) {
        log_message(tapecart_log, "failed dir lookup");
    }

    transfer_buffer[0] = 1;
    processing_delay = (dir_name_len + dir_data_len) * dir_entries;
    return transmit_1bit(processing_delay, transfer_buffer,
                         1, cmdmode_receive_command);
}

/*
 * main command dispatcher and implementation of "simple" commands
 */
static clock_t cmdmode_dispatch_command(void)
{
    uint8_t *ptr;

    if (tapecart_loglevel > 0) {
        log_message(tapecart_log, "received command 0x%02x",
                    transfer_buffer[0]);
    }

    switch (transfer_buffer[0]) {
        case CMD_EXIT:
            tapecart_set_mode(MODE_STREAM);
            break;

        case CMD_READ_DEVICEINFO:
            transmit_1bit(0, (uint8_t *)idstring,
                          (unsigned int)(sizeof(idstring)),
                          cmdmode_receive_command);
            break;

        case CMD_READ_DEVICESIZES:
            ptr = put_u24_le(transfer_buffer, TAPECART_FLASH_SIZE);
            ptr = put_u16_le(ptr, FLASH_PAGE_SIZE);
            ptr = put_u16_le(ptr, FLASH_ERASE_SIZE / FLASH_PAGE_SIZE);
            transmit_1bit(0, transfer_buffer, 7, cmdmode_receive_command);
            break;

        case CMD_READ_CAPABILITIES:
            /* no extended capabilities currently exist, so none are supported */
            memset(transfer_buffer, 0, 4);
            transmit_1bit(0, transfer_buffer, 4, cmdmode_receive_command);
            break;

        case CMD_READ_FLASH:
            receive_1bit(0, transfer_buffer, 5, cmd_read_flash);
            break;

        case CMD_READ_FLASH_FAST:
            receive_1bit(0, transfer_buffer, 5, cmd_read_flash_fast);
            break;

        case CMD_WRITE_FLASH:
            receive_1bit(0, transfer_buffer, 5, cmd_write_flash);
            break;

        case CMD_ERASE_FLASH_64K:
            receive_1bit(0, transfer_buffer, 3, cmd_erase_flash_64k);
            break;

        case CMD_ERASE_FLASH_BLOCK:
            receive_1bit(0, transfer_buffer, 3, cmd_erase_flash_block);
            break;

        case CMD_CRC32_FLASH:
            receive_1bit(0, transfer_buffer, 6, cmd_crc32_flash);
            break;

        case CMD_READ_LOADER:
            transmit_1bit(0, tapecart_memory->loader, TAPECART_LOADER_SIZE,
                          cmdmode_receive_command);
            break;

        case CMD_READ_LOADINFO:
            ptr = put_u16_le(transfer_buffer, tapecart_memory->data_offset);
            ptr = put_u16_le(ptr, tapecart_memory->data_length);
            ptr = put_u16_le(ptr, tapecart_memory->call_address);
            memcpy(ptr, tapecart_memory->filename, TAPECART_FILENAME_SIZE);
            transmit_1bit(0, transfer_buffer, 6 + TAPECART_FILENAME_SIZE,
                          cmdmode_receive_command);
            break;

        case CMD_WRITE_LOADER:
            receive_1bit(0, tapecart_memory->loader, TAPECART_LOADER_SIZE,
                         cmdmode_receive_command);
            tapecart_memory->changed = 1;
            break;

        case CMD_WRITE_LOADINFO:
            receive_1bit(0, transfer_buffer, 6 + TAPECART_FILENAME_SIZE,
                         cmd_write_loadinfo);
            break;

        case CMD_LED_OFF:
        case CMD_LED_ON:
            /* no parameters, ignore */
            break;

        case CMD_READ_DEBUGFLAGS:
            /* debugflags are stored, but ignored */
            /* technically this is incorrect for bit 0, */
            /* but I suspect nobody will ever use that over the tapeport */
            transmit_1bit(0, tapecart_debugflags, 2, cmdmode_receive_command);
            break;

        case CMD_WRITE_DEBUGFLAGS:
            receive_1bit(0, tapecart_debugflags, 2, cmdmode_receive_command);
            break;

        case CMD_DIR_SETPARAMS:
            receive_1bit(0, transfer_buffer, 7, cmd_dir_setparams);
            break;

        case CMD_DIR_LOOKUP:
            if (dir_name_len > 0) {
                receive_1bit(0, transfer_buffer, dir_name_len, cmd_dir_lookup);
            } else {
                cmd_dir_lookup();
            }
            break;

        default:
            if (tapecart_loglevel > 0) {
                log_message(tapecart_log,
                            "switching to streaming mode due to unknown command");
            }
            tapecart_set_mode(MODE_STREAM);
            break;
    }

    return 0;
}

static clock_t cmdmode_receive_command(void)
{
    alarm_unset(tapecart_logic_alarm);
    return receive_1bit(0, transfer_buffer, 1, cmdmode_dispatch_command);
}

static clock_t cmdmode_send_pulses(void)
{
    wait_handler    = cmdmode_receive_command;
    wait_for_signal = WAIT_WRITE_LOW;

    alarm_trigger_callback = cmdmode_send_pulses;
    tapeport_trigger_flux_change(1, TAPEPORT_PORT_1);

    return CBMPULSE_SHORT * PULSE_CYCLES;
}

static clock_t command_mode_init(void)
{
    set_sense(1);
    wait_handler    = cmdmode_send_pulses;
    wait_for_signal = WAIT_WRITE_HIGH;

    return 0;
}

/* ---------------------------------------------------------------------*/
/*  common fastload/command mode functions                              */
/* ---------------------------------------------------------------------*/

static void tapecart_set_mode(tapecart_mode_t mode)
{
    clock_t delay = 0;

    if (tapecart_mode == mode) {
        return;
    }

    /* clear any pending alarms */
    alarm_unset(tapecart_logic_alarm);
    alarm_unset(tapecart_pulse_alarm);

    tapecart_mode = mode;

    switch (mode) {
        case MODE_REINIT:
        case MODE_STREAM:
            /* reset stream */
            tapecart_mode = MODE_STREAM;
            requested_mode = MODE_STREAM;
            construct_pulsestream();
            set_sense(0);
            sense_pause_ticks = 0;
            alarm_set(tapecart_pulse_alarm, maincpu_clk + get_next_pulse());
            return;

        case MODE_FASTLOAD:
            delay = fastload_mode_init();
            break;

        case MODE_COMMAND:
            /* just one ms delay here */
            alarm_trigger_callback = command_mode_init;
            delay = machine_get_cycles_per_second() / 1000;
            break;

        default:
            break;
    }

    if (delay) {
        alarm_set(tapecart_logic_alarm, (CLOCK)(maincpu_clk + delay));
    }
}

static void tapecart_pulse_alarm_handler(CLOCK offset, void *data)
{
    alarm_unset(tapecart_pulse_alarm);

    /* pulses are only sent when the motor is active */
    if (tapecart_mode == MODE_STREAM && motor_state) {
        int pulselen = get_next_pulse();

        if (pulselen < 0) {
            /* at end of stream, release sense for 200ms */
            set_sense(1);
            sense_pause_ticks = 210; /* slightly more because of rounding */
            alarm_set(tapecart_logic_alarm,
                      (CLOCK)(maincpu_clk + machine_get_cycles_per_second() / 1000));
        } else {
            tapeport_trigger_flux_change(1, TAPEPORT_PORT_1);
            alarm_set(tapecart_pulse_alarm, maincpu_clk + pulselen - offset);
        }
    } /* no alarm needed if motor was turned off */
}

static void tapecart_logic_alarm_handler(CLOCK offset, void *data)
{
    clock_t next_alarm;

    alarm_unset(tapecart_logic_alarm);

    switch (tapecart_mode) {
        case MODE_STREAM:
            /* pulse completed, advance stream state */

            if (sense_pause_ticks > 0) {
                sense_pause_ticks--;
                if (sense_pause_ticks == 0) {
                    /* end of pause, activate sense again */
                    set_sense(0);
                    if (motor_state) {
                        alarm_set(tapecart_pulse_alarm, maincpu_clk + 10);
                    }
                } else {
                    /* check for mode change */
                    switch (requested_mode) {
                        case MODE_FASTLOAD:
                            if (tapecart_loglevel > 0) {
                                log_message(tapecart_log, "entering fastload mode");
                            }
                            tapecart_set_mode(MODE_FASTLOAD);
                            break;

                        case MODE_COMMAND:
                            if (tapecart_loglevel > 0) {
                                log_message(tapecart_log, "entering command mode");
                            }
                            tapecart_set_mode(MODE_COMMAND);
                            break;

                        default:
                            /* nothing, check again in one ms */
                            alarm_set(tapecart_logic_alarm,
                                      maincpu_clk + (CLOCK)machine_get_cycles_per_second() / 1000);
                            break;
                    }
                }
            }
            break;

        case MODE_FASTLOAD:
        case MODE_COMMAND:
            if (alarm_trigger_callback == NULL) {
                log_error(tapecart_log,
                          "ERROR: alarm_trigger_callback is NULL, will segfault now");
            }
            next_alarm = alarm_trigger_callback();

            if (next_alarm != 0) {
                alarm_set(tapecart_logic_alarm,
                        (CLOCK)(maincpu_clk + next_alarm - offset));
            }
            break;

        default:
            log_message(tapecart_log, "alarm while in unhandled mode %u",
                        tapecart_mode);
            break;
    }
}


/* ---------------------------------------------------------------------*/

static void tapecart_shutdown(void)
{
    update_tcrt();
}

static void tapecart_store_motor(int port, int state)
{
    /* called by VICE with the inverted state of the processor port bit, */
    /* which is the physical state of the line */
    motor_state = state;

    if (tapecart_mode == MODE_STREAM) {
        if (motor_state) {
            /* capture write line when motor changes */
            tapecart_shiftreg =
                ((tapecart_shiftreg << 1) | !!write_state) & 0xffff;
            if (tapecart_loglevel > 2) {
                log_message(tapecart_log, "shift contents now $%04x",
                            tapecart_shiftreg);
            }

            /* start sending pulses - the exact delay doesn't matter here */
            alarm_set(tapecart_pulse_alarm, maincpu_clk + 10);

            switch (tapecart_shiftreg) {
                case SREG_MAGIC_VALUE_FASTLOAD:
                    if (tapecart_loglevel > 1) {
                        log_message(tapecart_log,
                                    "found fastload mode magic value in shift register");
                    }
                    requested_mode = MODE_FASTLOAD;
                    break;

                case SREG_MAGIC_VALUE_COMMAND:
                    if (tapecart_loglevel > 1) {
                        log_message(tapecart_log,
                                    "found command mode magic value in shift register");
                    }
                    requested_mode = MODE_COMMAND;
                    break;

                default:
                    break;
            }
        }
    } else {
        if (motor_state) {
            /* technically not 100% like the real cart, but close enough */
            if (tapecart_loglevel > 0) {
                log_message(tapecart_log,
                            "switching to stream mode because motor is active");
            }
            tapecart_set_mode(MODE_STREAM);
        }
    }
}

static void tapecart_store_write(int port, int state)
{
    /* called by VICE with the actual state of the processor port bit */
    clock_t delta_ticks;

    write_state = state;

    if ((state == 0 && wait_for_signal == WAIT_WRITE_LOW) ||
        (state != 0 && wait_for_signal == WAIT_WRITE_HIGH)) {
        wait_for_signal = WAIT_NONE;
        delta_ticks = wait_handler();

        if (delta_ticks > 0) {
            alarm_set(tapecart_logic_alarm, (CLOCK)(maincpu_clk + delta_ticks));
        }
    }
}

static void tapecart_store_sense(int port, int inv_state)
{
    /* called by VICE with the INVERTED state of the processor port bit */
    int state = !inv_state;
    clock_t delta_ticks;

    sense_state = state;

    if ((state == 0 && wait_for_signal == WAIT_SENSE_LOW) ||
        (state != 0 && wait_for_signal == WAIT_SENSE_HIGH)) {
        wait_for_signal = WAIT_NONE;
        delta_ticks = wait_handler();

        if (delta_ticks > 0) {
            alarm_set(tapecart_logic_alarm, (CLOCK)(maincpu_clk + delta_ticks));
        }
    }
}

/* ---------------------------------------------------------------------*/
/*  TCRT handling                                                       */
/* ---------------------------------------------------------------------*/

#define TCRT_VERSION              1
#define TCRT_SIGNATURE_SIZE       16
#define TCRT_OFFSET_VERSION       16
#define TCRT_OFFSET_DATAADDR      18
#define TCRT_OFFSET_DATALENGTH    20
#define TCRT_OFFSET_CALLADDR      22
#define TCRT_OFFSET_FILENAME      24
#define TCRT_OFFSET_FLAGS         40
#define TCRT_FLAG_LOADERPRESENT   1
#define TCRT_FLAG_DATABLOCKOFFSET 2
#define TCRT_OFFSET_LOADER        41
#define TCRT_OFFSET_FLASHLENGTH   212
#define TCRT_OFFSET_FLASHDATA     216

#define TCRT_HEADER_SIZE          TCRT_OFFSET_FLASHDATA

const uint8_t tcrt_signature[TCRT_SIGNATURE_SIZE] = {
    0x74, 0x61, 0x70, 0x65,
    0x63, 0x61, 0x72, 0x74,
    0x49, 0x6d, 0x61, 0x67,
    0x65, 0x0d, 0x0a, 0x1a
};

/* the default tapecart loader, in case it isn't included in the TCRT file */
static const unsigned char default_loader[TAPECART_LOADER_SIZE] = {
#  include "tapecart-loader.h"
};

static int load_tcrt(const char *filename, tapecart_memory_t *tcmem)
{
    int retval = 0;
    FILE *fd;
    uint8_t tcrt_header[TCRT_HEADER_SIZE];
    uint32_t flashlen;

    fd = fopen(filename, MODE_READ);
    if (fd == NULL) {
        return 0;
    }

    do {
        /* sanity checks */
        if (fread(tcrt_header, TCRT_HEADER_SIZE, 1, fd) < 1) {
            log_error(LOG_DEFAULT, "could not read TCRT header.");
            break;
        }

        if (memcmp(tcrt_header, tcrt_signature, TCRT_SIGNATURE_SIZE)) {
            log_error(LOG_DEFAULT, "TCRT header invalid.");
            break;
        }

        if (tcrt_header[TCRT_OFFSET_VERSION] != TCRT_VERSION) {
            log_error(LOG_DEFAULT, "unknown TCRT version: %d",
                      tcrt_header[TCRT_OFFSET_VERSION]);
            break;
        }

        flashlen = get_u32_le(tcrt_header + TCRT_OFFSET_FLASHLENGTH);
        if (flashlen > TAPECART_FLASH_SIZE) {
            log_error(LOG_DEFAULT, "invalid flash data size in TCRT header: %u",
                      flashlen);
            break;
        }

        /* read data */
        if (fread(tcmem->flash, flashlen, 1, fd) < 1) {
            log_error(LOG_DEFAULT, "could not read data from TCRT file.");
            break;
        }
        memset(tcmem->flash + flashlen, 0xff, TAPECART_FLASH_SIZE - flashlen);

        if (tcrt_header[TCRT_OFFSET_FLAGS] & TCRT_FLAG_LOADERPRESENT) {
            memcpy(tcmem->loader, tcrt_header + TCRT_OFFSET_LOADER,
                   TAPECART_LOADER_SIZE);
        } else {
            memcpy(tcmem->loader, default_loader, TAPECART_LOADER_SIZE);
        }

        memcpy(tcmem->filename, tcrt_header + TCRT_OFFSET_FILENAME, 16);

        tcmem->data_offset  = get_u16_le(tcrt_header + TCRT_OFFSET_DATAADDR);
        tcmem->data_length  = get_u16_le(tcrt_header + TCRT_OFFSET_DATALENGTH);
        tcmem->call_address = get_u16_le(tcrt_header + TCRT_OFFSET_CALLADDR);
        tcmem->changed      = 0;

        retval = 1;
    } while (0);

    fclose(fd);
    return retval;
}

static int save_tcrt(const char *filename, tapecart_memory_t *tcmem)
{
    FILE *fd;
    uint8_t *ptr;
    uint8_t tcrt_header[TCRT_HEADER_SIZE];
    uint32_t flashlen = TAPECART_FLASH_SIZE;
    int retval = 0;

    if (tapecart_optimize_tcrt) {
        /* scan for last non-0xff byte */
        while (flashlen > 0 && tcmem->flash[flashlen - 1] == 0xff) {
            flashlen--;
        }
    }

    /* construct a tcrt header */
    memset(tcrt_header, 0, TCRT_HEADER_SIZE);
    memcpy(tcrt_header, tcrt_signature, TCRT_SIGNATURE_SIZE);

    ptr = put_u16_le(tcrt_header + TCRT_OFFSET_VERSION, TCRT_VERSION);
    ptr = put_u16_le(ptr, tapecart_memory->data_offset);
    ptr = put_u16_le(ptr, tapecart_memory->data_length);
    ptr = put_u16_le(ptr, tapecart_memory->call_address);
    memcpy(ptr, tapecart_memory->filename, TAPECART_FILENAME_SIZE);

    tcrt_header[TCRT_OFFSET_FLAGS] = TCRT_FLAG_LOADERPRESENT;
    memcpy(tcrt_header + TCRT_OFFSET_LOADER, tapecart_memory->loader,
           TAPECART_LOADER_SIZE);
    put_u32_le(tcrt_header + TCRT_OFFSET_FLASHLENGTH, flashlen);

    fd = fopen(filename, MODE_WRITE);
    if (fd == NULL) {
        return 0;
    }

    do {
        if (fwrite(tcrt_header, TCRT_HEADER_SIZE, 1, fd) != 1) {
            break;
        }

        if (fwrite(tapecart_memory->flash, flashlen, 1, fd) != 1) {
            break;
        }

        retval = 1;
    } while (0);

    fclose(fd);
    return retval;
}

/* update TCRT if updates enabled and changes pending */
static void update_tcrt(void)
{
    if (tcrt_filename && tapecart_memory->changed && tapecart_update_tcrt) {
        save_tcrt(tcrt_filename, tapecart_memory);
    }
}


/** \brief  Check magic of the tapecart \a filename
 *
 * Checks the first 13 bytes for 'tapecartImage', which seems to be the TCRT
 * header magic.
 *
 * \return  bool
 */
int tapecart_is_valid(const char *filename)
{
    unsigned char buffer[TCRT_SIGNATURE_SIZE];
    FILE *fp;

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        return 0;
    }

    if (fread(buffer, 1, TCRT_SIGNATURE_SIZE, fp) != TCRT_SIGNATURE_SIZE) {
        fclose(fp);
        return 0;
    }

    if (memcmp(buffer, tcrt_signature, TCRT_SIGNATURE_SIZE) != 0) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}


int tapecart_attach_tcrt(const char *filename, void *unused)
{
    if (!tapecart_enabled) {
        /* remember the name in case the tapecart is enabled later */
        if (tcrt_filename != NULL) {
            lib_free(tcrt_filename);
            tcrt_filename = NULL;
        }

        if (filename != NULL && *filename != 0) {
            tcrt_filename = lib_strdup(filename);
        }

        return 0;
    }

    update_tcrt();

    if (tcrt_filename != NULL) {
        lib_free(tcrt_filename);
        tcrt_filename = NULL;
    }

    if (filename == NULL || *filename == 0) {
        clear_memory(tapecart_memory);
    } else {
        if (!load_tcrt(filename, tapecart_memory)) {
            return -1;
        }

        tcrt_filename = lib_strdup(filename);
    }

    tapecart_set_mode(MODE_REINIT);
    return 0;
}


/** \brief  Flush current tapecart data to image file
 *
 * \return  0 on success, < 0 on failure
 */
int tapecart_flush_tcrt(void)
{
    if (tcrt_filename == NULL || tapecart_memory == NULL) {
        return -1;
    }
    if (!save_tcrt(tcrt_filename, tapecart_memory)) {
        return -1;
    }
    return 0;
}


/** \brief  Clean up memory used by the tapecart emulation
 *
 * `trct_filename` is allocated even when the tapecart is disabled, but never
 * freed. This will clean up the memory used by the filename.
 * Also, #tapecart_memory and #tapecart_buffers weren't properly freed.
 *
 * And yes, I know we use ${module}_shutdown() as the name for a cleanup
 * function, but #tapecart_shutdown is already taken.
 */
void tapecart_exit(void)
{
    if (tcrt_filename != NULL) {
        lib_free(tcrt_filename);
        tcrt_filename = NULL;
    }
    if (tapecart_memory != NULL) {
        lib_free(tapecart_memory);
        tapecart_memory = NULL;
    }
    if (tapecart_buffers != NULL) {
        lib_free(tapecart_buffers);
        tapecart_buffers = NULL;
    }
}


/* ---------------------------------------------------------------------*/
/*  snapshots                                                           */
/* ---------------------------------------------------------------------*/

static int tapecart_write_snapshot(int port, struct snapshot_s *s, int write_image)
{
    /* FIXME: Implement */
    log_error(tapecart_log,
              "taking tapecart snapshot not implemented yet");

    return 0; /* should be -1 */
}

static int tapecart_read_snapshot(int port, struct snapshot_s *s)
{
    /* FIXME: Implement */
    log_error(tapecart_log,
              "restoring tapecart from snapshot not implemented yet");

    return 0; /* should be -1 */
}
