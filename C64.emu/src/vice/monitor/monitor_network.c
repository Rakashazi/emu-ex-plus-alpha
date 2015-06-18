/*! \file monitor_network.c \n
 *  \author Spiro Trikaliotis
 *  \brief   Monitor implementation - network access
 *
 * monitor_network.c - Monitor implementation - network access.
 *
 * Written by
 *  Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "lib.h"
#include "log.h"
#include "monitor.h"
#include "monitor_network.h"
#include "montypes.h"
#include "resources.h"
#include "translate.h"
#include "ui.h"
#include "uiapi.h"
#include "util.h"
#include "vicesocket.h"

#ifdef HAVE_NETWORK

#define ADDR_LIMIT(x) ((WORD)(addr_mask(x)))

static vice_network_socket_t * listen_socket = NULL;
static vice_network_socket_t * connected_socket = NULL;

static char * monitor_server_address = NULL;
static int monitor_enabled = 0;

static int monitor_binary_input = 0;


int monitor_network_transmit(const char * buffer, size_t buffer_length)
{
    int error = 0;

    if (connected_socket) {
        size_t len = vice_network_send(connected_socket, buffer, buffer_length, 0);

        if (len != buffer_length) {
            error = -1;
        } else {
            error = len;
        }
    }

    return error;
}

static void monitor_network_quit(void)
{
    vice_network_socket_close(connected_socket);
    connected_socket = NULL;
}

int monitor_network_receive(char * buffer, size_t buffer_length)
{
    int count = 0;

    do {
        if (!connected_socket) {
            break;
        }

        count = vice_network_receive(connected_socket, buffer, buffer_length, 0);

        if (count < 0) {
            log_message(LOG_DEFAULT, "monitor_network_receive(): vice_network_receive() returned -1, breaking connection");
            monitor_network_quit();
        }
    } while (0);

    return count;
}

static int monitor_network_data_available(void)
{
    int available = 0;

    if (connected_socket != NULL) {
        available = vice_network_select_poll_one(connected_socket);
    } else if (listen_socket != NULL) {
        /* we have no connection yet, allow for connection */

        if (vice_network_select_poll_one(listen_socket)) {
            connected_socket = vice_network_accept(listen_socket);
        }
    }


    return available;
}

void monitor_check_remote(void)
{
    if (monitor_network_data_available()) {
        monitor_startup_trap();
    }
}

static char * monitor_network_extract_text_command_line(char * pbuffer, int buffer_size, int * pbuffer_pos)
{
    char * cr_start = NULL;
    char * cr_end = NULL;
    char * p = NULL;

    do {
        cr_start = strchr(pbuffer, '\n');
        cr_end = strchr(pbuffer, '\r');

        if (cr_start || cr_end) {
            if (cr_start == NULL) {
                cr_start = cr_end;
            } else if (cr_end == NULL) {
                cr_end = cr_start;
            } else if (cr_end < cr_start) {
                char * cr_temp = cr_end;
                cr_end = cr_start;
                cr_start = cr_temp;
            }

            assert(cr_start != NULL);
            assert(cr_end != NULL);
        }

        if (cr_start) {
            assert(cr_end != NULL);

            *cr_start = 0;
            p = lib_stralloc(pbuffer);

            memmove(pbuffer, cr_end + 1, strlen(cr_end + 1));

            *pbuffer_pos -= (int)(strlen(p) + (cr_end - cr_start) + 1);
            pbuffer[*pbuffer_pos] = 0;
            break;
        } else if (*pbuffer_pos >= buffer_size) {
            /* we have a command that is too large:
             * process it anyway, so the sender knows something is wrong
             */
            p = lib_stralloc(pbuffer);
            *pbuffer_pos = 0;
            pbuffer[0] = 0;
            break;
        }
    } while (0);

    return p;
}

/*
    The binary remote monitor commands are injected into the "normal" commands.
    The remote monitor detects a binary command because it starts with ASCII STX
    (0x02). After this, there is one byte telling the length of the command. The
    next byte describes the command. Currently, only 0x01 is implemented which
    is "memdump".

    Note that the command length byte (the one after STX) does *not* count the
    STX, the command length nor the command byte.

    Also note that there is no termination character. The command length acts as
    synchronisation point.

    For the memdump command, the next bytes are as follows:
    1. start address low
    2. start address high
    3. end address low
    4. end address high
    5. memspace

    The memspace describes which part of the computer you want to read:
    0 --> the computer (C64)
    1 --> drive 8, 2 --> drive 9, 3 --> drive 10, 4 --> drive 11

    So, for a memdump of 0xa0fe to 0xa123, you have to issue the bytes
    (in this order):

    0x02 (STX), 0x05 (command length), 0x01 (command: memdump), 0xfe (SA low),
    0xa0 (SA high), 0x23 (EA low), 0xa1 (EA high), 0x00 (computer memspace)

    The answer looks as follows:

    byte 0: STX (0x02)
    byte 1: answer length low
    byte 2: answer length (bits 8-15)
    byte 3: answer length (bits 16-23)
    byte 4: answer length (bits 24-31, that is, high)
    byte 5: error code
    byte 6 - (answer length+6): the binary answer
    [...]

    Error codes are currently:
    0x00: ok, everything worked
    0x80: command length is not long enough for this specific command
    0x81: an invalid parameter occurred

    If an error stats but "ok" occurs, then VICE will output more details for
    the reason into its log. [...]
*/

#define ASC_STX 0x02
#define MON_CMD_MEMDUMP 1

#define MON_ERR_OK            0
#define MON_ERR_CMD_TOO_SHORT 0x80  /* command length is not enough for this command */
#define MON_ERR_INVALID_PARAMETER 0x81  /* command has invalid parameters */

static void monitor_network_binary_answer(unsigned int length, unsigned char errorcode, unsigned char * answer)
{
    unsigned char binlength[6];

    binlength[0] = ASC_STX;
    binlength[1] = length & 0xFFu;
    binlength[2] = (length >> 8) & 0xFFu;
    binlength[3] = (length >> 16) & 0xFFu;
    binlength[4] = (length >> 24) & 0xFFu;
    binlength[5] = errorcode;

    monitor_network_transmit((char*)binlength, sizeof binlength);

    if (answer != NULL) {
        monitor_network_transmit((char*)answer, length);
    }
}

static void monitor_network_binary_error(unsigned char errorcode)
{
    monitor_network_binary_answer(0, errorcode, NULL);
}

static void monitor_network_process_binary_command(unsigned char * pbuffer, int buffer_size, int * pbuffer_pos, unsigned int command_length)
{
    int command = pbuffer[2];
    int ok = 1;

    switch (command) {
        case MON_CMD_MEMDUMP:
            if (command_length < 5) {
                monitor_network_binary_error(MON_ERR_CMD_TOO_SHORT);
            } else {
                unsigned int startaddress = pbuffer[3] | (pbuffer[4] << 8);
                unsigned int endaddress = pbuffer[5] | (pbuffer[6] << 8);

                MEMSPACE memspace = e_default_space;

                switch (pbuffer[7]) {
                    case 0: memspace = e_comp_space; break;
                    case 1: memspace = e_disk8_space; break;
                    case 2: memspace = e_disk9_space; break;
                    case 3: memspace = e_disk10_space; break;
                    case 4: memspace = e_disk11_space; break;
                    default:
                        monitor_network_binary_error(MON_ERR_INVALID_PARAMETER);
                        log_message(LOG_DEFAULT, "monitor_network binary memdump: Unknown memspace %u", pbuffer[7]);
                        ok = 0;
                }

                if (startaddress >= endaddress) {
                    monitor_network_binary_error(MON_ERR_INVALID_PARAMETER);
                    log_message(LOG_DEFAULT, "monitor_network binary memdump: wrong start and/or end address %04x - %04x",
                                startaddress, endaddress);

                    ok = 0;
                }

                if (ok) {
                    unsigned int length = endaddress - startaddress + 1;
                    unsigned int i;

                    unsigned char * p = lib_malloc(length);

                    for (i = 0; i < length; i++) {
                        p[i] = mon_get_mem_val(memspace, (WORD)ADDR_LIMIT(startaddress + i));
                    }

                    monitor_network_binary_answer(length, MON_ERR_OK, p);
                    lib_free(p);
                }
            }
            break;

        default:
            log_message(LOG_DEFAULT, "monitor_network binary command: unknown command %u, skipping command length of %u", command, command_length);
            break;
    }

    *pbuffer_pos = 0;
    pbuffer[0] = 0;
}


char * monitor_network_get_command_line(void)
{
    static char buffer[260] = { 0 };
    static int bufferpos = 0;

    char * p = NULL;

    do {
        /* Do not read more from network until all commands in current buffer is fully processed */
        if (bufferpos == 0) {
            int n = monitor_network_receive(buffer + bufferpos, sizeof buffer - bufferpos - 1);

            if (n > 0) {
                bufferpos += n;
            } else if (n <= 0) {
                monitor_network_quit();
                break;
            }

            /* check if we got a binary command */
            if (bufferpos == n) {
                if (buffer[0] == ASC_STX) {
                    monitor_binary_input = 1;
                }
            }
        }

        if (monitor_binary_input) {
            if (bufferpos > 2) {
                /* we already got the length, get it */
                unsigned int command_length = buffer[1];

                if (3 + command_length <= (unsigned int)bufferpos) {
                    monitor_network_process_binary_command((unsigned char*)buffer, sizeof buffer, &bufferpos, command_length);
                    monitor_binary_input = 0;
                }
            } else {
                bufferpos = 0;
            }
            monitor_binary_input = 0;
        } else {
            p = monitor_network_extract_text_command_line(buffer, sizeof buffer, &bufferpos);
            if (p) {
                break;
            } else {
                /* if no cmd was returned - reset buffer to start and fetch new cmd. */
                bufferpos = 0;
            }
        }

        ui_dispatch_events();
    } while (1);

    return p;
}

static int monitor_network_activate(void)
{
    vice_network_socket_address_t * server_addr = NULL;
    int error = 1;

    do {
        if (!monitor_server_address) {
            break;
        }

        server_addr = vice_network_address_generate(monitor_server_address, 0);
        if (!server_addr) {
            break;
        }

        listen_socket = vice_network_server(server_addr);
        if (!listen_socket) {
            break;
        }

        error = 0;
    } while (0);

    if (server_addr) {
        vice_network_address_close(server_addr);
    }

    return error;
}

static int monitor_network_deactivate(void)
{
    if (listen_socket) {
        vice_network_socket_close(listen_socket);
        listen_socket = NULL;
    }

    return 0;
}

/* ------------------------------------------------------------------------- */

/*! \internal \brief set the network monitor to the enabled or disabled state

 \param val
   if 0, disable the network monitor; else, enable it.

 \param param
   unused

 \return
   0 on success. else -1.
*/
static int set_monitor_enabled(int value, void *param)
{
    int val = value ? 1 : 0;

    if (!val) {
        if (monitor_enabled) {
            if (monitor_network_deactivate() < 0) {
                return -1;
            }
        }
        monitor_enabled = 0;
        return 0;
    } else {
        if (!monitor_enabled) {
            if (monitor_network_activate() < 0) {
                return -1;
            }
        }

        monitor_enabled = 1;
        return 0;
    }
}

/*! \internal \brief set the network address of the network monitor

 \param name
   pointer to a buffer which holds the network server addresss.

 \param param
   unused

 \return
   0 on success, else -1.
*/
static int set_server_address(const char *name, void *param)
{
    if (monitor_server_address != NULL && name != NULL
        && strcmp(name, monitor_server_address) == 0) {
        return 0;
    }

    if (monitor_enabled) {
        monitor_network_deactivate();
    }
    util_string_set(&monitor_server_address, name);

    if (monitor_enabled) {
        monitor_network_activate();
    }

    return 0;
}

/*! \brief string resources used by the network monitor module */
static const resource_string_t resources_string[] = {
    { "MonitorServerAddress", "ip4://127.0.0.1:6510", RES_EVENT_NO, NULL,
      &monitor_server_address, set_server_address, NULL },
    { NULL }
};

/*! \brief integer resources used by the network monitor module */
static const resource_int_t resources_int[] = {
    { "MonitorServer", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &monitor_enabled, set_monitor_enabled, NULL },
    { NULL }
};

/*! \brief initialize the network monitor resources
 \return
   0 on success, else -1.

 \remark
   Registers the string and the integer resources
*/
int monitor_network_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*! \brief uninitialize the network monitor resources */
void monitor_network_resources_shutdown(void)
{
    monitor_network_deactivate();
    monitor_network_quit();

    lib_free(monitor_server_address);
}

/* ------------------------------------------------------------------------- */

static const cmdline_option_t cmdline_options[] =
{
    { "-remotemonitor", SET_RESOURCE, 0,
      NULL, NULL, "MonitorServer", (resource_value_t)1,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_ENABLE_REMOTE_MONITOR,
      NULL, NULL },
    { "+remotemonitor", SET_RESOURCE, 0,
      NULL, NULL, "MonitorServer", (resource_value_t)0,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_DISABLE_REMOTE_MONITOR,
      NULL, NULL },
    { "-remotemonitoraddress", SET_RESOURCE, 1,
      NULL, NULL, "MonitorServerAddress", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_NAME, IDCLS_REMOTE_MONITOR_ADDRESS,
      NULL, NULL },
    { NULL }
};

/*! \brief initialize the command-line options'
 \return
   0 on success, else -1.

 \remark
   Registers the command-line options
*/
int monitor_network_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}

int monitor_is_remote(void)
{
    return connected_socket != NULL;
}

ui_jam_action_t monitor_network_ui_jam_dialog(const char *format, ...)
{
    char * txt;

    char init_string[] = "An error occurred: ";
    char end_string[] = "\n";

    va_list ap;
    va_start(ap, format);
    txt = lib_mvsprintf(format, ap);
    va_end(ap);

    monitor_network_transmit(init_string, sizeof init_string - 1);
    monitor_network_transmit(txt, strlen(txt));
    monitor_network_transmit(end_string, sizeof end_string - 1);

    lib_free(txt);

    return UI_JAM_MONITOR;
}

#else

int monitor_network_resources_init(void)
{
    return 0;
}

void monitor_network_resources_shutdown(void)
{
}

int monitor_network_cmdline_options_init(void)
{
    return 0;
}

void monitor_check_remote(void)
{
}

int monitor_network_transmit(const char * buffer, size_t buffer_length)
{
    return 0;
}

char * monitor_network_get_command_line(void)
{
    return 0;
}

int monitor_is_remote(void)
{
    return 0;
}

ui_jam_action_t monitor_network_ui_jam_dialog(const char *format, ...)
{
    return UI_JAM_HARD_RESET;
}

#endif
