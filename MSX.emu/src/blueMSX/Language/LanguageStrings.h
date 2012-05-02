/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageStrings.h,v $
**
** $Revision: 1.91 $
**
** $Date: 2009-04-04 20:57:19 $
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
#ifndef LANGUAGE_STRINGS_H
#define LANGUAGE_STRINGS_H

typedef struct {
    //----------------------
    // Language lines
    //----------------------

    char* langCatalan;
    char* langChineseSimplified;
    char* langChineseTraditional;
    char* langDutch;
    char* langEnglish;
    char* langFinnish;
    char* langFrench;
    char* langGerman;
    char* langItalian;
    char* langJapanese;
    char* langKorean;
    char* langPolish;
    char* langPortuguese;
    char* langRussian;
    char* langSpanish;
    char* langSwedish;


    //----------------------
    // Generic lines
    //----------------------

    char* textDevice;
    char* textFilename;
    char* textFile;
    char* textNone;
    char* textUnknown;


    //----------------------
    // Warning and Error lines
    //----------------------

    char* warningTitle;
    char* warningDiscardChanges;
    char* warningOverwriteFile;
    char* errorTitle;
    char* errorEnterFullscreen;
    char* errorDirectXFailed;
    char* errorNoRomInZip;
    char* errorNoDskInZip;
    char* errorNoCasInZip;
    char* errorNoHelp;
    char* errorStartEmu;
    char* errorPortableReadonly;


    //----------------------
    // File related lines
    //----------------------

    char* fileRom;
    char* fileAll;
    char* fileCpuState;
    char* fileVideoCapture;
    char* fileDisk;
    char* fileCas;
    char* fileAvi;


    //----------------------
    // Menu related lines
    //----------------------

    char* menuNoRecentFiles;
    char* menuInsert;
    char* menuEject;

    char* menuCartGameReader;
    char* menuCartIde;
    char* menuCartBeerIde;
    char* menuCartGIde;
    char* menuCartSunriseIde;
    char* menuCartScsi;
    char* menuCartMegaSCSI;
    char* menuCartWaveSCSI;
    char* menuCartGoudaSCSI;
    char* menuJoyrexPsg;
    char* menuCartSCC;
    char* menuCartSCCPlus;
    char* menuCartFMPac;
    char* menuCartPac;
    char* menuCartHBI55;
    char* menuCartInsertSpecial;
    char* menuCartMegaRam;
    char* menuCartExternalRam;
    char* menuCartEseRam;
    char* menuCartEseSCC;
    char* menuCartMegaFlashRom;

    char* menuDiskInsertNew;
    char* menuDiskInsertCdrom;
    char* menuDiskDirInsert;
    char* menuDiskAutoStart;
    char* menuCartAutoReset;

    char* menuCasRewindAfterInsert;
    char* menuCasUseReadOnly;
    char* lmenuCasSaveAs;
    char* menuCasSetPosition;
    char* menuCasRewind;

    char* menuVideoLoad;
    char* menuVideoPlay;
    char* menuVideoRecord;
    char* menuVideoRecording;
    char* menuVideoRecAppend;
    char* menuVideoStop;
    char* menuVideoRender;

    char* menuPrnFormfeed;

    char* menuZoomNormal;
    char* menuZoomDouble;
    char* menuZoomFullscreen;

    char* menuPropsEmulation;
    char* menuPropsVideo;
    char* menuPropsSound;
    char* menuPropsControls;
    char* menuPropsPerformance;
    char* menuPropsSettings;
    char* menuPropsFile;
    char* menuPropsDisk;
    char* menuPropsLanguage;
    char* menuPropsPorts;

    char* menuVideoSource;
    char* menuVideoSourceDefault;
    char* menuVideoChipAutodetect;
    char* menuVideoInSource;
    char* menuVideoInBitmap;
    
    char* menuEthInterface;

    char* menuHelpHelp;
    char* menuHelpAbout;

    char* menuFileCart;
    char* menuFileDisk;
    char* menuFileCas;
    char* menuFilePrn;
    char* menuFileLoadState;
    char* menuFileSaveState;
    char* menuFileQLoadState;
    char* menuFileQSaveState;
    char* menuFileCaptureAudio;
    char* menuFileCaptureVideo;
    char* menuFileScreenShot;
    char* menuFileExit;
    char* menuFileHarddisk;
    char* menuFileHarddiskNoPesent;
    char* menuFileHarddiskRemoveAll;

    char* menuRunRun;
    char* menuRunPause;
    char* menuRunStop;
    char* menuRunSoftReset;
    char* menuRunHardReset;
    char* menuRunCleanReset;

    char* menuToolsMachine;
    char* menuToolsShortcuts;
    char* menuToolsKeyboard;
    char* menuToolsCtrlEditor;
    char* menuToolsMixer;
    char* menuToolsDebugger;
    char* menuToolsTrainer;
    char* menuToolsTraceLogger;

    char* menuFile;
    char* menuRun;
    char* menuWindow;
    char* menuOptions;
    char* menuTools;
    char* menuHelp;


    //----------------------
    // Dialog related lines
    //----------------------

    char* dlgOK;
    char* dlgOpen;
    char* dlgCancel;
    char* dlgSave;
    char* dlgSaveAs;
    char* dlgRun;
    char* dlgClose;

    char* dlgLoadRom;
    char* dlgLoadDsk;
    char* dlgLoadCas;
    char* dlgLoadRomDskCas;
    char* dlgLoadRomDesc;
    char* dlgLoadDskDesc;
    char* dlgLoadCasDesc;
    char* dlgLoadRomDskCasDesc;
    char* dlgLoadState;
    char* dlgLoadVideoCapture;
    char* dlgSaveState;
    char* dlgSaveCassette;
    char* dlgSaveVideoClipAs;
    char* dlgAmountCompleted;
    char* dlgInsertRom1;
    char* dlgInsertRom2;
    char* dlgInsertDiskA;
    char* dlgInsertDiskB;
    char* dlgInsertHarddisk;
    char* dlgInsertCas;
    char* dlgRomType;
    char* dlgDiskSize;

    char* dlgTapeTitle;
    char* dlgTapeFrameText;
    char* dlgTapeCurrentPos;
    char* dlgTapeTotalTime;
    char* dlgTapeSetPosText;
    char* dlgTapeCustom;
    char* dlgTabPosition;
    char* dlgTabType;
    char* dlgTabFilename;
    char* dlgZipReset;

    char* dlgAboutTitle;

    char* dlgLangLangText;
    char* dlgLangLangTitle;

    char* dlgAboutAbout;
    char* dlgAboutVersion;
    char* dlgAboutBuildNumber;
    char* dlgAboutBuildDate;
    char* dlgAboutCreat;
    char* dlgAboutDevel;
    char* dlgAboutThanks;
    char* dlgAboutLisence;

    char* dlgSavePreview;
    char* dlgSaveDate;
    
    char* dlgRenderVideoCapture;


    //----------------------
    // Properties related lines
    //----------------------

    char* propTitle;
    char* propEmulation;
    char* propVideo;
    char* propSound;
    char* propControls;
    char* propPerformance;
    char* propSettings;
    char* propFile;
    char* propDisk;
    char* propPorts;

    char* propEmuGeneralGB;
    char* propEmuFamilyText;
    char* propEmuMemoryGB;
    char* propEmuRamSizeText;
    char* propEmuVramSizeText;
    char* propEmuSpeedGB;
    char* propEmuSpeedText;
    char* propEmuFrontSwitchGB;
    char* propEmuFrontSwitch;
    char* propEmuFdcTiming;
    char* propEmuReversePlay;
    char* propEmuPauseSwitch;
    char* propEmuAudioSwitch;
    char* propVideoFreqText;
    char* propVideoFreqAuto;
    char* propSndOversampleText;
    char* propSndMidiInGB;
    char* propSndYkInGB;
    char* propSndMidiOutGB;
    char* propSndMidiChannel;
    char* propSndMidiAll;

    char* propMonMonGB;
    char* propMonTypeText;
    char* propMonEmuText;
    char* propVideoTypeText;
    char* propWindowSizeText;
    char* propMonHorizStretch;
    char* propMonVertStretch;
    char* propMonDeInterlace;
    char* propBlendFrames;
    char* propMonBrightness;
    char* propMonContrast;
    char* propMonSaturation;
    char* propMonGamma;
    char* propMonScanlines;
    char* propMonColorGhosting;
    char* propMonEffectsGB;

    char* propPerfVideoDrvGB;
    char* propPerfVideoDispDrvText;
    char* propPerfFrameSkipText;
    char* propPerfAudioDrvGB;
    char* propPerfAudioDrvText;
    char* propPerfAudioBufSzText;
    char* propPerfEmuGB;
    char* propPerfSyncModeText;
    char* propFullscreenResText;

    char* propSndChipEmuGB;
    char* propSndMsxMusic;
    char* propSndMsxAudio;
    char* propSndMoonsound;
    char* propSndMt32ToGm;

    char* propPortsLptGB;
    char* propPortsComGB;
    char* propPortsLptText;
    char* propPortsCom1Text;
    char* propPortsNone;
    char* propPortsSimplCovox;
    char* propPortsFile;
    char* propPortsComFile;
    char* propPortsOpenLogFile;
    char* propPortsEmulateMsxPrn;

    char* propSetFileHistoryGB;
    char* propSetFileHistorySize;
    char* propSetFileHistoryClear;
    char* propFileTypes;
    char* propWindowsEnvGB;
    char* propSetScreenSaver;
    char* propDisableWinKeys;
    char* propPriorityBoost;
    char* propScreenshotPng;
    char* propEjectMediaOnExit;
    char* propClearHistory;
    char* propOpenRomGB;
    char* propDefaultRomType;
    char* propGuessRomType;

    char* propSettDefSlotGB;
    char* propSettDefSlots;
    char* propSettDefSlot;
    char* propSettDefDrives;
    char* propSettDefDrive;

    char* propThemeGB;
    char* propTheme;

    char* propCdromGB;
    char* propCdromMethod;
    char* propCdromMethodNone;
    char* propCdromMethodIoctl;
    char* propCdromMethodAspi;
    char* propCdromDrive;


    //----------------------
    // Dropdown related lines
    //----------------------

    char* enumVideoMonColor;
    char* enumVideoMonGrey;
    char* enumVideoMonGreen;
    char* enumVideoMonAmber;

    char* enumVideoTypePAL;
    char* enumVideoTypeNTSC;

    char* enumVideoEmuNone;
    char* enumVideoEmuYc;
    char* enumVideoEmuMonitor;
    char* enumVideoEmuYcBlur;
    char* enumVideoEmuComp;
    char* enumVideoEmuCompBlur;
    char* enumVideoEmuScale2x;
    char* enumVideoEmuHq2x;

    char* enumVideoSize1x;
    char* enumVideoSize2x;
    char* enumVideoSizeFullscreen;

    char* enumVideoDrvDirectDrawHW;
    char* enumVideoDrvDirectDraw;
    char* enumVideoDrvGDI;

    char* enumVideoFrameskip0;
    char* enumVideoFrameskip1;
    char* enumVideoFrameskip2;
    char* enumVideoFrameskip3;
    char* enumVideoFrameskip4;
    char* enumVideoFrameskip5;

    char* enumSoundDrvNone;
    char* enumSoundDrvWMM;
    char* enumSoundDrvDirectX;

    char* enumEmuSync1ms;
    char* enumEmuSyncAuto;
    char* enumEmuSyncNone;
    char* enumEmuSyncVblank;
    char* enumEmuAsyncVblank;

    char* enumControlsJoyNone;
    char* enumControlsJoyMouse;
    char* enumControlsJoyTetris2Dongle;
    char* enumControlsJoyTMagicKeyDongle;
    char* enumControlsJoy2Button;
    char* enumControlsJoyGunstick;
    char* enumControlsJoyAsciiLaser;
    char* enumControlsArkanoidPad;
    char* enumControlsJoyColeco;
    
    char* enumDiskMsx35Dbl9Sect;
    char* enumDiskMsx35Dbl8Sect;
    char* enumDiskMsx35Sgl9Sect;
    char* enumDiskMsx35Sgl8Sect;
    char* enumDiskSvi525Dbl;
    char* enumDiskSvi525Sgl;
    char* enumDiskSf3Sgl;


    //----------------------
    // Configuration related lines
    //----------------------

    char* confTitle;
    char* confConfigText;
    char* confSlotLayout;
    char* confMemory;
    char* confChipEmulation;
    char* confChipExtras;

    char* confOpenRom;
    char* confSaveTitle;
    char* confSaveText;
    char* confSaveAsTitle;
    char* confSaveAsMachineName;
    char* confDiscardTitle;
    char* confExitSaveTitle;
    char* confExitSaveText;

    char* confSlotLayoutGB;
    char* confSlotExtSlotGB;
    char* confBoardGB;
    char* confBoardText;
    char* confSlotPrimary;
    char* confSlotExpanded;

    char* confSlotCart;
    char* confSlot;
    char* confSubslot;

    char* confMemAdd;
    char* confMemEdit;
    char* confMemRemove;
    char* confMemSlot;
    char* confMemAddresss;
    char* confMemType;
    char* confMemRomImage;

    char* confChipVideoGB;
    char* confChipVideoChip;
    char* confChipVideoRam;
    char* confChipSoundGB;
    char* confChipPsgStereoText;

    char* confCmosGB;
    char* confCmosEnable;
    char* confCmosBattery;

    char* confCpuFreqGB;
    char* confZ80FreqText;
    char* confR800FreqText;
    char* confFdcGB;
    char* confCFdcNumDrivesText;

    char* confEditMemTitle;
    char* confEditMemGB;
    char* confEditMemType;
    char* confEditMemFile;
    char* confEditMemAddress;
    char* confEditMemSize;
    char* confEditMemSlot;


    //----------------------
    // Shortcut lines
    //----------------------

    char* shortcutKey;
    char* shortcutDescription;

    char* shortcutSaveConfig;
    char* shortcutOverwriteConfig;
    char* shortcutExitConfig;
    char* shortcutDiscardConfig;
    char* shortcutSaveConfigAs;
    char* shortcutConfigName;
    char* shortcutNewProfile;
    char* shortcutConfigTitle;
    char* shortcutAssign;
    char* shortcutPressText;
    char* shortcutScheme;
    char* shortcutCartInsert1;
    char* shortcutCartRemove1;
    char* shortcutCartInsert2;
    char* shortcutCartRemove2;
    char* shortcutSpecialMenu1;
    char* shortcutSpecialMenu2;
    char* shortcutCartAutoReset;
    char* shortcutDiskInsertA;
    char* shortcutDiskDirInsertA;
    char* shortcutDiskRemoveA;
    char* shortcutDiskChangeA;
    char* shortcutDiskAutoResetA;
    char* shortcutDiskInsertB;
    char* shortcutDiskDirInsertB;
    char* shortcutDiskRemoveB;
    char* shortcutCasInsert;
    char* shortcutCasEject;
    char* shortcutCasAutorewind;
    char* shortcutCasReadOnly;
    char* shortcutCasSetPosition;
    char* shortcutCasRewind;
    char* shortcutCasSave;
    char* shortcutPrnFormFeed;
    char* shortcutCpuStateLoad;
    char* shortcutCpuStateSave;
    char* shortcutCpuStateQload;
    char* shortcutCpuStateQsave;
    char* shortcutAudioCapture;
    char* shortcutScreenshotOrig;
    char* shortcutScreenshotSmall;
    char* shortcutScreenshotLarge;
    char* shortcutQuit;
    char* shortcutRunPause;
    char* shortcutStop;
    char* shortcutResetHard;
    char* shortcutResetSoft;
    char* shortcutResetClean;
    char* shortcutSizeSmall;
    char* shortcutSizeNormal;
    char* shortcutSizeFullscreen;
    char* shortcutSizeMinimized;
    char* shortcutToggleFullscren;
    char* shortcutVolumeIncrease;
    char* shortcutVolumeDecrease;
    char* shortcutVolumeMute;
    char* shortcutVolumeStereo;
    char* shortcutSwitchMsxAudio;
    char* shortcutSwitchFront;
    char* shortcutSwitchPause;
    char* shortcutToggleMouseLock;
    char* shortcutEmuSpeedMax;
    char* shortcutEmuPlayReverse;
    char* shortcutEmuSpeedToggle;
    char* shortcutEmuSpeedNormal;
    char* shortcutEmuSpeedInc;
    char* shortcutEmuSpeedDec;
    char* shortcutThemeSwitch;
    char* shortcutShowEmuProp;
    char* shortcutShowVideoProp;
    char* shortcutShowAudioProp;
    char* shortcutShowCtrlProp;
    char* shortcutShowPerfProp;
    char* shortcutShowSettProp;
    char* shortcutShowPorts;
    char* shortcutShowLanguage;
    char* shortcutShowMachines;
    char* shortcutShowShortcuts;
    char* shortcutShowKeyboard;
    char* shortcutShowMixer;
    char* shortcutShowDebugger;
    char* shortcutShowTrainer;
    char* shortcutShowHelp;
    char* shortcutShowAbout;
    char* shortcutShowFiles;
    char* shortcutToggleSpriteEnable;
    char* shortcutToggleFdcTiming;
    char* shortcutToggleCpuTrace;
    char* shortcutVideoLoad;
    char* shortcutVideoPlay;
    char* shortcutVideoRecord;
    char* shortcutVideoStop;
    char* shortcutVideoRender;


    //----------------------
    // Keyboard config lines
    //----------------------

    char* keyconfigSelectedKey;
    char* keyconfigMappedTo;
    char* keyconfigMappingScheme;

    
    //----------------------
    // Rom type lines
    //----------------------

    char* romTypeStandard;

    char* romTypeZenima80;
    char* romTypeZenima90;
    char* romTypeZenima126;

    char* romTypeSccMirrored;
    char* romTypeSccExtended;

    char* romTypeKonamiGeneric;

    char* romTypeMirrored;
    char* romTypeNormal;
    char* romTypeDiskPatch;
    char* romTypeCasPatch;
    char* romTypeTc8566afFdc;
    char* romTypeTc8566afTrFdc;
    char* romTypeMicrosolFdc;
    char* romTypeNationalFdc;
    char* romTypePhilipsFdc;
    char* romTypeSvi738Fdc;
    char* romTypeMappedRam;
    char* romTypeMirroredRam1k;
    char* romTypeMirroredRam2k;
    char* romTypeNormalRam;

    char* romTypeTurborPause;
    char* romTypeF4deviceNormal;
    char* romTypeF4deviceInvert;

    char* romTypeTurborTimer;

    char* romTypeNormal4000;
    char* romTypeNormalC000;

    char* romTypeExtRam;
    char* romTypeExtRam16;
    char* romTypeExtRam32;
    char* romTypeExtRam48;
    char* romTypeExtRam64;
    char* romTypeExtRam512;
    char* romTypeExtRam1mb;
    char* romTypeExtRam2mb;
    char* romTypeExtRam4mb;

    char* romTypeSvi328Cart;
    char* romTypeSvi328Fdc;
    char* romTypeSvi328Prn;
    char* romTypeSvi328Uart;
    char* romTypeSvi328col80;
    char* romTypeSvi328RsIde;
    char* romTypeSvi727col80;
    char* romTypeColecoCart;
    char* romTypeSg1000Cart;
    char* romTypeSc3000Cart;

    char* romTypeMsxPrinter;
    char* romTypeTurborPcm;

    char* romTypeNms8280Digitiz;
    char* romTypeHbiV1Digitiz;

    //----------------------
    // Debug type lines
    // Note: Only needs translation if debugger is translated
    //----------------------
    
    char* dbgMemVisible;
    char* dbgMemRamNormal;
    char* dbgMemRamMapped;
    char* dbgMemYmf278;
    char* dbgMemAy8950;
    char* dbgMemScc;

    char* dbgCallstack;

    char* dbgRegs;
    char* dbgRegsCpu;
    char* dbgRegsYmf262;
    char* dbgRegsYmf278;
    char* dbgRegsAy8950;
    char* dbgRegsYm2413;

    char* dbgDevRamMapper;
    char* dbgDevRam;
    char* dbgDevF4Device;
    char* dbgDevKorean80;
    char* dbgDevKorean90;
    char* dbgDevKorean128;
    char* dbgDevFdcMicrosol;

    char* dbgDevPrinter;

    char* dbgDevSviFdc;
    char* dbgDevSviPrn;
    char* dbgDevSvi80Col;

    char* dbgDevRtc;
    char* dbgDevTrPause;


    //----------------------
    // Debug type lines
    // Note: Can only be translated to european languages
    //----------------------
    char* aboutScrollThanksTo;
    char* aboutScrollAndYou;

} LanguageStrings;

#endif

