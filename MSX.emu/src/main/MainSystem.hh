#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/base/ApplicationContext.hh>

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

class MsxSystem final: public EmuSystem
{
public:
	unsigned activeBoardType = BOARD_MSX;
	FS::FileString cartName[2];
	FS::FileString diskName[2];
	IG::StaticString<128> optionDefaultMachineNameStr{optionMachineNameDefault};
	IG::StaticString<128> optionSessionMachineNameStr;
	FS::PathString firmwarePath;

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
	void setCurrentMachineName(EmuApp &, std::string_view machineName, bool insertMediaFiles = true);
	void clearAllMediaNames();
	bool createBoard(EmuApp &app);
	void destroyBoard(bool clearMediaNames = true);
	bool createBoardFromLoadGame(EmuApp &app);
	void destroyMachine(bool clearMediaNames = true);

	// required API functions
	void loadContent(IO &, EmuSystemCreateParams, OnLoadProgressDelegate);
	[[gnu::hot]] void runFrame(EmuSystemTaskContext task, EmuVideo *video, EmuAudio *audio);
	FS::FileString stateFilename(int slot, std::string_view name) const;
	std::string_view stateFilenameExt() const { return ".sta"; }
	void loadState(EmuApp &, CStringView uri);
	void saveState(CStringView path);
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

FS::PathString makeMachineBasePath(IG::ApplicationContext, FS::PathString customPath);
bool hasMSXTapeExtension(std::string_view name);
bool hasMSXDiskExtension(std::string_view name);
bool hasMSXROMExtension(std::string_view name);
bool insertROM(EmuApp &, const char *name, unsigned slot = 0);
bool insertDisk(EmuApp &, const char *name, unsigned slot = 0);
FS::PathString machineBasePath(MsxSystem &);
void setupVKeyboardMap(EmuApp &, unsigned boardType);
const char *currentMachineName();
bool mixerEnableOption(MixerAudioType type);
void setMixerEnableOption(MixerAudioType type, bool on);
uint8_t mixerVolumeOption(MixerAudioType type);
uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
uint8_t mixerPanOption(MixerAudioType type);
uint8_t setMixerPanOption(MixerAudioType type, int pan);

}
