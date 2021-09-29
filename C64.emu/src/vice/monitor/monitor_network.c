/** \file   monitor_network.c
 *  \brief  Monitor implementation - network access
 *
 *  \author Spiro Trikaliotis <spiro.trikaliotis@gmx.de>
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
#include "ui.h"
#include "uiapi.h"
#include "util.h"
#include "vicesocket.h"

#ifdef HAVE_NETWORK

#define ADDR_LIMIT(x) ((uint16_t)(addr_mask(x)))

static vice_network_socket_t * listen_socket = NULL;
static vice_network_socket_t * connected_socket = NULL;

static char * monitor_server_address = NULL;
static int monitor_enabled = 0;

int monitor_network_transmit(const char * buffer, size_t buffer_length)
{
    int error = 0;

    if (connected_socket) {
        size_t len = (size_t)vice_network_send(connected_socket, buffer, buffer_length, 0);

        if (len != buffer_length) {
            error = -1;
        } else {
            error = (int)len;
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
            p = lib_strdup(pbuffer);

            memmove(pbuffer, cr_end + 1, strlen(cr_end + 1));

            *pbuffer_pos -= (int)(((long)strlen(p) + (cr_end - cr_start) + 1));
            pbuffer[*pbuffer_pos] = 0;
            break;
        } else if (*pbuffer_pos >= buffer_size) {
            /* we have a command that is too large:
             * process it anyway, so the sender knows something is wrong
             */
            p = lib_strdup(pbuffer);
            *pbuffer_pos = 0;
            pbuffer[0] = 0;
            break;
        }
    } while (0);

    return p;
}

int monitor_network_get_command_line(char **prompt)
{
    static char buffer[260] = { 0 };
    static int bufferpos = 0;

    if(!monitor_network_data_available()) {
        return 1;
    }

    do {
        /* Do not read more from network until all commands in current buffer is fully processed */
        if (bufferpos == 0) {
            int n = monitor_network_receive(buffer + bufferpos,
                            sizeof buffer - (size_t)(bufferpos - 1));

            if (n > 0) {
                bufferpos += n;
            } else if (n <= 0) {
                monitor_network_quit();
                *prompt = NULL;
                return 0;
            }
        }

        *prompt = monitor_network_extract_text_command_line(buffer, sizeof buffer, &bufferpos);
        if (*prompt) {
            break;
        } else {
            /* if no cmd was returned - reset buffer to start and fetch new cmd. */
            bufferpos = 0;
        }

        ui_dispatch_events();
    } while (1);

    return 1;
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
    RESOURCE_STRING_LIST_END
};

/*! \brief integer resources used by the network monitor module */
static const resource_int_t resources_int[] = {
    { "MonitorServer", 0, RES_EVENT_STRICT, (resource_value_t)0,
      &monitor_enabled, set_monitor_enabled, NULL },
    RESOURCE_INT_LIST_END
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
    { "-remotemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MonitorServer", (resource_value_t)1,
      NULL, "Enable remote monitor" },
    { "+remotemonitor", SET_RESOURCE, CMDLINE_ATTRIB_NONE,
      NULL, NULL, "MonitorServer", (resource_value_t)0,
      NULL, "Disable remote monitor" },
    { "-remotemonitoraddress", SET_RESOURCE, CMDLINE_ATTRIB_NEED_ARGS,
      NULL, NULL, "MonitorServerAddress", NULL,
      "<Name>", "The local address the remote monitor should bind to" },
    CMDLINE_LIST_END
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

vice_network_socket_t *monitor_get_connected_socket() {
    return connected_socket;
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

int monitor_network_get_command_line(char **prompt)
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
