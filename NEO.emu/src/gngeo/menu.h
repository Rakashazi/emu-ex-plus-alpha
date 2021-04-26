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

#ifndef _MENU_H_
#define _MENU_H_

#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

//#include "SDL.h"
#include "list.h"

#define MENU_ACTION 1
#define MENU_CHECK  2
#define MENU_VALUE  3
#define MENU_LIST   4

typedef struct GN_MENU_ITEM {
    char *name;
    Uint32 type; /* ACTION, CHECK */
    int enabled;
        int val;
        char **lval;
        char *str;
        void *arg;
	int (*action)(struct GN_MENU_ITEM *self,void *param);
	void (*draw)(struct GN_MENU_ITEM *self);
}GN_MENU_ITEM;

typedef struct GN_MENU {
	char *title;
	int nb_elem;
	int current;
        int lastpos;
	int draw_type;
	LIST *item;
	int (*event_handling)(struct GN_MENU *self);
	void (*draw)(struct GN_MENU *self);
}GN_MENU;

//#define GN_MENU GN_LIST ???

#define MENU_TXT_X 62
#define MENU_TXT_Y 62


GN_MENU_ITEM* gn_menu_create_item(char *name,Uint32 type,
				  int (*action)(GN_MENU_ITEM *self,void *param),void *param);
int gn_menu_delete_item(GN_MENU_ITEM *menu);

int gn_init_skin(void);
int gn_loop_menu(GN_MENU *m);
Uint32 run_menu(void);
void gn_reset_pbar(void);
//void gn_init_pbar(const char *name,int size);
enum { PBAR_ACTION_LOADROM, PBAR_ACTION_DECRYPT, PBAR_ACTION_LOADGNO, PBAR_ACTION_SAVEGNO };
void gn_init_pbar(unsigned action,int size);
void gn_update_pbar(int pos);
void gn_terminate_pbar(void);

void gn_popup_error(char *name,char *fmt,...);
int gn_popup_question(char *name,char *fmt,...);

#define gn_popup_info gn_popup_error


#endif
