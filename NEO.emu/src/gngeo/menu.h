/*  gngeo, a neogeo emulator
 *  Copyright (C) 2001 Peponas Thomas & Peponas Mathieu
 * 
 *  This program is free software; you can redistribute it and/or modify  
 *  it under the terms of the GNU General Public License as published by   
 *  the Free Software Foundation; either version 2 of the License, or    
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. 
 */

#pragma once

#include <gngeo-config.h>

void gn_reset_pbar(void);
enum { PBAR_ACTION_LOADROM, PBAR_ACTION_DECRYPT, PBAR_ACTION_LOADGNO, PBAR_ACTION_SAVEGNO };
void gn_init_pbar(unsigned action,int size);
void gn_update_pbar(int pos);
void gn_terminate_pbar(void);

void gn_popup_error(char *name,char *fmt,...);
int gn_popup_question(char *name,char *fmt,...);
