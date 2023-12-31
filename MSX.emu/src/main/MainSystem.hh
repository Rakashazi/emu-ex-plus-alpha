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

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/base/ApplicationContext.hh>
#include <imagine/fs/ArchiveFS.hh>

extern "C"
{
	#include <blueMSX/Board/Board.h>
	#include <blueMSX/IoDevice/Casette.h>
	#include <blueMSX/Language/Language.h>
}

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

extern Byte1Option optionSkipFdcAccess;
extern BoardInfo boardInfo;
extern bool fdcActive;
extern Mixer *mixer;
constexpr std::string_view optionMachineNameDefault{"MSX2"};
constexpr std::string_view optionColecoMachineNameDefault{"COL - ColecoVision"};

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
	mutable FS::ArchiveIterator firmwareArchiveIt;

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
	FS::ArchiveIterator &firmwareArchiveIterator(CStringView path) const;

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	size_t stateSize() { return 0x200000; }
	void readState(EmuApp &, std::span<uint8_t> buff);
	size_t writeState(std::span<uint8_t> buff, SaveStateFlags = {});
	bool readConfig(ConfigType, MapIO &, unsigned key, size_t readSize);
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
bool mixerEnableOption(MixerAudioType type);
void setMixerEnableOption(MixerAudioType type, bool on);
uint8_t mixerVolumeOption(MixerAudioType type);
uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
uint8_t mixerPanOption(MixerAudioType type);
uint8_t setMixerPanOption(MixerAudioType type, int pan);

}
