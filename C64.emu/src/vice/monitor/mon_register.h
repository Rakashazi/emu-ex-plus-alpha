/*
 * mon_register.h - The VICE built-in monitor register functions.
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

#ifndef VICE_MON_REGISTER_H
#define VICE_MON_REGISTER_H

struct mon_reg_list_s {
    /* Name of the register.  */
    const char *name;
    /* the register ID used by various functions */
    unsigned int id;
    /* Size of the register in bits.  */
    unsigned int size;
    /* flags, see below  */
    unsigned int flags;
    /* extra info, see below  */
    unsigned int extra;
    /* Value of the register.  */
    unsigned int val;
};
typedef struct mon_reg_list_s mon_reg_list_t;

/* bits for mon_reg_list_t->flags */
#define MON_REGISTER_IS_FLAGS   0x01    /* register contains flags and will be
                                           printed as bits */
#define MON_REGISTER_IS_MEMORY  0x02    /* memory mapped register, extra contains
                                           the memory address */

struct monitor_cpu_type_s;

/* CPU specific functions */

void mon_register6502_init(struct monitor_cpu_type_s *monitor_cpu_type);
void mon_register6502dtv_init(struct monitor_cpu_type_s *monitor_cpu_type);
void mon_registerR65C02_init(struct monitor_cpu_type_s *monitor_cpu_type);
void mon_register65816_init(struct monitor_cpu_type_s *monitor_cpu_type);
void mon_registerz80_init(struct monitor_cpu_type_s *monitor_cpu_type);
void mon_register6809_init(struct monitor_cpu_type_s *monitor_cpu_type);

/* common functions */

/* checks if a given register id is valid, returns 1 on valid, 0 on invalid */
int mon_register_valid(int mem, int reg_id);
/* takes a register by name, and returns its id. returns -1 on error */
int mon_register_name_to_id(int mem, char *name);
/* checks if a given string is a valid register name, returns 1 on valid, 0 on invalid */
int mon_register_name_valid(int mem, char *name);
/* takes a register by id, and returns its value. returns -1 on error */
int mon_register_id_to_value(int mem, int reg_id);
/* takes a register by name, and returns its value. returns -1 on error */
int mon_register_name_to_value(int mem, char *name);

#endif
