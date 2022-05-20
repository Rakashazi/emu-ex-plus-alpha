#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <mednafen/mednafen.h>
#include <mednafen/pce_fast/pce.h>
#include <mednafen/pce_fast/vdc.h>

namespace EmuEx::Controls
{
extern const unsigned gamepadKeys;
}

namespace EmuEx
{

class EmuApp;

enum
{
	CFGKEY_SYSCARD_PATH = 275, CFGKEY_ARCADE_CARD = 276,
	CFGKEY_6_BTN_PAD = 277
};

void set6ButtonPadEnabled(EmuApp &, bool);
bool hasHuCardExtension(std::string_view name);

class PceSystem final: public EmuSystem
{
public:
	std::array<uint16, 5> inputBuff{}; // 5 gamepad buffers
	static constexpr int vidBufferX = 512, vidBufferY = 242;
	alignas(8) uint32_t pixBuff[vidBufferX*vidBufferY]{};
	IG::Pixmap mSurfacePix{};
	bool prevUsing263Lines{};
	std::vector<CDInterface *> CDInterfaces;
	FS::PathString sysCardPath{};
	Byte1Option optionArcadeCard{CFGKEY_ARCADE_CARD, 1};
	Byte1Option option6BtnPad{CFGKEY_6_BTN_PAD, 0};

	PceSystem(ApplicationContext ctx):
		EmuSystem{ctx} {}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, IO &io, unsigned key, size_t readSize);
	void writeConfig(ConfigType, IO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	unsigned translateInputAction(unsigned input, bool &turbo);
	VController::Map vControllerMap(int player);
	void configAudioRate(FloatSeconds frameTime, int rate);

	// optional API functions
	void closeSystem();
	void onFlushBackupMemory(BackupMemoryDirtyFlags);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
	WP multiresVideoBaseSize() const;
	void onSessionOptionsLoaded(EmuApp &);
	bool resetSessionOptions(EmuApp &);
};

using MainSystem = PceSystem;

}

namespace MDFN_IEN_PCE_FAST
{
void applyVideoFormat(Mednafen::EmulateSpecStruct *);
void applySoundFormat(Mednafen::EmulateSpecStruct *);
extern vce_t vce;
}

extern Mednafen::MDFNGI EmulatedPCE_Fast;
static Mednafen::MDFNGI *emuSys = &EmulatedPCE_Fast;
