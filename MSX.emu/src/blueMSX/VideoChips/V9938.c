/*****************************************************************************
**
** This command engine may be used in any MSX emulator. It does not matter 
** under which conditions that the emulator gets distributed. Be it open-
** source or closed-source. Be it commercially or free-of-charge.
** 
** This command engine may not be used in any other kind of softwares.
** Using this command engine is at own risk. The author is not responsible 
** nor liable for any damages that may occur intentionally or unintentionally 
** from it's usage.
**
******************************************************************************
*/
#include "V9938.h"
#include <string.h>
#include <stdlib.h>
#include "VDP.h"
#include "Board.h"
#include "SaveState.h"

/*************************************************************
** Different compilers inline C functions differently.
**************************************************************
*/
#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE static
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define VDPSTATUS_TR 0x80
#define VDPSTATUS_BO 0x10
#define VDPSTATUS_CE 0x01

/*************************************************************
** Other useful defines
**************************************************************
*/
#define VDP_VRMP5R(s, X, Y) ((s)->vramRead + (((Y & 1023) << 7) + (((X & 255) >> 1)) & (s)->maskRead))
#define VDP_VRMP6R(s, X, Y) ((s)->vramRead + (((Y & 1023) << 7) + (((X & 511) >> 2)) & (s)->maskRead))
#define VDP_VRMP7R(s, X, Y) ((s)->vramRead + (((Y &  511) << 7) + ((((X & 511) >> 2) + ((X & 2) << 15))) & (s)->maskRead))
#define VDP_VRMP8R(s, X, Y) ((s)->vramRead + (((Y &  511) << 7) + ((((X & 255) >> 1) + ((X & 1) << 16))) & (s)->maskRead))

#define VDP_VRMP5W(s, X, Y) ((s)->vramWrite + (((Y & 1023) << 7) + (((X & 255) >> 1)) & (s)->maskWrite))
#define VDP_VRMP6W(s, X, Y) ((s)->vramWrite + (((Y & 1023) << 7) + (((X & 511) >> 2)) & (s)->maskWrite))
#define VDP_VRMP7W(s, X, Y) ((s)->vramWrite + (((Y &  511) << 7) + ((((X & 511) >> 2) + ((X & 2) << 15))) & (s)->maskWrite))
#define VDP_VRMP8W(s, X, Y) ((s)->vramWrite + (((Y &  511) << 7) + ((((X & 255) >> 1) + ((X & 1) << 16))) & (s)->maskWrite))

#define CM_ABRT  0x0
#define CM_NOOP1 0x1
#define CM_NOOP2 0x2
#define CM_NOOP3 0x3
#define CM_POINT 0x4
#define CM_PSET  0x5
#define CM_SRCH  0x6
#define CM_LINE  0x7
#define CM_LMMV  0x8
#define CM_LMMM  0x9
#define CM_LMCM  0xA
#define CM_LMMC  0xB
#define CM_HMMV  0xC
#define CM_HMMM  0xD
#define CM_YMMM  0xE
#define CM_HMMC  0xF

/*************************************************************
** Many VDP commands are executed in some kind of loop but
** essentially, there are only a few basic loop structures
** that are re-used. We define the loop structures that are
** re-used here so that they have to be entered only once
**************************************************************
*/
#define pre_loop \
    while (cnt > 0) {

/* Loop over DX, DY */
#define post__x_y(MX) \
        ADX += TX; \
		if (--ANX == 0 || (ADX & MX)) { \
            DY += TY; \
            ADX = DX; ANX = NX; \
            if ((--NY & 1023) == 0 || DY == -1) {\
                break; \
		    } \
        } \
        cnt-=delta; \
    }

#define post__xyy(MX) \
        ADX += TX; \
        if (ADX & MX) { \
			SY += TY; DY += TY; \
			ADX = DX; \
			if ((--NY & 1023) == 0 || SY == -1 || DY == -1) { \
				break; \
			} \
		} \
        cnt-=delta; \
	}

#define post_xxyy(MX) \
		ASX += TX; ADX += TX; \
		if (--ANX == 0 || ((ASX | ADX) & MX)) { \
			SY += TY; DY += TY; \
			ASX = SX; ADX = DX; ANX = NX; \
			if ((--NY & 1023) == 0 || SY == -1 || DY == -1) { \
				break; \
			} \
		} \
        cnt-=delta; \
	}


#define pre_loop2 \
    while (vdpCmd->VdpOpsCnt > 0) {

#define post_xxyy2(MX) \
		vdpCmd->ASX += vdpCmd->TX; vdpCmd->ADX += vdpCmd->TX; \
		if (--vdpCmd->ANX == 0 || ((vdpCmd->ASX | vdpCmd->ADX) & MX)) { \
			vdpCmd->SY += vdpCmd->TY; vdpCmd->DY += vdpCmd->TY; \
			vdpCmd->ASX = vdpCmd->SX; vdpCmd->ADX = vdpCmd->DX; vdpCmd->ANX = vdpCmd->NX; \
			if ((--vdpCmd->NY & 1023) == 0 || vdpCmd->SY == -1 || vdpCmd->DY == -1) { \
				break; \
			} \
		} \
        vdpCmd->VdpOpsCnt -= delta; \
	}

/*************************************************************
** VdpCmd State instance
**************************************************************
*/
struct VdpCmdState {
    UInt8* vramBase;
    UInt8* vramRead;
    UInt8* vramWrite;
    int    maskRead;
    int    maskWrite;
    int    vramOffset[2];
    int    vramMask[2];
    int   SX;
    int   SY;
    int   DX;
    int   DY;
    int   NX;
    int   kNX;
    int   NY;
    int   ASX;
    int   ADX;
    int   ANX;
    UInt8 ARG;
    UInt8 CL;
    UInt8 LO;
    UInt8 CM;
    
    UInt8 status;
    UInt16 borderX;

    int   TX;
    int   TY;
    int   MX;
    int   VdpOpsCnt;
    UInt32 systemTime;
    int    screenMode;
    int    newScrMode;
    int    timingMode;
};

// Pointer to initialized command engine. This should be a list
// but since there are only one instance at the time its not
// a big deal. It is also only used in vdpCmdFlushAll()
static VdpCmdState* vdpCmdGlobal = NULL;


/*************************************************************
** Forward declarations
**************************************************************
*/
static UInt8 *getVramPointerW(VdpCmdState* vdpCmd, UInt8 M, int X, int Y);

static UInt8 getPixel(VdpCmdState* vdpCmd, UInt8 SM, int SX, int SY);
static UInt8 getPixel5(VdpCmdState* vdpCmd, int SX, int SY);
static UInt8 getPixel6(VdpCmdState* vdpCmd, int SX, int SY);
static UInt8 getPixel7(VdpCmdState* vdpCmd, int SX, int SY);
static UInt8 getPixel8(VdpCmdState* vdpCmd, int SX, int SY);

static void setPixel(VdpCmdState* vdpCmd, UInt8 SM, int DX, int DY, UInt8 CL, UInt8 OP);
static void setPixel5(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP);
static void setPixel6(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP);
static void setPixel7(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP);
static void setPixel8(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP);

static void setPixelLow(UInt8 *P, UInt8 CL, UInt8 M, UInt8 OP);

static void SrchEngine(VdpCmdState* vdpCmd);
static void LineEngine(VdpCmdState* vdpCmd);
static void LmmvEngine(VdpCmdState* vdpCmd);
static void LmmmEngine(VdpCmdState* vdpCmd);
static void LmcmEngine(VdpCmdState* vdpCmd);
static void LmmcEngine(VdpCmdState* vdpCmd);
static void HmmvEngine(VdpCmdState* vdpCmd);
static void HmmmEngine(VdpCmdState* vdpCmd);
static void YmmmEngine(VdpCmdState* vdpCmd);
static void HmmcEngine(VdpCmdState* vdpCmd);


/*************************************************************
** Constants
**************************************************************
*/
static UInt8 Mask[4] = { 0x0f, 0x03, 0x0f, 0xff };
static int   PPB[4]  = { 2, 4, 2, 1 };
static int   PPL[4]  = { 256, 512, 512, 256 };


static int srch_timing[8] = { 92,  125, 92,  92  };
static int line_timing[8] = { 120, 147, 120, 120 };
//static int line_timing[8] = { 120, 147, 120, 132 };
static int hmmv_timing[8] = { 49,  65,  49,  62  };
static int lmmv_timing[8] = { 98,  137, 98,  124 };
static int ymmm_timing[8] = { 65,  125, 65,  68  };
static int hmmm_timing[8] = { 92,  136, 92,  97  };
static int lmmm_timing[8] = { 129, 197, 129, 132 };

/*************************************************************
** getVramPointerW
**
** Description:
**      Calculate addr of a pixel in vram
**************************************************************
*/
INLINE UInt8 *getVramPointerW(VdpCmdState* vdpCmd, UInt8 M,int X,int Y)
{
    switch(M) {
    case 0: 
        return VDP_VRMP5W(vdpCmd, X, Y);
    case 1: 
        return VDP_VRMP6W(vdpCmd, X, Y);
    case 2: 
        return VDP_VRMP7W(vdpCmd, X, Y);
    case 3: 
        return VDP_VRMP8W(vdpCmd, X, Y);
    }

    return vdpCmd->vramWrite;
}


/*************************************************************
** getPixel5
**
** Description:
**      Get a pixel on screen 5
**************************************************************
*/
INLINE UInt8 getPixel5(VdpCmdState* vdpCmd, int SX, int SY)
{
    return (*VDP_VRMP5R(vdpCmd, SX, SY) >> (((~SX)&1)<<2))&15;
}

/*************************************************************
** getPixel6
**
** Description:
**      Get a pixel on screen 6
**************************************************************
*/
INLINE UInt8 getPixel6(VdpCmdState* vdpCmd, int SX, int SY)
{
    return (*VDP_VRMP6R(vdpCmd, SX, SY) >>(((~SX)&3)<<1))&3;
}

/*************************************************************
** getPixel7
**
**    Get a pixel on screen 7
**************************************************************
*/
INLINE UInt8 getPixel7(VdpCmdState* vdpCmd, int SX, int SY)
{
    return (*VDP_VRMP7R(vdpCmd, SX, SY) >>(((~SX)&1)<<2))&15;
}

/*************************************************************
** getPixel8
**
** Description:
**      Get a pixel on screen 8
**************************************************************
*/
INLINE UInt8 getPixel8(VdpCmdState* vdpCmd, int SX, int SY)
{
    return *VDP_VRMP8R(vdpCmd, SX, SY);
}

/*************************************************************
** getPixel
**
**    Get a pixel on a screen
**************************************************************
*/
INLINE UInt8 getPixel(VdpCmdState* vdpCmd, UInt8 SM, int SX, int SY)
{
    switch(SM) {
    case 0: 
        return getPixel5(vdpCmd, SX, SY);
    case 1: 
        return getPixel6(vdpCmd, SX, SY);
    case 2: 
        return getPixel7(vdpCmd, SX, SY);
    case 3: 
        return getPixel8(vdpCmd, SX, SY);
    }

    return 0;
}

/*************************************************************
** setPixelLow
**
** Description:
**      Low level function to set a pixel on a screen
**************************************************************
*/
INLINE void setPixelLow(UInt8 *P, UInt8 CL, UInt8 M, UInt8 OP)
{
    switch (OP) {
    case 0: 
        *P = (*P & M) | CL; 
        break;
    case 1: 
        *P = *P & (CL | M); 
        break;
    case 2: 
        *P |= CL; 
        break;
    case 3: 
        *P ^= CL; 
        break;
    case 4: 
        *P = (*P & M) | ~(CL | M); 
        break;
    case 8: 
        if (CL) *P = (*P & M) | CL; 
        break;
    case 9: 
        if (CL) *P = *P & (CL | M); 
        break;
    case 10: 
        if (CL) {
            *P |= CL; 
        }
        break;
    case 11: 
        if (CL) {
            *P ^= CL; 
        }
        break;
    case 12: 
        if (CL) {
            *P = (*P & M) | ~(CL|M); 
        }
        break;
    }
}

/*************************************************************
** setPixel5
**
** Description:
**      Set a pixel on screen 5
**************************************************************
*/
INLINE void setPixel5(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP)
{
    UInt8 SH = ((~DX)&1)<<2;

    setPixelLow(VDP_VRMP5W(vdpCmd, DX, DY), CL << SH, ~(15<<SH), OP);
}
 
/*************************************************************
** setPixel6
**
**    Set a pixel on screen 6
**************************************************************
*/
INLINE void setPixel6(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP)
{
    UInt8 SH = ((~DX)&3)<<1;

    setPixelLow(VDP_VRMP6W(vdpCmd, DX, DY), CL << SH, ~(3<<SH), OP);
}

/*************************************************************
** setPixel7
**
** Description:
**      Set a pixel on screen 7
**************************************************************
*/
INLINE void setPixel7(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP)
{
    UInt8 SH = ((~DX)&1)<<2;

    setPixelLow(VDP_VRMP7W(vdpCmd, DX, DY), CL << SH, ~(15<<SH), OP);
}

/*************************************************************
** setPixel8
**
**    Set a pixel on screen 8
**************************************************************
*/
INLINE void setPixel8(VdpCmdState* vdpCmd, int DX, int DY, UInt8 CL, UInt8 OP)
{
    setPixelLow(VDP_VRMP8W(vdpCmd, DX, DY), CL, 0, OP);
}

/*************************************************************
** setPixel
**
** Description:
**      Set a pixel on a screen
**************************************************************
*/
INLINE void setPixel(VdpCmdState* vdpCmd, UInt8 SM, int DX, int DY, UInt8 CL, UInt8 OP)
{
    switch (SM) {
    case 0: setPixel5(vdpCmd, DX, DY, CL, OP); break;
    case 1: setPixel6(vdpCmd, DX, DY, CL, OP); break;
    case 2: setPixel7(vdpCmd, DX, DY, CL, OP); break;
    case 3: setPixel8(vdpCmd, DX, DY, CL, OP); break;
    }
}

/*************************************************************
** SrchEgine
**
** Description:
**      Search a dot
**************************************************************
*/
static void SrchEngine(VdpCmdState* vdpCmd)
{
    int SX=vdpCmd->SX;
    int SY=vdpCmd->SY;
    int TX=vdpCmd->TX;
    int ANX=vdpCmd->ANX;
    UInt8 CL=vdpCmd->CL & Mask[vdpCmd->screenMode];
    int delta = srch_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

#define pre_srch \
    pre_loop \
    if ((
#define post_srch(MX) \
    ==CL) ^ANX) { \
        vdpCmd->status|=VDPSTATUS_BO; /* Border detected */ \
        break; \
    } \
    if ((SX+=TX) & MX) { \
        vdpCmd->status&=~VDPSTATUS_BO; /* Border not detected */ \
        break; \
    } \
    cnt-=delta; \
} 

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_srch getPixel5(vdpCmd, SX, SY) post_srch(256)
        break;
    case 1: 
        pre_srch getPixel6(vdpCmd, SX, SY) post_srch(512)
        break;
    case 2: 
        pre_srch getPixel7(vdpCmd, SX, SY) post_srch(512)
        break;
    case 3: 
        pre_srch getPixel8(vdpCmd, SX, SY) post_srch(256)
        break;
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM=0;
        /* Update SX in VDP registers */
        vdpCmd->borderX = 0xfe00 | SX;
    }
    else {
        vdpCmd->SX=SX;
    }
}

/*************************************************************
** LineEgine
**
** Description:
**      Draw a line
**************************************************************
*/
static void LineEngine(VdpCmdState* vdpCmd)
{
    int DX=vdpCmd->DX;
    int DY=vdpCmd->DY;
    int TX=vdpCmd->TX;
    int TY=vdpCmd->TY;
    int NX=vdpCmd->NX;
    int NY=vdpCmd->NY;
    int ASX=vdpCmd->ASX;
    int ADX=vdpCmd->ADX;
    UInt8 CL=vdpCmd->CL & Mask[vdpCmd->screenMode];
    UInt8 LO=vdpCmd->LO;
    int delta = line_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

#define post_linexmaj(MX) \
    DX+=TX; \
    if (ADX++==NX || (DX&MX)) \
    break; \
    if ((ASX-=NY)<0) { \
    ASX+=NX; \
    DY+=TY; \
    } \
    ASX&=1023; /* Mask to 10 bits range */ \
        cnt-=delta; \
}
#define post_lineymaj(MX) \
    DY+=TY; \
    if ((ASX-=NY)<0) { \
    ASX+=NX; \
    DX+=TX; \
    } \
    ASX&=1023; /* Mask to 10 bits range */ \
    if (ADX++==NX || (DX&MX)) \
    break; \
        cnt-=delta; \
    }

    if ((vdpCmd->ARG&0x01)==0) {
        /* X-Axis is major direction */
        switch (vdpCmd->screenMode) {
        case 0: 
            pre_loop setPixel5(vdpCmd, DX, DY, CL, LO); post_linexmaj(256)
            break;
        case 1: 
            pre_loop setPixel6(vdpCmd, DX, DY, CL, LO); post_linexmaj(512)
            break;
        case 2: 
            pre_loop setPixel7(vdpCmd, DX, DY, CL, LO); post_linexmaj(512)
            break;
        case 3: 
            pre_loop setPixel8(vdpCmd, DX, DY, CL, LO); post_linexmaj(256)
            break;
        }
    }
    else {
        /* Y-Axis is major direction */
        switch (vdpCmd->screenMode) {
        case 0: 
            pre_loop setPixel5(vdpCmd, DX, DY, CL, LO); post_lineymaj(256)
            break;
        case 1: 
            pre_loop setPixel6(vdpCmd, DX, DY, CL, LO); post_lineymaj(512)
            break;
        case 2: 
            pre_loop setPixel7(vdpCmd, DX, DY, CL, LO); post_lineymaj(512)
            break;
        case 3: 
            pre_loop setPixel8(vdpCmd, DX, DY, CL, LO); post_lineymaj(256)
            break;
        }
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM=0;
        vdpCmd->DY=DY & 0x03ff;
    }
    else {
        vdpCmd->DX=DX;
        vdpCmd->DY=DY;
        vdpCmd->ASX=ASX;
        vdpCmd->ADX=ADX;
    }
}

/*************************************************************
** LmmvEngine
**
** Description:
**      VDP -> Vram
**************************************************************
*/
static void LmmvEngine(VdpCmdState* vdpCmd)
{
    int DX=vdpCmd->DX;
    int DY=vdpCmd->DY;
    int TX=vdpCmd->TX;
    int TY=vdpCmd->TY;
    int NX=vdpCmd->NX;
    int NY=vdpCmd->NY;
    int ADX=vdpCmd->ADX;
    int ANX=vdpCmd->ANX;
    UInt8 CL=vdpCmd->CL & Mask[vdpCmd->screenMode];
    UInt8 LO=vdpCmd->LO;
    int delta = lmmv_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_loop setPixel5(vdpCmd, ADX, DY, CL, LO); post__x_y(256)
        break;
    case 1: 
        pre_loop setPixel6(vdpCmd, ADX, DY, CL, LO); post__x_y(512)
        break;
    case 2: 
        pre_loop setPixel7(vdpCmd, ADX, DY, CL, LO); post__x_y(512)
        break;
    case 3: 
        pre_loop setPixel8(vdpCmd, ADX, DY, CL, LO); post__x_y(256)
        break;
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM=0;
        vdpCmd->DY=DY & 0x03ff;
        vdpCmd->NY=NY & 0x03ff;
    }
    else {
        vdpCmd->DY=DY;
        vdpCmd->NY=NY;
        vdpCmd->ANX=ANX;
        vdpCmd->ADX=ADX;
    }
}

/*************************************************************
** LmmmEngine
**
** Description:
**      Vram -> Vram
**************************************************************
*/
static void LmmmEngine(VdpCmdState* vdpCmd)
{
    int SX=vdpCmd->SX;
    int SY=vdpCmd->SY;
    int DX=vdpCmd->DX;
    int DY=vdpCmd->DY;
    int TX=vdpCmd->TX;
    int TY=vdpCmd->TY;
    int NX=vdpCmd->NX;
    int NY=vdpCmd->NY;
    int ASX=vdpCmd->ASX;
    int ADX=vdpCmd->ADX;
    int ANX=vdpCmd->ANX;
    UInt8 LO=vdpCmd->LO;
    int delta = lmmm_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_loop setPixel5(vdpCmd, ADX, DY, getPixel5(vdpCmd, ASX, SY), LO); post_xxyy(256)
        break;
    case 1: 
        pre_loop setPixel6(vdpCmd, ADX, DY, getPixel6(vdpCmd, ASX, SY), LO); post_xxyy(512)
        break;
    case 2: 
        pre_loop setPixel7(vdpCmd, ADX, DY, getPixel7(vdpCmd, ASX, SY), LO); post_xxyy(512)
        break;
    case 3: 
        pre_loop setPixel8(vdpCmd, ADX, DY, getPixel8(vdpCmd, ASX, SY), LO); post_xxyy(256)
        break;
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM=0;
        vdpCmd->DY=DY & 0x03ff;
        vdpCmd->SY=SY & 0x03ff;
        vdpCmd->NY=NY & 0x03ff;
    }
    else {
        vdpCmd->SY=SY;
        vdpCmd->DY=DY;
        vdpCmd->NY=NY;
        vdpCmd->ANX=ANX;
        vdpCmd->ASX=ASX;
        vdpCmd->ADX=ADX;
    }
}

/*************************************************************
** LmcmEngine
**
** Description:
**      Vram -> CPU
**************************************************************
*/
static void LmcmEngine(VdpCmdState* vdpCmd)
{
    if (!(vdpCmd->status & VDPSTATUS_TR)) {
        vdpCmd->CL = getPixel(vdpCmd, vdpCmd->screenMode, vdpCmd->ASX, vdpCmd->SY);
        vdpCmd->status |= VDPSTATUS_TR;

        if (!--vdpCmd->ANX || ((vdpCmd->ASX+=vdpCmd->TX)&vdpCmd->MX)) {
            vdpCmd->SY+=vdpCmd->TY;
            if (!(--vdpCmd->NY & 1023) || vdpCmd->SY==-1) {
                vdpCmd->status &= ~VDPSTATUS_CE;
                vdpCmd->CM = 0;
            }
            else {
                vdpCmd->ASX=vdpCmd->SX;
                vdpCmd->ANX=vdpCmd->NX;
            }
        }
    }
}

/*************************************************************
** LmmcEngine
**
** Description:
**      CPU -> Vram
**************************************************************
*/
static void LmmcEngine(VdpCmdState* vdpCmd)
{
    if (!(vdpCmd->status & VDPSTATUS_TR)) {
        UInt8 SM=vdpCmd->screenMode;

        UInt8 CL=vdpCmd->CL & Mask[SM];
        setPixel(vdpCmd, SM, vdpCmd->ADX, vdpCmd->DY, CL, vdpCmd->LO);
        vdpCmd->status |= VDPSTATUS_TR;

        if (!--vdpCmd->ANX || ((vdpCmd->ADX+=vdpCmd->TX)&vdpCmd->MX)) {
            vdpCmd->DY+=vdpCmd->TY;
            if (!(--vdpCmd->NY&1023) || vdpCmd->DY==-1) {
                vdpCmd->status &= ~VDPSTATUS_CE;
                vdpCmd->CM = 0;
            }
            else {
                vdpCmd->ADX=vdpCmd->DX;
                vdpCmd->ANX=vdpCmd->NX;
            }
        }
    }
}

/*************************************************************
** HmmvEngine
**
** Description:
**      VDP --> Vram
**************************************************************
*/
static void HmmvEngine(VdpCmdState* vdpCmd)
{
    int DX=vdpCmd->DX;
    int DY=vdpCmd->DY;
    int TX=vdpCmd->TX;
    int TY=vdpCmd->TY;
    int NX=vdpCmd->NX;
    int NY=vdpCmd->NY;
    int ADX=vdpCmd->ADX;
    int ANX=vdpCmd->ANX;
    UInt8 CL=vdpCmd->CL;
    int delta = hmmv_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_loop *VDP_VRMP5W(vdpCmd, ADX, DY) = CL; post__x_y(256)
        break;
    case 1: 
        pre_loop *VDP_VRMP6W(vdpCmd, ADX, DY) = CL; post__x_y(512)
        break;
    case 2: 
        pre_loop *VDP_VRMP7W(vdpCmd, ADX, DY) = CL; post__x_y(512)
        break;
    case 3: 
        pre_loop *VDP_VRMP8W(vdpCmd, ADX, DY) = CL; post__x_y(256)
        break;
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM = 0;
        vdpCmd->DY=DY & 0x03ff;
        vdpCmd->NY=NY & 0x03ff;
    }
    else {
        vdpCmd->DY=DY;
        vdpCmd->NY=NY;
        vdpCmd->ANX=ANX;
        vdpCmd->ADX=ADX;
    }
}

/*************************************************************
** HmmmEngine
**
** Description:
**      Vram -> Vram
**************************************************************
*/
static void HmmmEngine(VdpCmdState* vdpCmd)
{
    int delta = hmmm_timing[vdpCmd->timingMode];

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_loop2 *VDP_VRMP5W(vdpCmd, vdpCmd->ADX, vdpCmd->DY) = *VDP_VRMP5R(vdpCmd, vdpCmd->ASX, vdpCmd->SY); post_xxyy2(256)
        break;
    case 1: 
        pre_loop2 *VDP_VRMP6W(vdpCmd, vdpCmd->ADX, vdpCmd->DY) = *VDP_VRMP6R(vdpCmd, vdpCmd->ASX, vdpCmd->SY); post_xxyy2(512)
        break;
    case 2: 
        pre_loop2 *VDP_VRMP7W(vdpCmd, vdpCmd->ADX, vdpCmd->DY) = *VDP_VRMP7R(vdpCmd, vdpCmd->ASX, vdpCmd->SY); post_xxyy2(512)
        break;
    case 3: 
        pre_loop2 *VDP_VRMP8W(vdpCmd, vdpCmd->ADX, vdpCmd->DY) = *VDP_VRMP8R(vdpCmd, vdpCmd->ASX, vdpCmd->SY); post_xxyy2(256)
        break;
    }

    if (vdpCmd->VdpOpsCnt > 0) { \
        /* Command execution done */
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CM = 0;
    }
}

/*************************************************************
** YmmmEngine
**
** Description:
**      Vram -> Vram 
**************************************************************
*/
static void YmmmEngine(VdpCmdState* vdpCmd)
{
    int SY=vdpCmd->SY;
    int DX=vdpCmd->DX;
    int DY=vdpCmd->DY;
    int TX=vdpCmd->TX;
    int TY=vdpCmd->TY;
    int NY=vdpCmd->NY;
    int ADX=vdpCmd->ADX;
    int delta = ymmm_timing[vdpCmd->timingMode];
    int cnt;

    cnt = vdpCmd->VdpOpsCnt;

    switch (vdpCmd->screenMode) {
    case 0: 
        pre_loop *VDP_VRMP5W(vdpCmd, ADX, DY) = *VDP_VRMP5R(vdpCmd, ADX, SY); post__xyy(256)
        break;
    case 1: 
        pre_loop *VDP_VRMP6W(vdpCmd, ADX, DY) = *VDP_VRMP6R(vdpCmd, ADX, SY); post__xyy(512)
        break;
    case 2: 
        pre_loop *VDP_VRMP7W(vdpCmd, ADX, DY) = *VDP_VRMP7R(vdpCmd, ADX, SY); post__xyy(512)
        break;
    case 3: 
        pre_loop *VDP_VRMP8W(vdpCmd, ADX, DY) = *VDP_VRMP8R(vdpCmd, ADX, SY); post__xyy(256)
        break;
    }

    if ((vdpCmd->VdpOpsCnt=cnt)>0) {
        /* Command execution done */
        vdpCmd->status &=~VDPSTATUS_CE;
        vdpCmd->CM = 0;
        vdpCmd->DY=DY & 0x03ff;
        vdpCmd->SY=SY & 0x03ff;
        vdpCmd->NY=NY & 0x03ff;
    }
    else {
        vdpCmd->SY=SY;
        vdpCmd->DY=DY;
        vdpCmd->NY=NY;
        vdpCmd->ADX=ADX;
    }
}

/*************************************************************
** HmmcEngine
**
** Description:
**      CPU -> Vram 
**************************************************************
*/
static void HmmcEngine(VdpCmdState* vdpCmd)
{
    if (!(vdpCmd->status & VDPSTATUS_TR)) {
        *getVramPointerW(vdpCmd, vdpCmd->screenMode, vdpCmd->ADX, vdpCmd->DY)=vdpCmd->CL;
        vdpCmd->VdpOpsCnt-=hmmv_timing[vdpCmd->timingMode];
        vdpCmd->status |= VDPSTATUS_TR;

        if (!--vdpCmd->ANX || ((vdpCmd->ADX+=vdpCmd->TX)&vdpCmd->MX)) {
            vdpCmd->DY += vdpCmd->TY;
            if (!(--vdpCmd->NY&1023) || vdpCmd->DY==-1) {
                vdpCmd->status &= ~VDPSTATUS_CE;
                vdpCmd->CM = 0;
            }
            else {
                vdpCmd->ADX=vdpCmd->DX;
                vdpCmd->ANX=vdpCmd->NX;
            }
        }
    }
}


/*************************************************************
** vdpCmdInit
**
** Description:
**      Initializes the command engine.
**************************************************************
*/
VdpCmdState* vdpCmdCreate(int vramSize, UInt8* vramPtr, UInt32 systemTime)
{
    VdpCmdState* vdpCmd = calloc(1, sizeof(VdpCmdState));
    vdpCmd->systemTime = systemTime;
    vdpCmd->vramBase = vramPtr;

    vdpCmd->vramOffset[0] = 0;
    vdpCmd->vramOffset[1] = vramSize > 0x20000 ? 0x20000 : 0;
    vdpCmd->vramMask[0]   = vramSize > 0x20000 ? 0x1ffff : vramSize - 1;
    vdpCmd->vramMask[1]   = vramSize > 0x20000 ? 0xffff  : vramSize - 1;

    vdpCmd->vramRead  = vdpCmd->vramBase + vdpCmd->vramOffset[0];
    vdpCmd->vramWrite = vdpCmd->vramBase + vdpCmd->vramOffset[0];
    vdpCmd->maskRead  = vdpCmd->vramMask[0];
    vdpCmd->maskWrite = vdpCmd->vramMask[0];

    vdpCmdGlobal = vdpCmd; // Ugly fix to make the cmd engine flushable

    return vdpCmd;
}


/*************************************************************
** vdpCmdDestroy
**
** Description:
**      Destroys the command engine
**************************************************************
*/
void vdpCmdDestroy(VdpCmdState* vdpCmd)
{
    free(vdpCmd);
    vdpCmdGlobal = 0l;
}


/*************************************************************
** vdpCmdSetCommand
**
** Description:
**      Set VDP command to ececute
**************************************************************
*/
static void vdpCmdSetCommand(VdpCmdState* vdpCmd, UInt32 systemTime)
{
    vdpCmd->screenMode = vdpCmd->newScrMode;

    if (vdpCmd->screenMode < 0) {
        vdpCmd->CM = 0;
        vdpCmd->status &= ~VDPSTATUS_CE;
        return;
    }
    
    vdpCmd->SX &= 0x1ff;
    vdpCmd->SY &= 0x3ff;
    vdpCmd->DX &= 0x1ff;
    vdpCmd->DY &= 0x3ff;
    vdpCmd->NX &= 0x3ff;
    vdpCmd->NY &= 0x3ff;

    switch (vdpCmd->CM) {
    case CM_ABRT:
        vdpCmd->CM = 0;
        vdpCmd->status &= ~VDPSTATUS_CE;
        return;

    case CM_NOOP1:
    case CM_NOOP2:
    case CM_NOOP3:
        vdpCmd->CM = 0;
        return;

    case CM_POINT:
        vdpCmd->CM = 0;
        vdpCmd->status &= ~VDPSTATUS_CE;
        vdpCmd->CL = getPixel(vdpCmd, vdpCmd->screenMode, vdpCmd->SX, vdpCmd->SY);
        return;

    case CM_PSET:
        vdpCmd->CM = 0;
        vdpCmd->status &= ~VDPSTATUS_CE;
        setPixel(vdpCmd, vdpCmd->screenMode, vdpCmd->DX, vdpCmd->DY, vdpCmd->CL & Mask[vdpCmd->screenMode], vdpCmd->LO);
        return;
    }

    vdpCmd->MX  = PPL[vdpCmd->screenMode]; 
    vdpCmd->TY  = vdpCmd->ARG & 0x08? -1 : 1;

    /* Argument depends on UInt8 or dot operation */
    if ((vdpCmd->CM & 0x0C) == 0x0C) {
        vdpCmd->TX = vdpCmd->ARG & 0x04 ? -PPB[vdpCmd->screenMode] : PPB[vdpCmd->screenMode];
        vdpCmd->NX = vdpCmd->kNX/PPB[vdpCmd->screenMode];
    }
    else {
        vdpCmd->NX = vdpCmd->kNX;
        vdpCmd->TX = vdpCmd->ARG & 0x04 ? -1 : 1;
    }

    /* X loop variables are treated specially for LINE command */
    if (vdpCmd->CM == CM_LINE) {
        vdpCmd->ASX = (vdpCmd->NX - 1) >> 1;
        vdpCmd->ADX = 0;
    }
    else {
        vdpCmd->ASX = vdpCmd->SX;
        vdpCmd->ADX = vdpCmd->DX;
    }    

    /* NX loop variable is treated specially for SRCH command */
    if (vdpCmd->CM == CM_SRCH)
        vdpCmd->ANX=(vdpCmd->ARG&0x02)!=0; /* Do we look for "==" or "!="? */
    else
        vdpCmd->ANX = vdpCmd->NX;

    /* Command execution started */
    vdpCmd->status |= VDPSTATUS_CE;

    vdpCmd->systemTime = systemTime;
}

/*************************************************************
** vdpCmdWrite
**
** Description:
**      Writes a new command to the VDP
**************************************************************
*/
void vdpCmdWrite(VdpCmdState* vdpCmd, UInt8 reg, UInt8 value, UInt32 systemTime)
{
    switch (reg & 0x1f) {
	case 0x00: vdpCmd->SX = (vdpCmd->SX & 0xff00) | value;                   break;
	case 0x01: vdpCmd->SX = (vdpCmd->SX & 0x00ff) | ((value & 0x01) << 8);   break;
	case 0x02: vdpCmd->SY = (vdpCmd->SY & 0xff00) | value;                   break;
	case 0x03: vdpCmd->SY = (vdpCmd->SY & 0x00ff) | ((value & 0x03) << 8);   break;
	case 0x04: vdpCmd->DX = (vdpCmd->DX & 0xff00) | value;                   break;
	case 0x05: vdpCmd->DX = (vdpCmd->DX & 0x00ff) | ((value & 0x01) << 8);   break;
	case 0x06: vdpCmd->DY = (vdpCmd->DY & 0xff00) | value;                   break;
	case 0x07: vdpCmd->DY = (vdpCmd->DY & 0x00ff) | ((value & 0x03) << 8);   break;
	case 0x08: vdpCmd->kNX = (vdpCmd->kNX & 0xff00) | value;                 break;
	case 0x09: vdpCmd->kNX = (vdpCmd->kNX & 0x00ff) | ((value & 0x03) << 8); break;
	case 0x0a: vdpCmd->NY = (vdpCmd->NY & 0xff00) | value;                   break;
	case 0x0b: vdpCmd->NY = (vdpCmd->NY & 0x00ff) | ((value & 0x03) << 8);   break;
	case 0x0c: 
        vdpCmd->CL = value;
        vdpCmd->status &= ~VDPSTATUS_TR;
        break;
	case 0x0d: 
        if ((vdpCmd->ARG ^ value) & 0x30) {
            vdpCmd->vramRead  = vdpCmd->vramBase + vdpCmd->vramOffset[(value >> 4) & 1];
            vdpCmd->vramWrite = vdpCmd->vramBase + vdpCmd->vramOffset[(value >> 5) & 1];
            vdpCmd->maskRead  = vdpCmd->vramMask[(value >> 4) & 1];
            vdpCmd->maskWrite = vdpCmd->vramMask[(value >> 5) & 1];
        }
        vdpCmd->ARG = value; 
        break;
	case 0x0e: 
		vdpCmd->LO = value & 0x0F;
		vdpCmd->CM = value >> 4;
		vdpCmdSetCommand(vdpCmd, systemTime);
		break;
    }
}

/*************************************************************
** vdpCmdPeek
**
** Description:
**      Returns the current value of a VDP command register
**************************************************************
*/
UInt8 vdpCmdPeek(VdpCmdState* vdpCmd, UInt8 reg, UInt32 systemTime) 
{
    switch (reg & 0x1f) {
	case 0x00: return vdpCmd->SX & 0xff;
	case 0x01: return vdpCmd->SX >> 8;
	case 0x02: return vdpCmd->SY & 0xff;
	case 0x03: return vdpCmd->SY >> 8;
	case 0x04: return vdpCmd->DX & 0xff;
	case 0x05: return vdpCmd->DX >> 8;
	case 0x06: return vdpCmd->DY & 0xff;
	case 0x07: return vdpCmd->DY >> 8;
	case 0x08: return vdpCmd->kNX & 0xff;
	case 0x09: return vdpCmd->kNX >> 8;
	case 0x0a: return vdpCmd->NY & 0xff;
	case 0x0b: return vdpCmd->NY >> 8;
    case 0x0c: return vdpCmd->CL;
    case 0x0d: return vdpCmd->ARG;
    case 0x0e: return vdpCmd->LO | (vdpCmd->CM << 4);
    }
    return 0xff;
}

/*************************************************************
** vdpSetScreenMode
**
** Description:
**      Sets the current screen mode
**************************************************************
*/
void vdpSetScreenMode(VdpCmdState* vdpCmd, int screenMode, int commandEnable) {
    if (screenMode > 8 && screenMode <= 12) {
        screenMode = 3;
    }
    else if (screenMode < 5 || screenMode > 12) {
        if (commandEnable) {
            screenMode = 2;
        }
        else {
            screenMode = -1;
        }
    }
    else {
        screenMode -= 5;
    }
    if (vdpCmd->newScrMode != screenMode) {
        vdpCmd->newScrMode = screenMode;
        if (screenMode == -1) {
            vdpCmd->CM = 0;
            vdpCmd->status &= ~VDPSTATUS_CE;
        }
    }
}

/*************************************************************
** vdpSetTimingMode
**
** Description:
**      Sets the timing mode (sprites/nosprites, ...)
**************************************************************
*/
void vdpSetTimingMode(VdpCmdState* vdpCmd, UInt8 timingMode) {
    vdpCmd->timingMode = timingMode;
}

/*************************************************************
** vdpGetStatus
**
** Description:
**      Gets current status
**************************************************************
*/
UInt8 vdpGetStatus(VdpCmdState* vdpCmd) {
    return vdpCmd->status;
}

/*************************************************************
** vdpGetBorderX
**
** Description:
**      Gets the border X value
**************************************************************
*/
UInt16 vdpGetBorderX(VdpCmdState* vdpCmd) {
    return vdpCmd->borderX;
}

/*************************************************************
** vdpGetColor
**
** Description:
**      Gets the color value
**************************************************************
*/
UInt8 vdpGetColor(VdpCmdState* vdpCmd) {
    vdpCmd->status &= ~VDPSTATUS_TR;
    return vdpCmd->CL;
}

/*************************************************************
** vdpCmdFlush
**
** Description:
**      Flushes current VDP command
**************************************************************
*/
void vdpCmdFlush(VdpCmdState* vdpCmd) 
{
    while (vdpCmd->CM != 0 && !(vdpCmd->status & VDPSTATUS_TR)) {
        int opsCnt = vdpCmd->VdpOpsCnt += 1000000;
        vdpCmdExecute(vdpCmd, vdpCmd->systemTime + opsCnt);
        if (vdpCmd->VdpOpsCnt == 0 || vdpCmd->VdpOpsCnt == opsCnt) {
            break;
        }
    }
}


/*************************************************************
** vdpCmdFlush
**
** Description:
**      Flushes current VDP command on all created engines.
**************************************************************
*/
void vdpCmdFlushAll() 
{
    if (vdpCmdGlobal) {
        vdpCmdFlush(vdpCmdGlobal);
    }
}


/*************************************************************
** vdpCmdExecute
**
** Description:
**      Executes command engine until the given time.
**************************************************************
*/
void vdpCmdExecute(VdpCmdState* vdpCmd, UInt32 systemTime)
{
    vdpCmd->VdpOpsCnt += systemTime - vdpCmd->systemTime;
    vdpCmd->systemTime = systemTime;
    
    if (vdpCmd->VdpOpsCnt <= 0) {
        return;
    }

    switch (vdpCmd->CM) {
    case CM_SRCH:
        SrchEngine(vdpCmd);
        break;
    case CM_LINE:
        LineEngine(vdpCmd);
        break;
    case CM_LMMV:
        LmmvEngine(vdpCmd);
        break;
    case CM_LMMM:
        LmmmEngine(vdpCmd);
        break;
    case CM_LMCM:
        LmcmEngine(vdpCmd);
        break;
    case CM_LMMC:
        LmmcEngine(vdpCmd);
        break;
    case CM_HMMV:
        HmmvEngine(vdpCmd);
        break;
    case CM_HMMM:
        HmmmEngine(vdpCmd);
        break;
    case CM_YMMM:
        YmmmEngine(vdpCmd);
        break;
    case CM_HMMC:
        HmmcEngine(vdpCmd);  
        break;
    default:
        vdpCmd->VdpOpsCnt = 0;
    }
}

/*************************************************************
** vdpCmdLoadState
**
** Description:
**      Loads the vdpCmd of the command engine. 
**************************************************************
*/
void vdpCmdLoadState(VdpCmdState* vdpCmd)
{
    SaveState* state = saveStateOpenForRead("vdpCommandEngine");

    vdpCmd->SX            =         saveStateGet(state, "SX",         0);
    vdpCmd->SY            =         saveStateGet(state, "SY",         0);
    vdpCmd->DX            =         saveStateGet(state, "DX",         0);
    vdpCmd->DY            =         saveStateGet(state, "DY",         0);
    vdpCmd->NX            =         saveStateGet(state, "NX",         0);
    vdpCmd->NY            =         saveStateGet(state, "NY",         0);
    vdpCmd->ASX           =         saveStateGet(state, "ASX",        0);
    vdpCmd->ADX           =         saveStateGet(state, "ADX",        0);
    vdpCmd->ANX           =         saveStateGet(state, "ANX",        0);
    vdpCmd->ARG           = (UInt8) saveStateGet(state, "ARG",        0);
    vdpCmd->CL            = (UInt8) saveStateGet(state, "CL",         0);
    vdpCmd->LO            = (UInt8) saveStateGet(state, "LO",         0);
    vdpCmd->CM            = (UInt8) saveStateGet(state, "CM",         0);
    vdpCmd->status        = (UInt8) saveStateGet(state, "STATUS",     0);
    vdpCmd->borderX       = (UInt16)saveStateGet(state, "BORDERX",    0);
    vdpCmd->TX            =         saveStateGet(state, "TX",         0);
    vdpCmd->TY            =         saveStateGet(state, "TY",         0);
    vdpCmd->MX            =         saveStateGet(state, "MX",         0);
    vdpCmd->VdpOpsCnt     =         saveStateGet(state, "VdpOpsCnt",  0);
    vdpCmd->systemTime    =         saveStateGet(state, "systemTime", boardSystemTime());
    vdpCmd->newScrMode    =         saveStateGet(state, "newScrMode", 0);
    vdpCmd->screenMode    =         saveStateGet(state, "screenMode", 0);
    vdpCmd->timingMode    =         saveStateGet(state, "timingMode", 0);
    
    saveStateClose(state);

    vdpCmd->vramRead  = vdpCmd->vramBase + vdpCmd->vramOffset[(vdpCmd->ARG >> 4) & 1];
    vdpCmd->vramWrite = vdpCmd->vramBase + vdpCmd->vramOffset[(vdpCmd->ARG >> 5) & 1];
    vdpCmd->maskRead  = vdpCmd->vramMask[(vdpCmd->ARG >> 4) & 1];
    vdpCmd->maskWrite = vdpCmd->vramMask[(vdpCmd->ARG >> 5) & 1];
}


/*************************************************************
** vdpCmdSaveState
**
** Description:
**      Saves the vdpCmd of the command engine. 
**************************************************************
*/
void vdpCmdSaveState(VdpCmdState* vdpCmd)
{
    SaveState* state = saveStateOpenForWrite("vdpCommandEngine");

    saveStateSet(state, "SX",         vdpCmd->SX);
    saveStateSet(state, "SY",         vdpCmd->SY);
    saveStateSet(state, "DX",         vdpCmd->DX);
    saveStateSet(state, "DY",         vdpCmd->DY);
    saveStateSet(state, "NX",         vdpCmd->NX);
    saveStateSet(state, "NY",         vdpCmd->NY);
    saveStateSet(state, "ASX",        vdpCmd->ASX);
    saveStateSet(state, "ADX",        vdpCmd->ADX);
    saveStateSet(state, "ANX",        vdpCmd->ANX);
    saveStateSet(state, "ARG",        vdpCmd->ARG);
    saveStateSet(state, "CL",         vdpCmd->CL);
    saveStateSet(state, "LO",         vdpCmd->LO);
    saveStateSet(state, "CM",         vdpCmd->CM);
    saveStateSet(state, "STATUS",     vdpCmd->status);
    saveStateSet(state, "BORDERX",    vdpCmd->borderX);
    saveStateSet(state, "TX",         vdpCmd->TX);
    saveStateSet(state, "TY",         vdpCmd->TY);
    saveStateSet(state, "MX",         vdpCmd->MX);
    saveStateSet(state, "VdpOpsCnt",  vdpCmd->VdpOpsCnt);
    saveStateSet(state, "systemTime", vdpCmd->systemTime);
    saveStateSet(state, "screenMode", vdpCmd->screenMode);
    saveStateSet(state, "timingMode", vdpCmd->timingMode);
    
    saveStateClose(state);
}


