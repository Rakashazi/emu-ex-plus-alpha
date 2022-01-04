#pragma once

#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <imagine/base/ApplicationContext.hh>

extern "C"
{
	#include <blueMSX/Board/Board.h>
}

struct Mixer;

extern IG::FS::FileString hdName[4];
extern Machine *machine;

bool zipStartWrite(const char *fileName);
void zipEndWrite();
IG::Pixmap frameBufferPixmap();
HdType boardGetHdType(int hdIndex);

namespace EmuEx
{

class EmuApp;

extern Byte1Option optionSkipFdcAccess;
extern IG::StaticString<128> optionDefaultMachineNameStr;
extern IG::StaticString<128> optionSessionMachineNameStr;
extern FS::FileString cartName[2];
extern FS::FileString diskName[2];
extern unsigned activeBoardType;
extern BoardInfo boardInfo;
extern bool fdcActive;
extern Mixer *mixer;
extern IG::ApplicationContext appCtx;

void installFirmwareFiles(IG::ApplicationContext);
FS::PathString makeMachineBasePath(IG::ApplicationContext, FS::PathString customPath);
bool hasMSXTapeExtension(std::string_view name);
bool hasMSXDiskExtension(std::string_view name);
bool hasMSXROMExtension(std::string_view name);
bool insertROM(EmuApp &, const char *name, unsigned slot = 0);
bool insertDisk(EmuApp &, const char *name, unsigned slot = 0);
FS::PathString machineBasePath(IG::ApplicationContext app);
void setupVKeyboardMap(EmuApp &, unsigned boardType);
bool setDefaultMachineName(const char *name);
const char *currentMachineName();
void setCurrentMachineName(EmuApp &, std::string_view machineName, bool insertMediaFiles = true);
bool mixerEnableOption(MixerAudioType type);
void setMixerEnableOption(MixerAudioType type, bool on);
uint8_t mixerVolumeOption(MixerAudioType type);
uint8_t setMixerVolumeOption(MixerAudioType type, int volume);
uint8_t mixerPanOption(MixerAudioType type);
uint8_t setMixerPanOption(MixerAudioType type, int pan);

}
