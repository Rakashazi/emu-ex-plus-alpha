#pragma once

#include <imagine/base/ApplicationContext.hh>
#include <emuframework/Option.hh>
#include <mednafen/mednafen.h>

extern const Mednafen::MDFNGI EmulatedNGP;

namespace EmuEx
{

enum
{
	CFGKEY_NGPKEY_LANGUAGE = 269,
};

class NgpSystem final: public EmuSystem
{
public:
	Mednafen::MDFNGI mdfnGameInfo{EmulatedNGP};
	Byte1Option optionNGPLanguage{CFGKEY_NGPKEY_LANGUAGE, 1};
	uint8_t inputBuff{};
	IG::MutablePixmapView mSurfacePix{};
	static constexpr int vidBufferX = 160, vidBufferY = 152;
	alignas(8) uint32_t pixBuff[vidBufferX*vidBufferY]{};

	NgpSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		Mednafen::MDFNGameInfo = &mdfnGameInfo;
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".mca"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	unsigned translateInputAction(unsigned input, bool &turbo);
	VController::Map vControllerMap(int player);
	void configAudioRate(FloatSeconds frameTime, int rate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	void loadBackupMemory(EmuApp &);
	void onFlushBackupMemory(EmuApp &, BackupMemoryDirtyFlags);
	bool onVideoRenderFormatChange(EmuVideo &, IG::PixelFormat);
};

using MainSystem = NgpSystem;

}

namespace Mednafen
{
class MDFN_PixelFormat;
}

namespace MDFN_IEN_NGP
{
void applyVideoFormat(Mednafen::MDFN_PixelFormat);
}
