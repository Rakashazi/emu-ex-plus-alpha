#ifdef HAVE_CONFIG_H
#include <gngeo-config.h>
#endif

#ifdef HAVE_BASENAME
#if defined(__ANDROID__) || defined(__APPLE__)
#include <libgen.h> // Android has const char* version here, Apple doesn't
#else
#define _GNU_SOURCE // for basename() when using glibc
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdbool.h>
#include "roms.h"
#include "emu.h"
#include "memory.h"
//#include "unzip.h"
#if defined(HAVE_LIBZ)// && defined (HAVE_MMAP)
#include <zlib.h>
#endif
#include "unzip.h"

#include "video.h"
#include "transpack.h"
#include "conf.h"
#include "resfile.h"
#include "menu.h"
#include "neocrypt.h"
#ifdef GP2X
#include "gp2x.h"
#include "ym2610-940/940shared.h"
#endif
#include <imagine/logger/logger.h>

static int need_decrypt = 1;

int neogeo_fix_bank_type = 0;

int bankoffset_kof99[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, 0x3cc000,
	0x4cc000, 0x3f2000, 0x4f2000, 0x407800, 0x507800, 0x40d000, 0x50d000,
	0x417800, 0x517800, 0x420800, 0x520800, 0x424800, 0x524800, 0x429000,
	0x529000, 0x42e800, 0x52e800, 0x431800, 0x531800, 0x54d000, 0x551000,
	0x567000, 0x592800, 0x588800, 0x581800, 0x599800, 0x594800, 0x598000,
	/* rest not used? */
};
/* addr,uncramblecode,.... */
Uint8 scramblecode_kof99[7] = {0xF0, 14, 6, 8, 10, 12, 5,};

int bankoffset_garou[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
	0x2f0000, 0x3f0000, 0x400000, 0x500000, // 08
	0x420000, 0x520000, 0x440000, 0x540000, // 12
	0x498000, 0x598000, 0x4a0000, 0x5a0000, // 16
	0x4a8000, 0x5a8000, 0x4b0000, 0x5b0000, // 20
	0x4b8000, 0x5b8000, 0x4c0000, 0x5c0000, // 24
	0x4c8000, 0x5c8000, 0x4d0000, 0x5d0000, // 28
	0x458000, 0x558000, 0x460000, 0x560000, // 32
	0x468000, 0x568000, 0x470000, 0x570000, // 36
	0x478000, 0x578000, 0x480000, 0x580000, // 40
	0x488000, 0x588000, 0x490000, 0x590000, // 44
	0x5d0000, 0x5d8000, 0x5e0000, 0x5e8000, // 48
	0x5f0000, 0x5f8000, 0x600000, /* rest not used? */
};
Uint8 scramblecode_garou[7] = {0xC0, 5, 9, 7, 6, 14, 12,};
int bankoffset_garouo[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x280000, 0x380000, 0x2d0000, 0x3d0000, // 04
	0x2c8000, 0x3c8000, 0x400000, 0x500000, // 08
	0x420000, 0x520000, 0x440000, 0x540000, // 12
	0x598000, 0x698000, 0x5a0000, 0x6a0000, // 16
	0x5a8000, 0x6a8000, 0x5b0000, 0x6b0000, // 20
	0x5b8000, 0x6b8000, 0x5c0000, 0x6c0000, // 24
	0x5c8000, 0x6c8000, 0x5d0000, 0x6d0000, // 28
	0x458000, 0x558000, 0x460000, 0x560000, // 32
	0x468000, 0x568000, 0x470000, 0x570000, // 36
	0x478000, 0x578000, 0x480000, 0x580000, // 40
	0x488000, 0x588000, 0x490000, 0x590000, // 44
	0x5d8000, 0x6d8000, 0x5e0000, 0x6e0000, // 48
	0x5e8000, 0x6e8000, 0x6e8000, 0x000000, // 52
	0x000000, 0x000000, 0x000000, 0x000000, // 56
	0x000000, 0x000000, 0x000000, 0x000000, // 60
};
Uint8 scramblecode_garouo[7] = {0xC0, 4, 8, 14, 2, 11, 13,};

int bankoffset_mslug3[64] = {
	0x000000, 0x020000, 0x040000, 0x060000, // 00
	0x070000, 0x090000, 0x0b0000, 0x0d0000, // 04
	0x0e0000, 0x0f0000, 0x120000, 0x130000, // 08
	0x140000, 0x150000, 0x180000, 0x190000, // 12
	0x1a0000, 0x1b0000, 0x1e0000, 0x1f0000, // 16
	0x200000, 0x210000, 0x240000, 0x250000, // 20
	0x260000, 0x270000, 0x2a0000, 0x2b0000, // 24
	0x2c0000, 0x2d0000, 0x300000, 0x310000, // 28
	0x320000, 0x330000, 0x360000, 0x370000, // 32
	0x380000, 0x390000, 0x3c0000, 0x3d0000, // 36
	0x400000, 0x410000, 0x440000, 0x450000, // 40
	0x460000, 0x470000, 0x4a0000, 0x4b0000, // 44
	0x4c0000, /* rest not used? */
};
Uint8 scramblecode_mslug3[7] = {0xE4, 14, 12, 15, 6, 3, 9,};
int bankoffset_kof2000[64] = {
	0x000000, 0x100000, 0x200000, 0x300000, // 00
	0x3f7800, 0x4f7800, 0x3ff800, 0x4ff800, // 04
	0x407800, 0x507800, 0x40f800, 0x50f800, // 08
	0x416800, 0x516800, 0x41d800, 0x51d800, // 12
	0x424000, 0x524000, 0x523800, 0x623800, // 16
	0x526000, 0x626000, 0x528000, 0x628000, // 20
	0x52a000, 0x62a000, 0x52b800, 0x62b800, // 24
	0x52d000, 0x62d000, 0x52e800, 0x62e800, // 28
	0x618000, 0x619000, 0x61a000, 0x61a800, // 32
};
Uint8 scramblecode_kof2000[7] = {0xEC, 15, 14, 7, 3, 10, 5,};

/* Actuall Code */

/*
 TODO
 static DRIVER_INIT( fatfury2 )
 {
 DRIVER_INIT_CALL(neogeo);
 fatfury2_install_protection(machine);
 }

 static DRIVER_INIT( mslugx )
 {
 DRIVER_INIT_CALL(neogeo);
 mslugx_install_protection(machine);
 }

 */

static int init_mslugx(GAME_ROMS *r) {
	unsigned int i;
	Uint8 *RAM = r->cpu_m68k.p;
	if (need_decrypt) {
		for (i = 0; i < r->cpu_m68k.size; i += 2) {
			if ((READ_WORD_ROM(&RAM[i + 0]) == 0x0243)
					&& (READ_WORD_ROM(&RAM[i + 2]) == 0x0001) && /* andi.w  #$1, D3 */
					(READ_WORD_ROM(&RAM[i + 4]) == 0x6600)) { /* bne xxxx */

				WRITE_WORD_ROM(&RAM[i + 4], 0x4e71);
				WRITE_WORD_ROM(&RAM[i + 6], 0x4e71);
			}
		}

		WRITE_WORD_ROM(&RAM[0x3bdc], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3bde], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3be0], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0c], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c0e], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c10], 0x4e71);

		WRITE_WORD_ROM(&RAM[0x3c36], 0x4e71);
		WRITE_WORD_ROM(&RAM[0x3c38], 0x4e71);
	}
	return 0;
}

static int init_kof99(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof99_decrypt_68k(r);
		kof99_neogeo_gfx_decrypt(contextPtr, r, 0x00);
	}
	memory.bksw_offset = bankoffset_kof99;
	memory.bksw_unscramble = scramblecode_kof99;
	memory.sma_rng_addr = 0xF8FA;
	//kof99_install_protection(machine);
	return 0;
}

static int init_kof99n(void *contextPtr, GAME_ROMS *r) {
	neogeo_fix_bank_type = 1;
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0x00);
	return 0;
}

static int init_garou(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		logMsg("doing garou decrypt");
		garou_decrypt_68k(r);
		kof99_neogeo_gfx_decrypt(contextPtr, r, 0x06);
	}
	neogeo_fix_bank_type = 1;
	memory.bksw_offset = bankoffset_garou;
	memory.bksw_unscramble = scramblecode_garou;
	memory.sma_rng_addr = 0xCCF0;
	//garou_install_protection(machine);
	DEBUG_LOG("I HAS INITIALIZD GAROU");
	return 0;
}

static int init_garouo(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		garouo_decrypt_68k(r);
		kof99_neogeo_gfx_decrypt(contextPtr, r, 0x06);
	}
	neogeo_fix_bank_type = 1;
	memory.bksw_offset = bankoffset_garouo;
	memory.bksw_unscramble = scramblecode_garouo;
	memory.sma_rng_addr = 0xCCF0;

	//garouo_install_protection(machine);
	return 0;
}

/*
 int init_garoup(GAME_ROMS *r) {
 garou_decrypt_68k(r);
 kof99_neogeo_gfx_decrypt(r, 0x06);

 return 0;
 }
 */
static int init_garoubl(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg support */
	if (need_decrypt) {
		neogeo_bootleg_sx_decrypt(r, 2);
		neogeo_bootleg_cx_decrypt(r);
	}
	return 0;
}

static int init_mslug3(void *contextPtr, GAME_ROMS *r) {
	logMsg("INIT MSLUG3");
	if (need_decrypt) {
		mslug3_decrypt_68k(r);
		kof99_neogeo_gfx_decrypt(contextPtr, r, 0xad);
	}
	neogeo_fix_bank_type = 1;
	memory.bksw_offset = bankoffset_mslug3;
	memory.bksw_unscramble = scramblecode_mslug3;
	//memory.sma_rng_addr=0xF8FA;
	memory.sma_rng_addr = 0;

	//mslug3_install_protection(r);
	return 0;
}

static int init_mslug3h(void *contextPtr, GAME_ROMS *r) {
	neogeo_fix_bank_type = 1;
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0xad);
	return 0;
}

static int init_mslug3b6(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg support */
	if (need_decrypt) {
		neogeo_bootleg_sx_decrypt(r, 2);
		cmc42_neogeo_gfx_decrypt(contextPtr, r, 0xad);
	}
	return 0;
}

static int init_kof2000(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2000_decrypt_68k(r);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x00);
	}
	neogeo_fix_bank_type = 2;
	memory.bksw_offset = bankoffset_kof2000;
	memory.bksw_unscramble = scramblecode_kof2000;
	memory.sma_rng_addr = 0xD8DA;
	//kof2000_install_protection(r);
	return 0;

}

static int init_kof2000n(void *contextPtr, GAME_ROMS *r) {
	neogeo_fix_bank_type = 2;
	if (need_decrypt) {
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x00);
	}
	return 0;
}

static int init_kof2001(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x1e);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
	}
	return 0;

}

/*

 TODO:
 static DRIVER_INIT( cthd2003 )
 {
 decrypt_cthd2003(machine);
 DRIVER_INIT_CALL(neogeo);
 patch_cthd2003(machine);
 }

 static DRIVER_INIT ( ct2k3sp )
 {
 decrypt_ct2k3sp(machine);
 DRIVER_INIT_CALL(neogeo);
 patch_cthd2003(machine);
 }

 static DRIVER_INIT ( ct2k3sa )
 {
 decrypt_ct2k3sa(machine);
 DRIVER_INIT_CALL(neogeo);
 patch_ct2k3sa(machine);
 }

 */

static int init_mslug4(void *contextPtr, GAME_ROMS *r) {
	neogeo_fix_bank_type = 1; /* USA violent content screen is wrong --
							 * not a bug, confirmed on real hardware! */
	if (need_decrypt) {
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x31);

		neo_pcm2_snk_1999(r, 8);
	}
	return 0;

}

static int init_ms4plus(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		cmc50_neogeo_gfx_decrypt(contextPtr, r, 0x31);
		neo_pcm2_snk_1999(r, 8);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
	}
	return 0;
}

static int init_ganryu(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0x07);
	return 0;
}

static int init_s1945p(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0x05);
	return 0;
}

static int init_preisle2(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0x9f);
	return 0;
}

static int init_bangbead(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0xf8);
	return 0;
}

static int init_nitd(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0xff);
	return 0;
}

static int init_zupapa(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0xbd);
	return 0;
}

static int init_sengoku3(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof99_neogeo_gfx_decrypt(contextPtr, r, 0xfe);
	return 0;
}

static int init_kof98(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) kof98_decrypt_68k(r);
	return 0;
}

static int init_rotd(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		neo_pcm2_snk_1999(r, 16);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x3f);
	}
	return 0;
}

static int init_kof2002(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2002_decrypt_68k(r);
		neo_pcm2_swap(r, 0);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0xec);
	}
	return 0;
}

static int init_kof2002b(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg */
	if (need_decrypt) {
		kof2002_decrypt_68k(r);
		neo_pcm2_swap(r, 0);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2002b_gfx_decrypt(r, r->tiles.p,0x4000000);
		kof2002b_gfx_decrypt(r, r->game_sfix.p,0x20000);
	}
	return 0;
}

static int init_kf2k2pls(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2002_decrypt_68k(r);
		neo_pcm2_swap(r, 0);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		cmc50_neogeo_gfx_decrypt(contextPtr, r, 0xec);
	}
	return 0;
}

static int init_kf2k2mp(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg */
	if (need_decrypt) {
		//kf2k2mp_decrypt(r);
		neo_pcm2_swap(r, 0);
		//neogeo_bootleg_sx_decrypt(r, 2);
		cmc50_neogeo_gfx_decrypt(contextPtr, r, 0xec);
	}
	return 0;
}

static int init_kof2km2(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg */
	if (need_decrypt) {
		//kof2km2_px_decrypt(r);
		neo_pcm2_swap(r, 0);
		//neogeo_bootleg_sx_decrypt(r, 1);
		cmc50_neogeo_gfx_decrypt(contextPtr, r, 0xec);
	}
	return 0;
}

/*

 TODO
 static DRIVER_INIT( kof10th )
 {
 decrypt_kof10th(machine);
 DRIVER_INIT_CALL(neogeo);
 install_kof10th_protection(machine);
 }

 static DRIVER_INIT( kf10thep )
 {
 decrypt_kf10thep(machine);
 DRIVER_INIT_CALL(neogeo);
 }

 static DRIVER_INIT( kf2k5uni )
 {
 decrypt_kf2k5uni(machine);
 DRIVER_INIT_CALL(neogeo);
 }

 static DRIVER_INIT( kof2k4se )
 {
 decrypt_kof2k4se_68k(machine);
 DRIVER_INIT_CALL(neogeo);
 }

 static DRIVER_INIT( matrimbl )
 {
 matrim_decrypt_68k(machine);
 neogeo_fixed_layer_bank_type = 2;
 matrimbl_decrypt(machine);
 neogeo_sfix_decrypt(machine);
 DRIVER_INIT_CALL(neogeo);
 }

 */

static int init_matrim(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		matrim_decrypt_68k(r);
		neo_pcm2_swap(r, 1);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x6a);
	}
	neogeo_fix_bank_type = 2;
	return 0;
}

static int init_pnyaa(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		neo_pcm2_snk_1999(r, 4);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x2e);
	}
	return 0;
}

static int init_mslug5(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		mslug5_decrypt_68k(r);
		neo_pcm2_swap(r, 2);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x19);
	}
	hasPvc = true;
	return 0;
}

/*
 TODO:
 static TIMER_CALLBACK( ms5pcb_bios_timer_callback )
 {
 int harddip3 = input_port_read(machine, "HARDDIP") & 1;
 memory_set_bankptr(machine, NEOGEO_BANK_BIOS, memory_region(machine, "mainbios")
 +0x20000+harddip3*0x20000);
 }

 */
static int init_ms5pcb(void *contextPtr, GAME_ROMS *r) {

	/* TODO: start a timer that will check the BIOS select DIP every second */
	//timer_set(machine, attotime_zero, NULL, 0, ms5pcb_bios_timer_callback);
	//timer_pulse(machine, ATTOTIME_IN_MSEC(1000), NULL, 0, ms5pcb_bios_timer_callback);
	if (need_decrypt) {
		mslug5_decrypt_68k(r);
		svcpcb_gfx_decrypt(r);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x19);
		svcpcb_s1data_decrypt(r);
		neo_pcm2_swap(r, 2);
	}
	hasPvc = true;
	return 0;
}

static int init_ms5plus(void *contextPtr, GAME_ROMS *r) {
	/* TODO: Bootleg */
	if (need_decrypt) {
		cmc50_neogeo_gfx_decrypt(contextPtr, r, 0x19);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		neo_pcm2_swap(r, 2);
		neogeo_bootleg_sx_decrypt(r, 1);
	}
	return 0;
}

static int init_samsho5(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		samsho5_decrypt_68k(r);
		neo_pcm2_swap(r, 4);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x0f);
	}
	return 0;
}

static int init_samsh5sp(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		samsh5sp_decrypt_68k(r);
		neo_pcm2_swap(r, 6);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x0d);
	}
	return 0;
}

static int init_svc(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		svc_px_decrypt(r);
		neo_pcm2_swap(r, 3);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x57);
	}
	neogeo_fix_bank_type = 2;
	hasPvc = true;
	return 0;
}

static int init_kof2003(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2003_decrypt_68k(r);
		neo_pcm2_swap(r, 5);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x9d);
	}
	neogeo_fix_bank_type = 2;
	hasPvc = true;
	return 0;
}

static int init_kof2003h(void *contextPtr, GAME_ROMS *r) {
	if (need_decrypt) {
		kof2003h_decrypt_68k(r);
		neo_pcm2_swap(r, 5);
		neogeo_cmc50_m1_decrypt(contextPtr, r);
		kof2000_neogeo_gfx_decrypt(contextPtr, r, 0x9d);
	}
	neogeo_fix_bank_type = 2;
	hasPvc = true;
	return 0;
}

#if 0
// TODO:

static TIMER_CALLBACK(svcpcb_bios_timer_callback) {
	int harddip3 = input_port_read(machine, "HARDDIP") & 1;
	memory_set_bankptr(machine, NEOGEO_BANK_BIOS, memory_region(machine, "mainbios")
			+ 0x20000 + harddip3 * 0x20000);
}

static DRIVER_INIT(svcpcb) {
	/* start a timer that will check the BIOS select DIP every second */
	timer_set(machine, attotime_zero, NULL, 0, svcpcb_bios_timer_callback);
	timer_pulse(machine, ATTOTIME_IN_MSEC(1000), NULL, 0, svcpcb_bios_timer_callback);

	svc_px_decrypt(machine);
	svcpcb_gfx_decrypt(machine);
	neogeo_cmc50_m1_decrypt(machine);
	kof2000_neogeo_gfx_decrypt(machine, 0x57);
	svcpcb_s1data_decrypt(machine);
	neo_pcm2_swap(machine, 3);
	neogeo_fixed_layer_bank_type = 2;
	DRIVER_INIT_CALL(neogeo);
	install_pvc_protection(machine);
}

static DRIVER_INIT(svcboot) {
	svcboot_px_decrypt(machine);
	svcboot_cx_decrypt(machine);
	DRIVER_INIT_CALL(neogeo);
	install_pvc_protection(machine);
}

static DRIVER_INIT(svcplus) {
	svcplus_px_decrypt(machine);
	svcboot_cx_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 1);
	svcplus_px_hack(machine);
	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(svcplusa) {
	svcplusa_px_decrypt(machine);
	svcboot_cx_decrypt(machine);
	svcplus_px_hack(machine);
	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(svcsplus) {
	svcsplus_px_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 2);
	svcboot_cx_decrypt(machine);
	svcsplus_px_hack(machine);
	DRIVER_INIT_CALL(neogeo);
	install_pvc_protection(machine);
}

static DRIVER_INIT(samsho5b) {
	samsho5b_px_decrypt(machine);
	samsho5b_vx_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 1);
	neogeo_bootleg_cx_decrypt(machine);
	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(kf2k3pcb) {
	kf2k3pcb_decrypt_68k(machine);
	kf2k3pcb_gfx_decrypt(machine);
	kof2003biosdecode(machine);
	neogeo_cmc50_m1_decrypt(machine);

	/* extra little swap on the m1 - this must be performed AFTER the m1 decrypt
	 or the m1 checksum (used to generate the key) for decrypting the m1 is
	 incorrect */
	{
		int i;
		UINT8* rom = memory_region(machine, "audiocpu");
		for (i = 0; i < 0x90000; i++) {
			rom[i] = BITSWAP8(rom[i], 5, 6, 1, 4, 3, 0, 7, 2);
		}

	}

	kof2000_neogeo_gfx_decrypt(machine, 0x9d);
	kf2k3pcb_decrypt_s1data(machine);
	neo_pcm2_swap(machine, 5);
	neogeo_fixed_layer_bank_type = 2;
	DRIVER_INIT_CALL(neogeo);
	install_pvc_protection(machine);
	memory_install_read16_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0xc00000, 0xc7ffff, 0, 0,
			(read16_space_func) SMH_BANK(6)); // 512k bios
}

static DRIVER_INIT(kf2k3bl) {
	cmc50_neogeo_gfx_decrypt(machine, 0x9d);
	neo_pcm2_swap(machine, 5);
	neogeo_bootleg_sx_decrypt(machine, 1);
	DRIVER_INIT_CALL(neogeo);
	kf2k3bl_install_protection(machine);
}

static DRIVER_INIT(kf2k3pl) {
	cmc50_neogeo_gfx_decrypt(machine, 0x9d);
	neo_pcm2_swap(machine, 5);
	kf2k3pl_px_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 1);
	DRIVER_INIT_CALL(neogeo);
	kf2k3pl_install_protection(machine);
}

static DRIVER_INIT(kf2k3upl) {
	cmc50_neogeo_gfx_decrypt(machine, 0x9d);
	neo_pcm2_swap(machine, 5);
	kf2k3upl_px_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 2);
	DRIVER_INIT_CALL(neogeo);
	kf2k3upl_install_protection(machine);
}

static DRIVER_INIT(jockeygp) {
	UINT16* extra_ram;

	neogeo_fixed_layer_bank_type = 1;
	neogeo_cmc50_m1_decrypt(machine);
	kof2000_neogeo_gfx_decrypt(machine, 0xac);

	/* install some extra RAM */
	extra_ram = auto_alloc_array(machine, UINT16, 0x2000 / 2);
	state_save_register_global_pointer(machine, extra_ram, 0x2000 / 2);

	memory_install_readwrite16_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0x200000, 0x201fff, 0, 0,
			(read16_space_func) SMH_BANK(8), (write16_space_func) SMH_BANK(8));
	memory_set_bankptr(machine, NEOGEO_BANK_EXTRA_RAM, extra_ram);

	//  memory_install_read_port_handler(cputag_get_address_space(machine,
	//"maincpu", ADDRESS_SPACE_PROGRAM), 0x280000, 0x280001, 0, 0, "IN5");
	//  memory_install_read_port_handler(cputag_get_address_space(machine,
	//"maincpu", ADDRESS_SPACE_PROGRAM), 0x2c0000, 0x2c0001, 0, 0, "IN6");

	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(vliner) {
	UINT16* extra_ram;

	/* install some extra RAM */
	extra_ram = auto_alloc_array(machine, UINT16, 0x2000 / 2);
	state_save_register_global_pointer(machine, extra_ram, 0x2000 / 2);

	memory_install_readwrite16_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0x200000, 0x201fff, 0, 0, (read16_space_func)
			SMH_BANK(8), (write16_space_func) SMH_BANK(8));
	memory_set_bankptr(machine, NEOGEO_BANK_EXTRA_RAM, extra_ram);

	memory_install_read_port_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0x280000, 0x280001, 0, 0, "IN5");
	memory_install_read_port_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0x2c0000, 0x2c0001, 0, 0, "IN6");

	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(kog) {
	/* overlay cartridge ROM */
	memory_install_read_port_handler(cputag_get_address_space(machine, "maincpu",
			ADDRESS_SPACE_PROGRAM), 0x0ffffe, 0x0fffff, 0, 0, "JUMPER");

	kog_px_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 1);
	neogeo_bootleg_cx_decrypt(machine);
	DRIVER_INIT_CALL(neogeo);
}

static DRIVER_INIT(lans2004) {
	lans2004_decrypt_68k(machine);
	lans2004_vx_decrypt(machine);
	neogeo_bootleg_sx_decrypt(machine, 1);
	neogeo_bootleg_cx_decrypt(machine);
	DRIVER_INIT_CALL(neogeo);
}

#endif

struct roms_init_func {
	char *name;
	int (*init)(void *contextPtr, GAME_ROMS * r);
} init_func_table[] = {
	//	{"mslugx",init_mslugx},
	{ "kof99", init_kof99},
	{ "kof99n", init_kof99n},
	{ "garou", init_garou},
	{ "garouo", init_garouo},
	//	{"garoup",init_garoup},
	{ "garoubl", init_garoubl},
	{ "mslug3", init_mslug3},
	{ "mslug3h", init_mslug3h},
	{ "mslug3b6", init_mslug3b6},
	{ "kof2000", init_kof2000},
	{ "kof2000n", init_kof2000n},
	{ "kof2001", init_kof2001},
	{ "kof2001h", init_kof2001},
	{ "mslug4", init_mslug4},
	{ "ms4plus", init_ms4plus},
	{ "ganryu", init_ganryu},
	{ "s1945p", init_s1945p},
	{ "preisle2", init_preisle2},
	{ "bangbead", init_bangbead},
	{ "nitd", init_nitd},
	{ "zupapa", init_zupapa},
	{ "sengoku3", init_sengoku3},
	{ "kof98", init_kof98},
	{ "rotd", init_rotd},
	{ "kof2002", init_kof2002},
	{ "kof2002b", init_kof2002b},
	{ "kf2k2pls", init_kf2k2pls},
	{ "kf2k2mp", init_kf2k2mp},
	{ "kof2km2", init_kof2km2},
	{ "kof2003", init_kof2003},
	{ "kof2003h", init_kof2003h},
	{ "matrim", init_matrim},
	{ "pnyaa", init_pnyaa},
	{ "mslug5", init_mslug5},
	{ "mslug5h", init_mslug5},
	{ "ms5pcb", init_ms5pcb},
	{ "ms5plus", init_ms5plus},
	{ "samsho5", init_samsho5},
	{ "samsho5h", init_samsho5},
	{ "samsh5sp", init_samsh5sp},
	{ "svc", init_svc},
	{ NULL, NULL}
};

static int allocate_region(ROM_REGION *r, Uint32 size, int region) {
	DEBUG_LOG("Allocating 0x%08x byte for Region %d", size, region);
	if (size != 0) {
#ifdef GP2X
		switch (region) {
			case REGION_AUDIO_CPU_CARTRIDGE:
				r->p = gp2x_ram_malloc(size, 1);
#ifdef ENABLE_940T
				shared_data->sm1 = (Uint8*) ((r->p - gp2x_ram2) + 0x1000000);
				logMsg("Z80 code: %08x\n", (Uint32) shared_data->sm1);
#endif
				break;
			case REGION_AUDIO_DATA_1:
				r->p = gp2x_ram_malloc(size, 0);
#ifdef ENABLE_940T
				shared_data->pcmbufa = (Uint8*) (r->p - gp2x_ram);
				logMsg("SOUND1 code: %08x\n", (Uint32) shared_data->pcmbufa);
				shared_data->pcmbufa_size = size;
#endif
				break;
			case REGION_AUDIO_DATA_2:
				r->p = gp2x_ram_malloc(size, 0);
#ifdef ENABLE_940T
				shared_data->pcmbufb = (Uint8*) (r->p - gp2x_ram);
				logMsg("SOUND2 code: %08x\n", (Uint32) shared_data->pcmbufa);
				shared_data->pcmbufb_size = size;
#endif
				break;
			default:
				r->p = malloc(size);
				break;

		}
#else
		r->p = malloc(size);
#endif
		if (r->p == 0) {
			r->size = 0;
			logMsg("Error allocating");
			/* TODO: Be more permissive, allow at least a dump */
			logMsg("Not enough memory :( exiting");
			exit(1);
			return 1;
		}
		memset(r->p, 0, size);
	} else
		r->p = NULL;
	r->size = size;
	return 0;
}

static void free_region(ROM_REGION *r) {
	DEBUG_LOG("Free Region %p %p %d", r, r->p, r->size);
	if (r->p)
		free(r->p);
	r->size = 0;
	r->p = NULL;
}

static int zip_seek_current_file(struct ZFILE *gz, Uint32 offset) {
	const Uint32 s = 1024 * 32;
	Uint8 buf[s];
	while (offset) {
		Uint32 c = offset;
		if (c > s)
			c = s;

		c = gn_unzip_fread(gz, buf, c);
		if (c <= 0) {
			break;
		}
		offset -= c;
	}
	return 0;

}

static int read_counter;

static int read_data_i(struct ZFILE *gz, ROM_REGION *r, uint32_t dest, uint32_t size) {
	uint8_t *p = r->p + dest;
	if (r->p == NULL || r->size < (dest & ~0x1) + (size * 2)) {
		logMsg("Region not allocated or not big enough %08x %08x", r->size,
				dest + (size * 2));
		return -1;
	}
	uint8_t *buf = malloc(size);
	int c = gn_unzip_fread(gz, buf, size);
	if (c <= 0)
	{
		free(buf);
		return 0;
	}
	for(size_t i = 0; i < c; i++)
	{
		//printf("%d %d\n",i,c);
		*p = buf[i];
		p += 2;
	}
	free(buf);
	read_counter += c;
	gn_update_pbar(read_counter);
	return 0;
}

static int read_data_i16(struct ZFILE *gz, ROM_REGION *r, uint32_t dest, uint32_t size)
{
	uint16_t *p = (uint16_t*)(r->p + dest);
	uint16_t *buf = malloc(size);
	int c = gn_unzip_fread(gz, (uint8_t*)buf, size);
	if (c <= 0)
	{
		free(buf);
		return 0;
	}
	for(size_t i = 0; i < c / 2; i++)
	{
		*p = buf[i];
		p += 2;
	}
	free(buf);
	read_counter += c;
	gn_update_pbar(read_counter);
	return 0;
}

static int read_data_p(struct ZFILE *gz, ROM_REGION *r, Uint32 dest, Uint32 size) {
	if (r->p == NULL || r->size < dest + size) {
		logMsg("Region not allocated or not big enough");
		return -1;
	}
	Uint32 c = gn_unzip_fread(gz, r->p + dest, size);
	if (c <= 0) {
		return 0;
	}
	read_counter += c;
	gn_update_pbar(read_counter);
	return 0;
}

static int load_region(struct PKZIP *pz, GAME_ROMS *r, int region, Uint32 src,
		Uint32 dest, Uint32 size, Uint32 crc, char *filename, bool interleaved68kRom) {
	int rc;
	int badcrc = 0;
	struct ZFILE *gz;


	gz = gn_unzip_fopen(pz, filename, crc);
	if (gz == NULL) {
		//DEBUG_LOG("KO\n");
		DEBUG_LOG("Load file %-17s in region %d: KO", filename, region);
		return 1;
	}

	if (src != 0) { /* TODO: Reuse an allready opened zfile */

		if (region == REGION_SPRITES)
			rc = zip_seek_current_file(gz, src / 2);
		else
			rc = zip_seek_current_file(gz, src);
		DEBUG_LOG("setoffset: %d %08x %08x %08x", rc, src, dest, size);
	}

	DEBUG_LOG("Trying to load file %-17s in region %d", filename, region);

	switch (region) {
		case REGION_SPRITES: /* Special interleaved loading  */
			read_data_i(gz, &r->tiles, dest, size);
			break;
		case REGION_AUDIO_CPU_CARTRIDGE:
			read_data_p(gz, &r->cpu_z80, dest, size);
			break;
		case REGION_AUDIO_CPU_ENCRYPTED:
			read_data_p(gz, &r->cpu_z80c, dest, size);
			break;
		case REGION_MAIN_CPU_CARTRIDGE:
			if(interleaved68kRom && dest <= 2)
			{
				logMsg("loading interleaved 68K ROM");
				read_data_i16(gz, &r->cpu_m68k, dest, size);
			}
			else
				read_data_p(gz, &r->cpu_m68k, dest, size);
			break;
		case REGION_FIXED_LAYER_CARTRIDGE:
			read_data_p(gz, &r->game_sfix, dest, size);
			break;
		case REGION_AUDIO_DATA_1:
			read_data_p(gz, &r->adpcma, dest, size);
			break;
		case REGION_AUDIO_DATA_2:
			read_data_p(gz, &r->adpcmb, dest, size);
			break;
		case REGION_MAIN_CPU_BIOS:
			read_data_p(gz, &r->bios_m68k, dest, size);
			break;
		case REGION_AUDIO_CPU_BIOS:
			read_data_p(gz, &r->bios_audio, dest, size);
			break;
		case REGION_FIXED_LAYER_BIOS:
			read_data_p(gz, &r->bios_sfix, dest, size);
			break;

		default:
			DEBUG_LOG("Unhandled region %d", region);
			break;

	}
	DEBUG_LOG("Load file %-17s in region %d: OK %s", filename, region,
			(badcrc ? "(Bad CRC)" : ""));
	//unzCloseCurrentFile(gz);
	gn_unzip_fclose(gz);
	return 0;
}

static int convert_roms_tile(Uint8 *g, int tileno) {
	unsigned char swap[128];
	unsigned int *gfxdata;
	int x, y;
	unsigned int pen, usage = 0;
	gfxdata = (Uint32*) & g[tileno << 7];

	memcpy(swap, gfxdata, 128);

	//filed=1;
	for (y = 0; y < 16; y++) {
		unsigned int dw;

		dw = 0;
		for (x = 0; x < 8; x++) {
			pen = ((swap[64 + (y << 2) + 3] >> x) & 1) << 3;
			pen |= ((swap[64 + (y << 2) + 1] >> x) & 1) << 2;
			pen |= ((swap[64 + (y << 2) + 2] >> x) & 1) << 1;
			pen |= (swap[64 + (y << 2)] >> x) & 1;
			//if (!pen) filed=0;
			dw |= pen << ((7 - x) << 2);
			//memory.pen_usage[tileno]  |= (1 << pen);
			usage |= (1 << pen);
		}
		*(gfxdata++) = dw;

		dw = 0;
		for (x = 0; x < 8; x++) {
			pen = ((swap[(y << 2) + 3] >> x) & 1) << 3;
			pen |= ((swap[(y << 2) + 1] >> x) & 1) << 2;
			pen |= ((swap[(y << 2) + 2] >> x) & 1) << 1;
			pen |= (swap[(y << 2)] >> x) & 1;
			//if (!pen) filed=0;
			dw |= pen << ((7 - x) << 2);
			//memory.pen_usage[tileno]  |= (1 << pen);
			usage |= (1 << pen);
		}
		*(gfxdata++) = dw;
	}

	//if ((usage & ~1) == 0) pen_usage|=(TILE_INVISIBLE<<((tileno&0xF)*2));
	/* TODO transpack support */
	if ((usage & ~1) == 0)
		return (TILE_INVISIBLE << ((tileno & 0xF) * 2));
	else
		return 0;

}

void convert_all_tile(GAME_ROMS *r) {
	Uint32 i;
	allocate_region(&r->spr_usage, (r->tiles.size >> 11) * sizeof (Uint32), REGION_SPR_USAGE);
	memset(r->spr_usage.p, 0, r->spr_usage.size);
	for (i = 0; i < r->tiles.size >> 7; i++) {
		((Uint32*) r->spr_usage.p)[i >> 4] |= convert_roms_tile(r->tiles.p, i);
	}
}

void convert_all_char(Uint8 *Ptr, int Taille,
		Uint8 *usage_ptr) {
	int i, j;
	unsigned char usage;

	Uint8 *Src;
	Uint8 *sav_src;

	Src = (Uint8*) malloc(Taille);
	if (!Src) {
		logMsg("Not enought memory!!");
		return;
	}
	sav_src = Src;
	memcpy(Src, Ptr, Taille);
#ifdef WORDS_BIGENDIAN
#define CONVERT_TILE *Ptr++ = *(Src+8);\
	             usage |= *(Src+8);\
                     *Ptr++ = *(Src);\
		     usage |= *(Src);\
		     *Ptr++ = *(Src+24);\
		     usage |= *(Src+24);\
		     *Ptr++ = *(Src+16);\
		     usage |= *(Src+16);\
		     Src++;
#else
#define CONVERT_TILE *Ptr++ = *(Src+16);\
	             usage |= *(Src+16);\
                     *Ptr++ = *(Src+24);\
		     usage |= *(Src+24);\
		     *Ptr++ = *(Src);\
		     usage |= *(Src);\
		     *Ptr++ = *(Src+8);\
		     usage |= *(Src+8);\
		     Src++;
#endif
	for (i = Taille; i > 0; i -= 32) {
		usage = 0;
		for (j = 0; j < 8; j++) {
			CONVERT_TILE
		}
		Src += 24;
		*usage_ptr++ = usage;
	}
	free(sav_src);
#undef CONVERT_TILE
}

static int init_roms(void *contextPtr, GAME_ROMS *r) {
	int i = 0;
	//printf("INIT ROM %s\n",r->info.name);
	neogeo_fix_bank_type = 0;
	memory.bksw_handler = 0;
	memory.bksw_unscramble = NULL;
	memory.bksw_offset = NULL;
	memory.sma_rng_addr = 0;
	hasPvc = false;

	while (init_func_table[i].name) {
		//printf("INIT ROM ? %s %s\n",init_func_table[i].name,r->info.name);
		if (strcmp(init_func_table[i].name, r->info.name) == 0
				&& init_func_table[i].init != NULL) {
			DEBUG_LOG("Special init func");
			return init_func_table[i].init(contextPtr, r);
		}
		i++;
	}
	DEBUG_LOG("Default roms init");
	return 0;
}

static bool loadUnibios(GAME_ROMS *r, const char *unibiosFilename, uint32_t file_crc, struct PKZIP *pz, char *rpath, char romerror[1024])
{
	/* First check in neogeo.zip */
	r->bios_m68k.p = gn_unzip_file_malloc(pz, unibiosFilename, file_crc, &r->bios_m68k.size);
	if(!r->bios_m68k.p)
	{
		sprintf(romerror, "%s missing or invalid, make sure it's in your neogeo.zip", unibiosFilename);
		return false;
	}
	return true;
}

bool dr_load_bios(void *contextPtr, GAME_ROMS *r, char romerror[1024]) {
	int i;
	struct PKZIP *pz;
	struct ZFILE *z;
	unsigned int size;
	struct PathArray pArr = get_rom_path(contextPtr);
	char *rpath = pArr.data;
	const char *romfile;

	logMsg("opening neogeo.zip");
	pz = open_rom_zip(contextPtr, rpath, "neogeo");
	if (pz == NULL) {
		sprintf(romerror, "Can't open BIOS archive (neogeo.zip)");
		return false;
	}

	logMsg("opening 000-lo.lo");
	memory.ng_lo = gn_unzip_file_malloc(pz, "000-lo.lo", 0x5a86cff2, &size);
	if (memory.ng_lo == NULL) {
		sprintf(romerror, "000-lo.lo missing or invalid, make sure it's in your neogeo.zip");
		return false;
	}

	if (!(r->info.flags & HAS_CUSTOM_SFIX_BIOS)) {
		logMsg("opening Sfix");
		r->bios_sfix.p = gn_unzip_file_malloc(pz, "sfix.sfx", 0xc2ea0cfd,
				&r->bios_sfix.size);
		if (r->bios_sfix.p == NULL) {
			logMsg("Couldn't find sfix.sfx, try sfix.sfix");
			r->bios_sfix.p = gn_unzip_file_malloc(pz, "sfix.sfix", 0xc2ea0cfd,
					&r->bios_sfix.size);
			if (r->bios_sfix.p == NULL) {
				sprintf(romerror, "sfix.sfix missing or invalid, make sure it's in your neogeo.zip");
				return false;
			}
		}
	}
	/* convert bios fix char */
	convert_all_char(memory.rom.bios_sfix.p, 0x20000, memory.fix_board_usage);

	if (!(r->info.flags & HAS_CUSTOM_CPU_BIOS)) {
		logMsg("opening 68K BIOS");
		if (conf.system == SYS_UNIBIOS) {
			if(!loadUnibios(r, "uni-bios_2_3.rom", 0x27664eb5, pz, rpath, romerror))
			{
				return false;
			}
		} else if (conf.system == SYS_UNIBIOS_3_0) {
			if(!loadUnibios(r, "uni-bios_3_0.rom", 0xa97c89a9, pz, rpath, romerror))
			{
				return false;
			}
		} else if (conf.system == SYS_UNIBIOS_3_1) {
			if(!loadUnibios(r, "uni-bios_3_1.rom", 0x0c58093f, pz, rpath, romerror))
			{
				return false;
			}
		} else if (conf.system == SYS_UNIBIOS_3_2) {
			if(!loadUnibios(r, "uni-bios_3_2.rom", 0xa4e8b9b3, pz, rpath, romerror))
			{
				return false;
			}
		} else if (conf.system == SYS_UNIBIOS_3_3) {
			if(!loadUnibios(r, "uni-bios_3_3.rom", 0x24858466, pz, rpath, romerror))
			{
				return false;
			}
		} else if (conf.system == SYS_UNIBIOS_4_0) {
			if(!loadUnibios(r, "uni-bios_4_0.rom", 0xa7aab458, pz, rpath, romerror))
			{
				return false;
			}
		} else {
			uint32_t crc32 = 0;
			if (conf.system == SYS_HOME) {
				romfile = "aes-bios.bin";
			} else {
				switch (conf.country) {
					case CTY_JAPAN:
						romfile = "vs-bios.rom";
						crc32 = 0xf0e8f27d;
						break;
					case CTY_USA:
						romfile = "usa_2slt.bin";
						crc32 = 0xe72943de;
						break;
					case CTY_ASIA:
						romfile = "asia-s3.rom";
						crc32 = 0x91b64be3;
						break;
					default:
						romfile = "sp-s2.sp1";
						crc32 = 0x9036d879;
						break;
				}
			}
			DEBUG_LOG("Loading %s", romfile);
			r->bios_m68k.p = gn_unzip_file_malloc(pz, romfile, crc32,
					&r->bios_m68k.size);
			if (r->bios_m68k.p == NULL) {
				sprintf(romerror, "%s missing or invalid, make sure it's in your neogeo.zip", romfile);
				goto error;
			}
		}
	}

	gn_close_zip(pz);
	return true;

error:
	gn_close_zip(pz);
	return false;
}

ROM_DEF *dr_check_zip(void *contextPtr, const char *filename) {

	char *z;
	ROM_DEF *drv;
#ifdef HAVE_BASENAME
	#ifdef __APPLE__
		// case when basename() can modify argument
		char *tempFilename = strdup(filename);
		char *game = strdup(basename(tempFilename));
		free(tempFilename);
	#else
		char *game = strdup(basename(filename));
	#endif
#else
	char *game = strdup(strrchr(filename, '/'));
#endif
	//	printf("Game=%s\n", game);
	if (game == NULL)
		return NULL;
	z = strstr(game, ".");
	//	printf("z=%s\n", game);
	if (z == NULL)
	{
		free(game);
		return NULL;
	}
	z[0] = 0;
	drv = res_load_drv(contextPtr, game);
	free(game);
	return drv;
}

int dr_load_roms(void *contextPtr, GAME_ROMS *r, char *rom_path, char *name, char romerror[1024]) {
	//unzFile *gz,*gzp=NULL,*rdefz;
	struct PKZIP *gz, *gzp = NULL;
	ROM_DEF *drv;
	int i;
	int romsize;

	memset(r, 0, sizeof (GAME_ROMS));

	drv = res_load_drv(contextPtr, name);
	if (!drv) {
		sprintf(romerror, "Can't find rom driver for %s", name);
		return false;
	}

	gz = open_rom_zip(contextPtr, rom_path, name);
	if (gz == NULL) {
		sprintf(romerror,"Game %s.zip not found", name);
		return false;
	}

	/* Open Parent.
	 For now, only one parent is supported, no recursion
	 */
	gzp = open_rom_zip(contextPtr, rom_path, drv->parent);
	if (gzp == NULL) {
		sprintf(romerror,"%s.zip not found, make sure it's in your ROM directory", drv->parent);
		return false;
	}

	//printf("year %d\n",drv->year);
	//return;

	bool interleaved68kRom = drv->rom[1].region == 8 && drv->rom[1].dest == 2;
	strcpy(r->info.name, drv->name);
	strcpy(r->info.longname, drv->longname);
	r->info.year = drv->year;
	r->info.flags = 0;
	allocate_region(&r->cpu_m68k, drv->romsize[REGION_MAIN_CPU_CARTRIDGE],
			REGION_MAIN_CPU_CARTRIDGE);
	if (drv->romsize[REGION_AUDIO_CPU_CARTRIDGE] == 0
			&& drv->romsize[REGION_AUDIO_CPU_ENCRYPTED] != 0) {
		//allocate_region(&r->cpu_z80,drv->romsize[REGION_AUDIO_CPU_ENCRYPTED]);
		//allocate_region(&r->cpu_z80c,drv->romsize[REGION_AUDIO_CPU_ENCRYPTED]);
		allocate_region(&r->cpu_z80c, 0x80000, REGION_AUDIO_CPU_ENCRYPTED);
		allocate_region(&r->cpu_z80, 0x90000, REGION_AUDIO_CPU_CARTRIDGE);
	} else {
		allocate_region(&r->cpu_z80, drv->romsize[REGION_AUDIO_CPU_CARTRIDGE],
				REGION_AUDIO_CPU_CARTRIDGE);
	}
	allocate_region(&r->tiles, drv->romsize[REGION_SPRITES], REGION_SPRITES);
	allocate_region(&r->game_sfix, drv->romsize[REGION_FIXED_LAYER_CARTRIDGE],
			REGION_FIXED_LAYER_CARTRIDGE);
	allocate_region(&r->gfix_usage, r->game_sfix.size >> 5,
			REGION_GAME_FIX_USAGE);

	allocate_region(&r->adpcma, drv->romsize[REGION_AUDIO_DATA_1],
			REGION_AUDIO_DATA_1);
	allocate_region(&r->adpcmb, drv->romsize[REGION_AUDIO_DATA_2],
			REGION_AUDIO_DATA_2);

	/* Allocate bios if necessary */
	DEBUG_LOG("BIOS SIZE %08x %08x %08x", drv->romsize[REGION_MAIN_CPU_BIOS],
			drv->romsize[REGION_AUDIO_CPU_BIOS],
			drv->romsize[REGION_FIXED_LAYER_BIOS]);
	if (drv->romsize[REGION_MAIN_CPU_BIOS] != 0) {
		r->info.flags |= HAS_CUSTOM_CPU_BIOS;
		allocate_region(&r->bios_m68k, drv->romsize[REGION_MAIN_CPU_BIOS],
				REGION_MAIN_CPU_BIOS);
	}
	if (drv->romsize[REGION_AUDIO_CPU_BIOS] != 0) {
		logMsg("has custom audio BIOS");
		r->info.flags |= HAS_CUSTOM_AUDIO_BIOS;
		allocate_region(&r->bios_audio, drv->romsize[REGION_AUDIO_CPU_BIOS],
				REGION_AUDIO_CPU_BIOS);
	}
//	if (drv->romsize[REGION_FIXED_LAYER_BIOS] != 0) {
//		r->info.flags |= HAS_CUSTOM_SFIX_BIOS;
//		allocate_region(&r->bios_sfix, drv->romsize[REGION_FIXED_LAYER_BIOS],
//				REGION_FIXED_LAYER_BIOS);
//	}

	/* Now, load the roms */
	read_counter = 0;
	romsize = 0;
	for (i = 0; i < (int)drv->nb_romfile; i++)
		romsize += drv->rom[i].size;
	gn_init_pbar(PBAR_ACTION_LOADROM, romsize);
	for (i = 0; i < (int)drv->nb_romfile; i++) {
		if(drv->rom[i].region == REGION_FIXED_LAYER_BIOS)
		{
			logMsg("skipping BIOS SFIX defined in driver");
			continue;
		}
		if (load_region(gz, r, drv->rom[i].region, drv->rom[i].src,
				drv->rom[i].dest, drv->rom[i].size, drv->rom[i].crc,
				drv->rom[i].filename, interleaved68kRom) != 0) {
			/* File not found in the roms, try the parent */
			if (gzp) {
				int region = drv->rom[i].region;
				int pi;
				pi = load_region(gzp, r, drv->rom[i].region, drv->rom[i].src,
						drv->rom[i].dest, drv->rom[i].size, drv->rom[i].crc,
						drv->rom[i].filename, interleaved68kRom);
				DEBUG_LOG("From parent %d", pi);
				if (pi && (region != 5 && region != 0 && region != 7)) {
					sprintf(romerror, "File check for %s failed, ROM set not compatible",
							drv->rom[i].filename);
					goto error1;
				}
			} else {
				int region = drv->rom[i].region;
				if (region != 5 && region != 0 && region != 7) {
					sprintf(romerror, "File check for %s failed, ROM set not compatible",
							drv->rom[i].filename);
					goto error1;
				}

			}
		}

	}
	gn_terminate_pbar();
	/* Close/clean up */
	gn_close_zip(gz);
	if (gzp) gn_close_zip(gzp);
	free(drv);

	if (r->adpcmb.size == 0) {
		r->adpcmb.p = r->adpcma.p;
		r->adpcmb.size = r->adpcma.size;
	}

	memory.fix_game_usage = r->gfix_usage.p; //malloc(r->game_sfix.size >> 5);
	/*	memory.pen_usage = malloc((r->tiles.size >> 11) * sizeof(Uint32));
	CHECK_ALLOC(memory.pen_usage);
	memset(memory.pen_usage, 0, (r->tiles.size >> 11) * sizeof(Uint32));
	 */
	memory.nb_of_tiles = r->tiles.size >> 7;

	/* Init rom and bios */
	init_roms(contextPtr, r);
	convert_all_tile(r);
	return dr_load_bios(contextPtr, r, romerror);

error1:
	gn_terminate_pbar();
	//unzClose(gz);
	//if (gzp) unzClose(gzp);
	gn_close_zip(gz);
	if (gzp)
		gn_close_zip(gzp);

	free(drv);
	return false;
}

int dr_load_game(void *contextPtr, char *name, char romerror[1024]) {
	//GAME_ROMS rom;
	struct PathArray pArr = get_rom_path(contextPtr);
	char *rpath = pArr.data;
	int rc;
	logMsg("Loading %s/%s\n", rpath, name);
	memory.bksw_handler = 0;
	memory.bksw_unscramble = NULL;
	memory.bksw_offset = NULL;
	need_decrypt = 1;

	rc = dr_load_roms(contextPtr, &memory.rom, rpath, name, romerror);
	if (rc == false) {
		return false;
	}
	conf.game = memory.rom.info.name;
	/* TODO *///neogeo_fix_bank_type =0;
	/* TODO */
	//	set_bankswitchers(0);

	memcpy(memory.game_vector, memory.rom.cpu_m68k.p, 0x80);
	memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);

	convert_all_char(memory.rom.game_sfix.p, memory.rom.game_sfix.size,
			memory.fix_game_usage);

	/* TODO: Move this somewhere else. */
	init_video();

	return true;

}

#if defined(HAVE_LIBZ)//&& defined (HAVE_MMAP)

static int dump_region(FILE *gno, const ROM_REGION *rom, Uint8 id, Uint8 type,
		Uint32 block_size, unsigned verbose) {
	if (rom->p == NULL)
		return false;
	fwrite(&rom->size, sizeof (Uint32), 1, gno);
	fwrite(&id, sizeof (Uint8), 1, gno);
	fwrite(&type, sizeof (Uint8), 1, gno);
	if (type == 0) {
		if(verbose) logMsg("Dump %d %08x", id, rom->size);
		fwrite(rom->p, rom->size, 1, gno);
	} else {
		Uint32 nb_block = rom->size / block_size;
		Uint32 *block_offset;
		Uint32 cur_offset = 0;
		long offset_pos;
		Uint32 i;
		const Uint8 *inbuf = rom->p;
		Uint8 *outbuf;
		uLongf outbuf_len;
		uLongf outlen;
		Uint32 outlen32;
		Uint32 cmpsize = 0;
		int rc;
		if(verbose) logMsg("nb_block=%d", nb_block);
		fwrite(&block_size, sizeof (Uint32), 1, gno);
		if ((rom->size & (block_size - 1)) != 0) {
			if(verbose) logMsg("Waring: Block_size and totsize not compatible %x %x\n",
					rom->size, block_size);
		}
		block_offset = malloc(nb_block * sizeof (Uint32));
		/* Zlib compress output buffer need to be at least the size
		 of inbuf + 0.1% + 12 byte */
		outbuf_len = compressBound(block_size);
		outbuf = malloc(outbuf_len);
		offset_pos = ftell(gno);
		fseek(gno, nb_block * 4 + 4, SEEK_CUR); /* Skip all the offset table + the total compressed size */

		for (i = 0; i < nb_block; i++) {
			cur_offset = ftell(gno);
			block_offset[i] = cur_offset;
			outlen = outbuf_len;
			rc = compress(outbuf, &outlen, inbuf, block_size);
			if(verbose) logMsg("%d %ld", rc, outlen);
			//cur_offset += outlen;
			cmpsize += outlen;
			if(verbose) logMsg("cmpsize=%d %ld", cmpsize, (long int)sizeof (uLongf));
			inbuf += block_size;
			outlen32 = (Uint32) outlen;
			fwrite(&outlen32, sizeof (Uint32), 1, gno);
			if(verbose) logMsg("bank %d outlen=%d offset=%d", i, outlen32, cur_offset);
			fwrite(outbuf, outlen, 1, gno);
		}
		free(outbuf);
		/* Now, write the offset table */
		fseek(gno, offset_pos, SEEK_SET);
		fwrite(block_offset, sizeof (Uint32), nb_block, gno);
		free(block_offset);
		fwrite(&cmpsize, sizeof (Uint32), 1, gno);
		if(verbose) logMsg("cmpsize=%d", cmpsize);
		fseek(gno, 0, SEEK_END);
		offset_pos = ftell(gno);
		if(verbose) logMsg("currpos=%li", offset_pos);
	}
	return true;
}

int dr_save_gno(GAME_ROMS *r, char *filename) {
	FILE *gno;
	char *fid = "gnodmpv1";
	char fname[9];
	Uint8 nb_sec = 0;
	int i;

	gn_init_pbar(PBAR_ACTION_SAVEGNO, 4);
	gno = fopen(filename, "wb");
	if (!gno)
		return false;

	/* restore game vector */
	memcpy(memory.rom.cpu_m68k.p, memory.game_vector, 0x80);
	/*for (i = 0; i < 0x80; i++)
		printf("%02x ", memory.rom.cpu_m68k.p[i]);
	printf("\n");*/

	if (r->cpu_m68k.p)
		nb_sec++;
	if (r->cpu_z80.p)
		nb_sec++;
	if (r->adpcma.p)
		nb_sec++;
	if (r->adpcmb.p && (r->adpcmb.p != r->adpcma.p))
		nb_sec++;
	if (r->game_sfix.p)
		nb_sec++;
	if (r->tiles.p)
		nb_sec += 2; /* Sprite + Sprite usage */
	if (r->gfix_usage.p)
		nb_sec++;
	/* Do we need Custom Bios? */
	if ((r->info.flags & HAS_CUSTOM_CPU_BIOS)) {
		logMsg("Has custom CPU BIOS");
		nb_sec++;
	}
	if ((r->info.flags & HAS_CUSTOM_SFIX_BIOS)) {
		logMsg("Has custom SFIX BIOS");
		nb_sec++;
	}


	/* Header information */
	fwrite(fid, 8, 1, gno);
	snprintf(fname, 9, "%-8s", r->info.name);
	fwrite(fname, 8, 1, gno);
	fwrite(&r->info.flags, sizeof (Uint32), 1, gno);
	fwrite(&nb_sec, sizeof (Uint8), 1, gno);

	/* Now each section */
	dump_region(gno, &r->cpu_m68k, REGION_MAIN_CPU_CARTRIDGE, 0, 0, 0);
	dump_region(gno, &r->cpu_z80, REGION_AUDIO_CPU_CARTRIDGE, 0, 0, 0);
	gn_update_pbar(1);
	dump_region(gno, &r->adpcma, REGION_AUDIO_DATA_1, 0, 0, 0);
	if (r->adpcma.p != r->adpcmb.p)
		dump_region(gno, &r->adpcmb, REGION_AUDIO_DATA_2, 0, 0, 0);
	gn_update_pbar(2);
	dump_region(gno, &r->game_sfix, REGION_FIXED_LAYER_CARTRIDGE, 0, 0, 0);
	dump_region(gno, &r->spr_usage, REGION_SPR_USAGE, 0, 0, 0);
	dump_region(gno, &r->gfix_usage, REGION_GAME_FIX_USAGE, 0, 0, 0);
	if ((r->info.flags & HAS_CUSTOM_CPU_BIOS)) {
		dump_region(gno, &r->bios_m68k, REGION_MAIN_CPU_BIOS, 0, 0, 0);
	}
	if ((r->info.flags & HAS_CUSTOM_SFIX_BIOS)) {
		dump_region(gno, &r->bios_sfix, REGION_FIXED_LAYER_BIOS, 0, 0, 0);
	}
	gn_update_pbar(3);
	/* TODO, there is a bug in the loading routine, only one compressed (type 1)
	 * region can be present at the end of the file */
	dump_region(gno, &r->tiles, REGION_SPRITES, 1, 4096, 0);


	fclose(gno);
	return true;
}

int read_region(FILE *gno, GAME_ROMS *roms) {
	Uint32 size;
	Uint8 lid, type;
	ROM_REGION *r = NULL;
	size_t totread = 0;
	Uint32 cache_size[] = {64, 32, 24, 16, 8, 6, 4, 2, 1, 0};
	int i = 0;

	/* Read region header */
	totread = fread(&size, sizeof (Uint32), 1, gno);
	totread += fread(&lid, sizeof (Uint8), 1, gno);
	totread += fread(&type, sizeof (Uint8), 1, gno);

	switch (lid) {
		case REGION_MAIN_CPU_CARTRIDGE:
			r = &roms->cpu_m68k;
			break;
		case REGION_AUDIO_CPU_CARTRIDGE:
			r = &roms->cpu_z80;
			break;
		case REGION_AUDIO_DATA_1:
			r = &roms->adpcma;
			break;
		case REGION_AUDIO_DATA_2:
			r = &roms->adpcmb;
			break;
		case REGION_FIXED_LAYER_CARTRIDGE:
			r = &roms->game_sfix;
			break;
		case REGION_SPRITES:
			r = &roms->tiles;
			break;
		case REGION_SPR_USAGE:
			r = &roms->spr_usage;
			break;
		case REGION_GAME_FIX_USAGE:
			r = &roms->gfix_usage;
			break;
		case REGION_FIXED_LAYER_BIOS:
			r = &roms->bios_sfix;
			break;
		case REGION_MAIN_CPU_BIOS://break;
			logMsg("reading custom CPU BIOS");
			r = &roms->bios_m68k;
			break;
		default:
			return false;
	}

	logMsg("Read region %d %08X type %d\n", lid, size, type);
	if (type == 0) {
		/* TODO: Support ADPCM streaming for platform with less that 64MB of Mem */
		allocate_region(r, size, lid);
		logMsg("Load %d %08x\n", lid, r->size);
		totread += fread(r->p, r->size, 1, gno);
	} else {
		Uint32 nb_block, block_size;
		Uint32 cmp_size;
		totread += fread(&block_size, sizeof (Uint32), 1, gno);
		nb_block = size / block_size;

		logMsg("Region size=%08X\n", size);
		r->size = size;


		memory.vid.spr_cache.offset = malloc(sizeof (Uint32) * nb_block);
		totread += fread(memory.vid.spr_cache.offset, sizeof (Uint32), nb_block, gno);
		memory.vid.spr_cache.gno = gno;

		totread += fread(&cmp_size, sizeof (Uint32), 1, gno);

		fseek(gno, cmp_size, SEEK_CUR);

		/* TODO: Find the best cache size dynamically! */
		for (i = 0; cache_size[i] != 0; i++) {
			if (init_sprite_cache(cache_size[i]*1024 * 1024, block_size) == 0) {
				logMsg("Cache size=%dMB\n", cache_size[i]);
				break;
			}
		}
	}
	return true;
}

int dr_open_gno(void *contextPtr, char *filename, char romerror[1024]) {
	FILE *gno;
	char fid[9]; // = "gnodmpv1";
	char name[9] = {0,};
	GAME_ROMS *r = &memory.rom;
	Uint8 nb_sec;
	int i;
	char *a;
	size_t totread = 0;

	memory.bksw_handler = 0;
	memory.bksw_unscramble = NULL;
	memory.bksw_offset = NULL;

	need_decrypt = 0;

	gno = fopen(filename, "rb");
	if (!gno)
	{
		sprintf(romerror, "Can't open %s", filename);
		return false;
	}

	totread += fread(fid, 8, 1, gno);
	if (strncmp(fid, "gnodmpv1", 8) != 0) {
		fclose(gno);
		sprintf(romerror, "Invalid GNO file");
		return false;
	}
	totread += fread(name, 8, 1, gno);
	a = strchr(name, ' ');
	if (a) a[0] = 0;
	strcpy(r->info.name, name);

	totread += fread(&r->info.flags, sizeof (Uint32), 1, gno);
	totread += fread(&nb_sec, sizeof (Uint8), 1, gno);

	gn_init_pbar(PBAR_ACTION_LOADGNO, nb_sec);
	for (i = 0; i < nb_sec; i++) {
		gn_update_pbar(i);
		read_region(gno, r);
	}
	gn_terminate_pbar();

	if (r->adpcmb.p == NULL) {
		r->adpcmb.p = r->adpcma.p;
		r->adpcmb.size = r->adpcma.size;
	}
	//fclose(gno);

	memory.fix_game_usage = r->gfix_usage.p;
	/*	memory.pen_usage = malloc((r->tiles.size >> 11) * sizeof(Uint32));
	CHECK_ALLOC(memory.pen_usage);
	memset(memory.pen_usage, 0, (r->tiles.size >> 11) * sizeof(Uint32));*/
	memory.nb_of_tiles = r->tiles.size >> 7;

	/* Init rom and bios */
	init_roms(contextPtr, r);
	//convert_all_tile(r);
	if(!dr_load_bios(contextPtr, r, romerror))
		return false;

	conf.game = memory.rom.info.name;

	memcpy(memory.game_vector, memory.rom.cpu_m68k.p, 0x80);
	memcpy(memory.rom.cpu_m68k.p, memory.rom.bios_m68k.p, 0x80);
	init_video();

	return true;
}

char *dr_gno_romname(char *filename) {
	FILE *gno;
	char fid[9]; // = "gnodmpv1";
	char name[9] = {0,};
	size_t totread = 0;

	gno = fopen(filename, "rb");
	if (!gno)
		return NULL;

	totread += fread(fid, 8, 1, gno);
	if (strncmp(fid, "gnodmpv1", 8) != 0) {
		fclose(gno);
		logMsg("Invalid GNO file");
		return NULL;
	}

	totread += fread(name, 8, 1, gno);
	fclose(gno);
	return strdup(name);
}


#else

static int dump_region(FILE *gno, ROM_REGION *rom, Uint8 id, Uint8 type, Uint32 block_size) {
	return TRUE;
}

int dr_save_gno(GAME_ROMS *r, char *filename) {
	return TRUE;
}
#endif

void dr_free_roms(GAME_ROMS *r) {
	free_region(&r->cpu_m68k);
	free_region(&r->cpu_z80c);

	if (!memory.vid.spr_cache.data) {
		logMsg("Free tiles\n");
		free_region(&r->tiles);
	} else {
		fclose(memory.vid.spr_cache.gno);
		free_sprite_cache();
		free(memory.vid.spr_cache.offset);
	}
	free_region(&r->game_sfix);

	free_region(&r->cpu_z80);
	free_region(&r->bios_audio);
	if (r->adpcmb.p != r->adpcma.p)
		free_region(&r->adpcmb);
	else {
		r->adpcmb.p = NULL;
		r->adpcmb.size = 0;
	}

	free_region(&r->adpcma);

	free_region(&r->bios_m68k);
	free_region(&r->bios_sfix);

	free(memory.ng_lo);
	free(memory.fix_game_usage);
	free_region(&r->spr_usage);

	//free(r->info.name);
	//free(r->info.longname);

	conf.game = NULL;
}
