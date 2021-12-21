#pragma once

#include <string.h>
#include <stdlib.h>
#include <string.h>

/* Global data */
extern t_config config;
void ROMCheatUpdate();

static void osd_input_Update() { }

#define GG_ROM    "./ggenie.bin"
#define AR_ROM    "./areplay.bin"
#define SK_UPMEM  "./sk2chip.bin"
#define SK_ROM    "./sk.bin"
