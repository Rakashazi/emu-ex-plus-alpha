#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>

namespace EmuEx
{

enum
{
	CFGKEY_NGPKEY_LANGUAGE = 269,
};

class NgpSystem final: public EmuSystem
{
public:
	Byte1Option optionNGPLanguage{CFGKEY_NGPKEY_LANGUAGE, 1};
	uint8_t inputBuff{};
	IG::MutablePixmapView mSurfacePix{};
	static constexpr int vidBufferX = 160, vidBufferY = 152;
	alignas(8) uint32_t pixBuff[vidBufferX*vidBufferY]{};

	NgpSystem(ApplicationContext ctx):
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
};

using MainSystem = NgpSystem;

}

namespace MDFN_IEN_NGP
{
void applyVideoFormat(Mednafen::EmulateSpecStruct *);
void applySoundFormat(Mednafen::EmulateSpecStruct *);
}

extern Mednafen::MDFNGI EmulatedNGP;
static Mednafen::MDFNGI *emuSys = &EmulatedNGP;
