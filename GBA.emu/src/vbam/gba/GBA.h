#ifndef GBA_H
#define GBA_H

#include "../System.h"
#include "../common/Port.h"
#include "../NLS.h"
#include "Flash.h"
#include <util/preprocessor/repeat.h>
#include <util/cLang.h>
#include <util/builtins.h>
#include <util/ansiTypes.h>
#include <logger/interface.h>

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION_9 9
#define SAVE_GAME_VERSION_10 10
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_10

struct GBAMem
{
#ifndef __clang__
	constexpr GBAMem() { }
#endif

	union IoMem
	{
		constexpr IoMem(): b{0} { }
		uint8 b[0x400] __attribute__ ((aligned(4)));
		struct
		{
			uint16 DISPCNT;
			uint16 greenSwap;
			uint16 DISPSTAT;
			uint16 VCOUNT;
			uint16 BG0CNT;
			uint16 BG1CNT;
			uint16 BG2CNT;
			uint16 BG3CNT;
			uint16 BG0HOFS;
			uint16 BG0VOFS;
			uint16 BG1HOFS;
			uint16 BG1VOFS;
			uint16 BG2HOFS;
			uint16 BG2VOFS;
			uint16 BG3HOFS;
			uint16 BG3VOFS;
			uint16 BG2PA;
			uint16 BG2PB;
			uint16 BG2PC;
			uint16 BG2PD;
			uint16 BG2X_L;
			uint16 BG2X_H;
			uint16 BG2Y_L;
			uint16 BG2Y_H;
			uint16 BG3PA;
			uint16 BG3PB;
			uint16 BG3PC;
			uint16 BG3PD;
			uint16 BG3X_L;
			uint16 BG3X_H;
			uint16 BG3Y_L;
			uint16 BG3Y_H;
			uint16 WIN0H;
			uint16 WIN1H;
			uint16 WIN0V;
			uint16 WIN1V;
			uint16 WININ;
			uint16 WINOUT;
			uint16 MOSAIC;
			uint16 unused4e;
			uint16 BLDMOD; // BLDCNT
			uint16 COLEV; // BLDALPHA
			uint16 COLY;
			// 0x56
			uint8 unused56[0xA];
			// 0x60
			uint8 soundRegs[0x50];
			// 0xB0
			uint16 DM0SAD_L;
			uint16 DM0SAD_H;
			uint16 DM0DAD_L;
			uint16 DM0DAD_H;
			uint16 DM0CNT_L;
			uint16 DM0CNT_H;
			uint16 DM1SAD_L;
			uint16 DM1SAD_H;
			uint16 DM1DAD_L;
			uint16 DM1DAD_H;
			uint16 DM1CNT_L;
			uint16 DM1CNT_H;
			uint16 DM2SAD_L;
			uint16 DM2SAD_H;
			uint16 DM2DAD_L;
			uint16 DM2DAD_H;
			uint16 DM2CNT_L;
			uint16 DM2CNT_H;
			uint16 DM3SAD_L;
			uint16 DM3SAD_H;
			uint16 DM3DAD_L;
			uint16 DM3DAD_H;
			uint16 DM3CNT_L;
			uint16 DM3CNT_H;
			// 0xE0
			uint8 unusedE0[0x20];
			// 0x100
			uint16 TM0D; // TM0CNT_L
			uint16 TM0CNT; // TM0CNT_H
			uint16 TM1D; // TM1CNT_L
			uint16 TM1CNT; // TM1CNT_H
			uint16 TM2D; // TM2CNT_L
			uint16 TM2CNT; // TM2CNT_H
			uint16 TM3D; // TM3CNT_L
			uint16 TM3CNT; // TM3CNT_H
			// 0x110
			uint8 unused110[0xF0];
			uint16 IE;
			uint16 IF;
			uint16 WAITCNT;
			uint16 unused206;
			uint16 IME;
		};

		void resetDmaRegs()
		{
		  DM0SAD_L = 0x0000;
		  DM0SAD_H = 0x0000;
		  DM0DAD_L = 0x0000;
		  DM0DAD_H = 0x0000;
		  DM0CNT_L = 0x0000;
		  DM0CNT_H = 0x0000;
		  DM1SAD_L = 0x0000;
		  DM1SAD_H = 0x0000;
		  DM1DAD_L = 0x0000;
		  DM1DAD_H = 0x0000;
		  DM1CNT_L = 0x0000;
		  DM1CNT_H = 0x0000;
		  DM2SAD_L = 0x0000;
		  DM2SAD_H = 0x0000;
		  DM2DAD_L = 0x0000;
		  DM2DAD_H = 0x0000;
		  DM2CNT_L = 0x0000;
		  DM2CNT_H = 0x0000;
		  DM3SAD_L = 0x0000;
		  DM3SAD_H = 0x0000;
		  DM3DAD_L = 0x0000;
		  DM3DAD_H = 0x0000;
		  DM3CNT_L = 0x0000;
		  DM3CNT_H = 0x0000;
		}

		void resetLcdRegs(bool useBios, bool skipBios)
		{
			DISPCNT  = 0x0080;
			DISPSTAT = 0x0000;
			VCOUNT   = (useBios && !skipBios) ? 0 :0x007E;
			BG0CNT   = 0x0000;
			BG1CNT   = 0x0000;
			BG2CNT   = 0x0000;
			BG3CNT   = 0x0000;
			BG0HOFS  = 0x0000;
			BG0VOFS  = 0x0000;
			BG1HOFS  = 0x0000;
			BG1VOFS  = 0x0000;
			BG2HOFS  = 0x0000;
			BG2VOFS  = 0x0000;
			BG3HOFS  = 0x0000;
			BG3VOFS  = 0x0000;
			BG2PA    = 0x0100;
			BG2PB    = 0x0000;
			BG2PC    = 0x0000;
			BG2PD    = 0x0100;
			BG2X_L   = 0x0000;
			BG2X_H   = 0x0000;
			BG2Y_L   = 0x0000;
			BG2Y_H   = 0x0000;
			BG3PA    = 0x0100;
			BG3PB    = 0x0000;
			BG3PC    = 0x0000;
			BG3PD    = 0x0100;
			BG3X_L   = 0x0000;
			BG3X_H   = 0x0000;
			BG3Y_L   = 0x0000;
			BG3Y_H   = 0x0000;
			WIN0H    = 0x0000;
			WIN1H    = 0x0000;
			WIN0V    = 0x0000;
			WIN1V    = 0x0000;
			WININ    = 0x0000;
			WINOUT   = 0x0000;
			MOSAIC   = 0x0000;
			BLDMOD   = 0x0000;
			COLEV    = 0x0000;
			COLY     = 0x0000;
		}
	};

	u8 bios[0x4000] __attribute__ ((aligned(4))) {0};
	IoMem ioMem;
	u8 internalRAM[0x8000] __attribute__ ((aligned(4))) {0};
	u8 workRAM[0x40000] __attribute__ ((aligned(4))) {0};
	u8 rom[0x2000000] __attribute__ ((aligned(4)))
#ifndef __clang__
	{0}
#endif
	;
};

struct GBADMA
{
	constexpr GBADMA() { }

	int cpuDmaTicksToUpdate = 0;
	int cpuDmaCount = 0;
	bool cpuDmaHack = false;
	u32 cpuDmaLast = 0;

	u32 dma0Source = 0;
	u32 dma0Dest = 0;
	u32 dma1Source = 0;
	u32 dma1Dest = 0;
	u32 dma2Source = 0;
	u32 dma2Dest = 0;
	u32 dma3Source = 0;
	u32 dma3Dest = 0;

	void reset(GBAMem::IoMem &ioMem)
	{
	  dma0Source = 0;
	  dma0Dest = 0;
	  dma1Source = 0;
	  dma1Dest = 0;
	  dma2Source = 0;
	  dma2Dest = 0;
	  dma3Source = 0;
	  dma3Dest = 0;

	  ioMem.resetDmaRegs();
	}
};

struct GBATimers
{
	constexpr GBATimers() { }

	u8 timerOnOffDelay = 0;
	bool timer0On = false;
	bool timer1On = false;
	bool timer2On = false;
	bool timer3On = false;
	int timer0Ticks = 0;
	int timer0Reload = 0;
	int timer0ClockReload = 0;
	int timer1Ticks = 0;
	int timer1Reload = 0;
	int timer1ClockReload = 0;
	int timer2Ticks = 0;
	int timer2Reload = 0;
	int timer2ClockReload = 0;
	int timer3Ticks = 0;
	int timer3Reload = 0;
	int timer3ClockReload = 0;

	u16 timer0Value = 0;
	u16 timer1Value = 0;
	u16 timer2Value = 0;
	u16 timer3Value = 0;
};

static const int layerSettings = 0xff00;

typedef u16 MixColorType;
struct GBALCD;
void mode0RenderLine(MixColorType *, GBALCD &lcd, const GBAMem::IoMem &ioMem);

struct GBALCD
{
	constexpr GBALCD() { }

#ifndef GBALCD_TEMP_LINE_BUFFER
	u32 line0[240] {0};
	u32 line1[240] {0};
	u32 line2[240] {0};
	u32 line3[240] {0};
	u32 lineOBJ[240] {0};
#endif
	u32 lineOBJWin[240] {0};
	bool gfxInWin0[240] {0};
	bool gfxInWin1[240] {0};
	int lineOBJpixleft[128] {0};
	u16 pix[240 * 160] __attribute__ ((aligned(8))) {0};
	u8 vram[0x20000] __attribute__ ((aligned(4))) {0};
	u8 paletteRAM[0x400] __attribute__ ((aligned(4))) {0};
	u8 oam[0x400] __attribute__ ((aligned(4))) {0};
	typedef void (*RenderLineFunc)(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem);
	RenderLineFunc renderLine = mode0RenderLine;
	bool fxOn = false;
	bool windowOn = false;
	MixColorType *lineMix = nullptr;
	uint layerEnable = 0;
	int gfxBG2Changed = 0;
	int gfxBG3Changed = 0;
	int gfxBG2X = 0;
	int gfxBG2Y = 0;
	int gfxBG3X = 0;
	int gfxBG3Y = 0;
	int layerEnableDelay = 0;
	int lcdTicks = 0;
	u16 gfxLastVCOUNT = 0;

	void registerRamReset(u32 flags)
	{
    if(flags & 0x04) {
      // clear palette RAM
      memset(paletteRAM, 0, 0x400);
    }
    if(flags & 0x08) {
      // clear VRAM
      memset(vram, 0, 0x18000);
    }
    if(flags & 0x10) {
      // clean OAM
      memset(oam, 0, 0x400);
    }
	}

	void reset()
	{
		memset(paletteRAM, 0, sizeof(paletteRAM));
		memset(vram, 0, sizeof(vram));
		memset(oam, 0, sizeof(oam));
		memset(pix, 0, sizeof(pix));
	}

	void resetAll(bool useBios, bool skipBios, GBAMem::IoMem &ioMem)
	{
		reset();
		ioMem.resetLcdRegs(useBios, skipBios);
		layerEnable = ioMem.DISPCNT & layerSettings;
	}
};

typedef union {
  struct {
#ifdef WORDS_BIGENDIAN
    u8 B3;
    u8 B2;
    u8 B1;
    u8 B0;
#else
    u8 B0;
    u8 B1;
    u8 B2;
    u8 B3;
#endif
  } B;
  struct {
#ifdef WORDS_BIGENDIAN
    u16 W1;
    u16 W0;
#else
    u16 W0;
    u16 W1;
#endif
  } W;
#ifdef WORDS_BIGENDIAN
  volatile u32 I;
#else
	u32 I;
#endif
} reg_pair;

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

struct ARM7TDMI;
static inline u32 CPUReadByteQuick(ARM7TDMI &cpu, u32 addr);

static inline u32 CPUReadHalfWordQuick(ARM7TDMI &cpu, u32 addr);

static inline u32 CPUReadMemoryQuick(ARM7TDMI &cpu, u32 addr);

static const uint cpuBitsSet[256] =
{
		0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
		4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

//#define USE_MEM_HANDLERS
struct memoryMap {
	typedef u32 (*readFunc)(ARM7TDMI &cpu, u32 address);
	constexpr memoryMap() { }
	#ifdef USE_MEM_HANDLERS
	constexpr memoryMap(u8 *address, u32 mask, readFunc read8, readFunc read16, readFunc read32):
			address(address), mask(mask), read8(read8), read16(read16), read32(read32) { }
	#else
	constexpr memoryMap(u8 *address, u32 mask, readFunc read8, readFunc read16, readFunc read32):
		address(address), mask(mask) { }
	#endif
  u8 *address = nullptr;
  u32 mask = 0;
#ifdef USE_MEM_HANDLERS
  u32 (*read8)(ARM7TDMI &cpu, u32 address) = nullptr;
  u32 (*read16)(ARM7TDMI &cpu, u32 address) = nullptr;
  u32 (*read32)(ARM7TDMI &cpu, u32 address) = nullptr;
  /*void (*write8)(u32 address, u32 data) = nullptr;
  void (*write16)(u32 address, u32 data) = nullptr;
  void (*write32)(u32 address, u32 data) = nullptr;*/
#endif
};

//#define VBAM_USE_SWITICKS
//#define VBAM_USE_IRQTICKS
#define VBAM_USE_CPU_PREFETCH
#define VBAM_USE_DELAYED_CPU_FLAGS

struct GBASys;

struct ARM7TDMI
{
	constexpr ARM7TDMI(GBASys *gba): gba(gba) { }

	reg_pair reg[45] {{{0}}};
	u32 armNextPC = 0;
	int armMode = 0x1f;
	int cpuNextEvent = 0;
	int cpuTotalTicks = 0;
#ifdef VBAM_USE_SWITICKS
	int SWITicks = 0;
#endif
#ifdef VBAM_USE_IRQTICKS
	int IRQTicks = 0;
#endif
#ifdef VBAM_USE_CPU_PREFETCH
private:
	u32 cpuPrefetch[2] {0};
#endif
public:
	u32 busPrefetchCount = 0;
	/*u16 IE = 0;
	u16 IF = 0;
	u16 IME = 0;*/
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
	u32 lastArithmeticRes = 1;
	/*u32 lastArithmeticLhs = 1;
	u32 lastArithmeticRhs = 0;
	bool lastArithmeticAdd = 0;*/
#else
	bool N_FLAG = 0;
	bool Z_FLAG = 0;
#endif
	// TODO: eliminate C_FLAG and V_FLAG
	bool C_FLAG = 0;
	bool V_FLAG = 0;
	bool busPrefetch = 0;
	bool busPrefetchEnable = 0;
	bool armState = true;
	bool armIrqEnable = true;
	bool holdState = false;
	//u8 cpuBitsSet[256];
	//u8 cpuLowestBitSet[256];
	GBASys *gba;
	uint memoryWait[16] =
	  { 0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0 };
	uint memoryWait32[16] =
	  { 0, 0, 5, 0, 0, 1, 1, 0, 7, 7, 9, 9, 13, 13, 4, 0 };
	uint memoryWaitSeq[16] =
	  { 0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 0 };
	uint memoryWaitSeq32[16] =
	  { 0, 0, 5, 0, 0, 1, 1, 0, 5, 5, 9, 9, 17, 17, 4, 0 };
	memoryMap map[256];

	static bool calcNFlag(u32 result)
	{
		return (s32)result < 0;
	}

	static bool calcZFlag(u32 result)
	{
		return result == 0;
	}

	bool nFlag() const
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			return calcNFlag(lastArithmeticRes);
		#else
			return N_FLAG;
		#endif
	}

	bool zFlag() const
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			return calcZFlag(lastArithmeticRes);
		#else
			return Z_FLAG;
		#endif
	}

	bool cFlag() const
	{
		return C_FLAG;
	}

	bool vFlag() const
	{
		return V_FLAG;
	}

	void setNZFlag(bool n, bool z)
	{
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
		if(n)
			lastArithmeticRes = -1;
		else if(z)
			lastArithmeticRes = 0;
		else
			lastArithmeticRes = 1;
#else
		N_FLAG = n;
		Z_FLAG = z;
#endif
	}

	void resetFlags()
	{
		setNZFlag(0, 0);
		//setCFlag(0);
		//setVFlag(0);
	}

	void setNZFlagParam(u32 res)
	{
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
		lastArithmeticRes = res;
#else
		N_FLAG = calcNFlag(res);
		Z_FLAG = calcZFlag(res);
#endif
	}

	void setNZFlagParam(u32 low, u32 high)
	{
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
		lastArithmeticRes = low; if(!lastArithmeticRes && high) lastArithmeticRes = 1;
#else
		N_FLAG = (low & 0x80000000) ? true : false;
		Z_FLAG = low || high ? false : true;
#endif
	}

	void updateNZFlags(bool N_FLAG, bool Z_FLAG/*, bool C_FLAG, bool V_FLAG*/)
	{
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
		//logMsg("updateNZCVFlags %d:%d:%d:%d", N_FLAG, Z_FLAG, C_FLAG, V_FLAG);
		setNZFlag(N_FLAG, Z_FLAG);
		//setCFlag(C_FLAG);
		//setVFlag(V_FLAG);
#else
		var_selfs(N_FLAG);
		var_selfs(Z_FLAG);
		//var_selfs(C_FLAG);
		//var_selfs(V_FLAG);
#endif
	}

	void ARM_PREFETCH() ATTRS(always_inline)
  {
#ifdef VBAM_USE_CPU_PREFETCH
    cpuPrefetch[0] = CPUReadMemoryQuick(*this, armNextPC);
    cpuPrefetch[1] = CPUReadMemoryQuick(*this, armNextPC+4);
#endif
  }

	void THUMB_PREFETCH() ATTRS(always_inline)
  {
#ifdef VBAM_USE_CPU_PREFETCH
    cpuPrefetch[0] = CPUReadHalfWordQuick(*this, armNextPC);
    cpuPrefetch[1] = CPUReadHalfWordQuick(*this, armNextPC+2);
#endif
  }

	void ARM_PREFETCH_NEXT() ATTRS(always_inline)
	{
#ifdef VBAM_USE_CPU_PREFETCH
		cpuPrefetch[1] = CPUReadMemoryQuick(*this, armNextPC+4);
#endif
	}

	void THUMB_PREFETCH_NEXT() ATTRS(always_inline)
	{
#ifdef VBAM_USE_CPU_PREFETCH
		cpuPrefetch[1] = CPUReadHalfWordQuick(*this, armNextPC+2);
#endif
	}

	int prefetchArmOpcode() ATTRS(always_inline)
	{
#ifdef VBAM_USE_CPU_PREFETCH
		int opcode = cpuPrefetch[0];
		cpuPrefetch[0] = cpuPrefetch[1];
		return opcode;
#else
		return CPUReadMemoryQuick(armNextPC);
#endif
	}

	int prefetchThumbOpcode() ATTRS(always_inline)
	{
#ifdef VBAM_USE_CPU_PREFETCH
		int opcode = cpuPrefetch[0];
		cpuPrefetch[0] = cpuPrefetch[1];
		return opcode;
#else
		return CPUReadHalfWordQuick(armNextPC);
#endif
	}

	void softReset(int b)
	{
		armState = true;
		armMode = 0x1F;
		armIrqEnable = false;
		resetFlags();
		C_FLAG = V_FLAG = /*N_FLAG = Z_FLAG =*/ false;
		reg[13].I = 0x03007F00;
		reg[14].I = 0x00000000;
		reg[16].I = 0x00000000;
		reg[R13_IRQ].I = 0x03007FA0;
		reg[R14_IRQ].I = 0x00000000;
		reg[SPSR_IRQ].I = 0x00000000;
		reg[R13_SVC].I = 0x03007FE0;
		reg[R14_SVC].I = 0x00000000;
		reg[SPSR_SVC].I = 0x00000000;

		if(b) {
			armNextPC = 0x02000000;
			reg[15].I = 0x02000004;
		} else {
			armNextPC = 0x08000000;
			reg[15].I = 0x08000004;
		}
	}

	void reset(GBAMem::IoMem &ioMem, bool cpuIsMultiBoot, bool useBios, bool skipBios)
	{
		memset(&reg[0], 0, sizeof(reg));

		ioMem.IE       = 0x0000;
		ioMem.IF       = 0x0000;
		ioMem.IME      = 0x0000;

		armMode = 0x1F;

		if(cpuIsMultiBoot) {
			reg[13].I = 0x03007F00;
			reg[15].I = 0x02000000;
			reg[16].I = 0x00000000;
			reg[R13_IRQ].I = 0x03007FA0;
			reg[R13_SVC].I = 0x03007FE0;
			armIrqEnable = true;
		} else {
			if(useBios && !skipBios) {
				reg[15].I = 0x00000000;
				armMode = 0x13;
				armIrqEnable = false;
			} else {
				reg[13].I = 0x03007F00;
				reg[15].I = 0x08000000;
				reg[16].I = 0x00000000;
				reg[R13_IRQ].I = 0x03007FA0;
				reg[R13_SVC].I = 0x03007FE0;
				armIrqEnable = true;
			}
		}
		armState = true;
		resetFlags();
		C_FLAG = V_FLAG = /*N_FLAG = Z_FLAG =*/ false;

		// disable FIQ
		reg[16].I |= 0x40;

		updateCPSR();

		armNextPC = reg[15].I;
		reg[15].I += 4;

		holdState = 0;
#ifdef VBAM_USE_SWITICKS
		SWITicks = 0;
#endif
	}

	void updateCPSR()
	{
	  u32 CPSR = reg[16].I & 0x40;
	  if(nFlag())
	    CPSR |= 0x80000000;
	  if(zFlag())
	    CPSR |= 0x40000000;
	  if(C_FLAG)
	    CPSR |= 0x20000000;
	  if(V_FLAG)
	    CPSR |= 0x10000000;
	  if(!armState)
	    CPSR |= 0x00000020;
	  if(!armIrqEnable)
	    CPSR |= 0x80;
	  CPSR |= (armMode & 0x1F);
	  reg[16].I = CPSR;
	}

	void updateFlags(const GBAMem::IoMem &ioMem, bool breakLoop = true)
	{
	  u32 CPSR = reg[16].I;

	  bool N_FLAG = (CPSR & 0x80000000) ? true: false;
	  bool Z_FLAG = (CPSR & 0x40000000) ? true: false;
	  updateNZFlags(N_FLAG, Z_FLAG);
	  C_FLAG = (CPSR & 0x20000000) ? true: false;
	  V_FLAG = (CPSR & 0x10000000) ? true: false;
	  armState = (CPSR & 0x20) ? false : true;
	  armIrqEnable = (CPSR & 0x80) ? false : true;
	  if(breakLoop) {
	      if (armIrqEnable && (ioMem.IF & ioMem.IE) && (ioMem.IME & 1))
	        cpuNextEvent = cpuTotalTicks;
	  }
	}

	void undefinedException(const GBAMem::IoMem &ioMem)
	{
	  u32 PC = reg[15].I;
	  bool savedArmState = armState;
	  switchMode(ioMem, 0x1b, true, false);
	  reg[14].I = PC - (savedArmState ? 4 : 2);
	  reg[15].I = 0x04;
	  armState = true;
	  armIrqEnable = false;
	  armNextPC = 0x04;
	  ARM_PREFETCH();
	  reg[15].I += 4;
	}

	void softwareInterrupt(const GBAMem::IoMem &ioMem)
	{
	  u32 PC = reg[15].I;
	  bool savedArmState = armState;
	  switchMode(ioMem, 0x13, true, false);
	  reg[14].I = PC - (savedArmState ? 4 : 2);
	  reg[15].I = 0x08;
	  armState = true;
	  armIrqEnable = false;
	  armNextPC = 0x08;
	  ARM_PREFETCH();
	  reg[15].I += 4;
	}

	void interrupt(const GBAMem::IoMem &ioMem)
	{
		u32 PC = reg[15].I;
		bool savedState = armState;
		switchMode(ioMem, 0x12, true, false);
		reg[14].I = PC;
		if(!savedState)
			reg[14].I += 2;
		reg[15].I = 0x18;
		armState = true;
		armIrqEnable = false;

		armNextPC = reg[15].I;
		reg[15].I += 4;
		ARM_PREFETCH();
	}

	#ifdef WORDS_BIGENDIAN
	static void CPUSwap(volatile u32 *a, volatile u32 *b)
	{
		volatile u32 c = *b;
		*b = *a;
		*a = c;
	}
	#else
	static void CPUSwap(u32 *a, u32 *b)
	{
		u32 c = *b;
		*b = *a;
		*a = c;
	}
	#endif


	void switchMode(const GBAMem::IoMem &ioMem, int mode, bool saveState, bool breakLoop)
	{
	  //  if(armMode == mode)
	  //    return;

	  updateCPSR();

	  switch(armMode) {
	  case 0x10:
	  case 0x1F:
	    reg[R13_USR].I = reg[13].I;
	    reg[R14_USR].I = reg[14].I;
	    reg[17].I = reg[16].I;
	    break;
	  case 0x11:
	    CPUSwap(&reg[R8_FIQ].I, &reg[8].I);
	    CPUSwap(&reg[R9_FIQ].I, &reg[9].I);
	    CPUSwap(&reg[R10_FIQ].I, &reg[10].I);
	    CPUSwap(&reg[R11_FIQ].I, &reg[11].I);
	    CPUSwap(&reg[R12_FIQ].I, &reg[12].I);
	    reg[R13_FIQ].I = reg[13].I;
	    reg[R14_FIQ].I = reg[14].I;
	    reg[SPSR_FIQ].I = reg[17].I;
	    break;
	  case 0x12:
	    reg[R13_IRQ].I  = reg[13].I;
	    reg[R14_IRQ].I  = reg[14].I;
	    reg[SPSR_IRQ].I =  reg[17].I;
	    break;
	  case 0x13:
	    reg[R13_SVC].I  = reg[13].I;
	    reg[R14_SVC].I  = reg[14].I;
	    reg[SPSR_SVC].I =  reg[17].I;
	    break;
	  case 0x17:
	    reg[R13_ABT].I  = reg[13].I;
	    reg[R14_ABT].I  = reg[14].I;
	    reg[SPSR_ABT].I =  reg[17].I;
	    break;
	  case 0x1b:
	    reg[R13_UND].I  = reg[13].I;
	    reg[R14_UND].I  = reg[14].I;
	    reg[SPSR_UND].I =  reg[17].I;
	    break;
	  }

	  u32 CPSR = reg[16].I;
	  u32 SPSR = reg[17].I;

	  switch(mode) {
	  case 0x10:
	  case 0x1F:
	    reg[13].I = reg[R13_USR].I;
	    reg[14].I = reg[R14_USR].I;
	    reg[16].I = SPSR;
	    break;
	  case 0x11:
	    CPUSwap(&reg[8].I, &reg[R8_FIQ].I);
	    CPUSwap(&reg[9].I, &reg[R9_FIQ].I);
	    CPUSwap(&reg[10].I, &reg[R10_FIQ].I);
	    CPUSwap(&reg[11].I, &reg[R11_FIQ].I);
	    CPUSwap(&reg[12].I, &reg[R12_FIQ].I);
	    reg[13].I = reg[R13_FIQ].I;
	    reg[14].I = reg[R14_FIQ].I;
	    if(saveState)
	      reg[17].I = CPSR;
	    else
	      reg[17].I = reg[SPSR_FIQ].I;
	    break;
	  case 0x12:
	    reg[13].I = reg[R13_IRQ].I;
	    reg[14].I = reg[R14_IRQ].I;
	    reg[16].I = SPSR;
	    if(saveState)
	      reg[17].I = CPSR;
	    else
	      reg[17].I = reg[SPSR_IRQ].I;
	    break;
	  case 0x13:
	    reg[13].I = reg[R13_SVC].I;
	    reg[14].I = reg[R14_SVC].I;
	    reg[16].I = SPSR;
	    if(saveState)
	      reg[17].I = CPSR;
	    else
	      reg[17].I = reg[SPSR_SVC].I;
	    break;
	  case 0x17:
	    reg[13].I = reg[R13_ABT].I;
	    reg[14].I = reg[R14_ABT].I;
	    reg[16].I = SPSR;
	    if(saveState)
	      reg[17].I = CPSR;
	    else
	      reg[17].I = reg[SPSR_ABT].I;
	    break;
	  case 0x1b:
	    reg[13].I = reg[R13_UND].I;
	    reg[14].I = reg[R14_UND].I;
	    reg[16].I = SPSR;
	    if(saveState)
	      reg[17].I = CPSR;
	    else
	      reg[17].I = reg[SPSR_UND].I;
	    break;
	  default:
	    systemMessage(MSG_UNSUPPORTED_ARM_MODE, N_("Unsupported ARM mode %02x"), mode);
	    break;
	  }
	  armMode = mode;
	  updateFlags(ioMem, breakLoop);
	  updateCPSR();
	}

	void switchMode(const GBAMem::IoMem &ioMem, int mode, bool saveState)
	{
	  switchMode(ioMem, mode, saveState, true);
	}

	u32 oldPC()
	{
		return armMode ? armNextPC - 4: armNextPC - 2;
	}
};

struct GBASys
{
#ifndef __clang__
	constexpr GBASys() { }
#endif
	bool intState = false;
	bool stopState = false;
	ARM7TDMI cpu {this};
	u8 biosProtected[4] {0};
	GBALCD lcd;
	GBATimers timers;
	GBADMA dma;
	GBAMem mem;
};

extern GBASys gGba;

u32 biosRead8(ARM7TDMI &cpu, u32 address);
u32 biosRead16(ARM7TDMI &cpu, u32 address);
u32 biosRead32(ARM7TDMI &cpu, u32 address);

u32 ioMemRead8(ARM7TDMI &cpu, u32 address);
u32 ioMemRead16(ARM7TDMI &cpu, u32 address);
u32 ioMemRead32(ARM7TDMI &cpu, u32 address);

u32 vramRead8(ARM7TDMI &cpu, u32 address);
u32 vramRead16(ARM7TDMI &cpu, u32 address);
u32 vramRead32(ARM7TDMI &cpu, u32 address);

u32 rtcRead16(ARM7TDMI &cpu, u32 address);

u32 eepromRead32(ARM7TDMI &cpu, u32 address);

u32 flashRead32(ARM7TDMI &cpu, u32 address);

static inline u32 CPUReadByteQuick(ARM7TDMI &cpu, u32 addr)
	{ return cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask]; }

static inline u32 CPUReadHalfWordQuick(ARM7TDMI &cpu, u32 addr)
	{ return READ16LE(((u16*)&cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask])); }

static inline u32 CPUReadMemoryQuick(ARM7TDMI &cpu, u32 addr)
	{ return READ32LE(((u32*)&cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask])); }

extern void (*cpuSaveGameFunc)(u32,u8);

#ifdef BKPT_SUPPORT
extern u8 freezeWorkRAM[0x40000];
extern u8 freezeInternalRAM[0x8000];
extern u8 freezeVRAM[0x18000];
extern u8 freezeOAM[0x400];
extern u8 freezePRAM[0x400];
extern bool debugger_last;
extern int  oldreg[18];
extern char oldbuffer[10];
#endif

extern bool CPUReadGSASnapshot(const char *);
extern bool CPUReadGSASPSnapshot(const char *);
extern bool CPUWriteGSASnapshot(const char *, const char *, const char *, const char *);
extern bool CPUWriteBatteryFile(const char *);
extern bool CPUReadBatteryFile(const char *);
extern bool CPUExportEepromFile(const char *);
extern bool CPUImportEepromFile(const char *);
extern bool CPUWritePNGFile(const char *);
extern bool CPUWriteBMPFile(const char *);
extern void CPUCleanUp();
extern void CPUUpdateRender(GBASys &gba);
extern bool CPUReadMemState(GBASys &gba, char *, int);
extern bool CPUReadState(GBASys &gba, const char *);
extern bool CPUWriteMemState(GBASys &gba, char *, int);
extern bool CPUWriteState(GBASys &gba, const char *);
extern int CPULoadRom(GBASys &gba, const char *);
extern void doMirroring(GBASys &gba, bool);
extern void CPUUpdateRegister(ARM7TDMI &cpu, u32, u16);
extern void applyTimer(ARM7TDMI &cpu);
extern void CPUInit(GBASys &gba, const char *,bool);
extern void CPUReset(GBASys &gba);
extern void CPULoop(int);
extern void CPUCheckDMA(GBASys &gba, ARM7TDMI &cpu, int,int);
extern bool CPUIsGBAImage(const char *);
extern bool CPUIsZipFile(const char *);
#ifdef PROFILING
#include "prof/prof.h"
extern void cpuProfil(profile_segment *seg);
extern void cpuEnableProfiling(int hz);
#endif

#if 0
extern struct EmulatedSystem GBASystem;
#endif

#include "Cheats.h"
#include "Globals.h"
#include "EEprom.h"
#include "Flash.h"

#endif // GBA_H
