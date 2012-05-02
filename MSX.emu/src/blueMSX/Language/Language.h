/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/Language.h,v $
**
** $Revision: 1.99 $
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
#ifndef LANGUAGE_H
#define LANGUAGE_H

typedef enum { 
    EMU_LANG_ENGLISH     = 0, 
    EMU_LANG_SWEDISH     = 1, 
    EMU_LANG_JAPANESE    = 2, 
    EMU_LANG_PORTUGUESE  = 3, 
    EMU_LANG_FRENCH      = 4, 
    EMU_LANG_DUTCH       = 5,
    EMU_LANG_SPANISH     = 6,
    EMU_LANG_ITALIAN     = 7,
    EMU_LANG_FINNISH     = 8,
    EMU_LANG_KOREAN      = 9,
    EMU_LANG_GERMAN      = 10,
    EMU_LANG_POLISH      = 11,
    EMU_LANG_CHINESESIMP = 12,
    EMU_LANG_CHINESETRAD = 13,
    EMU_LANG_RUSSIAN     = 14,
    EMU_LANG_CATALAN     = 15,
    EMU_LANG_COUNT       = 16,
    EMU_LANG_UNKNOWN     = -1 
} EmuLanguageType;

void langInit();

int langSetLanguage(EmuLanguageType languageType);
EmuLanguageType langGetLanguage();
EmuLanguageType langFromName(char* name, int translate);
const char* langToName(EmuLanguageType languageType, int translate);
EmuLanguageType langGetType(int i);


//----------------------
// Language lines
//----------------------

char* langLangCatalan();
char* langLangChineseSimplified();
char* langLangChineseTraditional();
char* langLangDutch();
char* langLangEnglish();
char* langLangFinnish();
char* langLangFrench();
char* langLangGerman();
char* langLangItalian();
char* langLangJapanese();
char* langLangKorean();
char* langLangPolish();
char* langLangPortuguese();
char* langLangRussian();
char* langLangSpanish();
char* langLangSwedish();


//----------------------
// Generic lines
//----------------------

char* langTextDevice();
char* langTextFilename();
char* langTextFile();
char* langTextNone();
char* langTextUnknown();


//----------------------
// Warning and Error lines
//----------------------

char* langWarningTitle();
char* langWarningDiscardChanges();
char* langWarningOverwriteFile();
char* langErrorTitle();
char* langErrorEnterFullscreen();
char* langErrorDirectXFailed();
char* langErrorNoRomInZip();
char* langErrorNoDskInZip();
char* langErrorNoCasInZip();
char* langErrorNoHelp();
char* langErrorStartEmu();
char* langErrorPortableReadonly();


//----------------------
// File related lines
//----------------------

char* langFileRom();
char* langFileAll();
char* langFileCpuState();
char* langFileVideoCapture();
char* langFileDisk();
char* langFileCas();
char* langFileAvi();


//----------------------
// Menu related lines
//----------------------

char* langMenuNoRecentFiles();
char* langMenuInsert();
char* langMenuEject();

char* langMenuCartGameReader();
char* langMenuCartIde();
char* langMenuCartBeerIde();
char* langMenuCartGIde();
char* langMenuCartSunriseIde();
char* langMenuCartScsi();
char* langMenuCartMegaSCSI();
char* langMenuCartWaveSCSI();
char* langMenuCartGoudaSCSI();
char* langMenuCartSCC();
char* langMenuCartJoyrexPsg();
char* langMenuCartSCCPlus();
char* langMenuCartFMPac();
char* langMenuCartPac();
char* langMenuCartHBI55();
char* langMenuCartInsertSpecial();
char* langMenuCartExternalRam();
char* langMenuCartMegaRam();
char* langMenuCartEseRam();
char* langMenuCartEseSCC();
char* langMenuCartMegaFlashRom();

char* langMenuDiskDirInsert();
char* langMenuDiskDirInsertCdrom();
char* langMenuDiskInsertNew();
char* langMenuDiskAutoStart();
char* langMenuCartAutoReset();

char* langMenuCasRewindAfterInsert();
char* langMenuCasUseReadOnly();
char* langMenuCasSaveAs();
char* langMenuCasSetPosition();
char* langMenuCasRewind();

char* langMenuVideoLoad();
char* langMenuVideoPlay();
char* langMenuVideoRecord();
char* langMenuVideoRecording();
char* langMenuVideoRecAppend();
char* langMenuVideoStop();
char* langMenuVideoRender();

char* langMenuPrnFormfeed();

char* langMenuZoomNormal();
char* langMenuZoomDouble();
char* langMenuZoomFullscreen();

char* langMenuPropsEmulation();
char* langMenuPropsVideo();
char* langMenuPropsSound();
char* langMenuPropsControls();
char* langMenuPropsPerformance();
char* langMenuPropsSettings();
char* langMenuPropsFile();
char* langMenuPropsDisk();
char* langMenuPropsLanguage();
char* langMenuPropsPorts();

char* langMenuVideoSource();
char* langMenuVideoSourceDefault();
char* langMenuVideoChipAutodetect();
char* langMenuVideoInSource();
char* langMenuVideoInBitmap();

char* langMenuEthInterface();

char* langMenuHelpHelp();
char* langMenuHelpAbout();

char* langMenuFileCart();
char* langMenuFileDisk();
char* langMenuFileCas();
char* langMenuFilePrn();
char* langMenuFileLoadState();
char* langMenuFileSaveState();
char* langMenuFileQLoadState();
char* langMenuFileQSaveState();
char* langMenuFileCaptureAudio();
char* langMenuFileCaptureVideo();
char* langMenuFileScreenShot();
char* langMenuFileExit();
char* langMenuFileHarddisk();
char* langMenuFileHarddiskNoPresent();
char* langMenuFileHarddiskRemoveAll();

char* langMenuRunRun();
char* langMenuRunPause();
char* langMenuRunStop();
char* langMenuRunSoftReset();
char* langMenuRunHardReset();
char* langMenuRunCleanReset();

char* langMenuToolsMachine();
char* langMenuToolsShortcuts();
char* langMenuToolsCtrlEditor();
char* langMenuToolsMixer();
char* langMenuToolsDebugger();
char* langMenuToolsTrainer();
char* langMenuToolsTraceLogger();

char* langMenuFile();
char* langMenuRun();
char* langMenuWindow();
char* langMenuOptions();
char* langMenuTools();
char* langMenuHelp();


//----------------------
// Dialog related lines
//----------------------

char* langDlgOK();
char* langDlgOpen();
char* langDlgCancel();
char* langDlgSave();
char* langDlgSaveAs();
char* langDlgRun();
char* langDlgClose();

char* langDlgLoadRom();
char* langDlgLoadDsk();
char* langDlgLoadCas();
char* langDlgLoadRomDskCas();
char* langDlgLoadRomDesc();
char* langDlgLoadDskDesc();
char* langDlgLoadCasDesc();
char* langDlgLoadRomDskCasDesc();
char* langDlgLoadState();
char* langDlgLoadVideoCapture();
char* langDlgSaveState();
char* langDlgSaveCassette();
char* langDlgSaveVideoClipAs();
char* langDlgAmountCompleted();
char* langDlgInsertRom1();
char* langDlgInsertRom2();
char* langDlgInsertDiskA();
char* langDlgInsertDiskB();
char* langDlgInsertHarddisk();
char* langDlgInsertCas();
char* langDlgRomType();
char* langDlgDiskSize();

char* langDlgTapeTitle();
char* langDlgTapeFrameText();
char* langDlgTapeCurrentPos();
char* langDlgTapeSetPosText();
char* langDlgTapeCustom();
char* langDlgTabPosition();
char* langDlgTabType();
char* langDlgTabFilename();
char* langDlgTapeTotalTime();
char* langDlgZipReset();

char* langDlgAboutTitle();

char* langDlgLangLangText();
char* langDlgLangTitle();

char* langDlgAboutAbout();
char* langDlgAboutVersion();
char* langDlgAboutBuildNumber();
char* langDlgAboutBuildDate();
char* langDlgAboutCreat();
char* langDlgAboutDevel();
char* langDlgAboutThanks();
char* langDlgAboutLisence();

char* langDlgSavePreview();
char* langDlgSaveDate();

char* langDlgRenderVideoCapture();


//----------------------
// Properties related lines
//----------------------

char* langPropTitle();
char* langPropEmulation();
char* langPropVideo();
char* langPropSound();
char* langPropControls();
char* langPropPerformance();
char* langPropSettings();
char* langPropFile();
char* langPropDisk();
char* langPropPorts();

char* langPropEmuGeneralGB();
char* langPropEmuFamilyText();
char* langPropEmuMemoryGB();
char* langPropEmuRamSizeText();
char* langPropEmuVramSizeText();
char* langPropEmuSpeedGB();
char* langPropEmuSpeedText();
char* langPropEmuFrontSwitchGB();
char* langPropEmuFrontSwitch();
char* langPropEmuFdcTiming();
char* langPropEmuReversePlay();
char* langPropEmuPauseSwitch();
char* langPropEmuAudioSwitch();
char* langPropVideoFreqText();
char* langPropVideoFreqAuto();
char* langPropSndOversampleText();
char* langPropSndYkInGB();
char* langPropSndMidiInGB();
char* langPropSndMidiOutGB();
char* langPropSndMidiChannel();
char* langPropSndMidiAll();

char* langPropMonMonGB();
char* langPropMonTypeText();
char* langPropMonEmuText();
char* langPropVideoTypeText();
char* langPropWindowSizeText();
char* langPropMonHorizStretch();
char* langPropMonVertStretch();
char* langPropMonDeInterlace();
char* langPropMonBlendFrames();
char* langPropMonBrightness();
char* langPropMonContrast();
char* langPropMonSaturation();
char* langPropMonGamma();
char* langPropMonScanlines();
char* langPropMonColorGhosting();
char* langPropMonEffectsGB();

char* langPropPerfVideoDrvGB();
char* langPropPerfVideoDispDrvText();
char* langPropPerfFrameSkipText();
char* langPropPerfAudioDrvGB();
char* langPropPerfAudioDrvText();
char* langPropPerfAudioBufSzText();
char* langPropPerfEmuGB();
char* langPropPerfSyncModeText();
char* langPropFullscreenResText();

char* langPropSndChipEmuGB();
char* langPropSndMsxMusic();
char* langPropSndMsxAudio();
char* langPropSndMoonsound();
char* langPropSndMt32ToGm();

char* langPropPortsLptGB();
char* langPropPortsComGB();
char* langPropPortsLptText();
char* langPropPortsCom1Text();
char* langPropPortsNone();
char* langPropPortsSimplCovox();
char* langPropPortsFile();
char* langPropPortsComFile();
char* langPropPortsOpenLogFile();
char* langPropPortsEmulateMsxPrn();

char* langPropSetFileHistoryGB();
char* langPropSetFileHistorySize();
char* langPropSetFileHistoryClear();
char* langPropWindowsEnvGB();
char* langPropScreenSaver();
char* langPropFileTypes();
char* langPropDisableWinKeys();
char* langPropPriorityBoost();
char* langPropScreenshotPng();
char* langPropEjectMediaOnExit();
char* langPropClearFileHistory();
char* langPropOpenRomGB();
char* langPropDefaultRomType();
char* langPropGuessRomType();

char* langPropSettDefSlotGB();
char* langPropSettDefSlots();
char* langPropSettDefSlot();
char* langPropSettDefDrives();
char* langPropSettDefDrive();

char* langPropThemeGB();
char* langPropTheme();

char* langPropCdromGB();
char* langPropCdromMethod();
char* langPropCdromMethodNone();
char* langPropCdromMethodIoctl();
char* langPropCdromMethodAspi();
char* langPropCdromDrive();

//----------------------
// Dropdown related lines
//----------------------

char* langEnumVideoMonColor();
char* langEnumVideoMonGrey();
char* langEnumVideoMonGreen();
char* langEnumVideoMonAmber();

char* langEnumVideoTypePAL();
char* langEnumVideoTypeNTSC();

char* langEnumVideoEmuNone();
char* langEnumVideoEmuYc();
char* langEnumVideoEmuMonitor();
char* langEnumVideoEmuYcBlur();
char* langEnumVideoEmuComp();
char* langEnumVideoEmuCompBlur();
char* langEnumVideoEmuScale2x();
char* langEnumVideoEmuHq2x();
char* langEnumVideoEmuStreched();

char* langEnumVideoSize1x();
char* langEnumVideoSize2x();
char* langEnumVideoSizeFullscreen();

char* langEnumVideoDrvDirectDrawHW();
char* langEnumVideoDrvDirectDraw();
char* langEnumVideoDrvGDI();

char* langEnumVideoFrameskip0();
char* langEnumVideoFrameskip1();
char* langEnumVideoFrameskip2();
char* langEnumVideoFrameskip3();
char* langEnumVideoFrameskip4();
char* langEnumVideoFrameskip5();

char* langEnumSoundDrvNone();
char* langEnumSoundDrvWMM();
char* langEnumSoundDrvDirectX();

char* langEnumEmuSync1ms();
char* langEnumEmuSyncAuto();
char* langEnumEmuSyncNone();
char* langEnumEmuSyncVblank();
char* langEnumEmuAsyncVblank();

char* langEnumControlsJoyNone();
char* langEnumControlsJoyTetrisDongle();
char* langEnumControlsJoyMagicKeyDongle();
char* langEnumControlsJoyMouse();
char* langEnumControlsJoy2Button();
char* langEnumControlsJoyGunStick();
char* langEnumControlsJoyAsciiLaser();
char* langEnumControlsJoyArkanoidPad();
char* langEnumControlsJoyColeco();
    
char* langEnumDiskMsx35Dbl9Sect();
char* langEnumDiskMsx35Dbl8Sect();
char* langEnumDiskMsx35Sgl9Sect();
char* langEnumDiskMsx35Sgl8Sect();
char* langEnumDiskSvi525Dbl();
char* langEnumDiskSvi525Sgl();
char* langEnumDiskSf3Sgl();

//----------------------
// Configuration related lines
//----------------------

char* langConfTitle();
char* langConfConfigText();
char* langConfSlotLayout();
char* langConfMemory();
char* langConfChipEmulation();
char* langConfChipExtras();

char* langConfOpenRom();
char* langConfSaveTitle();
char* langConfSaveAsTitle();
char* langConfSaveText();
char* langConfSaveAsMachineName();
char* langConfDiscardTitle();
char* langConfExitSaveTitle();
char* langConfExitSaveText();

char* langConfSlotLayoutGB();
char* langConfSlotExtSlotGB();
char* langConfBoardGB();
char* langConfBoardText();
char* langConfSlotPrimary();
char* langConfSlotExpanded();

char* langConfCartridge();
char* langConfSlot();
char* langConfSubslot();

char* langConfMemAdd();
char* langConfMemEdit();
char* langConfMemRemove();
char* langConfMemSlot();
char* langConfMemAddress();
char* langConfMemType();
char* langConfMemRomImage();

char* langConfChipVideoGB();
char* langConfChipVideoChip();
char* langConfChipVideoRam();
char* langConfChipSoundGB();
char* langConfChipPsgStereoText();

char* langConfCmosGB();
char* langConfCmosEnableText();
char* langConfCmosBatteryText();

char* langConfChipCpuFreqGB();
char* langConfChipZ80FreqText();
char* langConfChipR800FreqText();
char* langConfChipFdcGB();
char* langConfChipFdcNumDrivesText();

char* langConfEditMemTitle();
char* langConfEditMemGB();
char* langConfEditMemType();
char* langConfEditMemFile();
char* langConfEditMemAddress();
char* langConfEditMemSize();
char* langConfEditMemSlot();


//----------------------
// Shortcut lines
//----------------------

char* langShortcutKey();
char* langShortcutDescription();

char* langShortcutSaveConfig();
char* langShortcutOverwriteConfig();
char* langShortcutExitConfig();
char* langShortcutDiscardConfig();
char* langShortcutSaveConfigAs();
char* langShortcutConfigName();
char* langShortcutNewProfile();
char* langShortcutConfigTitle();
char* langShortcutAssign();
char* langShortcutPressText();
char* langShortcutScheme();
char* langShortcutCartInsert1();
char* langShortcutCartRemove1();
char* langShortcutCartInsert2();
char* langShortcutCartRemove2();
char* langShortcutCartSpecialMenu1();
char* langShortcutCartSpecialMenu2();
char* langShortcutCartAutoReset();
char* langShortcutDiskInsertA();
char* langShortcutDiskDirInsertA();
char* langShortcutDiskRemoveA();
char* langShortcutDiskChangeA();
char* langShortcutDiskAutoResetA();
char* langShortcutDiskInsertB();
char* langShortcutDiskDirInsertB();
char* langShortcutDiskRemoveB();
char* langShortcutCasInsert();
char* langShortcutCasEject();
char* langShortcutCasAutorewind();
char* langShortcutCasReadOnly();
char* langShortcutCasSetPosition();
char* langShortcutCasRewind();
char* langShortcutCasSave();
char* langShortcutPrnFormFeed();
char* langShortcutCpuStateLoad();
char* langShortcutCpuStateSave();
char* langShortcutCpuStateQload();
char* langShortcutCpuStateQsave();
char* langShortcutAudioCapture();
char* langShortcutScreenshotOrig();
char* langShortcutScreenshotSmall();
char* langShortcutScreenshotLarge();
char* langShortcutQuit();
char* langShortcutRunPause();
char* langShortcutStop();
char* langShortcutResetHard();
char* langShortcutResetSoft();
char* langShortcutResetClean();
char* langShortcutSizeSmall();
char* langShortcutSizeNormal();
char* langShortcutSizeMinimized();
char* langShortcutSizeFullscreen();
char* langShortcutToggleFullscren();
char* langShortcutVolumeIncrease();
char* langShortcutVolumeDecrease();
char* langShortcutVolumeMute();
char* langShortcutVolumeStereo();
char* langShortcutSwitchMsxAudio();
char* langShortcutSwitchFront();
char* langShortcutSwitchPause();
char* langShortcutToggleMouseLock();
char* langShortcutEmuSpeedMax();
char* langShortcutEmuPlayReverse();
char* langShortcutEmuSpeedMaxToggle();
char* langShortcutEmuSpeedNormal();
char* langShortcutEmuSpeedInc();
char* langShortcutEmuSpeedDec();
char* langShortcutThemeSwitch();
char* langShortcutShowEmuProp();
char* langShortcutShowVideoProp();
char* langShortcutShowAudioProp();
char* langShortcutShowCtrlProp();
char* langShortcutShowPerfProp();
char* langShortcutShowSettProp();
char* langShortcutShowPorts();
char* langShortcutShowLanguage();
char* langShortcutShowMachines();
char* langShortcutShowShortcuts();
char* langShortcutShowKeyboard();
char* langShortcutShowMixer();
char* langShortcutShowDebugger();
char* langShortcutShowTrainer();
char* langShortcutShowHelp();
char* langShortcutShowAbout();
char* langShortcutShowFiles();
char* langShortcutToggleSpriteEnable();
char* langShortcutToggleFdcTiming();
char* langShortcutToggleCpuTrace();
char* langShortcutVideoLoad();
char* langShortcutVideoPlay();
char* langShortcutVideoRecord();
char* langShortcutVideoStop();
char* langShortcutVideoRender();


//----------------------
// Keyboard config lines
//----------------------

char* langKeyconfigSelectedKey();
char* langKeyconfigMappedTo();
char* langKeyconfigMappingScheme();


//----------------------
// Rom type lines
//----------------------

char* langRomTypeStandard();
char* langRomTypeMsxdos2();
char* langRomTypeKonamiScc();
char* langRomTypeManbow2();
char* langRomTypeMegaFlashRomScc();
char* langRomTypeKonami();
char* langRomTypeAscii8();
char* langRomTypeAscii16();
char* langRomTypeGameMaster2();
char* langRomTypeAscii8Sram();
char* langRomTypeAscii16Sram();
char* langRomTypeRtype();
char* langRomTypeCrossblaim();
char* langRomTypeHarryFox();
char* langRomTypeMajutsushi();
char* langRomTypeZenima80();
char* langRomTypeZenima90();
char* langRomTypeZenima126();
char* langRomTypeScc();
char* langRomTypeSccPlus();
char* langRomTypeSnatcher();
char* langRomTypeSdSnatcher();
char* langRomTypeSccMirrored();
char* langRomTypeSccExtended();
char* langRomTypeFmpac();
char* langRomTypeFmpak();
char* langRomTypeKonamiGeneric();
char* langRomTypeSuperPierrot();
char* langRomTypeMirrored();
char* langRomTypeNormal();
char* langRomTypeDiskPatch();
char* langRomTypeCasPatch();
char* langRomTypeTc8566afFdc();
char* langRomTypeTc8566afTrFdc();
char* langRomTypeMicrosolFdc();
char* langRomTypeNationalFdc();
char* langRomTypePhilipsFdc();
char* langRomTypeSvi738Fdc();
char* langRomTypeMappedRam();
char* langRomTypeMirroredRam1k();
char* langRomTypeMirroredRam2k();
char* langRomTypeNormalRam();
char* langRomTypeKanji();
char* langRomTypeHolyQuran();
char* langRomTypeMatsushitaSram();
char* langRomTypeMasushitaSramInv();
char* langRomTypePanasonic8();
char* langRomTypePanasonicWx16();
char* langRomTypePanasonic16();
char* langRomTypePanasonic32();
char* langRomTypePanasonicModem();
char* langRomTypeDram();
char* langRomTypeBunsetsu();
char* langRomTypeJisyo();
char* langRomTypeKanji12();
char* langRomTypeNationalSram();
char* langRomTypeS1985();
char* langRomTypeS1990();
char* langRomTypeTurborPause();
char* langRomTypeF4deviceNormal();
char* langRomTypeF4deviceInvert();
char* langRomTypeMsxMidi();
char* langRomTypeTurborTimer();
char* langRomTypeKoei();
char* langRomTypeBasic();
char* langRomTypeHalnote();
char* langRomTypeLodeRunner();
char* langRomTypeNormal4000();
char* langRomTypeNormalC000();
char* langRomTypeKonamiSynth();
char* langRomTypeKonamiKbdMast();
char* langRomTypeKonamiWordPro();
char* langRomTypePac();
char* langRomTypeMegaRam();
char* langRomTypeMegaRam128();
char* langRomTypeMegaRam256();
char* langRomTypeMegaRam512();
char* langRomTypeMegaRam768();
char* langRomTypeMegaRam2mb();
char* langRomTypeExtRam();
char* langRomTypeExtRam16();
char* langRomTypeExtRam32();
char* langRomTypeExtRam48();
char* langRomTypeExtRam64();
char* langRomTypeExtRam512();
char* langRomTypeExtRam1mb();
char* langRomTypeExtRam2mb();
char* langRomTypeExtRam4mb();
char* langRomTypeMsxMusic();
char* langRomTypeMsxAudio();
char* langRomTypeY8950();
char* langRomTypeMoonsound();
char* langRomTypeSvi328Cart();
char* langRomTypeSvi328Fdc();
char* langRomTypeSvi328Prn();
char* langRomTypeSvi328Uart();
char* langRomTypeSvi328col80();
char* langRomTypeSvi328RsIde();
char* langRomTypeSvi727col80();
char* langRomTypeColecoCart();
char* langRomTypeSg1000Cart();
char* langRomTypeSc3000Cart();
char* langRomTypeTheCastle();
char* langRomTypeSegaBasic();
char* langRomTypeSonyHbi55();
char* langRomTypeMsxPrinter();
char* langRomTypeTurborPcm();
char* langRomTypeGameReader();
char* langRomTypeSunriseIde();
char* langRomTypeBeerIde();
char* langRomTypeGide();
char* langRomTypeVmx80();
char* langRomTypeNms8280Digitiz();
char* langRomTypeHbiV1Digitiz();
char* langRomTypePlayBall();
char* langRomTypeFmdas();
char* langRomTypeSfg01();
char* langRomTypeSfg05();
char* langRomTypeObsonet();
char* langRomTypeDumas();
char* langRomTypeNoWind();
char* langRomTypeMegaSCSI();
char* langRomTypeMegaSCSI128();
char* langRomTypeMegaSCSI256();
char* langRomTypeMegaSCSI512();
char* langRomTypeMegaSCSI1mb();
char* langRomTypeEseRam();
char* langRomTypeEseRam128();
char* langRomTypeEseRam256();
char* langRomTypeEseRam512();
char* langRomTypeEseRam1mb();
char* langRomTypeWaveSCSI();
char* langRomTypeWaveSCSI128();
char* langRomTypeWaveSCSI256();
char* langRomTypeWaveSCSI512();
char* langRomTypeWaveSCSI1mb();
char* langRomTypeEseSCC();
char* langRomTypeEseSCC128();
char* langRomTypeEseSCC256();
char* langRomTypeEseSCC512();
char* langRomTypeGoudaSCSI();

//----------------------
// Debug type lines
//----------------------

char* langDbgMemVisible();
char* langDbgMemRamNormal();
char* langDbgMemRamMapped();
char* langDbgMemVram();
char* langDbgMemYmf278();
char* langDbgMemAy8950();
char* langDbgMemScc();

char* langDbgCallstack();

char* langDbgRegs();
char* langDbgRegsCpu();
char* langDbgRegsYmf262();
char* langDbgRegsYmf278();
char* langDbgRegsAy8950();
char* langDbgRegsYm2413();

char* langDbgDevRamMapper();
char* langDbgDevRam();
char* langDbgDevIdeBeer();
char* langDbgDevIdeGide();
char* langDbgDevIdeSviRs();
char* langDbgDevScsiGouda();
char* langDbgDevF4Device();
char* langDbgDevFmpac();
char* langDbgDevFmpak();
char* langDbgDevKanji();
char* langDbgDevKanji12();
char* langDbgDevKonamiKbd();
char* langDbgDevKorean80();
char* langDbgDevKorean90();
char* langDbgDevKorean128();
char* langDbgDevMegaRam();
char* langDbgDevFdcMicrosol();
char* langDbgDevMoonsound();
char* langDbgDevMsxAudio();
char* langDbgDevMsxAudioMidi();
char* langDbgDevMsxMusic();
char* langDbgDevPrinter();
char* langDbgDevRs232();
char* langDbgDevS1990();
char* langDbgDevSfg05();
char* langDbgDevHbi55();
char* langDbgDevSviFdc();
char* langDbgDevSviPrn();
char* langDbgDevSvi80Col();
char* langDbgDevPcm();
char* langDbgDevMatsushita();
char* langDbgDevS1985();
char* langDbgDevCrtc6845();
char* langDbgDevTms9929A();
char* langDbgDevTms99x8A();
char* langDbgDevV9938();
char* langDbgDevV9958();
char* langDbgDevZ80();
char* langDbgDevMsxMidi();
char* langDbgDevPpi();
char* langDbgDevRtc();
char* langDbgDevTrPause();
char* langDbgDevAy8910();
char* langDbgDevScc();


//----------------------
// Debug type lines
// Note: Can only be translated to european languages
//----------------------
char* langAboutScrollThanksTo();
char* langAboutScrollAndYou();

#endif

