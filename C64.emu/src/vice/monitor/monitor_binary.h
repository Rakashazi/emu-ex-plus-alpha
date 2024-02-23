/** \file   monitor_binary.h
 *  \brief  Monitor implementation - binary network access
 *
 *  \author EmpathicQubit <empathicqubit@entan.gl>
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


#ifndef VICE_MONITOR_BINARY_H
#define VICE_MONITOR_BINARY_H

#include "types.h"
#include "uiapi.h"
#include "mon_breakpoint.h"
#include "vicesocket.h"

int monitor_binary_resources_init(void);
void monitor_binary_resources_shutdown(void);
int monitor_binary_cmdline_options_init(void);

void monitor_binary_response_checkpoint_info(uint32_t request_id, mon_checkpoint_t *checkpt, bool hit);
void monitor_binary_event_opened(void);
void monitor_binary_event_closed(void);

void monitor_check_binary(void);

int monitor_binary_receive(unsigned char *buffer, size_t buffer_length);
int monitor_binary_transmit(const unsigned char *buffer, size_t buffer_length);
int monitor_binary_get_command_line(void);

int monitor_is_binary(void);
vice_network_socket_t *monitor_binary_get_connected_socket(void);

ui_jam_action_t monitor_binary_ui_jam_dialog(const char *format, ...) VICE_ATTR_PRINTF;

#endif
