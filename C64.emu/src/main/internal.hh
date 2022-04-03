#pragma once

#include "VicePlugin.hh"
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>
#include <vector>
#include <string>
#include <string_view>

namespace EmuEx
{

class EmuAudio;

enum Vic20Ram : uint8_t
{
	BLOCK_0 = 1,
	BLOCK_1 = 1 << 1,
	BLOCK_2 = 1 << 2,
	BLOCK_3 = 1 << 3,
	BLOCK_5 = 1 << 5
};

enum JoystickMode : uint8_t
{
	NORMAL = 0,
	SWAPPED = 1,
	KEYBOARD = 2,
};

static constexpr uint8_t SYSTEM_FLAG_NO_AUTOSTART = IG::bit(0);

extern VicePlugin plugin;
extern ViceSystem currSystem;
extern FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3];
extern const char *sysFileDir;
extern EmuAudio *audioPtr;
extern std::binary_semaphore execSem, execDoneSem;
extern double systemFrameRate;
extern struct video_canvas_s *activeCanvas;
extern IG::Pixmap canvasSrcPix;
extern IG::PixelFormat pixFmt;
extern Byte1Option optionDriveTrueEmulation;
extern Byte1Option optionCropNormalBorders;
extern Byte1Option optionAutostartWarp;
extern Byte1Option optionAutostartTDE;
extern Byte1Option optionAutostartBasicLoad;
extern Byte1Option optionViceSystem;
extern SByte1Option optionModel;
extern SByte1Option optionDefaultModel;
extern Byte1Option optionBorderMode;
extern Byte1Option optionSidEngine;
extern Byte1Option optionReSidSampling;
extern Byte1Option optionSwapJoystickPorts;
extern Byte1Option optionAutostartOnLaunch;
extern Byte1Option optionVic20RamExpansions;
extern Byte2Option optionC64RamExpansionModule;
extern std::string defaultPaletteName;

int intResource(const char *name);
void setIntResource(const char *name, int val);
void resetIntResource(const char *name);
int defaultIntResource(const char *name);
const char *stringResource(const char *name);
void setStringResource(const char *name, const char *val);
void setBorderMode(int mode);
void setSidEngine(int engine);
void setReSidSampling(int sampling);
void setDriveTrueEmulation(bool on);
bool driveTrueEmulation();
void setAutostartWarp(bool on);
void setAutostartTDE(bool on);
void setAutostartBasicLoad(bool on);
bool autostartBasicLoad();
void setSysModel(int model);
int sysModel();
void setDefaultModel(int model);
bool hasC64DiskExtension(std::string_view name);
bool hasC64TapeExtension(std::string_view name);
bool hasC64CartExtension(std::string_view name);
void applySessionOptions();
int systemCartType(ViceSystem system);
std::vector<std::string> systemFilesWithExtension(const char *ext);
const char *videoChipStr();
void setPaletteResources(const char *palName);
bool usingExternalPalette();
const char *externalPaletteName();
const char *paletteName();
void setJoystickMode(JoystickMode);
bool currSystemIsC64();
bool currSystemIsC64Or128();
void setRuntimeReuSize(int size);

}

void setCanvasSkipFrame(bool on);
void startCanvasRunningFrame();
void resetCanvasSourcePixmap(struct video_canvas_s *c);
bool updateCanvasPixelFormat(struct video_canvas_s *c, IG::PixelFormat);
