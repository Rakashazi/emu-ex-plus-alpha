/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageDutch.h,v $
**
** $Revision: 1.61 $
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
#ifndef LANGUAGE_DUTCH_H
#define LANGUAGE_DUTCH_H

#include "LanguageStrings.h"

void langInitDutch(LanguageStrings* ls)
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalaans";
    ls->langChineseSimplified   = "Chinees Simpel";
    ls->langChineseTraditional  = "Chinees Traditioneel";
    ls->langDutch               = "Nederlands";
    ls->langEnglish             = "Engels";
    ls->langFinnish             = "Fins";
    ls->langFrench              = "Frans";
    ls->langGerman              = "Duits";
    ls->langItalian             = "Italiaans";
    ls->langJapanese            = "Japans";
    ls->langKorean              = "Koreaans";
    ls->langPolish              = "Pools";
    ls->langPortuguese          = "Portugees";
    ls->langRussian             = "Russisch";
    ls->langSpanish             = "Spaans";
    ls->langSwedish             = "Zweeds";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "Apparaat:";
    ls->textFilename            = "Bestandsnaam:";
    ls->textFile                = "Bestand";
    ls->textNone                = "Geen";
    ls->textUnknown             = "Onbekend";


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle            = "blueMSX - Waarschuwing";
    ls->warningDiscardChanges   = "Wilt u de wijzigingen ongedaan maken?";
    ls->warningOverwriteFile    = "Wilt u het volgende bestand overschrijven?";
    ls->errorTitle              = "blueMSX - Foutmelding";
    ls->errorEnterFullscreen    = "Het is niet gelukt om over te schakelen naar de volledige schermmodus.          \n";
    ls->errorDirectXFailed      = "Het is niet gelukt om DirectX te initialiseren. \nDe GDI wordt nu gebruikt.\nKijk de videoinstellingen na...";
    ls->errorNoRomInZip         = "Er zijn geen .ROM-bestanden gevonden in het ZIP-bestand.";
    ls->errorNoDskInZip         = "Er zijn geen .DSK-bestanden gevonden in het ZIP-bestand.";
    ls->errorNoCasInZip         = "Er zijn geen .CAS-bestanden gevonden in het ZIP-bestand.";
    ls->errorNoHelp             = "Het blueMSX helpbestand is niet gevonden.";
    ls->errorStartEmu           = "Het is niet gelukt om de MSX emulator te starten.";
    ls->errorPortableReadonly   = "Het draagbare apparaat is Alleen-Lezen";


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "ROM bestand";
    ls->fileAll                 = "Alle bestanden";
    ls->fileCpuState            = "CPU-status";
    ls->fileVideoCapture        = "Videoopname";
    ls->fileDisk                = "Diskettebestand";
    ls->fileCas                 = "Cassettebestand";
    ls->fileAvi                 = "Videobestand";


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- Geen recente bestanden -";
    ls->menuInsert              = "Invoeren";
    ls->menuEject               = "Uitwerpen";

    ls->menuCartGameReader      = "Game Reader";
    ls->menuCartIde             = "IDE";
    ls->menuCartBeerIde         = "Beer";
    ls->menuCartGIde            = "GIDE";
    ls->menuCartSunriseIde      = "Sunrise";  
    ls->menuCartScsi            = "SCSI";
    ls->menuCartMegaSCSI        = "MEGA-SCSI";
    ls->menuCartWaveSCSI        = "WAVE-SCSI";
    ls->menuCartGoudaSCSI       = "Gouda SCSI";
    ls->menuJoyrexPsg           = "Joyrex PSG Cartridge"; // New in 2.9
    ls->menuCartSCC             = "SCC Cartridge";
    ls->menuCartSCCPlus         = "SCC-I Cartridge";
    ls->menuCartFMPac           = "FM-PAC Cartridge";
    ls->menuCartPac             = "PAC Cartridge";
    ls->menuCartHBI55           = "Sony HBI-55 Cartridge";
    ls->menuCartInsertSpecial   = "Voer Speciale Cartridge in";
    ls->menuCartMegaRam         = "MegaRAM";
    ls->menuCartExternalRam     = "Extern RAM-geheugen";
    ls->menuCartEseRam          = "Ese-RAM";
    ls->menuCartEseSCC          = "Ese-SCC";
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";

    ls->menuDiskInsertNew       = "Voer Nieuw Diskettebestand in";
    ls->menuDiskInsertCdrom     = "Voer CD-ROM in";
    ls->menuDiskDirInsert       = "Voer Map in";
    ls->menuDiskAutoStart       = "Herstart na invoeren";
    ls->menuCartAutoReset       = "Herstart na invoeren/uitwerpen";

    ls->menuCasRewindAfterInsert= "Terugspoelen na invoeren";
    ls->menuCasUseReadOnly      = "Gebruik Cassettebestand als Alleen-Lezen";
    ls->lmenuCasSaveAs          = "Cassettebestand opslaan als...";
    ls->menuCasSetPosition      = "Bepaal positie";
    ls->menuCasRewind           = "Terugspoelen cassette";

    ls->menuVideoLoad           = "Laden...";
    ls->menuVideoPlay           = "Laatste opname afspelen";
    ls->menuVideoRecord         = "Opnemen";
    ls->menuVideoRecording      = "Bezig met opnemen";
    ls->menuVideoRecAppend      = "Opnemen (invoeren)";
    ls->menuVideoStop           = "Stoppen";
    ls->menuVideoRender         = "Maak Videobestand";

    ls->menuPrnFormfeed         = "Paginadoorvoer";

    ls->menuZoomNormal          = "Normale grootte";
    ls->menuZoomDouble          = "Dubbele grootte";
    ls->menuZoomFullscreen      = "Volledig scherm";

    ls->menuPropsEmulation      = "Emulatie";
    ls->menuPropsVideo          = "Beeld";
    ls->menuPropsSound          = "Geluid";
    ls->menuPropsControls       = "Besturing";
    ls->menuPropsPerformance    = "Prestatie";
    ls->menuPropsSettings       = "Instellingen";
    ls->menuPropsFile           = "Bestand";
    ls->menuPropsDisk           = "Diskettes";
    ls->menuPropsLanguage       = "Taal";
    ls->menuPropsPorts          = "Poorten";

    ls->menuVideoSource         = "Video Uit-bron";
    ls->menuVideoSourceDefault  = "Geen Video Uit-bron verbonden";
    ls->menuVideoChipAutodetect = "Automatische detectie Videochip";
    ls->menuVideoInSource       = "Video In-bron";
    ls->menuVideoInBitmap       = "Afbeedingsbestand";

    ls->menuEthInterface        = "Ethernet koppeling";

    ls->menuHelpHelp            = "Help";
    ls->menuHelpAbout           = "Over blueMSX";

    ls->menuFileCart            = "Cartridgeslot";
    ls->menuFileDisk            = "Diskettestation";
    ls->menuFileCas             = "Cassette";
    ls->menuFilePrn             = "Printer";
    ls->menuFileLoadState       = "Laad CPU-status";
    ls->menuFileSaveState       = "Opslaan CPU-status";
    ls->menuFileQLoadState      = "Snel laden CPU-status";
    ls->menuFileQSaveState      = "Snel opslaan CPU-status";
    ls->menuFileCaptureAudio    = "Opslaan Geluid";
    ls->menuFileCaptureVideo    = "Opslaan Video";
    ls->menuFileScreenShot      = "Schermafdruk maken";
    ls->menuFileExit            = "Afsluiten";

    ls->menuFileHarddisk        = "Harde schijf";
    ls->menuFileHarddiskNoPesent= "Geen harde schijfcontrollers aanwezig";
    ls->menuFileHarddiskRemoveAll= "Werp alle harde schijven uit";

    ls->menuRunRun              = "Start";
    ls->menuRunPause            = "Pauze";
    ls->menuRunStop             = "Stop";
    ls->menuRunSoftReset        = "Zachte Herstart";
    ls->menuRunHardReset        = "Harde Herstart";
    ls->menuRunCleanReset       = "Algemene Herstart";

    ls->menuToolsMachine        = "Machineconfiguratie";
    ls->menuToolsShortcuts      = "Snelkoppelingen";
    ls->menuToolsCtrlEditor     = "Besturing";
    ls->menuToolsMixer          = "Geluidsmixer";
    ls->menuToolsDebugger       = "Debugger";
    ls->menuToolsTrainer        = "Trainer";
    ls->menuToolsTraceLogger    = "Trace Logger";

    ls->menuFile                = "Bestand";
    ls->menuRun                 = "Emulatie";
    ls->menuWindow              = "Scherm";
    ls->menuOptions             = "Opties";
    ls->menuTools               = "Extra";
    ls->menuHelp                = "Help";


    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "OK";
    ls->dlgOpen                 = "Openen";
    ls->dlgCancel               = "Annuleren";
    ls->dlgSave                 = "Opslaan";
    ls->dlgSaveAs               = "Opslaan als...";
    ls->dlgRun                  = "Start";
    ls->dlgClose                = "Afsluiten";

    ls->dlgLoadRom              = "blueMSX - Selecteer een ROM om te laden";
    ls->dlgLoadDsk              = "blueMSX - Selecteer een DSK om te laden";
    ls->dlgLoadCas              = "blueMSX - Selecteer een CAS om te laden";
    ls->dlgLoadRomDskCas        = "blueMSX - Selecteer een ROM, DSK, of CAS-bestand om te laden";
    ls->dlgLoadRomDesc          = "Selecteer een ROM om te laden:";
    ls->dlgLoadDskDesc          = "Selecteer een diskette om te laden:";
    ls->dlgLoadCasDesc          = "Selecteer een cassette om te laden:";
    ls->dlgLoadRomDskCasDesc    = "Selecteer een cartridge, diskette of cassette om te laden:";
    ls->dlgLoadState            = "Laad CPU-status";
    ls->dlgLoadVideoCapture     = "Laad Video-opname";
    ls->dlgSaveState            = "Opslaan CPU-status";
    ls->dlgSaveCassette         = "blueMSX - Opslaan Cassettebestand";
    ls->dlgSaveVideoClipAs      = "Videoclip opslaan als...";
    ls->dlgAmountCompleted      = "Aantal gereed:";
    ls->dlgInsertRom1           = "Voer ROM-cartridge in Slot 1";
    ls->dlgInsertRom2           = "Voer ROM-cartridge in Slot 2";
    ls->dlgInsertDiskA          = "Voer diskettebestand in Station A";
    ls->dlgInsertDiskB          = "Voer diskettebestand in Station B";
    ls->dlgInsertHarddisk       = "Voer harde schijf in";
    ls->dlgInsertCas            = "Voer cassettebestand in casettespeler";
    ls->dlgRomType              = "ROM Type:";
    ls->dlgDiskSize             = "Diskomvang:";

    ls->dlgTapeTitle            = "blueMSX - Cassettepositie";
    ls->dlgTapeFrameText        = "Cassettepositie";
    ls->dlgTapeCurrentPos       = "Huidige positie";
    ls->dlgTapeTotalTime        = "Totale tijd";
    ls->dlgTapeCustom           = "Laat Speciale Bestanden zien";
    ls->dlgTapeSetPosText       = "Cassettepositie:";
    ls->dlgTabPosition          = "Positie";
    ls->dlgTabType              = "Type";
    ls->dlgTabFilename          = "Bestandsnaam";
    ls->dlgZipReset             = "Herstart na invoeren";

    ls->dlgAboutTitle           = "Over blueMSX";

    ls->dlgLangLangText         = "Kies de taal die blueMSX moet gebruiken";
    ls->dlgLangLangTitle        = "blueMSX - Taal";

    ls->dlgAboutAbout           = "Info\r\n===";
    ls->dlgAboutVersion         = "Versie:";
    ls->dlgAboutBuildNumber     = "Gemaakt:";
    ls->dlgAboutBuildDate       = "Datum:";
    ls->dlgAboutCreat           = "Gemaakt door Daniel Vik";
    ls->dlgAboutDevel           = "ONTWIKKELAARS\r\n==========";
    ls->dlgAboutThanks          = "MET DANK AAN\r\n=========";
    ls->dlgAboutLisence         = "LICENTIE\r\n"
                                  "======\r\n\r\n"
                                  "Deze software wordt gegeven 'as-is', zonder enige vorm van garantie. "
                                  "De auteur(s) is/zijn niet verantwoordelijk voor elke vorm van schade "
                                  "die onstaat door het gebruik van deze software.\r\n\r\n"
                                  "Bezoek www.bluemsx.com voor meer details.";

    ls->dlgSavePreview          = "Voorbeeld weergeven";
    ls->dlgSaveDate             = "Opgeslagen op:";

    ls->dlgRenderVideoCapture   = "blueMSX - Videoclip wordt gemaakt...";


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - Eigenschappen";
    ls->propEmulation           = "Emulatie";
    ls->propVideo               = "Beeld";
    ls->propSound               = "Geluid";
    ls->propControls            = "Besturing";
    ls->propPerformance         = "Prestaties";
    ls->propSettings            = "Instellingen";
    ls->propFile                = "Bestand";
    ls->propDisk                = "Diskettes";
    ls->propPorts               = "Poorten";

    ls->propEmuGeneralGB        = "Algemeen ";
    ls->propEmuFamilyText       = "MSX-familie:";
    ls->propEmuMemoryGB         = "Geheugen ";
    ls->propEmuRamSizeText      = "RAM-grootte:";
    ls->propEmuVramSizeText     = "VRAM-grootte:";
    ls->propEmuSpeedGB          = "Emulatiesnelheid ";
    ls->propEmuSpeedText        = "Emulatiesnelheid:";
    ls->propEmuFrontSwitchGB    = "Panasonic schakelaars ";
    ls->propEmuFrontSwitch      = " Voorpanel Schakelaar";
    ls->propEmuFdcTiming        = " Diskettestation niet synchroniseren";
    ls->propEmuReversePlay      = " Enable reverse playback"; // New in 2.8.3
    ls->propEmuPauseSwitch      = " Pauze Schakelaar";
    ls->propEmuAudioSwitch      = " MSX-AUDIO cartridge Schakelaar";
    ls->propVideoFreqText       = "Videofrequentie:";
    ls->propVideoFreqAuto       = "Automatisch";
    ls->propSndOversampleText   = "Oversample:";
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 In ";
    ls->propSndMidiInGB         = "MIDI In ";
    ls->propSndMidiOutGB        = "MIDI Out ";
    ls->propSndMidiChannel      = "MIDI-kanaal:";
    ls->propSndMidiAll          = "Alle";

    ls->propMonMonGB            = "Monitor ";
    ls->propMonTypeText         = "Monitortype:";
    ls->propMonEmuText          = "Monitoremulatie:";
    ls->propVideoTypeText       = "Beeldtype:";
    ls->propWindowSizeText      = "Schermgrootte:";
    ls->propMonHorizStretch     = " Horizontaal uitrekken";
    ls->propMonVertStretch      = " Verticaal uitrekken";
    ls->propMonDeInterlace      = " Deïnterlace";
    ls->propBlendFrames         = " Opeenvolgende frames mengen";
    ls->propMonBrightness       = "Helderheid:";
    ls->propMonContrast         = "Contrast:";
    ls->propMonSaturation       = "Verzadiging:";
    ls->propMonGamma            = "Gamma:";
    ls->propMonScanlines        = " Beeldlijnen:";
    ls->propMonColorGhosting    = " RF-modulator:";
    ls->propMonEffectsGB        = "Effecten ";

    ls->propPerfVideoDrvGB      = "Beeldinstellingen ";
    ls->propPerfVideoDispDrvText= "Stuurprogramma:";
    ls->propPerfFrameSkipText   = "Frames overslaan:";
    ls->propPerfAudioDrvGB      = "Geluidsinstellingen ";
    ls->propPerfAudioDrvText    = "Stuurprogramma:";
    ls->propPerfAudioBufSzText  = "Buffergrootte:";
    ls->propPerfEmuGB           = "Emulatie ";
    ls->propPerfSyncModeText    = "Synchronisatiemodus";
    ls->propFullscreenResText   = "Resolutie volledig scherm:";

    ls->propSndChipEmuGB        = "Emulatie geluidschip ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound        = " Moonsound";
    ls->propSndMt32ToGm         = " Map MT-32-instrumenten naar General MIDI-indeling";

    ls->propPortsLptGB          = "Parallelle poort ";
    ls->propPortsComGB          = "Seriele poorten ";
    ls->propPortsLptText        = "Poort:";
    ls->propPortsCom1Text       = "Poort 1:";
    ls->propPortsNone           = "Geen";
    ls->propPortsSimplCovox     = "SiMPL / Covox DAC";
    ls->propPortsFile           = "Afdrukken naar bestand";
    ls->propPortsComFile        = "Stuur naar bestand";
    ls->propPortsOpenLogFile    = "Logbestand openen";
    ls->propPortsEmulateMsxPrn  = "Emulatie:";

    ls->propSetFileHistoryGB     = "Bestandsgeschiedenis ";
    ls->propSetFileHistorySize   = "Aantal bestanden in geschiedenis:";
    ls->propSetFileHistoryClear  = "Leegmaken Geschiedenis";
    ls->propFileTypes            = " Bestanden registreren bij blueMSX (.ROM, .DSK, .CAS, .STA)";
    ls->propWindowsEnvGB         = "Windows Omgeving ";
    ls->propSetScreenSaver       = " Schermbeveiliging uitschakelen als blueMSX draait";
    ls->propDisableWinKeys       = " Automatische MSX-functie voor menutoetsen Windows";
    ls->propPriorityBoost       = " blueMSX een hogere prioriteit geven";
    ls->propScreenshotPng       = " Gebruik PNG formaat in plaats van BMP bij schermafdrukken";
    ls->propEjectMediaOnExit    = " Werp alle media uit bij afsluiten";
    ls->propClearHistory         = "Weet u zeker dat u de bestandsgeschiedenis wilt wissen?";
    ls->propOpenRomGB           = "Openen ROM bestand";
    ls->propDefaultRomType      = "Standaardtype:";
    ls->propGuessRomType        = "Raden type";

    ls->propSettDefSlotGB       = "Slepen en neerzetten ";
    ls->propSettDefSlots        = "Voer cartridge in:";
    ls->propSettDefSlot         = " Slot";
    ls->propSettDefDrives       = "Voer diskette in:";
    ls->propSettDefDrive        = " Station";

    ls->propThemeGB             = "Thema ";
    ls->propTheme               = "Thema";

    ls->propCdromGB             = "CD-ROM ";
    ls->propCdromMethod         = "Leesmethode:";
    ls->propCdromMethodNone     = "Geen";
    ls->propCdromMethodIoctl    = "IOCTL";
    ls->propCdromMethodAspi     = "ASPI";
    ls->propCdromDrive          = "Station:";


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "Kleur";
    ls->enumVideoMonGrey        = "Zwart-Wit";
    ls->enumVideoMonGreen       = "Groen";
    ls->enumVideoMonAmber       = "Amber";

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "Geen";
    ls->enumVideoEmuYc          = "Y/C-kabel (Scherp)";
    ls->enumVideoEmuMonitor     = "Monitor";
    ls->enumVideoEmuYcBlur      = "Ruis Y/C-kabel (Scherp)";
    ls->enumVideoEmuComp        = "Compositie (Wazig)";
    ls->enumVideoEmuCompBlur    = "Ruis Compositie (Wazig)";
    ls->enumVideoEmuScale2x     = "Schaal 2x";
    ls->enumVideoEmuHq2x        = "Hq2x";

    ls->enumVideoSize1x         = "Normaal - 320x200";
    ls->enumVideoSize2x         = "Dubbel - 640x400";
    ls->enumVideoSizeFullscreen = "Volledig Scherm";

    ls->enumVideoDrvDirectDrawHW = "DirectDraw HW accel.";
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "Geen";
    ls->enumVideoFrameskip1     = "1 frame";
    ls->enumVideoFrameskip2     = "2 frames";
    ls->enumVideoFrameskip3     = "3 frames";
    ls->enumVideoFrameskip4     = "4 frames";
    ls->enumVideoFrameskip5     = "5 frames";

    ls->enumSoundDrvNone        = "Geen Geluid";
    ls->enumSoundDrvWMM         = "WMM-stuurprogramma";
    ls->enumSoundDrvDirectX     = "DirectX-stuurprogramma";

    ls->enumEmuSync1ms          = "Synchroniseren tijdens MSX refresh";
    ls->enumEmuSyncAuto         = "Automatisch (snel)";
    ls->enumEmuSyncNone         = "Geen";
    ls->enumEmuSyncVblank       = "Synchroon met PC Vertical Blank";
    ls->enumEmuAsyncVblank      = "Asynchroon met PC Vblank";

    ls->enumControlsJoyNone            = "Geen";
    ls->enumControlsJoyMouse           = "Muis";
    ls->enumControlsJoyTetris2Dongle   = "Tetris 2-dongle";
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey-dongle";
    ls->enumControlsJoy2Button         = "Joystick (2 knoppen)";
    ls->enumControlsJoyGunstick        = "Gun Stick";
    ls->enumControlsJoyAsciiLaser      = "ASCII Plus-X Terminator Laser";
    ls->enumControlsArkanoidPad        = "Arkanoid Pad";
    ls->enumControlsJoyColeco          = "ColecoVision Joystick";

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" Dubbelzijdig, 9 Sectoren";
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" Dubbelzijdig, 8 Sectoren";
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" Enkelzijdig, 9 Sectoren";
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" Enkelzijdig, 8 Sectoren";
    ls->enumDiskSvi525Dbl        = "SVI-328 5.25\" Dubbelzijdig";
    ls->enumDiskSvi525Sgl        = "SVI-328 5.25\" Enkelzijdig";
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" Enkelzijdig";


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle               = "blueMSX - Machineconfiguratie aanpassen";
    ls->confConfigText          = "Configuratie:";
    ls->confSlotLayout          = "Slotinstellingen";
    ls->confMemory              = "Geheugen";
    ls->confChipEmulation       = "Chipemulatie";
    ls->confChipExtras          = "Extra's";

    ls->confOpenRom             = "Open ROM bestand";
    ls->confSaveTitle           = "blueMSX - Machineconfiguratie opslaan";
    ls->confSaveText            = "Deze machine configuratie vervangen? :";
    ls->confSaveAsTitle         = "Configuratie opslaan als...";
    ls->confSaveAsMachineName   = "Configuratienaam:";
    ls->confDiscardTitle        = "blueMSX - Configuratie";
    ls->confExitSaveTitle       = "blueMSX - Machineconfiguratie afsluiten";
    ls->confExitSaveText        = "De gemaakte wijzigingen in de huidige configuratie worden niet opgeslagen. Wilt u doorgaan?";

    ls->confSlotLayoutGB        = "Slotinstellingen ";
    ls->confSlotExtSlotGB       = "Externe Slots ";
    ls->confBoardGB             = "Systeem ";
    ls->confBoardText           = "Systeemtype:";
    ls->confSlotPrimary         = "Primair";
    ls->confSlotExpanded        = "Uitgebreid (vier sub-slots)";

    ls->confSlotCart            = "Cartridge";
    ls->confSlot                = "Slot";
    ls->confSubslot             = "Subslot";

    ls->confMemAdd              = "Invoeren...";
    ls->confMemEdit             = "Aanpassen...";
    ls->confMemRemove           = "Uitwerpen";
    ls->confMemSlot             = "Slot";
    ls->confMemAddresss         = "Adres";
    ls->confMemType             = "Type";
    ls->confMemRomImage         = "ROM bestand";

    ls->confChipVideoGB         = "Weergave ";
    ls->confChipVideoChip       = "Videochip:";
    ls->confChipVideoRam        = "VideoRAM:";
    ls->confChipSoundGB         = "Geluid ";
    ls->confChipPsgStereoText    = " PSG Stereo";

    ls->confCmosGB              = "CMOS ";
    ls->confCmosEnable          = " Aanzetten CMOS";
    ls->confCmosBattery         = " Geladen batterij gebruiken";

    ls->confCpuFreqGB           = "CPU-frequentie ";
    ls->confZ80FreqText         = "Z80-frequentie:";
    ls->confR800FreqText        = "R800-frequentie:";
    ls->confFdcGB               = "Diskettecontroller ";
    ls->confCFdcNumDrivesText   = "Aantal stations:";

    ls->confEditMemTitle        = "blueMSX - Aanpassen Mapper";
    ls->confEditMemGB           = "Mapperdetails ";
    ls->confEditMemType         = "Type:";
    ls->confEditMemFile         = "Bestand:";
    ls->confEditMemAddress      = "Addres:";
    ls->confEditMemSize         = "Grootte:";
    ls->confEditMemSlot         = "Slot:";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "Sneltoets";
    ls->shortcutDescription     = "Snelkoppeling";

    ls->shortcutSaveConfig      = "blueMSX - Configuratie opslaan";
    ls->shortcutOverwriteConfig = "Huidige configuratie overschrijven?:";
    ls->shortcutExitConfig      = "blueMSX - Snelkoppelingconfiguratie afsluiten";
    ls->shortcutDiscardConfig   = "Weet u zeker dat u de gemaakte wijzigingen in de huidige configuratie niet wilt toepassen?";
    ls->shortcutSaveConfigAs    = "blueMSX - Configuratie opslaan als...";
    ls->shortcutConfigName      = "Snelkoppelingsnaam:";
    ls->shortcutNewProfile      = "< Nieuw Profiel >";
    ls->shortcutConfigTitle     = "blueMSX - Snelkoppelingconfiguratie";
    ls->shortcutAssign          = "Toewijzen";
    ls->shortcutPressText       = "Snelkoppelingstoets(en):";
    ls->shortcutScheme          = "Schema:";
    ls->shortcutCartInsert1     = "Voer Cartridge in Slot 1";
    ls->shortcutCartRemove1     = "Werp Cartridge uit Slot 1";
    ls->shortcutCartInsert2     = "Voer Cartridge in Slot 2";
    ls->shortcutCartRemove2     = "Werp Cartridge uit Slot 2";
    ls->shortcutSpecialMenu1    = "Toon Speciaal menu voor ROM in Slot 1";
    ls->shortcutSpecialMenu2    = "Toon Speciaal menu voor ROM in Slot 2";
    ls->shortcutCartAutoReset   = "Herstart na invoeren cartridge";
    ls->shortcutDiskInsertA     = "Voeren Diskette in Station A";
    ls->shortcutDiskDirInsertA  = "Invoeren Map als Diskette A";
    ls->shortcutDiskRemoveA     = "Werp Diskette uit Station A";
    ls->shortcutDiskChangeA     = "Snel veranderen van diskette in Station A";
    ls->shortcutDiskAutoResetA  = "Herstart na invoeren diskette in Station A";
    ls->shortcutDiskInsertB     = "Voer Diskette in Station B";
    ls->shortcutDiskDirInsertB  = "Invoeren Map als Diskette B";
    ls->shortcutDiskRemoveB     = "Werp Diskette uit Station B";
    ls->shortcutCasInsert       = "Voer Cassette in";
    ls->shortcutCasEject        = "Werp Cassette uit";
    ls->shortcutCasAutorewind   = "Automatisch cassette terugspoelen ja/nee";
    ls->shortcutCasReadOnly     = "Alleen-Lezen cassettebestand ja/nee";
    ls->shortcutCasSetPosition  = "Bepalen Cassettepositie";
    ls->shortcutCasRewind       = "Terugspoelen Cassette";
    ls->shortcutCasSave         = "Opslaan Cassettebestand";
    ls->shortcutPrnFormFeed     = "Printer paginadoorvoer";
    ls->shortcutCpuStateLoad    = "Laden CPU-status";
    ls->shortcutCpuStateSave    = "Opslaan CPU-status";
    ls->shortcutCpuStateQload   = "Snel laden CPU-status";
    ls->shortcutCpuStateQsave   = "Snel opslaan CPU-status";
    ls->shortcutAudioCapture    = "Start/stop het audio opslaan";
    ls->shortcutScreenshotOrig  = "Opslaan schermafdruk";
    ls->shortcutScreenshotSmall = "Opslaan klein ongefilterde schermafdruk";
    ls->shortcutScreenshotLarge = "Opslaan groot ongefilterde schermafdruk";
    ls->shortcutQuit            = "Afsluiten blueMSX";
    ls->shortcutRunPause        = "Starten/Pauseren emulatie";
    ls->shortcutStop            = "Stoppen emulatie";
    ls->shortcutResetHard       = "Harde Herstart";
    ls->shortcutResetSoft       = "Zachte Herstart";
    ls->shortcutResetClean      = "Algemene Herstart";
    ls->shortcutSizeSmall       = "Kiezen normale grootte voor het scherm";
    ls->shortcutSizeNormal      = "Kiezen dubbele grootte voor het scherm";
    ls->shortcutSizeFullscreen  = "Kiezen volledig scherm";
    ls->shortcutSizeMinimized   = "Minimaliseer scherm";
    ls->shortcutToggleFullscren = "Kiezen volledig scherm of onvolledig scherm";
    ls->shortcutVolumeIncrease  = "Geluidsterkte verhogen";
    ls->shortcutVolumeDecrease  = "Geluidsterkte verminderen";
    ls->shortcutVolumeMute      = "Geluid uitzetten";
    ls->shortcutVolumeStereo    = "Wisselen mono of stereo mode";
    ls->shortcutSwitchMsxAudio  = "MSX-AUDIO schakelaar aan/uit";
    ls->shortcutSwitchFront     = "Panasonic voorpanel schakelaar aan/uit";
    ls->shortcutSwitchPause     = "Pauze schakelaar aan/uit";
    ls->shortcutToggleMouseLock = "Muis vastzetten aan/uit";
    ls->shortcutEmuSpeedMax     = "Maximale emulatie snelheid";
    ls->shortcutEmuPlayReverse  = "Rewind emulation";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "Maximum snelheid van de emulatie aan/uit";
    ls->shortcutEmuSpeedNormal  = "Normale emulatie snelheid";
    ls->shortcutEmuSpeedInc     = "Verhoog emulatie snelheid";
    ls->shortcutEmuSpeedDec     = "Verlaag emulatie snelheid";
    ls->shortcutThemeSwitch     = "Ander thema kiezen";
    ls->shortcutShowEmuProp     = "Toon Emulatie-eigenschappen";
    ls->shortcutShowVideoProp   = "Toon Beeldeigenschappen";
    ls->shortcutShowAudioProp   = "Toon Geluidseigenschappen";
    ls->shortcutShowCtrlProp    = "Toon Besturingseigenschappen";
    ls->shortcutShowPerfProp    = "Toon Prestatie-eigenschappen";
    ls->shortcutShowSettProp    = "Toon Instellingen eigenschappen";
    ls->shortcutShowPorts       = "Toon Poorteigenschappen";
    ls->shortcutShowLanguage    = "Taalconfiguratie weergeven";
    ls->shortcutShowMachines    = "Machineconfiguratie weergeven";
    ls->shortcutShowShortcuts   = "Snelkoppelingen weergeven";
    ls->shortcutShowKeyboard    = "Besturingsconfiguratie weergeven";
    ls->shortcutShowMixer       = "Geluidsmixer weergeven";
    ls->shortcutShowDebugger    = "Debugger weergeven";
    ls->shortcutShowTrainer     = "Trainer weergeven";
    ls->shortcutShowHelp        = "Help weergeven";
    ls->shortcutShowAbout       = "Toon 'Over blueMSX' venster";
    ls->shortcutShowFiles       = "Toon Bestandseigenschappen";
    ls->shortcutToggleSpriteEnable = "Tonen/Verbergen sprites";
    ls->shortcutToggleFdcTiming = "Synchroniseren/Niet synchroniseren";
    ls->shortcutToggleCpuTrace  = "CPU tracer aan/uit";
    ls->shortcutVideoLoad       = "Videoclip laden";
    ls->shortcutVideoPlay       = "Laatste videoclip afspelen";
    ls->shortcutVideoRecord     = "Videoclip opnemen";
    ls->shortcutVideoStop       = "Videoclip opname stoppen";
    ls->shortcutVideoRender     = "Maak Videobestand";


    //----------------------
    // Keyboard config lines
    //----------------------

    ls->keyconfigSelectedKey    = "Geselecteerde toets:";
    ls->keyconfigMappedTo       = "Gekoppeld aan:";
    ls->keyconfigMappingScheme  = "Koppelschema:";


    //----------------------
    // ROM type lines
    //----------------------

    ls->romTypeStandard         = "Standaard";
    ls->romTypeZenima80         = "Zemina 80 in 1";
    ls->romTypeZenima90         = "Zemina 90 in 1";
    ls->romTypeZenima126        = "Zemina 126 in 1";
    ls->romTypeSccMirrored      = "SCC mirrored";
    ls->romTypeSccExtended      = "SCC extended";
    ls->romTypeKonamiGeneric    = "Konami Generic";
    ls->romTypeMirrored         = "Mirrored ROM";
    ls->romTypeNormal           = "Normal ROM";
    ls->romTypeDiskPatch        = "Normal + Disk Patch";
    ls->romTypeCasPatch         = "Normal + Cassette Patch";
    ls->romTypeTc8566afFdc      = "TC8566AF Disk Controller";
    ls->romTypeTc8566afTrFdc    = "TC8566AF Turbo-R Disk Controller";
    ls->romTypeMicrosolFdc      = "Microsol Disk Controller";
    ls->romTypeNationalFdc      = "National Disk Controller";
    ls->romTypePhilipsFdc       = "Philips Disk Controller";
    ls->romTypeSvi738Fdc        = "SVI-738 Disk Controller";
    ls->romTypeMappedRam        = "Mapped RAM";
    ls->romTypeMirroredRam1k    = "1kB Mirrored RAM";
    ls->romTypeMirroredRam2k    = "2kB Mirrored RAM";
    ls->romTypeNormalRam        = "Normal RAM";
    ls->romTypeTurborPause      = "Turbo-R Pause";
    ls->romTypeF4deviceNormal   = "F4 Device Normal";
    ls->romTypeF4deviceInvert   = "F4 Device Inverted";
    ls->romTypeTurborTimer      = "Turbo-R Timer";
    ls->romTypeNormal4000       = "Normal 4000h";
    ls->romTypeNormalC000       = "Normal C000h";
    ls->romTypeExtRam           = "External RAM";
    ls->romTypeExtRam16         = "16kB External RAM";
    ls->romTypeExtRam32         = "32kB External RAM";
    ls->romTypeExtRam48         = "48kB External RAM";
    ls->romTypeExtRam64         = "64kB External RAM";
    ls->romTypeExtRam512        = "512kB External RAM";
    ls->romTypeExtRam1mb        = "1MB External RAM";
    ls->romTypeExtRam2mb        = "2MB External RAM";
    ls->romTypeExtRam4mb        = "4MB External RAM";
    ls->romTypeSvi328Cart       = "SVI-328 Cartridge";
    ls->romTypeSvi328Fdc        = "SVI-328 Disk Controller";
    ls->romTypeSvi328Prn        = "SVI-328 Printer";
    ls->romTypeSvi328Uart       = "SVI-328 Serial Port";
    ls->romTypeSvi328col80      = "SVI-328 80 Column Card";
    ls->romTypeSvi727col80      = "SVI-727 80 Column Card";
    ls->romTypeColecoCart       = "Coleco Cartridge";
    ls->romTypeSg1000Cart       = "SG-1000 Cartridge";
    ls->romTypeSc3000Cart       = "SC-3000 Cartridge";
    ls->romTypeMsxPrinter       = "MSX Printer";
    ls->romTypeTurborPcm        = "Turbo-R PCM Chip";
    ls->romTypeNms8280Digitiz   = "Philips NMS-8280 Digitizer";
    ls->romTypeHbiV1Digitiz     = "Sony HBI-V1 Digitizer";


    //----------------------
    // Debug type lines
    // Note: Only needs translation if debugger is translated
    //----------------------

    ls->dbgMemVisible           = "Zichtbaar geheugen";
    ls->dbgMemRamNormal         = "Normaal";
    ls->dbgMemRamMapped         = "Mapped";
    ls->dbgMemYmf278            = "YMF278 Sample RAM";
    ls->dbgMemAy8950            = "AY8950 Sample RAM";
    ls->dbgMemScc               = "Geheugen";

    ls->dbgCallstack            = "Callstack";

    ls->dbgRegs                 = "Registers";
    ls->dbgRegsCpu              = "CPU Registers";
    ls->dbgRegsYmf262           = "YMF262 Registers";
    ls->dbgRegsYmf278           = "YMF278 Registers";
    ls->dbgRegsAy8950           = "AY8950 Registers";
    ls->dbgRegsYm2413           = "YM2413 Registers";

    ls->dbgDevRamMapper         = "RAM Mapper";
    ls->dbgDevRam               = "RAM";
    ls->dbgDevF4Device          = "F4 Device";
    ls->dbgDevKorean80          = "Korean 80";
    ls->dbgDevKorean90          = "Korean 90";
    ls->dbgDevKorean128         = "Korean 128";
    ls->dbgDevFdcMicrosol       = "Microsol FDC";
    ls->dbgDevPrinter           = "Printer";
    ls->dbgDevSviFdc            = "SVI FDC";
    ls->dbgDevSviPrn            = "SVI Printer";
    ls->dbgDevSvi80Col          = "SVI 80 Column";
    ls->dbgDevRtc               = "RTC";
    ls->dbgDevTrPause           = "TR Pause";


    //----------------------
    // Debug type lines
    // Note: Can only be translated to european languages
    //----------------------
    ls->aboutScrollThanksTo     = "Speciale dank aan: ";
    ls->aboutScrollAndYou       = "en U !!!!";
};

#endif
