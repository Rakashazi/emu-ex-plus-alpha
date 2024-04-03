#ifndef _NEOCRYPT_H_
#define _NEOCRYPT_H_

#include "roms.h"

void kof98_decrypt_68k(GAME_ROMS *r);
void kof99_decrypt_68k(GAME_ROMS *r);
void garou_decrypt_68k(GAME_ROMS *r);
void garouo_decrypt_68k(GAME_ROMS *r);
void mslug3_decrypt_68k(GAME_ROMS *r);
void kof2000_decrypt_68k(GAME_ROMS *r);
void kof2002_decrypt_68k(GAME_ROMS *r);
void matrim_decrypt_68k(GAME_ROMS *r);
void samsho5_decrypt_68k(GAME_ROMS *r);
void samsh5sp_decrypt_68k(GAME_ROMS *r);
void mslug5_decrypt_68k(GAME_ROMS *r);
void svc_px_decrypt(GAME_ROMS *r);
void kf2k3pcb_decrypt_s1data(GAME_ROMS *r);
void kf2k3pcb_decrypt_68k(GAME_ROMS *r);
void kof2003_decrypt_68k(GAME_ROMS *r);
void kof2003h_decrypt_68k(GAME_ROMS *r);
void kof99_neogeo_gfx_decrypt(void *contextPtr, GAME_ROMS *r, int extra_xor);
void kof2000_neogeo_gfx_decrypt(void *contextPtr, GAME_ROMS *r, int extra_xor);
void cmc50_neogeo_gfx_decrypt(void *contextPtr, GAME_ROMS *r, int extra_xor);
void cmc42_neogeo_gfx_decrypt(void *contextPtr, GAME_ROMS *r, int extra_xor);
void neogeo_bootleg_cx_decrypt(GAME_ROMS *r);
void neogeo_bootleg_sx_decrypt(GAME_ROMS *r, int extra_xor);
void svcpcb_gfx_decrypt(GAME_ROMS *r);
void svcpcb_s1data_decrypt(GAME_ROMS *r);
void neo_pcm2_swap(GAME_ROMS *r, int value);
void neo_pcm2_snk_1999(GAME_ROMS *r, int value);
void neogeo_cmc50_m1_decrypt(void *contextPtr, GAME_ROMS *r);

void kof2002b_gfx_decrypt(GAME_ROMS *machine, uint8_t *src, int size);

#endif
