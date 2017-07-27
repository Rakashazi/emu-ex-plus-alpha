#pragma once

#include "VicePlugin.hh"
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/PixelFormat.hh>
#include <emuframework/OptionView.hh>
#include <emuframework/EmuSystem.hh>

extern VicePlugin plugin;
extern ViceSystem currSystem;
extern FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3];
extern bool doAudio;
extern bool runningFrame;
extern bool autostartOnLoad;
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
extern IG::Semaphore execSem, execDoneSem;
extern double systemFrameRate;
extern struct video_canvas_s *activeCanvas;
extern IG::Pixmap canvasSrcPix;
extern FS::PathString firmwareBasePath;
extern Byte1Option optionDriveTrueEmulation;
extern Byte1Option optionVirtualDeviceTraps;
extern Byte1Option optionCropNormalBorders;
extern Byte1Option optionAutostartWarp;
extern Byte1Option optionAutostartTDE;
extern Byte1Option optionViceSystem;
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
extern Byte1Option optionSwapJoystickPorts;
extern PathOption optionFirmwarePath;

int intResource(const char *name);
void setBorderMode(int mode);
void setSidEngine(int engine);
void setDriveTrueEmulation(bool on);
bool driveTrueEmulation();
void setVirtualDeviceTraps(bool on);
bool virtualDeviceTraps();
void setAutostartWarp(bool on);
void setAutostartTDE(bool on);
void setSysModel(int model);
void setCanvasSkipFrame(bool on);
int sysModel();
void setDefaultNTSCModel();
void setDefaultPALModel();
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
int optionModel(ViceSystem system);
void resetCanvasSourcePixmap(struct video_canvas_s *c);

