/*****************************************************************************\
     Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#ifndef _SNES9X_H_
#define _SNES9X_H_

#ifndef VERSION
#define VERSION	"1.63"
#endif

#include "port.h"
#include "65c816.h"
#include "messages.h"

#ifdef ZLIB
#include <zlib.h>
#define FSTREAM					gzFile
#define READ_FSTREAM(p, l, s)	gzread(s, p, l)
#define WRITE_FSTREAM(p, l, s)	gzwrite(s, p, l)
#define GETS_FSTREAM(p, l, s)	gzgets(s, p, l)
#define GETC_FSTREAM(s)			gzgetc(s)
#define OPEN_FSTREAM(f, m)		gzopenHelper(f, m)
#define REOPEN_FSTREAM(f, m)		gzdopen(f, m)
#define FIND_FSTREAM(f)			gztell(f)
#define REVERT_FSTREAM(s, o, p)	gzseek(s, o, p)
#define CLOSE_FSTREAM(s)			gzclose(s)
gzFile gzopenHelper(const char *filename, const char *mode);
#else
#define FSTREAM					FILE *
#define READ_FSTREAM(p, l, s)	fread(p, 1, l, s)
#define WRITE_FSTREAM(p, l, s)	fwrite(p, 1, l, s)
#define GETS_FSTREAM(p, l, s)	fgets(p, l, s)
#define GETC_FSTREAM(s)			fgetc(s)
#define OPEN_FSTREAM(f, m)		fopenHelper(f, m)
#define REOPEN_FSTREAM(f, m)		fdopen(f, m)
#define FIND_FSTREAM(s)			ftell(s)
#define REVERT_FSTREAM(s, o, p)	fseek(s, o, p)
#define CLOSE_FSTREAM(s)			fclose(s)
#endif

FILE *fopenHelper(const char *filename, const char *mode);
void removeFileHelper(const char *filename);

#include "stream.h"

#define STREAM					Stream *
#define READ_STREAM(p, l, s)	s->read(p,l)
#define WRITE_STREAM(p, l, s)	s->write(p,l)
#define GETS_STREAM(p, l, s)	s->gets(p,l)
#define GETC_STREAM(s)			s->get_char()
#define OPEN_STREAM(f, m)		openStreamFromFSTREAM(f, m)
#define REOPEN_STREAM(f, m)		reopenStreamFromFd(f, m)
#define FIND_STREAM(s)			s->pos()
#define REVERT_STREAM(s, o, p)	s->revert(p, o)
#define CLOSE_STREAM(s)			s->closeStream()

#define SNES_WIDTH					256
#define SNES_HEIGHT					224
#define SNES_HEIGHT_EXTENDED		239
#define MAX_SNES_WIDTH				(SNES_WIDTH * 2)
#define MAX_SNES_HEIGHT				(SNES_HEIGHT_EXTENDED * 2)

#define	NTSC_MASTER_CLOCK			21477272.727272 // 21477272 + 8/11 exact
#define	PAL_MASTER_CLOCK			21281370.0
#define NTSC_PROGRESSIVE_FRAME_RATE	60.09881389744051
#define NTSC_INTERLACED_FRAME_RATE	59.94005994
#define PAL_PROGRESSIVE_FRAME_RATE	50.006977968


#define SNES_MAX_NTSC_VCOUNTER		262
#define SNES_MAX_PAL_VCOUNTER		312
#define SNES_HCOUNTER_MAX			341

#ifndef ALLOW_CPU_OVERCLOCK
#define ONE_CYCLE					6
#define SLOW_ONE_CYCLE				8
#define TWO_CYCLES					12
#else
#define ONE_CYCLE      (Settings.OneClockCycle)
#define SLOW_ONE_CYCLE (Settings.OneSlowClockCycle)
#define TWO_CYCLES     (Settings.TwoClockCycles)
#endif
#define	ONE_DOT_CYCLE				4

#define SNES_CYCLES_PER_SCANLINE	(SNES_HCOUNTER_MAX * ONE_DOT_CYCLE)
#define SNES_SCANLINE_TIME			(SNES_CYCLES_PER_SCANLINE / NTSC_MASTER_CLOCK)

#define SNES_WRAM_REFRESH_HC_v1		530
#define SNES_WRAM_REFRESH_HC_v2		538
#define SNES_WRAM_REFRESH_CYCLES	40

#define SNES_HBLANK_START_HC		1096					// H=274
#define	SNES_HDMA_START_HC			1106					// FIXME: not true
#define	SNES_HBLANK_END_HC			4						// H=1
#define	SNES_HDMA_INIT_HC			20						// FIXME: not true
#define	SNES_RENDER_START_HC		(128 * ONE_DOT_CYCLE)	// FIXME: Snes9x renders a line at a time.

#define SNES_TR_MASK		(1 <<  4)
#define SNES_TL_MASK		(1 <<  5)
#define SNES_X_MASK			(1 <<  6)
#define SNES_A_MASK			(1 <<  7)
#define SNES_RIGHT_MASK		(1 <<  8)
#define SNES_LEFT_MASK		(1 <<  9)
#define SNES_DOWN_MASK		(1 << 10)
#define SNES_UP_MASK		(1 << 11)
#define SNES_START_MASK		(1 << 12)
#define SNES_SELECT_MASK	(1 << 13)
#define SNES_Y_MASK			(1 << 14)
#define SNES_B_MASK			(1 << 15)

#define DEBUG_MODE_FLAG		(1 <<  0)	// debugger
#define TRACE_FLAG			(1 <<  1)	// debugger
#define SINGLE_STEP_FLAG	(1 <<  2)	// debugger
#define BREAK_FLAG			(1 <<  3)	// debugger
#define SCAN_KEYS_FLAG		(1 <<  4)	// CPU
#define HALTED_FLAG			(1 << 12)	// APU
#define FRAME_ADVANCE_FLAG	(1 <<  9)

#define ROM_NAME_LEN	23
#define AUTO_FRAMERATE	200

struct SCPUState
{
	uint32	Flags;
	int32	Cycles;
	int32	PrevCycles;
	int32	V_Counter;
	uint8	*PCBase;
	bool8	NMIPending;
	bool8	IRQLine;
	bool8	IRQTransition;
	bool8	IRQLastState;
	bool8	IRQExternal;
	int32	IRQPending;
	int32	MemSpeed;
	int32	MemSpeedx2;
	int32	FastROMSpeed;
	bool8	InDMA;
	bool8	InHDMA;
	bool8	InDMAorHDMA;
	bool8	InWRAMDMAorHDMA;
	uint8	HDMARanInDMA;
	int32	CurrentDMAorHDMAChannel;
	uint8	WhichEvent;
	int32	NextEvent;
	bool8	WaitingForInterrupt;
	void (*exec)();
};

enum
{
	HC_HBLANK_START_EVENT = 1,
	HC_HDMA_START_EVENT   = 2,
	HC_HCOUNTER_MAX_EVENT = 3,
	HC_HDMA_INIT_EVENT    = 4,
	HC_RENDER_EVENT       = 5,
	HC_WRAM_REFRESH_EVENT = 6
};

enum
{
	IRQ_NONE        = 0x0,
	IRQ_SET_FLAG    = 0x1,
	IRQ_CLEAR_FLAG  = 0x2,
	IRQ_TRIGGER_NMI = 0x4
};

struct STimings
{
	int32	H_Max_Master;
	int32	H_Max;
	int32	V_Max_Master;
	int32	V_Max;
	int32	HBlankStart;
	int32	HBlankEnd;
	int32	HDMAInit;
	int32	HDMAStart;
	int32	NMITriggerPos;
	int32	NextIRQTimer;
	int32	IRQTriggerCycles;
	int32	WRAMRefreshPos;
	int32	RenderPos;
	bool8	InterlaceField;
	int32	DMACPUSync;		// The cycles to synchronize DMA and CPU. Snes9x cannot emulate correctly.
	int32	NMIDMADelay;	// The delay of NMI trigger after DMA transfers. Snes9x cannot emulate correctly.
	int32	IRQFlagChanging;	// This value is just a hack.
	int32	APUSpeedup;
	float SuperFX2SpeedMultiplier;
	bool8	APUAllowTimeOverflow;
};

struct SSettings
{
#ifdef DEBUGGER
	bool8	TraceDMA;
	bool8	TraceHDMA;
	bool8	TraceVRAM;
	bool8	TraceUnknownRegisters;
	bool8	TraceDSP;
	bool8	TraceHCEvent;
	bool8	TraceSMP;
#endif

	bool8	SuperFX = 0;
	uint8	DSP = 0;
	bool8	SA1 = 0;
	bool8	C4 = 0;
	bool8	SDD1 = 0;
	bool8	SPC7110 = 0;
	bool8	SPC7110RTC = 0;
	bool8	OBC1 = 0;
	uint8	SETA = 0;
	bool8	SRTC = 0;
	bool8	BS = 0;
	bool8	BSXItself = 0;
	bool8	BSXBootup = 0;
	bool8	MSU1 = 0;
	bool8	MouseMaster = 1;
	bool8	SuperScopeMaster = 1;
	bool8	JustifierMaster = 1;
	bool8	MultiPlayer5Master = 1;
	bool8	MacsRifleMaster = 1;
	
	bool8	ForceLoROM = 0;
	bool8	ForceHiROM = 0;
	bool8	ForceHeader = 0;
	bool8	ForceNoHeader = 0;
	bool8	ForceInterleaved = 0;
	bool8	ForceInterleaved2 = 0;
	bool8	ForceInterleaveGD24 = 0;
	bool8	ForceNotInterleaved = 0;
	bool8	ForcePAL = 0;
	bool8	ForceNTSC = 0;
	bool8	PAL = 0;
	bool8 IdentifyAsPAL = 0;
	uint32	FrameTimePAL = 20000;
	uint32	FrameTimeNTSC = 16667;
	uint32	FrameTime = 0;

	static const bool8	SoundSync = 0;
	static const bool8	SixteenBitSound = 1;
	int	SoundPlaybackRate = 44100;
	double SoundInputRate = 32040.;
	static const bool8	Stereo = 1;
	static const bool8	ReverseStereo = 0;
	bool8	Mute = 0;
	static const bool8	DynamicRateControl = 0;
	static const int32	DynamicRateLimit = 5; /* Multiplied by 1000 */
	static const int32	InterpolationMethod = 2;

	static const bool8	Transparency = 1;
	uint8	BG_Forced = 0;
	static const bool8	DisableGraphicWindows = 0;
	uint16  ForcedBackdrop = 0;

	static const bool8	DisplayTime = 0;
	static const bool8	DisplayFrameRate = 0;
	static const bool8	DisplayWatchedAddresses = 0;
	static const bool8	DisplayPressedKeys = 0;
	static const bool8	DisplayMovieFrame = 0;
	static const bool	DisplayIndicators = 0;
	static const bool8	AutoDisplayMessages = 0;
	static const uint32	InitialInfoStringTimeout = 0;
	static uint16	DisplayColor;
	static const bool8	BilinearFilter = 0;
	static const bool	ShowOverscan = 0;

	static const bool8	Multi = 0;
	char	CartAName[PATH_MAX + 1]{};
	char	CartBName[PATH_MAX + 1]{};

	static const bool8	DisableGameSpecificHacks = 0;
	static const bool8	BlockInvalidVRAMAccessMaster = 1;
	static const bool8	BlockInvalidVRAMAccess = 0;
	int32	HDMATimingHack = 100;

	static const bool8	ForcedPause = 0;
	static const bool8	Paused = 0;
	static bool8	StopEmulation;

	static uint32	SkipFrames;
	static uint32	TurboSkipFrames;
	static const uint32	AutoMaxSkipFrames = 0;
	static const bool8	TurboMode = 0;
	static const uint32	HighSpeedSeek = 0;
	static const bool8	FrameAdvance = 0;
	static const bool8	Rewinding = 0;

	static const bool8	NetPlay = 0;
	static const bool8	NetPlayServer = 0;
	static constexpr char	ServerName[128]{};
	static const int		Port = 0;

	bool8	MovieTruncate = 0;
	bool8	MovieNotifyIgnored = 0;
	bool8	WrongMovieStateProtection = 1;
	static const bool8	DumpStreams = 0;
	static const int		DumpStreamsMaxFrames = 0;

	static const bool8	TakeScreenshot = 0;
	static const int8	StretchScreenshots = 0;
	static const bool8	SnapshotScreenshots = 0;
	static constexpr char    InitialSnapshotFilename[PATH_MAX + 1]{};
	static const bool8	FastSavestates = 0;

	static const bool8	ApplyCheats = 1;
	bool8	NoPatch = 0;
	bool8	IgnorePatchChecksum = 0;
	bool8	IsPatched = 0;
	static const int32	AutoSaveDelay = 30;
	static const bool8	DontSaveOopsSnapshot = 1;
	static const bool8	UpAndDown = 0;

  static const bool8  SeparateEchoBuffer = 0;
	uint32	SuperFXClockMultiplier = 100;
  static const int OverclockMode = 0;
	static const int	OneClockCycle = 6;
	static const int	OneSlowClockCycle = 8;
	static const int	TwoClockCycles = 12;
	static const int	MaxSpriteTilesPerLine = 34;
};

struct SSNESGameFixes
{
	uint8	SRAMInitialValue;
	uint8	Uniracers;
};

enum
{
	PAUSE_NETPLAY_CONNECT		= (1 << 0),
	PAUSE_TOGGLE_FULL_SCREEN	= (1 << 1),
	PAUSE_EXIT					= (1 << 2),
	PAUSE_MENU					= (1 << 3),
	PAUSE_INACTIVE_WINDOW		= (1 << 4),
	PAUSE_WINDOW_ICONISED		= (1 << 5),
	PAUSE_RESTORE_GUI			= (1 << 6),
	PAUSE_FREEZE_FILE			= (1 << 7)
};

void S9xSetPause(uint32);
void S9xClearPause(uint32);
void S9xExit(void);
void S9xMessage(int, int, const char *);
void S9xPrintf(const char* msg, ...) __attribute__ ((format (printf, 1, 2)));
void S9xPrintfError(const char* msg, ...) __attribute__ ((format (printf, 1, 2)));

extern struct SSettings			Settings;
extern struct SCPUState			CPU;
extern struct STimings			Timings;
extern struct SSNESGameFixes	SNESGameFixes;

#endif
