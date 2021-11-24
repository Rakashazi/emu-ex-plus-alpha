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

extern Byte1Option optionSkipFdcAccess;
extern IG::StaticString<128> optionDefaultMachineNameStr;
extern IG::StaticString<128> optionSessionMachineNameStr;
extern FS::FileString hdName[4];
extern FS::FileString cartName[2];
extern FS::FileString diskName[2];
extern unsigned activeBoardType;
extern BoardInfo boardInfo;
extern bool fdcActive;
extern Mixer *mixer;
extern Base::ApplicationContext appCtx;

void installFirmwareFiles(Base::ApplicationContext);
HdType boardGetHdType(int hdIndex);
FS::PathString makeMachineBasePath(Base::ApplicationContext, FS::PathString customPath);
bool hasMSXTapeExtension(std::string_view name);
bool hasMSXDiskExtension(std::string_view name);
bool hasMSXROMExtension(std::string_view name);
bool insertROM(EmuApp &, const char *name, unsigned slot = 0);
bool insertDisk(EmuApp &, const char *name, unsigned slot = 0);
bool zipStartWrite(const char *fileName);
void zipEndWrite();
FS::PathString machineBasePath(Base::ApplicationContext app);
void setupVKeyboardMap(EmuApp &, unsigned boardType);
IG::Pixmap frameBufferPixmap();
bool setDefaultMachineName(const char *name);
const char *currentMachineName();
void setCurrentMachineName(EmuApp &, std::string_view machineName, bool insertMediaFiles = true);
bool mixerEnableOption(MixerAudioType type);
void setMixerEnableOption(MixerAudioType type, bool on);
uint8_t mixerVolumeOption(MixerAudioType type);
uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
uint8_t mixerPanOption(MixerAudioType type);
uint8_t setMixerPanOption(MixerAudioType type, int pan);
