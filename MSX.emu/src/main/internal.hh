#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/pixmap/Pixmap.hh>

extern "C"
{
	#include <blueMSX/Board/Board.h>
}

class EmuApp;
struct Mixer;

extern bool canInstallCBIOS;
extern Byte1Option optionSkipFdcAccess;
extern PathOption optionFirmwarePath;
extern FS::PathString machineCustomPath;
extern FS::PathString machineBasePath;
extern PathOption optionDefaultMachineName;
extern PathOption optionMachineName;
extern FS::FileString hdName[4];
extern FS::FileString cartName[2];
extern FS::FileString diskName[2];
extern unsigned activeBoardType;
extern BoardInfo boardInfo;
extern bool fdcActive;
extern Mixer *mixer;

static const char *installFirmwareFilesMessage =
	#if defined CONFIG_BASE_ANDROID
	"Install the C-BIOS BlueMSX machine files to your storage device?";
	#elif defined CONFIG_ENV_WEBOS
	"Install the C-BIOS BlueMSX machine files to internal storage? If using WebOS 1.4.5, make sure you have a version without the write permission bug.";
	#elif defined CONFIG_BASE_IOS
	"Install the C-BIOS BlueMSX machine files to /User/Media/MSX.emu?";
	#else
	"Install the C-BIOS BlueMSX machine files to Machines directory?";
	#endif

void installFirmwareFiles(Base::ApplicationContext);
HdType boardGetHdType(int hdIndex);
FS::PathString makeMachineBasePath(Base::ApplicationContext, FS::PathString customPath);
bool hasMSXTapeExtension(const char *name);
bool hasMSXDiskExtension(const char *name);
bool hasMSXROMExtension(const char *name);
bool insertROM(EmuApp &, const char *name, unsigned slot = 0);
bool insertDisk(EmuApp &, const char *name, unsigned slot = 0);
bool zipStartWrite(const char *fileName);
void zipEndWrite();
const char *machineBasePathStr();
void setupVKeyboardMap(EmuApp &, unsigned boardType);
IG::Pixmap frameBufferPixmap();
bool setDefaultMachineName(const char *name);
const char *currentMachineName();
EmuSystem::Error setCurrentMachineName(EmuApp &, const char *machineName, bool insertMediaFiles = true);
bool mixerEnableOption(MixerAudioType type);
void setMixerEnableOption(MixerAudioType type, bool on);
uint8_t mixerVolumeOption(MixerAudioType type);
uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
uint8_t mixerPanOption(MixerAudioType type);
uint8_t setMixerPanOption(MixerAudioType type, int pan);
