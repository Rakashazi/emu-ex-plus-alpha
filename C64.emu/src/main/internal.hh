#pragma once

#include "VicePlugin.hh"
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/Pixmap.hh>
#include <emuframework/Option.hh>
#include <emuframework/EmuSystem.hh>

class EmuAudio;

enum Vic20Ram : uint8_t
{
	BLOCK_0 = 1,
	BLOCK_1 = 1 << 1,
	BLOCK_2 = 1 << 2,
	BLOCK_3 = 1 << 3,
	BLOCK_5 = 1 << 5
};

static constexpr uint8_t SYSTEM_FLAG_NO_AUTOSTART = IG::bit(0);

extern VicePlugin plugin;
extern ViceSystem currSystem;
extern Base::ApplicationContext appContext;
extern FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3];
extern EmuAudio *audioPtr;
extern IG::Semaphore execSem, execDoneSem;
extern double systemFrameRate;
extern struct video_canvas_s *activeCanvas;
extern IG::Pixmap canvasSrcPix;
extern FS::PathString firmwareBasePath;
extern IG::PixelFormat pixFmt;
extern Byte1Option optionDriveTrueEmulation;
extern Byte1Option optionVirtualDeviceTraps;
extern Byte1Option optionCropNormalBorders;
extern Byte1Option optionAutostartWarp;
extern Byte1Option optionAutostartTDE;
extern Byte1Option optionAutostartBasicLoad;
extern Byte1Option optionViceSystem;
extern SByte1Option optionModel;
extern Byte1Option optionC64Model;
extern Byte1Option optionDTVModel;
extern Byte1Option optionC128Model;
extern Byte1Option optionSuperCPUModel;
extern Byte1Option optionCBM2Model;
extern Byte1Option optionCBM5x0Model;
extern Byte1Option optionPETModel;
extern Byte1Option optionPlus4Model;
extern Byte1Option optionVIC20Model;
extern Byte1Option optionBorderMode;
extern Byte1Option optionSidEngine;
extern Byte1Option optionReSidSampling;
extern Byte1Option optionSwapJoystickPorts;
extern PathOption optionFirmwarePath;
extern Byte1Option optionAutostartOnLaunch;
extern Byte1Option optionVic20RamExpansions;

int intResource(const char *name);
void setBorderMode(int mode);
void setSidEngine(int engine);
void setReSidSampling(int sampling);
void setDriveTrueEmulation(bool on);
bool driveTrueEmulation();
void setVirtualDeviceTraps(bool on);
bool virtualDeviceTraps();
void setAutostartWarp(bool on);
void setAutostartTDE(bool on);
void setAutostartBasicLoad(bool on);
bool autostartBasicLoad();
void setSysModel(int model);
void setCanvasSkipFrame(bool on);
void startCanvasRunningFrame();
int sysModel();
void setDefaultC64Model(int model);
void setDefaultDTVModel(int model);
void setDefaultC128Model(int model);
void setDefaultSuperCPUModel(int model);
void setDefaultCBM2Model(int model);
void setDefaultCBM5x0Model(int model);
void setDefaultPETModel(int model);
void setDefaultPlus4Model(int model);
void setDefaultVIC20Model(int model);
bool hasC64DiskExtension(const char *name);
bool hasC64TapeExtension(const char *name);
bool hasC64CartExtension(const char *name);
int optionDefaultModel(ViceSystem system);
void resetCanvasSourcePixmap(struct video_canvas_s *c);
bool updateCanvasPixelFormat(struct video_canvas_s *c, IG::PixelFormat);
void applySessionOptions();
void updateKeyMappingArray(EmuApp &);
int systemCartType(ViceSystem system);
