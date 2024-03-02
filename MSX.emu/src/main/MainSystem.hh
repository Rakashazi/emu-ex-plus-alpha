#pragma once

/*  This file is part of MSX.emu.

	MSX.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	MSX.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with MSX.emu.  If not, see <http://www.gnu.org/licenses/> */

#include <emuframework/EmuSystem.hh>
#include <emuframework/EmuOptions.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/ArchiveFS.hh>

extern "C"
{
	#include <blueMSX/Board/Board.h>
	#include <blueMSX/IoDevice/Casette.h>
	#include <blueMSX/Language/Language.h>
}

enum
{
	CFGKEY_DEFAULT_MACHINE_NAME = 256, CFGKEY_SKIP_FDC_ACCESS = 257,
	CFGKEY_MACHINE_FILE_PATH = 258, CFGKEY_SESSION_MACHINE_NAME = 259,
	CFGKEY_MIXER_PSG_VOLUME = 260, CFGKEY_MIXER_PSG_PAN = 261,
	CFGKEY_MIXER_SCC_VOLUME = 262, CFGKEY_MIXER_SCC_PAN = 263,
	CFGKEY_MIXER_MSX_MUSIC_VOLUME = 264, CFGKEY_MIXER_MSX_MUSIC_PAN = 265,
	CFGKEY_MIXER_MSX_AUDIO_VOLUME = 266, CFGKEY_MIXER_MSX_AUDIO_PAN = 267,
	CFGKEY_MIXER_MOON_SOUND_VOLUME = 268, CFGKEY_MIXER_MOON_SOUND_PAN = 269,
	CFGKEY_MIXER_YAMAHA_SFG_VOLUME = 270, CFGKEY_MIXER_YAMAHA_SFG_PAN = 271,
	CFGKEY_MIXER_KEYBOARD_VOLUME = 272, CFGKEY_MIXER_KEYBOARD_PAN = 273,
	CFGKEY_MIXER_PCM_VOLUME = 274, CFGKEY_MIXER_PCM_PAN = 275,
	CFGKEY_MIXER_IO_VOLUME = 276, CFGKEY_MIXER_IO_PAN = 277,
	CFGKEY_MIXER_MIDI_VOLUME = 278, CFGKEY_MIXER_MIDI_PAN = 279,
	CFGKEY_DEFAULT_COLECO_MACHINE_NAME = 280
};

struct Mixer;

extern IG::FS::FileString hdName[4];
extern Machine *machine;

bool zipStartWrite(const char *fileName);
void zipEndWrite();
void setZipMemBuffer(std::span<uint8_t> buff);
size_t zipMemBufferSize();
IG::PixmapView frameBufferPixmap();
HdType boardGetHdType(int hdIndex);

namespace EmuEx
{

class EmuApp;

extern BoardInfo boardInfo;
extern bool fdcActive;
extern Mixer *mixer;
constexpr std::string_view optionMachineNameDefault{"MSX2"};
constexpr std::string_view optionColecoMachineNameDefault{"COL - ColecoVision"};

struct MixerFlags
{
	uint8_t volume:7{}, enable:1{true};
	constexpr bool operator==(const MixerFlags&) const = default;
};

constexpr bool volumeOptionIsValid(const auto &v)
{
	return v.volume <= 100;
}

constexpr bool panOptionIsValid(const auto &v)
{
	return v <= 100;
}

class MsxSystem final: public EmuSystem
{
public:
	unsigned activeBoardType = BOARD_MSX;
	FS::FileString cartName[2];
	FS::FileString diskName[2];
	std::string optionDefaultMachineNameStr{optionMachineNameDefault};
	std::string optionDefaultColecoMachineNameStr{optionColecoMachineNameDefault};
	std::string optionSessionMachineNameStr;
	FS::PathString firmwarePath_;
	mutable ArchiveIO firmwareArch;
	Property<bool, CFGKEY_SKIP_FDC_ACCESS,
		PropertyDesc<bool>{.defaultValue = true}> optionSkipFdcAccess;

	Property<MixerFlags, CFGKEY_MIXER_PSG_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {100}, .isValid = volumeOptionIsValid}> optionMixerPSGVolume;
	Property<MixerFlags, CFGKEY_MIXER_SCC_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {100}, .isValid = volumeOptionIsValid}> optionMixerSCCVolume;
	Property<MixerFlags, CFGKEY_MIXER_MSX_MUSIC_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {80}, .isValid = volumeOptionIsValid}> optionMixerMSXMUSICVolume;
	Property<MixerFlags, CFGKEY_MIXER_MSX_AUDIO_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {80}, .isValid = volumeOptionIsValid}> optionMixerMSXAUDIOVolume;
	Property<MixerFlags, CFGKEY_MIXER_MOON_SOUND_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {80}, .isValid = volumeOptionIsValid}> optionMixerMoonSoundVolume;
	Property<MixerFlags, CFGKEY_MIXER_YAMAHA_SFG_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {80}, .isValid = volumeOptionIsValid}> optionMixerYamahaSFGVolume;
	Property<MixerFlags, CFGKEY_MIXER_PCM_VOLUME,
		PropertyDesc<MixerFlags>{.defaultValue = {100}, .isValid = volumeOptionIsValid}> optionMixerPCMVolume;

	Property<uint8_t, CFGKEY_MIXER_PSG_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerPSGPan;
	Property<uint8_t, CFGKEY_MIXER_SCC_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerSCCPan;
	Property<uint8_t, CFGKEY_MIXER_MSX_MUSIC_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerMSXMUSICPan;
	Property<uint8_t, CFGKEY_MIXER_MSX_AUDIO_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerMSXAUDIOPan;
	Property<uint8_t, CFGKEY_MIXER_MOON_SOUND_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerMoonSoundPan;
	Property<uint8_t, CFGKEY_MIXER_YAMAHA_SFG_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerYamahaSFGPan;
	Property<uint8_t, CFGKEY_MIXER_PCM_PAN,
		PropertyDesc<uint8_t>{.defaultValue = 50, .isValid = panOptionIsValid}> optionMixerPCMPan;

	MsxSystem(ApplicationContext ctx):
		EmuSystem{ctx}
	{
		/*mediaDbCreateRomdb();
		mediaDbAddFromXmlFile("msxromdb.xml");
		mediaDbAddFromXmlFile("msxsysromdb.xml");*/

		// must create the mixer first since mainInitCommon() will access it
		mixer = mixerCreate();
		assert(mixer);

		// Init general emu
		langInit();
		videoManagerReset();
		tapeSetReadOnly(1);
		mediaDbSetDefaultRomType(ROM_UNKNOWN);

		// Init Mixer
		mixerSetMasterVolume(mixer, 100);
		mixerSetStereo(mixer, 1);
		mixerEnableMaster(mixer, 1);
		int logFrequency = 50;
		int frequency = (int)(3579545 * ::pow(2.0, (logFrequency - 50) / 15.0515));
		mixerSetBoardFrequencyFixed(frequency);
	}
	bool setDefaultMachineName(std::string_view name);
	bool setDefaultColecoMachineName(std::string_view name);
	void setCurrentMachineName(EmuApp &, std::string_view machineName, bool insertMediaFiles = true);
	void clearAllMediaNames();
	bool createBoard(EmuApp &app);
	void destroyBoard(bool clearMediaNames = true);
	bool createBoardFromLoadGame(EmuApp &app);
	void destroyMachine(bool clearMediaNames = true);
	void setFirmwarePath(CStringView path, FS::file_type);
	FS::PathString firmwarePath() const;
	ArchiveIO &firmwareArchive(CStringView path) const;
	bool mixerEnableOption(MixerAudioType type);
	void setMixerEnableOption(MixerAudioType type, bool on);
	uint8_t mixerVolumeOption(MixerAudioType type);
	uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
	uint8_t mixerPanOption(MixerAudioType type);
	uint8_t setMixerPanOption(MixerAudioType type, int pan);

	auto optionMixerVolume(MixerAudioType type, auto &&func)
	{
		switch(type)
		{
			default: [[fallthrough]];
			case MIXER_CHANNEL_PSG: return func(optionMixerPSGVolume);
			case MIXER_CHANNEL_SCC: return func(optionMixerSCCVolume);
			case MIXER_CHANNEL_MSXMUSIC: return func(optionMixerMSXMUSICVolume);
			case MIXER_CHANNEL_MSXAUDIO: return func(optionMixerMSXAUDIOVolume);
			case MIXER_CHANNEL_MOONSOUND: return func(optionMixerMoonSoundVolume);
			case MIXER_CHANNEL_YAMAHA_SFG: return func(optionMixerYamahaSFGVolume);
			case MIXER_CHANNEL_PCM: return func(optionMixerPCMVolume);
		}
	}

	auto optionMixerPan(MixerAudioType type, auto &&func)
	{
		switch(type)
		{
			default: [[fallthrough]];
			case MIXER_CHANNEL_PSG: return func(optionMixerPSGPan);
			case MIXER_CHANNEL_SCC: return func(optionMixerSCCPan);
			case MIXER_CHANNEL_MSXMUSIC: return func(optionMixerMSXMUSICPan);
			case MIXER_CHANNEL_MSXAUDIO: return func(optionMixerMSXAUDIOPan);
			case MIXER_CHANNEL_MOONSOUND: return func(optionMixerMoonSoundPan);
			case MIXER_CHANNEL_YAMAHA_SFG: return func(optionMixerYamahaSFGPan);
			case MIXER_CHANNEL_PCM: return func(optionMixerPCMPan);
		}
	}

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	size_t stateSize() { return 0x200000; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key);
	void writeConfig(ConfigType, FileIO &);
	void reset(EmuApp &, ResetMode mode);
	void clearInputBuffers(EmuInputView &view);
	void handleInputAction(EmuApp *, InputAction);
	SystemInputDeviceDesc inputDeviceDesc(int idx) const;
	FrameTime frameTime() const { return fromHz<FrameTime>(59.924); }
	void configAudioRate(FrameTime outputFrameTime, int outputRate);
	static std::span<const AspectRatioInfo> aspectRatioInfos();

	// optional API functions
	void closeSystem();
	bool resetSessionOptions(EmuApp &);
	void renderFramebuffer(EmuVideo &);
	void onOptionsLoaded();
	bool shouldFastForward() const;
	VController::KbMap vControllerKeyboardMap(VControllerKbMode mode);

private:
	void insertMedia(EmuApp &app);
	void saveBlueMSXState(const char *filename);
	void loadBlueMSXState(EmuApp &app, const char *filename);
};

using MainSystem = MsxSystem;

bool hasMSXTapeExtension(std::string_view name);
bool hasMSXDiskExtension(std::string_view name);
bool hasMSXROMExtension(std::string_view name);
bool insertROM(EmuApp &, const char *name, unsigned slot = 0);
bool insertDisk(EmuApp &, const char *name, unsigned slot = 0);
void setupVKeyboardMap(EmuApp &, unsigned boardType);
const char *currentMachineName();

}
