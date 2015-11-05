#pragma once

#include "VicePlugin.hh"
#include <imagine/thread/Semaphore.hh>
#include <imagine/pixmap/PixelFormat.hh>

extern VicePlugin plugin;
extern ViceSystem currSystem;
extern FS::PathString sysFilePath[Config::envIsLinux ? 5 : 3];
extern bool doAudio;
extern bool runningFrame;
extern uint c64VidX, c64VidY;
alignas(8) extern uint16 pix[1024*512];
static constexpr auto pixFmt = IG::PIXEL_FMT_RGB565;
extern IG::Semaphore execSem, execDoneSem;
extern double systemFrameRate;

void setCanvasSkipFrame(bool on);
