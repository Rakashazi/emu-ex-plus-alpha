#pragma once

void FCEUPPU_Init(void);
void FCEUPPU_Reset(void);
void FCEUPPU_Power(void);
int FCEUPPU_Loop(int skip, bool render);

void FCEUPPU_LineUpdate();
void FCEUPPU_SetVideoSystem(int w);

extern void (*PPU_hook)(uint32 A);
extern void (*GameHBIRQHook)(void), (*GameHBIRQHook2)(void);

/* For cart.c and banksw.h, mostly */
extern uint8 NTARAM[0x800], *vnapage[4];
extern uint8 PPUNTARAM;
extern uint8 PPUCHRRAM;

void FCEUPPU_SaveState(void);
void FCEUPPU_LoadState(int version);
uint8* FCEUPPU_GetCHR(uint32 vadr, uint32 refreshaddr);
void ppu_getScroll(int &xpos, int &ypos);


#ifdef _MSC_VER
#define FASTCALL __fastcall
#else
#define FASTCALL
#endif

void PPU_ResetHooks();
extern uint8 (FASTCALL *FFCEUX_PPURead)(uint32 A);
extern void (*FFCEUX_PPUWrite)(uint32 A, uint8 V);
extern uint8 FASTCALL FFCEUX_PPURead_Default(uint32 A);
void FFCEUX_PPUWrite_Default(uint32 A, uint8 V);

extern int scanline;
extern int g_rasterpos;
extern uint8 PPU[4];

enum PPUPHASE {
	PPUPHASE_VBL, PPUPHASE_BG, PPUPHASE_OBJ
};

extern PPUPHASE ppuphase;

// native pixel buffer
static const uint nesPixX = 256, nesPixY = 240, nesVisiblePixY = 224;

#ifdef USE_PIX_RGB565
#define NATIVE_PIX_TYPE uint16
#else
#define NATIVE_PIX_TYPE uint32
#endif

extern NATIVE_PIX_TYPE nativeCol[256];
extern NATIVE_PIX_TYPE nativePixBuff[nesPixX*nesVisiblePixY] __attribute__ ((aligned (8)));
