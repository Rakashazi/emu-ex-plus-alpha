/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Actions.h,v $
**
** $Revision: 1.34 $
**
** $Date: 2007-03-24 05:20:32 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#ifndef ACTIONS_H
#define ACTIONS_H

#include "Properties.h"
#include "VideoRender.h"
#include "AudioMixer.h"

void actionInit(Video* video, Properties* properties, Mixer* mixer);
void actionSetAudioCaptureSetDirectory(char* dir, char* prefix);
void actionSetVideoCaptureSetDirectory(char* dir, char* prefix);
void actionSetQuickSaveSetDirectory(char* dir, char* prefix);

void actionCartInsert(int cartNo);
void actionCartRemove(int cartNo);
void actionDiskInsert(int diskNo);
void actionDiskInsertDir(int diskNo);
void actionDiskInsertNew(int diskNo);
void actionDiskRemove(int diskNo);

void actionHarddiskInsert(int diskNo);
void actionHarddiskInsertCdrom(int diskNo);
void actionHarddiskInsertNew(int diskNo);
void actionHarddiskInsertDir(int diskNo);
void actionHarddiskRemove(int diskNo);
void actionHarddiskRemoveAll();

void actionQuit();
void actionLoadState();
void actionSaveState();
void actionQuickLoadState();
void actionQuickSaveState();
void actionCartInsert1();
void actionCartInsert2();
void actionEmuTogglePause();
void actionEmuStep();
void actionEmuStop();
void actionDiskInsertA();
void actionDiskInsertB();
void actionDiskDirInsertA();
void actionDiskDirInsertB();
void actionMaxSpeedSet();
void actionMaxSpeedRelease();
void actionStartPlayReverse();
void actionStopPlayReverse();
void actionDiskQuickChange();
void actionWindowSizeSmall();
void actionWindowSizeNormal();
void actionWindowSizeMinimized();
void actionWindowSizeFullscreen();
void actionEmuSpeedNormal();
void actionEmuSpeedDecrease();
void actionEmuSpeedIncrease();
void actionCasInsert();
void actionCasRewind();
void actionCasSetPosition(); 
void actionEmuResetSoft();
void actionEmuResetHard();
void actionEmuResetClean();
void actionScreenCapture();
void actionScreenCaptureUnfilteredSmall();
void actionScreenCaptureUnfilteredLarge();
void actionNextTheme();
void actionCasRemove();
void actionDiskRemoveA();
void actionDiskRemoveB();
void actionCartRemove1();
void actionCartRemove2();
void actionCasSave();
void actionPropShowEmulation();
void actionPropShowVideo();
void actionPropShowAudio();
void actionPropShowPerformance();
void actionPropShowSettings();
void actionPropShowDisk();
void actionPropShowApearance();
void actionPropShowPorts();
void actionOptionsShowLanguage();
void actionToolsShowMachineEditor();
void actionToolsShowShorcutEditor();
void actionToolsShowKeyboardEditor();
void actionToolsShowMixer();
void actionToolsShowDebugger();
void actionToolsShowTrainer();
void actionHelpShowHelp();
void actionHelpShowAbout();
void actionMaximizeWindow();
void actionMinimizeWindow();
void actionCloseWindow();
void actionVolumeIncrease();
void actionVolumeDecrease();

void actionMenuSpecialCart1(int x, int y);
void actionMenuSpecialCart2(int x, int y);
void actionMenuReset(int x, int y);
void actionMenuRun(int x, int y);
void actionMenuFile(int x, int y);
void actionMenuCart1(int x, int y);
void actionMenuCart2(int x, int y);
void actionMenuHarddisk(int x, int y);
void actionMenuDiskA(int x, int y);
void actionMenuDiskB(int x, int y);
void actionMenuCassette(int x, int y);
void actionMenuPrinter(int x, int y);
void actionMenuJoyPort1(int x, int y);
void actionMenuJoyPort2(int x, int y);
void actionMenuZoom(int x, int y);
void actionMenuOptions(int x, int y);
void actionMenuHelp(int x, int y);
void actionMenuTools(int x, int y);

void actionToggleCartAutoReset();
void actionToggleDiskAutoReset();
void actionToggleCasAutoRewind();
void actionToggleSpriteEnable();
void actionToggleFdcTiming();
void actionToggleMsxAudioSwitch();
void actionToggleFrontSwitch();
void actionTogglePauseSwitch();
void actionToggleWaveCapture();
void actionToggleMouseCapture();
void actionVideoCaptureLoad();
void actionVideoCapturePlay();
void actionVideoCaptureRec();
void actionVideoCaptureStop();
void actionVideoCaptureSave();
void actionMaxSpeedToggle();
void actionFullscreenToggle();
void actionCasToggleReadonly();
void actionVolumeToggleStereo();

void actionToggleHorizontalStretch();
void actionToggleVerticalStretch();
void actionToggleScanlinesEnable();
void actionToggleDeinterlaceEnable();
void actionToggleBlendFrameEnable();
void actionToggleRfModulatorEnable();

void actionMuteToggleMaster();
void actionMuteTogglePsg();
void actionMuteTogglePcm();
void actionMuteToggleIo();
void actionMuteToggleScc();
void actionMuteToggleKeyboard();
void actionMuteToggleMsxMusic();
void actionMuteToggleMsxAudio();
void actionMuteToggleMoonsound();
void actionMuteToggleYamahaSfg();
void actionMuteToggleMidi();

void actionPrinterForceFormFeed();

void actionSetCartAutoReset(int value);
void actionSetDiskAutoResetA(int value);
void actionSetCasAutoRewind(int value);
void actionSetSpriteEnable(int value);
void actionSetMsxAudioSwitch(int value);
void actionSetFdcTiming(int value);
void actionSetFrontSwitch(int value);
void actionSetPauseSwitch(int value);
void actionSetWaveCapture(int value);
void actionSetMouseCapture(int value);
void actionSetFullscreen(int value);
void actionSetCasReadonly(int value);
void actionSetVolumeMute(int value);
void actionSetVolumeStereo(int value);

void actionVideoSetGamma(int value);
void actionVideoSetBrightness(int value);
void actionVideoSetContrast(int value);
void actionVideoSetSaturation(int value);
void actionVideoSetScanlines(int value);
void actionVideoSetRfModulation(int value);
void actionVideoSetColorMode(int value);
void actionVideoSetFilter(int value);
void actionVideoEnableMon1(int value);
void actionVideoEnableMon2(int value);
void actionVideoEnableMon3(int value);

void actionVolumeSetMaster(int value);
void actionVolumeSetPsg(int value);
void actionVolumeSetPcm(int value);
void actionVolumeSetIo(int value);
void actionVolumeSetIo(int value);
void actionVolumeSetScc(int value);
void actionVolumeSetKeyboard(int value);
void actionVolumeSetMsxMusic(int value);
void actionVolumeSetMsxAudio(int value);
void actionVolumeSetMoonsound(int value);
void actionVolumeSetYamahaSfg(int value);
void actionVolumeSetMidi(int value);
void actionPanSetPsg(int value);
void actionPanSetPcm(int value);
void actionPanSetIo(int value);
void actionPanSetScc(int value);
void actionPanSetKeyboard(int value);
void actionPanSetMsxMusic(int value);
void actionPanSetMsxAudio(int value);
void actionPanSetMoonsound(int value);
void actionPanSetYamahaSfg(int value);
void actionPanSetMidi(int value);

void actionRenshaSetLevel(int value);
void actionEmuSpeedSet(int value);

void actionKeyPress(int keyCode, int pressed);

#endif

