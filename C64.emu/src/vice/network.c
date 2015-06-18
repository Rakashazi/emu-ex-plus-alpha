/*
 * network.c - Connecting emulators via network.
 *
 * Written by
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#ifdef HAVE_NETWORK

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include "archdep.h"
#include "cmdline.h"
#include "interrupt.h"
#include "lib.h"
#include "log.h"
#include "machine.h"
#include "maincpu.h"
#include "mos6510.h"
#include "network.h"
#include "resources.h"
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "util.h"
#include "vice-event.h"
#include "vicesocket.h"
#include "vsync.h"
#include "vsyncapi.h"

/* #define NETWORK_DEBUG */

static network_mode_t network_mode = NETWORK_IDLE;

static int current_send_frame;
static int last_received_frame;
static vice_network_socket_t * listen_socket;
static vice_network_socket_t * network_socket;
static int suspended;

static char *server_name = NULL;
static char *server_bind_address = NULL;
static unsigned short server_port;
static int res_server_port;
static int frame_delta;
static int network_control;

static int frame_buffer_full;
static int current_frame, frame_to_play;
static event_list_state_t *frame_event_list = NULL;
static char *snapshotfilename;

static int set_server_name(const char *val, void *param)
{
    util_string_set(&server_name, val);
    return 0;
}

static int set_server_bind_address(const char *val, void *param)
{
    util_string_set(&server_bind_address, val);
    return 0;
}

static int set_server_port(int val, void *param)
{
    if (val < 0 || val > 65535) {
        return -1;
    }

    res_server_port = val;

    server_port = (unsigned short)res_server_port;

    return 0;
}

static int set_network_control(int val, void *param)
{
    network_control = val;

    /* don't let the server loose control */
    network_control |= NETWORK_CONTROL_RSRC;

    return 0;
}

/*---------- Resources ------------------------------------------------*/

static const resource_string_t resources_string[] = {
    { "NetworkServerName", "127.0.0.1", RES_EVENT_NO, NULL,
      &server_name, set_server_name, NULL },
    { "NetworkServerBindAddress", "", RES_EVENT_NO, NULL,
      &server_bind_address, set_server_bind_address, NULL },
    { NULL }
};

static const resource_int_t resources_int[] = {
    { "NetworkServerPort", 6502, RES_EVENT_NO, NULL,
      &res_server_port, set_server_port, NULL },
    { "NetworkControl", NETWORK_CONTROL_DEFAULT, RES_EVENT_SAME, NULL,
      &network_control, set_network_control, NULL },
    { NULL }
};

int network_resources_init(void)
{
    if (resources_register_string(resources_string) < 0) {
        return -1;
    }

    return resources_register_int(resources_int);
}

/*---------------------------------------------------------------------*/

static void network_set_mask(int offset, int val)
{
    switch (val) {
        case 1:
            network_control |= 1 << offset;
            break;
        case 2:
            network_control |= 1 << (offset + NETWORK_CONTROL_CLIENTOFFSET);
            break;
        case 3:
            network_control |= 1 << offset;
            network_control |= 1 << (offset + NETWORK_CONTROL_CLIENTOFFSET);
            break;
        default:
        case 0:
            break;
    }
}

static int network_control_cmd(const char *param, void *extra_param)
{
    int keyb;
    int joy1;
    int joy2;
    int dev;
    int rsrc;

    if (strlen(param) != 9) {
        return -1;
    }

    if (param[1] != ',' || param[3] != ',' || param[5] != ',' || param[7] != ',') {
        return -1;
    }

    keyb = param[0] - '0';
    joy1 = param[2] - '0';
    joy2 = param[4] - '0';
    dev = param[6] - '0';
    rsrc = param[8] - '0';

    if (keyb < 0 || keyb > 3 || joy1 < 0 || joy1 > 3 || joy2 < 0 || joy2 > 3 || dev < 0 || dev > 3 || rsrc < 0 || rsrc > 2) {
        return -1;
    }

    network_control = 0;

    network_set_mask(0, keyb);
    network_set_mask(1, joy1);
    network_set_mask(2, joy2);
    network_set_mask(3, dev);
    network_set_mask(4, rsrc);

    return 0;
}

static const cmdline_option_t cmdline_options[] = {
    { "-netplayserver", SET_RESOURCE, 1,
      NULL, NULL, "NetworkServerName", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_HOSTNAME, IDCLS_SET_NETPLAY_SERVER,
      NULL, NULL },
    { "-netplaybind", SET_RESOURCE, 1,
      NULL, NULL, "NetworkServerBindAddress", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_HOSTNAME, IDCLS_SET_NETPLAY_BIND_ADDRESS,
      NULL, NULL },
    { "-netplayport", SET_RESOURCE, 1,
      NULL, NULL, "NetworkServerPort", NULL,
      USE_PARAM_ID, USE_DESCRIPTION_ID,
      IDCLS_P_PORT, IDCLS_SET_NETPLAY_PORT,
      NULL, NULL },
    { "-netplayctrl", CALL_FUNCTION, 1,
      network_control_cmd, NULL, NULL, NULL,
      USE_PARAM_STRING, USE_DESCRIPTION_ID,
      IDCLS_UNUSED, IDCLS_SET_NETPLAY_CONTROL,
      "<key,joy1,joy2,dev,rsrc>", NULL },
    { NULL }
};

int network_cmdline_options_init(void)
{
    return cmdline_register_options(cmdline_options);
}


/*---------------------------------------------------------------------*/

static void network_free_frame_event_list(void)
{
    int i;

    if (frame_event_list != NULL) {
        for (i = 0; i < frame_delta; i++) {
            event_clear_list(&(frame_event_list[i]));
        }
        lib_free(frame_event_list);
        frame_event_list = NULL;
    }
    event_destroy_image_list();
}

static void network_event_record_sync_test(WORD addr, void *data)
{
    BYTE regbuf[5 * 4];

    util_dword_to_le_buf(&regbuf[0 * 4], (DWORD)(maincpu_get_pc()));
    util_dword_to_le_buf(&regbuf[1 * 4], (DWORD)(maincpu_get_a()));
    util_dword_to_le_buf(&regbuf[2 * 4], (DWORD)(maincpu_get_x()));
    util_dword_to_le_buf(&regbuf[3 * 4], (DWORD)(maincpu_get_y()));
    util_dword_to_le_buf(&regbuf[4 * 4], (DWORD)(maincpu_get_sp()));

    network_event_record(EVENT_SYNC_TEST, (void *)regbuf, sizeof(regbuf));
}

static void network_init_frame_event_list(void)
{
    frame_event_list = lib_malloc(sizeof(event_list_state_t) * frame_delta);
    memset(frame_event_list, 0, sizeof(event_list_state_t) * frame_delta);
    current_frame = 0;
    frame_buffer_full = 0;
    event_register_event_list(&(frame_event_list[0]));
    event_init_image_list();
    interrupt_maincpu_trigger_trap(network_event_record_sync_test, (void *)0);
}

static void network_prepare_next_frame(void)
{
    current_frame = (current_frame + 1) % frame_delta;
    frame_to_play = (current_frame + 1) % frame_delta;
    event_clear_list(&(frame_event_list[current_frame]));
    event_register_event_list(&(frame_event_list[current_frame]));
    interrupt_maincpu_trigger_trap(network_event_record_sync_test, (void *)0);
}

static unsigned int network_create_event_buffer(BYTE **buf,
                                                event_list_state_t *list)
{
    int size;
    BYTE *bufptr;
    event_list_t *current_event, *last_event;
    int data_len = 0;
    int num_of_events;

    if (list == NULL) {
        return 0;
    }

    /* calculate the buffer length */
    num_of_events = 0;
    current_event = list->base;
    do {
        num_of_events++;
        data_len += current_event->size;
        last_event = current_event;
        current_event = current_event->next;
    } while (last_event->type != EVENT_LIST_END);

    size = num_of_events * 3 * sizeof(DWORD) + data_len;

    *buf = lib_malloc(size);

    /* fill the buffer with the events */
    current_event = list->base;
    bufptr = *buf;
    do {
        util_dword_to_le_buf(&bufptr[0], (DWORD)(current_event->type));
        util_dword_to_le_buf(&bufptr[4], (DWORD)(current_event->clk));
        util_dword_to_le_buf(&bufptr[8], (DWORD)(current_event->size));
        memcpy(&bufptr[12], current_event->data, current_event->size);
        bufptr += 12 + current_event->size;
        last_event = current_event;
        current_event = current_event->next;
    } while (last_event->type != EVENT_LIST_END);

    return size;
}

static event_list_state_t *network_create_event_list(BYTE *remote_event_buffer)
{
    event_list_state_t *list;
    unsigned int type, size;
    BYTE *data;
    BYTE *bufptr = remote_event_buffer;

    list = lib_malloc(sizeof(event_list_state_t));
    event_register_event_list(list);

    do {
        type = util_le_buf_to_dword(&bufptr[0]);
        /*  clk = util_le_buf_to_dword(&bufptr[4]); */
        size = util_le_buf_to_dword(&bufptr[8]);
        data = &bufptr[12];
        bufptr += 12 + size;
        event_record_in_list(list, type, data, size);
    } while (type != EVENT_LIST_END);

    return list;
}

static int network_recv_buffer(vice_network_socket_t * s, BYTE *buf, int len)
{
    int t;
    int received_total = 0;

    while (received_total < len) {
        t = vice_network_receive(s, buf, len - received_total, 0);

        if (t < 0) {
            return t;
        }

        received_total += t;
        buf += t;
    }
    return 0;
}

#ifdef MSG_NOSIGNAL
#define SEND_FLAGS MSG_NOSIGNAL
#else
#define SEND_FLAGS 0
#endif

static int network_send_buffer(vice_network_socket_t * s, const BYTE *buf, int len)
{
    int t;
    int sent_total = 0;

    while (sent_total < len) {
        t = vice_network_send(s, buf, len - sent_total, SEND_FLAGS);

        if (t < 0) {
            return t;
        }

        sent_total += t;
        buf += t;
    }
    return 0;
}

#define NUM_OF_TESTPACKETS 50

typedef struct {
    unsigned long t;
    unsigned char buf[0x60];
} testpacket;

static void network_test_delay(void)
{
    int i, j;
    BYTE new_frame_delta;
    unsigned char *buf;
    testpacket pkt;

    long packet_delay[NUM_OF_TESTPACKETS];
    char st[256];

    vsyncarch_init();

    ui_display_statustext(translate_text(IDGS_TESTING_BEST_FRAME_DELAY), 0);

    buf = (unsigned char*)&pkt;

    if (network_mode == NETWORK_SERVER_CONNECTED) {
        for (i = 0; i < NUM_OF_TESTPACKETS; i++) {
            pkt.t = vsyncarch_gettime();
            if (network_send_buffer(network_socket, buf, sizeof(testpacket)) < 0
                || network_recv_buffer(network_socket, buf, sizeof(testpacket)) < 0) {
                return;
            }
            packet_delay[i] = vsyncarch_gettime() - pkt.t;
        }
        /* Sort the packets delays*/
        for (i = 0; i < NUM_OF_TESTPACKETS - 1; i++) {
            for (j = i + 1; j < NUM_OF_TESTPACKETS; j++) {
                if (packet_delay[i] < packet_delay[j]) {
                    long d = packet_delay[i];
                    packet_delay[i] = packet_delay[j];
                    packet_delay[j] = d;
                }
            }
#ifdef NETWORK_DEBUG
            log_debug("packet_delay[%d]=%ld", i, packet_delay[i]);
#endif
        }
#ifdef NETWORK_DEBUG
        log_debug("vsyncarch_frequency = %ld", vsyncarch_frequency());
#endif
        /* calculate delay with 90% of packets beeing fast enough */
        /* FIXME: This needs some further investigation */
        new_frame_delta = 5 + (BYTE)(vsync_get_refresh_frequency()
                                     * packet_delay[(int)(0.1 * NUM_OF_TESTPACKETS)]
                                     / (float)vsyncarch_frequency());
        network_send_buffer(network_socket, &new_frame_delta,
                            sizeof(new_frame_delta));
    } else {
        /* network_mode == NETWORK_CLIENT */
        for (i = 0; i < NUM_OF_TESTPACKETS; i++) {
            if (network_recv_buffer(network_socket, buf, sizeof(testpacket)) < 0
                || network_send_buffer(network_socket, buf, sizeof(testpacket)) < 0) {
                return;
            }
        }
        network_recv_buffer(network_socket, &new_frame_delta,
                            sizeof(new_frame_delta));
    }
    network_free_frame_event_list();
    frame_delta = new_frame_delta;
    network_init_frame_event_list();
    sprintf(st, translate_text(IDGS_USING_D_FRAMES_DELAY), frame_delta);
    log_debug("netplay connected with %d frames delta.", frame_delta);
    ui_display_statustext(st, 1);
}

static void network_server_connect_trap(WORD addr, void *data)
{
    FILE *f;
    BYTE *buf;
    size_t buf_size;
    BYTE send_size4[4];
    long i;
    event_list_state_t settings_list;

    vsync_suspend_speed_eval();

    /* Create snapshot and send it */
    snapshotfilename = archdep_tmpnam();
    if (machine_write_snapshot(snapshotfilename, 1, 1, 0) == 0) {
        f = fopen(snapshotfilename, MODE_READ);
        if (f == NULL) {
            ui_error(translate_text(IDGS_CANNOT_LOAD_SNAPSHOT_TRANSFER));
            lib_free(snapshotfilename);
            return;
        }
        buf_size = util_file_length(f);
        buf = lib_malloc(buf_size);
        if (fread(buf, 1, buf_size, f) <= 0) {
            log_debug("network_server_connect_trap read failed.");
        }
        fclose(f);

        ui_display_statustext(translate_text(IDGS_SENDING_SNAPSHOT_TO_CLIENT), 0);
        util_int_to_le_buf4(send_size4, (int)buf_size);
        network_send_buffer(network_socket, send_size4, 4);
        i = network_send_buffer(network_socket, buf, (int)buf_size);
        lib_free(buf);
        if (i < 0) {
            ui_error(translate_text(IDGS_CANNOT_SEND_SNAPSHOT_TO_CLIENT));
            ui_display_statustext("", 0);
            lib_free(snapshotfilename);
            return;
        }

        network_mode = NETWORK_SERVER_CONNECTED;

        /* Send settings that need to be the same */
        event_register_event_list(&settings_list);
        resources_get_event_safe_list(&settings_list);

        buf_size = (size_t)network_create_event_buffer(&buf, &(settings_list));
        util_int_to_le_buf4(send_size4, (int)buf_size);

        network_send_buffer(network_socket, send_size4, 4);
        network_send_buffer(network_socket, buf, (int)buf_size);

        event_clear_list(&settings_list);
        lib_free(buf);

        current_send_frame = 0;
        last_received_frame = 0;

        network_test_delay();
    } else {
        ui_error(translate_text(IDGS_CANNOT_CREATE_SNAPSHOT_FILE_S), snapshotfilename);
    }
    lib_free(snapshotfilename);
}

static void network_client_connect_trap(WORD addr, void *data)
{
    BYTE *buf;
    size_t buf_size;
    BYTE recv_buf4[4];
    event_list_state_t *settings_list;

    /* Set proper settings */
    if (resources_set_event_safe() < 0) {
        ui_error("Warning! Failed to set netplay-safe settings.");
    }

    /* Receive settings that need to be same as on server */
    if (network_recv_buffer(network_socket, recv_buf4, 4) < 0) {
        return;
    }

    buf_size = (size_t)util_le_buf4_to_int(recv_buf4);
    buf = lib_malloc(buf_size);

    if (network_recv_buffer(network_socket, buf, (int)buf_size) < 0) {
        return;
    }

    settings_list = network_create_event_list(buf);
    lib_free(buf);

    event_playback_event_list(settings_list);

    event_clear_list(settings_list);
    lib_free(settings_list);

    /* read the snapshot */
    if (machine_read_snapshot(snapshotfilename, 0) != 0) {
        ui_error(translate_text(IDGS_CANNOT_OPEN_SNAPSHOT_FILE_S), snapshotfilename);
        lib_free(snapshotfilename);
        return;
    }

    current_send_frame = 0;
    last_received_frame = 0;

    network_mode = NETWORK_CLIENT;

    network_test_delay();
    lib_free(snapshotfilename);
}

/*-------------------------------------------------------------------------*/

void network_event_record(unsigned int type, void *data, unsigned int size)
{
    unsigned int control = 0;
    BYTE joyport;

    switch (type) {
        case EVENT_KEYBOARD_MATRIX:
        case EVENT_KEYBOARD_RESTORE:
        case EVENT_KEYBOARD_DELAY:
        case EVENT_KEYBOARD_CLEAR:
            control = NETWORK_CONTROL_KEYB;
            break;
        case EVENT_ATTACHDISK:
        case EVENT_ATTACHTAPE:
        case EVENT_DATASETTE:
            control = NETWORK_CONTROL_DEVC;
            break;
        case EVENT_RESOURCE:
        case EVENT_RESETCPU:
            control = NETWORK_CONTROL_RSRC;
            break;
        case EVENT_JOYSTICK_VALUE:
            joyport = ((BYTE*)data)[0];
            if (joyport == 1) {
                control = NETWORK_CONTROL_JOY1;
            }
            if (joyport == 2) {
                control = NETWORK_CONTROL_JOY2;
            }
            break;
        default:
            control = 0;
    }

    if (network_get_mode() == NETWORK_CLIENT) {
        control <<= NETWORK_CONTROL_CLIENTOFFSET;
    }

    if (control != 0 && (control & network_control) == 0) {
        return;
    }

    event_record_in_list(&(frame_event_list[current_frame]), type, data, size);
}

void network_attach_image(unsigned int unit, const char *filename)
{
    unsigned int control = NETWORK_CONTROL_DEVC;

    if (network_get_mode() == NETWORK_CLIENT) {
        control <<= NETWORK_CONTROL_CLIENTOFFSET;
    }

    if ((control & network_control) == 0) {
        return;
    }

    event_record_attach_in_list(&(frame_event_list[current_frame]), unit, filename, 1);
}

int network_get_mode(void)
{
    return network_mode;
}

int network_connected(void)
{
    if (network_mode == NETWORK_SERVER_CONNECTED
        || network_mode == NETWORK_CLIENT) {
        return 1;
    } else {
        return 0;
    }
}

int network_start_server(void)
{
    vice_network_socket_address_t * server_addr = NULL;
    int ret = -1;

    do {
        if (network_mode != NETWORK_IDLE) {
            break;
        }

        server_addr = vice_network_address_generate(server_bind_address, server_port);
        if (!server_addr) {
            break;
        }

        listen_socket = vice_network_server(server_addr);
        if (!listen_socket) {
            break;
        }

        /* Set proper settings */
        if (resources_set_event_safe() < 0) {
            ui_error("Warning! Failed to set netplay-safe settings.");
        }

        network_mode = NETWORK_SERVER;

        vsync_suspend_speed_eval();
        ui_display_statustext(translate_text(IDGS_SERVER_IS_WAITING_FOR_CLIENT), 1);

        ret = 0;
    } while (0);

    if (server_addr) {
        vice_network_address_close(server_addr);
    }

    return ret;
}


int network_connect_client(void)
{
    vice_network_socket_address_t * server_addr;
    FILE *f;
    BYTE *buf;
    BYTE recv_buf4[4];
    size_t buf_size;

    if (network_mode != NETWORK_IDLE) {
        return -1;
    }

    vsync_suspend_speed_eval();

    snapshotfilename = NULL;

    f = archdep_mkstemp_fd(&snapshotfilename, MODE_WRITE);
    if (f == NULL) {
        ui_error(translate_text(IDGS_CANNOT_CREATE_SNAPSHOT_S_SELECT));
        return -1;
    }

    server_addr = vice_network_address_generate(server_name, server_port);
    if (server_addr == NULL) {
        ui_error(translate_text(IDGS_CANNOT_RESOLVE_S), server_name);
        return -1;
    }
    network_socket = vice_network_client(server_addr);

    vice_network_address_close(server_addr);
    server_addr = NULL;

    if (!network_socket) {
        ui_error(translate_text(IDGS_CANNOT_CONNECT_TO_S), server_name, server_port);
        lib_free(snapshotfilename);
        return -1;
    }

    ui_display_statustext(translate_text(IDGS_RECEIVING_SNAPSHOT_SERVER), 0);
    if (network_recv_buffer(network_socket, recv_buf4, 4) < 0) {
        lib_free(snapshotfilename);
        vice_network_socket_close(network_socket);
        return -1;
    }

    buf_size = (size_t)util_le_buf4_to_int(recv_buf4);
    buf = lib_malloc(buf_size);

    if (network_recv_buffer(network_socket, buf, (int)buf_size) < 0) {
        lib_free(snapshotfilename);
        vice_network_socket_close(network_socket);
        return -1;
    }

    if (fwrite(buf, 1, buf_size, f) <= 0) {
        log_debug("network_connect_client write failed.");
    }
    fclose(f);
    lib_free(buf);

    interrupt_maincpu_trigger_trap(network_client_connect_trap, (void *)0);
    vsync_suspend_speed_eval();

    return 0;
}

void network_disconnect(void)
{
    vice_network_socket_close(network_socket);
    if (network_mode == NETWORK_SERVER_CONNECTED) {
        network_mode = NETWORK_SERVER;
    } else {
        vice_network_socket_close(listen_socket);
        network_mode = NETWORK_IDLE;
    }
}

void network_suspend(void)
{
    int dummy_buf_len = 0;

    if (!network_connected() || suspended == 1) {
        return;
    }

    network_send_buffer(network_socket, (BYTE*)&dummy_buf_len, sizeof(unsigned int));

    suspended = 1;
}

#ifdef NETWORK_DEBUG
long t1, t2, t3, t4;
#endif

static void network_hook_connected_send(void)
{
    BYTE *local_event_buf = NULL;
    unsigned int send_len;
    BYTE send_len4[4];

    /* create and send current event buffer */
    network_event_record(EVENT_LIST_END, NULL, 0);
    send_len = network_create_event_buffer(&local_event_buf, &(frame_event_list[current_frame]));

#ifdef NETWORK_DEBUG
    t1 = vsyncarch_gettime();
#endif

    util_int_to_le_buf4(send_len4, (int)send_len);
    if (network_send_buffer(network_socket, send_len4, 4) < 0
        || network_send_buffer(network_socket, local_event_buf, send_len) < 0) {
        ui_display_statustext(translate_text(IDGS_REMOTE_HOST_DISCONNECTED), 1);
        network_disconnect();
    }
#ifdef NETWORK_DEBUG
    t2 = vsyncarch_gettime();
#endif

    lib_free(local_event_buf);
}

static void network_hook_connected_receive(void)
{
    BYTE *remote_event_buf = NULL;
    unsigned int recv_len;
    BYTE recv_len4[4];
    event_list_state_t *remote_event_list;
    event_list_state_t *client_event_list, *server_event_list;

    suspended = 0;

    if (current_frame == frame_delta - 1) {
        frame_buffer_full = 1;
    }

    if (frame_buffer_full) {
        do {
            if (network_recv_buffer(network_socket, recv_len4, 4) < 0) {
                ui_display_statustext(translate_text(IDGS_REMOTE_HOST_DISCONNECTED), 1);
                network_disconnect();
                return;
            }

            recv_len = util_le_buf4_to_int(recv_len4);
            if (recv_len == 0 && suspended == 0) {
                /* remote host suspended emulation */
                ui_display_statustext(translate_text(IDGS_REMOTE_HOST_SUSPENDING), 0);
                suspended = 1;
                vsync_suspend_speed_eval();
            }
        } while (recv_len == 0);

        if (suspended == 1) {
            ui_display_statustext("", 0);
        }

        remote_event_buf = lib_malloc(recv_len);

        if (network_recv_buffer(network_socket, remote_event_buf,
                                recv_len) < 0) {
            lib_free(remote_event_buf);
            return;
        }

#ifdef NETWORK_DEBUG
        t3 = vsyncarch_gettime();
#endif

        remote_event_list = network_create_event_list(remote_event_buf);
        lib_free(remote_event_buf);

        if (network_mode == NETWORK_SERVER_CONNECTED) {
            client_event_list = remote_event_list;
            server_event_list = &(frame_event_list[frame_to_play]);
        } else {
            server_event_list = remote_event_list;
            client_event_list = &(frame_event_list[frame_to_play]);
        }

        /* test for sync */
        if (client_event_list->base->type == EVENT_SYNC_TEST
            && server_event_list->base->type == EVENT_SYNC_TEST) {
            int i;

            for (i = 0; i < 5; i++) {
                if (((DWORD *)client_event_list->base->data)[i]
                    != ((DWORD *)server_event_list->base->data)[i]) {
                    ui_error(translate_text(IDGS_NETWORK_OUT_OF_SYNC));
                    network_disconnect();
                    /* shouldn't happen but resyncing would be nicer */
                    break;
                }
            }
        }

        /* replay the event_lists; server first, then client */
        event_playback_event_list(server_event_list);
        event_playback_event_list(client_event_list);

        event_clear_list(remote_event_list);
        lib_free(remote_event_list);
    }
    network_prepare_next_frame();
#ifdef NETWORK_DEBUG
    t4 = vsyncarch_gettime();
#endif
}

void network_hook(void)
{
    if (network_mode == NETWORK_IDLE) {
        return;
    }

    if (network_mode == NETWORK_SERVER) {
        if (vice_network_select_poll_one(listen_socket) != 0) {
            network_socket = vice_network_accept(listen_socket);

            if (network_socket) {
                interrupt_maincpu_trigger_trap(network_server_connect_trap,
                                               (void *)0);
            }
        }
    }

    if (network_connected()) {
        network_hook_connected_send();
        network_hook_connected_receive();
#ifdef NETWORK_DEBUG
        log_debug("network_hook timing: %5ld %5ld %5ld; total: %5ld",
                  t2 - t1, t3 - t2, t4 - t3, t4 - t1);
#endif
    }
}

void network_shutdown(void)
{
    if (network_connected()) {
        network_disconnect();
    }

    network_free_frame_event_list();
    lib_free(server_name);
    lib_free(server_bind_address);
}

#else

#include "network.h"

int network_resources_init(void)
{
    return 0;
}

void network_attach_image(unsigned int unit, const char *filename)
{
}

void network_event_record(unsigned int type, void *data, unsigned int size)
{
}

int network_connected(void)
{
    return 0;
}

int network_start_server(void)
{
    return 0;
}

int network_connect_client(void)
{
    return 0;
}

void network_disconnect(void)
{
}

void network_suspend(void)
{
}

void network_hook(void)
{
}

void network_shutdown(void)
{
}

int network_get_mode(void)
{
    return NETWORK_IDLE;
}
#endif
