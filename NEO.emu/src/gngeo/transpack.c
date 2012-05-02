/*  gngeo a neogeo emulator
 *  Copyright (C) 2001 Peponas Mathieu
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "SDL.h"

#include <stdlib.h>
#include <string.h>

#include "transpack.h"


static void trans_pack_add(Uint32 begin,Uint32 end,Uint32 type)
{
    TRANS_PACK *t;
    t=(TRANS_PACK*)malloc(sizeof(TRANS_PACK));
    t->begin=begin;
    t->end=end;
    t->type=type;
    t->next=tile_trans;
    tile_trans=t;
}

TRANS_PACK* trans_pack_find(Uint32 tile)
{
    TRANS_PACK *t=tile_trans;
    while(t!=NULL) {
        if (tile>=t->begin && tile<=t->end)
            return t;
        t=t->next;
    }
    return NULL;
}

void trans_pack_free(void) {
    TRANS_PACK *t=tile_trans;
    TRANS_PACK *p;
    while(t!=NULL) {
    	p=t;
    	t=t->next;
    	free(p);
    }
    tile_trans=NULL;
}

/* Open a Nebula Transparency pack */
void trans_pack_open(char *filename)
{
    FILE *f;
    char buf[256];
    char *res;
    char range[32];
    int type, begin, end;
    char *t;

    tile_trans = NULL;
    if (strcmp(filename,"none")==0)
	return;

    f = fopen(filename, "r");

    if (f == NULL) {
	printf("Can't open %s\n", filename);
	return;
    }
    while (!feof(f)) {
	res=fgets(buf, 256, f);

	t = strchr(buf, ';');
	if (t)
	    t[0] = 0;

	t = strstr(buf, "Game");
	if (t)
	    continue;
	t = strstr(buf, "Name");
	if (t)
	    continue;

	if (sscanf(buf, " %s %d \n", range, &type) == 2) {
	    t = strchr(range, '-');
	    if (t) {
		t[0] = 0;
		t++;
		sscanf(range, "%x", &begin);
		sscanf(t, "%x", &end);
	    } else {
		sscanf(range, "%x", &begin);
		end = begin;
	    }
	    //printf("%x %x %d\n",begin,end,type);
	    trans_pack_add(begin, end, type);
	}
    }
    fclose(f);
}
