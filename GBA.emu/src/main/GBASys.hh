#pragma once

#include <core/base/system.h>
#include <core/base/port.h>
#include <core/gba/gba.h>
#include <imagine/util/used.hh>
#include <imagine/util/utility.h>

using MixColorType = uint16_t;
struct GBALCD;

struct GBAMem
{
	union IoMem
	{
		alignas(4) uint8_t b[0x400];
		struct
		{
			uint16_t DISPCNT;
			uint16_t greenSwap;
			uint16_t DISPSTAT;
			uint16_t VCOUNT;
			uint16_t BG0CNT;
			uint16_t BG1CNT;
			uint16_t BG2CNT;
			uint16_t BG3CNT;
			uint16_t BG0HOFS;
			uint16_t BG0VOFS;
			uint16_t BG1HOFS;
			uint16_t BG1VOFS;
			uint16_t BG2HOFS;
			uint16_t BG2VOFS;
			uint16_t BG3HOFS;
			uint16_t BG3VOFS;
			uint16_t BG2PA;
			uint16_t BG2PB;
			uint16_t BG2PC;
			uint16_t BG2PD;
			uint16_t BG2X_L;
			uint16_t BG2X_H;
			uint16_t BG2Y_L;
			uint16_t BG2Y_H;
			uint16_t BG3PA;
			uint16_t BG3PB;
			uint16_t BG3PC;
			uint16_t BG3PD;
			uint16_t BG3X_L;
			uint16_t BG3X_H;
			uint16_t BG3Y_L;
			uint16_t BG3Y_H;
			uint16_t WIN0H;
			uint16_t WIN1H;
			uint16_t WIN0V;
			uint16_t WIN1V;
			uint16_t WININ;
			uint16_t WINOUT;
			uint16_t MOSAIC;
			uint16_t unused4e;
			uint16_t BLDMOD; // BLDCNT
			uint16_t COLEV; // BLDALPHA
			uint16_t COLY;
			// 0x56
			uint8_t unused56[0xA];
			// 0x60
			uint8_t soundRegs[0x50];
			// 0xB0
			uint16_t DM0SAD_L;
			uint16_t DM0SAD_H;
			uint16_t DM0DAD_L;
			uint16_t DM0DAD_H;
			uint16_t DM0CNT_L;
			uint16_t DM0CNT_H;
			uint16_t DM1SAD_L;
			uint16_t DM1SAD_H;
			uint16_t DM1DAD_L;
			uint16_t DM1DAD_H;
			uint16_t DM1CNT_L;
			uint16_t DM1CNT_H;
			uint16_t DM2SAD_L;
			uint16_t DM2SAD_H;
			uint16_t DM2DAD_L;
			uint16_t DM2DAD_H;
			uint16_t DM2CNT_L;
			uint16_t DM2CNT_H;
			uint16_t DM3SAD_L;
			uint16_t DM3SAD_H;
			uint16_t DM3DAD_L;
			uint16_t DM3DAD_H;
			uint16_t DM3CNT_L;
			uint16_t DM3CNT_H;
			// 0xE0
			uint8_t unusedE0[0x20];
			// 0x100
			uint16_t TM0D; // TM0CNT_L
			uint16_t TM0CNT; // TM0CNT_H
			uint16_t TM1D; // TM1CNT_L
			uint16_t TM1CNT; // TM1CNT_H
			uint16_t TM2D; // TM2CNT_L
			uint16_t TM2CNT; // TM2CNT_H
			uint16_t TM3D; // TM3CNT_L
			uint16_t TM3CNT; // TM3CNT_H
			// 0x110
			uint8_t unused110[0xF0];
			uint16_t IE;
			uint16_t IF;
			uint16_t WAITCNT;
			uint16_t unused206;
			uint16_t IME;
		};

		constexpr void resetDmaRegs()
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

		constexpr void resetLcdRegs(bool useBios, bool skipBios)
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

		constexpr auto& operator[] (this auto&& self, int idx) { return self.b[idx]; }
		constexpr operator uint8_t*() { return b; }
	};

	uint8_t bios[0x4000] __attribute__ ((aligned(4)));
	IoMem ioMem;
	uint8_t internalRAM[0x8000] __attribute__ ((aligned(4)));
	uint8_t workRAM[0x40000] __attribute__ ((aligned(4)));
	uint8_t rom[0x2000000] __attribute__ ((aligned(4)));
};

struct GBADMA
{
	int cpuDmaTicksToUpdate{};
	int cpuDmaCount{};
	bool cpuDmaRunning{};
	uint32_t cpuDmaLast{};
	uint32_t cpuDmaPC{};

	uint32_t dma0Source{};
	uint32_t dma0Dest{};
	uint32_t dma1Source{};
	uint32_t dma1Dest{};
	uint32_t dma2Source{};
	uint32_t dma2Dest{};
	uint32_t dma3Source{};
	uint32_t dma3Dest{};

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
	uint8_t timerOnOffDelay{};
	bool timer0On{};
	bool timer1On{};
	bool timer2On{};
	bool timer3On{};
	int timer0Ticks{};
	int timer0Reload{};
	int timer0ClockReload{};
	int timer1Ticks{};
	int timer1Reload{};
	int timer1ClockReload{};
	int timer2Ticks{};
	int timer2Reload{};
	int timer2ClockReload{};
	int timer3Ticks{};
	int timer3Reload{};
	int timer3ClockReload{};

	uint16_t timer0Value{};
	uint16_t timer1Value{};
	uint16_t timer2Value{};
	uint16_t timer3Value{};
};

void mode0RenderLine(MixColorType *, GBALCD &lcd, const GBAMem::IoMem &ioMem);

struct GBALCD
{
	uint32_t line0[240];
	uint32_t line1[240];
	uint32_t line2[240];
	uint32_t line3[240];
	uint32_t lineOBJ[240];
	uint32_t lineOBJWin[240];
	bool gfxInWin0[240];
	bool gfxInWin1[240];
	int lineOBJpixleft[128];
	alignas(8) uint16_t pix[240 * 160];
	alignas(4) uint8_t vram[0x20000];
	alignas(4) uint8_t paletteRAM[0x400];
	alignas(4) uint8_t oam[0x400];
	typedef void (*RenderLineFunc)(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem);
	RenderLineFunc renderLine{mode0RenderLine};
	bool fxOn{};
	bool windowOn{};
	MixColorType *lineMix{};
	unsigned layerEnable{};
	int gfxBG2Changed{};
	int gfxBG3Changed{};
	int gfxBG2X{};
	int gfxBG2Y{};
	int gfxBG3X{};
	int gfxBG3Y{};
	int layerEnableDelay{};
	int lcdTicks{};
	uint16_t gfxLastVCOUNT{};

	void registerRamReset(uint32_t flags)
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
		layerEnable = ioMem.DISPCNT & coreOptions.layerSettings;
	}
};

const char *dispModeName(GBALCD::RenderLineFunc);

struct ARM7TDMI;

static inline uint32_t CPUReadByteQuick(ARM7TDMI &cpu, uint32_t addr);

static inline uint32_t CPUReadHalfWordQuick(ARM7TDMI &cpu, uint32_t addr);

static inline uint32_t CPUReadMemoryQuick(ARM7TDMI &cpu, uint32_t addr);

constexpr unsigned cpuBitsSet[256] =
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

#ifndef __arm__
#define VBAM_USE_SWITICKS
#define VBAM_USE_IRQTICKS
#endif
#define VBAM_USE_CPU_PREFETCH
#define VBAM_USE_DELAYED_CPU_FLAGS

struct GBASys;

struct ARM7TDMI
{
	constexpr ARM7TDMI(GBASys *gba): gba(gba) {}

	#ifdef VBAM_USE_SWITICKS
	static constexpr bool USE_SWITICKS{true};
	#else
	static constexpr bool USE_SWITICKS{};
	#endif
	#ifdef VBAM_USE_IRQTICKS
	static constexpr bool USE_IRQTICKS{true};
	#else
	static constexpr bool USE_IRQTICKS{};
	#endif

	std::array<reg_pair, 45> reg{};
	uint32_t armNextPC{};
	int armMode{0x1f};
	int cpuNextEvent{};
	int cpuTotalTicks{};
	ConditionalMember<USE_SWITICKS, int> SWITicks{};
	ConditionalMember<USE_IRQTICKS, int> IRQTicks{};
#ifdef VBAM_USE_CPU_PREFETCH
private:
	uint32_t cpuPrefetch[2]{};
#endif
public:
	uint32_t busPrefetchCount{};
	/*uint16_t IE = 0;
	uint16_t IF = 0;
	uint16_t IME = 0;*/
#ifdef VBAM_USE_DELAYED_CPU_FLAGS
	uint32_t lastArithmeticRes{1};
#else
	bool N_FLAG{};
	bool Z_FLAG{};
#endif
	bool C_FLAG{};
	bool V_FLAG{};
	bool busPrefetch{};
	bool busPrefetchEnable{};
	bool armState{true};
	bool armIrqEnable{true};
	bool holdState{};
	//uint8_t cpuBitsSet[256];
	//uint8_t cpuLowestBitSet[256];
	GBASys *gba;
	unsigned memoryWait[16]
	  {0, 0, 2, 0, 0, 0, 0, 0, 4, 4, 4, 4, 4, 4, 4, 0};
	unsigned memoryWait32[16]
	  {0, 0, 5, 0, 0, 1, 1, 0, 7, 7, 9, 9, 13, 13, 4, 0};
	unsigned memoryWaitSeq[16]
	  {0, 0, 2, 0, 0, 0, 0, 0, 2, 2, 4, 4, 8, 8, 4, 0};
	unsigned memoryWaitSeq32[16] =
	  {0, 0, 5, 0, 0, 1, 1, 0, 5, 5, 9, 9, 17, 17, 4, 0};
	std::array<memoryMap, 256> map{};

	static constexpr bool calcNFlag(auto result)
	{
		return (int32_t)result < 0;
	}

	static constexpr bool calcZFlag(auto result)
	{
		return result == 0;
	}

	constexpr bool nFlag() const
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			return calcNFlag(lastArithmeticRes);
		#else
			return N_FLAG;
		#endif
	}

	constexpr bool zFlag() const
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			return calcZFlag(lastArithmeticRes);
		#else
			return Z_FLAG;
		#endif
	}

	constexpr void setNZFlag(bool n, bool z)
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

	constexpr void resetFlags()
	{
		setNZFlag(false, false);
		C_FLAG = false;
		V_FLAG = false;
	}

	constexpr void setNZFlagParam(auto res)
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			lastArithmeticRes = res;
		#else
			N_FLAG = calcNFlag(res);
			Z_FLAG = calcZFlag(res);
		#endif
	}

	constexpr void setNZFlagParam(auto low, auto high)
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			lastArithmeticRes = low;
			if(!low && high)
				lastArithmeticRes = 1;
		#else
			N_FLAG = low & 0x80000000;
			Z_FLAG = low || high;
		#endif
	}

	constexpr void updateNZFlags(bool N_FLAG_, bool Z_FLAG_)
	{
		#ifdef VBAM_USE_DELAYED_CPU_FLAGS
			setNZFlag(N_FLAG_, Z_FLAG_);
		#else
			N_FLAG = N_FLAG_;
			Z_FLAG = Z_FLAG_;
		#endif
	}

	void ARM_PREFETCH() __attribute__((always_inline))
  {
#ifdef VBAM_USE_CPU_PREFETCH
    cpuPrefetch[0] = CPUReadMemoryQuick(*this, armNextPC);
    cpuPrefetch[1] = CPUReadMemoryQuick(*this, armNextPC + 4);
#endif
  }

	void THUMB_PREFETCH() __attribute__((always_inline))
  {
#ifdef VBAM_USE_CPU_PREFETCH
    cpuPrefetch[0] = CPUReadHalfWordQuick(*this, armNextPC);
    cpuPrefetch[1] = CPUReadHalfWordQuick(*this, armNextPC + 2);
#endif
  }

	void ARM_PREFETCH_NEXT() __attribute__((always_inline))
	{
#ifdef VBAM_USE_CPU_PREFETCH
		cpuPrefetch[1] = CPUReadMemoryQuick(*this, armNextPC + 4);
#endif
	}

	void THUMB_PREFETCH_NEXT() __attribute__((always_inline))
	{
#ifdef VBAM_USE_CPU_PREFETCH
		cpuPrefetch[1] = CPUReadHalfWordQuick(*this, armNextPC + 2);
#endif
	}

	int prefetchArmOpcode() __attribute__((always_inline))
	{
#ifdef VBAM_USE_CPU_PREFETCH
		int opcode = cpuPrefetch[0];
		cpuPrefetch[0] = cpuPrefetch[1];
		return opcode;
#else
		return CPUReadMemoryQuick(armNextPC);
#endif
	}

	int prefetchThumbOpcode() __attribute__((always_inline))
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
		reg = {};

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

		// disable FIQ
		reg[16].I |= 0x40;

		updateCPSR();

		armNextPC = reg[15].I;
		reg[15].I += 4;

		holdState = 0;
		SWITicks = 0;
	}

	void updateCPSR();

	void updateFlags(const GBAMem::IoMem &ioMem, bool breakLoop = true);

	void undefinedException(const GBAMem::IoMem &ioMem);
	void undefinedException();

	void softwareInterrupt(const GBAMem::IoMem &ioMem);
	void softwareInterrupt();

	void interrupt(const GBAMem::IoMem &ioMem);

	void switchMode(const GBAMem::IoMem &ioMem, int mode, bool saveState, bool breakLoop);
	void switchMode(const GBAMem::IoMem &ioMem, int mode, bool saveState);

	uint32_t oldPC()
	{
		return armMode ? armNextPC - 4: armNextPC - 2;
	}
};

struct GBASys
{
	bool intState{};
	bool stopState{};
	ARM7TDMI cpu{this};
	uint8_t biosProtected[4]{};
	GBALCD lcd;
	GBATimers timers;
	GBADMA dma;
	GBAMem mem;
};

extern GBASys gGba;

uint32_t biosRead8(ARM7TDMI &cpu, uint32_t address);
uint32_t biosRead16(ARM7TDMI &cpu, uint32_t address);
uint32_t biosRead32(ARM7TDMI &cpu, uint32_t address);

uint32_t ioMemRead8(ARM7TDMI &cpu, uint32_t address);
uint32_t ioMemRead16(ARM7TDMI &cpu, uint32_t address);
uint32_t ioMemRead32(ARM7TDMI &cpu, uint32_t address);

uint32_t vramRead8(ARM7TDMI &cpu, uint32_t address);
uint32_t vramRead16(ARM7TDMI &cpu, uint32_t address);
uint32_t vramRead32(ARM7TDMI &cpu, uint32_t address);

uint32_t rtcRead16(ARM7TDMI &cpu, uint32_t address);

uint32_t eepromRead32(ARM7TDMI &cpu, uint32_t address);

uint32_t flashRead32(ARM7TDMI &cpu, uint32_t address);

static inline uint32_t CPUReadByteQuick(ARM7TDMI &cpu, uint32_t addr)
	{ return cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask]; }

static inline uint32_t CPUReadHalfWordQuick(ARM7TDMI &cpu, uint32_t addr)
	{ return READ16LE(((uint16_t*)&cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask])); }

static inline uint32_t CPUReadMemoryQuick(ARM7TDMI &cpu, uint32_t addr)
	{ return READ32LE(((uint32_t*)&cpu.map[addr>>24].address[addr & cpu.map[addr>>24].mask])); }

inline void blankLine(MixColorType *lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
	for (int x = 0; x < 240; x++)
		lineMix[x] = 0x7fff;
	lcd.gfxLastVCOUNT = ioMem.VCOUNT;
}
