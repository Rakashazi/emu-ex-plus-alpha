#ifndef _TRANSPACK_H_
#define _TRANSPACK_H_

//#include "SDL.h"
#include <gngeoTypes.h>
enum {
	TILE_NORMAL,
	TILE_INVISIBLE,
	TILE_TRANSPARENT25,
	TILE_TRANSPARENT50,
};

typedef struct TRANS_PACK {
    Uint32 begin, end;
    Uint8 type;
    struct TRANS_PACK *next;
} TRANS_PACK;
extern TRANS_PACK *tile_trans;

TRANS_PACK* trans_pack_find(Uint32 tile);
void trans_pack_open(char *filename);
void trans_pack_free(void);

#endif
