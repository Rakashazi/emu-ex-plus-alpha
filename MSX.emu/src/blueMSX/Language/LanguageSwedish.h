/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageSwedish.h,v $
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
#ifndef LANGUAGE_SWEDISH_H
#define LANGUAGE_SWEDISH_H

#include "LanguageStrings.h"

void langInitSwedish(LanguageStrings* ls) 
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalan";
    ls->langChineseSimplified   = "Kinesiska Förenklad";
    ls->langChineseTraditional  = "Kinesiska Traditionell";
    ls->langDutch               = "Holländska";
    ls->langEnglish             = "Engelska";
    ls->langFinnish             = "Finska";
    ls->langFrench              = "Franska";
    ls->langGerman              = "Tyska";
    ls->langItalian             = "Italienska";
    ls->langJapanese            = "Japanska";
    ls->langKorean              = "Koreanska";
    ls->langPolish              = "Polska";
    ls->langPortuguese          = "Portugisiska";
    ls->langRussian             = "Ryska";            // v2.8
    ls->langSpanish             = "Spanska";
    ls->langSwedish             = "Svenska";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "Enhet:";
    ls->textFilename            = "Filnamn:";
    ls->textFile                = "Fil";
    ls->textNone                = "Ingen";
    ls->textUnknown             = "Okänd";


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle            = "blueMSX - Varning";
    ls->warningDiscardChanges   = "Vill du stänga verktyget utan att spara dina ändringar?";
    ls->warningOverwriteFile    = "Vill du skriva över filen:";
    ls->errorTitle              = "blueMSX - Fel";
    ls->errorEnterFullscreen    = "Misslyckades att byta till fullskärm.           \n";
    ls->errorDirectXFailed      = "Misslyckades att skapa DirectX objekt.          \nAnvänder GDI istället.\nKontrollera video inställningarna.";
    ls->errorNoRomInZip         = "Kunde inte hitta en .rom fil i zip arkivet.";
    ls->errorNoDskInZip         = "Kunde inte hitta en .dsk fil i zip arkivet.";
    ls->errorNoCasInZip         = "Kunde inte hitta en .cas fil i zip arkivet.";
    ls->errorNoHelp             = "Kunde inte hitta hjälpfilen.";
    ls->errorStartEmu           = "Misslyckades att starta emulatorn.";
    ls->errorPortableReadonly   = "Flyttbar enhet är inte skrivbar";


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "Rom fil";
    ls->fileAll                 = "Alla Filer";
    ls->fileCpuState            = "CPU-tillstånd";
    ls->fileVideoCapture        = "Videoinspelning"; 
    ls->fileDisk                = "Diskettfil";
    ls->fileCas                 = "Kassettfil";
    ls->fileAvi                 = "Videofil";    


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- ingen filhistoria -";
    ls->menuInsert              = "Sätt in";
    ls->menuEject               = "Ta ur";

    ls->menuCartGameReader      = "Game Reader";
    ls->menuCartIde             = "IDE";
    ls->menuCartBeerIde         = "Beer";
    ls->menuCartGIde            = "GIDE";
    ls->menuCartSunriseIde      = "Sunrise";  
    ls->menuCartScsi            = "SCSI";                // New in 2.7
    ls->menuCartMegaSCSI        = "MEGA-SCSI";           // New in 2.7
    ls->menuCartWaveSCSI        = "WAVE-SCSI";           // New in 2.7
    ls->menuCartGoudaSCSI       = "Gouda SCSI";          // New in 2.7
    ls->menuJoyrexPsg           = "Joyrex PSG Cartridge"; // New in 2.9
    ls->menuCartSCC             = "SCC Cartridge";
    ls->menuCartSCCPlus         = "SCC-I Cartridge";
    ls->menuCartFMPac           = "FM-PAC Cartridge";
    ls->menuCartPac             = "PAC Cartridge";
    ls->menuCartHBI55           = "Sony HBI-55 Cartridge";
    ls->menuCartInsertSpecial   = "Sätt in Special";
    ls->menuCartMegaRam         = "MegaRAM";
    ls->menuCartExternalRam     = "Externt RAM";
    ls->menuCartEseRam          = "Ese-RAM";             // New in 2.7
    ls->menuCartEseSCC          = "Ese-SCC";             // New in 2.7
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";      // New in 2.7

    ls->menuDiskInsertNew       = "Sätt in ny diskett fil";
    ls->menuDiskInsertCdrom     = "Sätt in CD-Rom";      // New in 2.7
    ls->menuDiskDirInsert       = "Sätt in mapp";
    ls->menuDiskAutoStart       = "Starta om efter insättning/urdragning";
    ls->menuCartAutoReset       = "Starta om efter insättning/urdragning";

    ls->menuCasRewindAfterInsert= "Spola tillbaka vid insättning";
    ls->menuCasUseReadOnly      = "Tillåt endast läsning av kassett";
    ls->lmenuCasSaveAs          = "Spara kassett som...";
    ls->menuCasSetPosition      = "Sätt position";
    ls->menuCasRewind           = "Spola tillbaka";

    ls->menuVideoLoad           = "Ladda...";             
    ls->menuVideoPlay           = "Spela upp senaste";   
    ls->menuVideoRecord         = "Spela in";              
    ls->menuVideoRecording      = "Spelar in";           
    ls->menuVideoRecAppend      = "Spela in (lägg till)";     
    ls->menuVideoStop           = "Stopp";                
    ls->menuVideoRender         = "Spara videofil";   

    ls->menuPrnFormfeed         = "Pappersmatning";

    ls->menuZoomNormal          = "Normal storlek";
    ls->menuZoomDouble          = "Dubbel storlek";
    ls->menuZoomFullscreen      = "Fullskärm";
    
    ls->menuPropsEmulation      = "Emulering";
    ls->menuPropsVideo          = "Video";
    ls->menuPropsSound          = "Ljud";
    ls->menuPropsControls       = "Kontroller";
    ls->menuPropsPerformance    = "Prestanda";
    ls->menuPropsSettings       = "Inställningar";
    ls->menuPropsFile           = "Filer";
    ls->menuPropsDisk           = "Enheter";               // New in 2.7
    ls->menuPropsLanguage       = "Språk";
    ls->menuPropsPorts          = "Portar";
    
    ls->menuVideoSource         = "Video ut källa";
    ls->menuVideoSourceDefault  = "Ingen videokälla inkopplad";
    ls->menuVideoChipAutodetect = "Automatisk detektering av video chip";    
    ls->menuVideoInSource       = "Video in källa";
    ls->menuVideoInBitmap       = "Bitmap fil";
    
    ls->menuEthInterface        = "Nätverksanslutning"; 

    ls->menuHelpHelp            = "Hjälp";
    ls->menuHelpAbout           = "Om blueMSX";

    ls->menuFileCart            = "Cartridge slot";
    ls->menuFileDisk            = "Diskettstation";
    ls->menuFileCas             = "Kassett";
    ls->menuFilePrn             = "Skrivare";
    ls->menuFileLoadState       = "Läs in CPU-tillstånd";
    ls->menuFileSaveState       = "Spara CPU-tillstånd";
    ls->menuFileQLoadState      = "Snabbladda CPU-tillstånd";
    ls->menuFileQSaveState      = "Snabbspara CPU-tillstånd";
    ls->menuFileCaptureAudio    = "Spara ljud";
    ls->menuFileCaptureVideo    = "Videoinspelning"; 
    ls->menuFileScreenShot      = "Spara skärmdump";
    ls->menuFileExit            = "Avsluta";

    ls->menuFileHarddisk        = "Hårddisk";
    ls->menuFileHarddiskNoPesent= "Inga enheter tillgängliga";
    ls->menuFileHarddiskRemoveAll= "Ta ur alla hårddiskar";    // New in 2.7

    ls->menuRunRun              = "Kör";
    ls->menuRunPause            = "Paus";
    ls->menuRunStop             = "Stanna";
    ls->menuRunSoftReset        = "Mjuk Omstart";
    ls->menuRunHardReset        = "Hård Omstart";
    ls->menuRunCleanReset       = "Full Omstart";

    ls->menuToolsMachine        = "Konfigureringsverktyg";
    ls->menuToolsShortcuts      = "Kortkommando Verktyg";
    ls->menuToolsKeyboard       = "Tangentbordseditor";
    ls->menuToolsCtrlEditor     = "Kontrollers / Tangenbordseditor"; 
    ls->menuToolsMixer          = "Mixer";
    ls->menuToolsDebugger       = "Avlusare";               
    ls->menuToolsTrainer        = "Spelfusk";                
    ls->menuToolsTraceLogger    = "Spårutskrift";           

    ls->menuFile                = "Arkiv";
    ls->menuRun                 = "Emulering";
    ls->menuWindow              = "Fönster";
    ls->menuOptions             = "Egenskaper";
    ls->menuTools               = "Verktyg";
    ls->menuHelp                = "Hjälp";
    

    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "OK";
    ls->dlgOpen                 = "Öppna";
    ls->dlgCancel               = "Avbryt";
    ls->dlgSave                 = "Spara";
    ls->dlgSaveAs               = "Spara Som...";
    ls->dlgRun                  = "Kör";
    ls->dlgClose                = "Stäng";

    ls->dlgLoadRom              = "blueMSX - Ladda en rom fil";
    ls->dlgLoadDsk              = "blueMSX - Ladda en dsk fil";
    ls->dlgLoadCas              = "blueMSX - Ladda en cas fil";
    ls->dlgLoadRomDskCas        = "blueMSX - Ladda en rom, dsk eller cas fil";
    ls->dlgLoadRomDesc          = "Välj en cartridge fil:";
    ls->dlgLoadDskDesc          = "Välj en diskett fil:";
    ls->dlgLoadCasDesc          = "Välj en kassett fil:";
    ls->dlgLoadRomDskCasDesc    = "Välj en fil:";
    ls->dlgLoadState            = "Ladda CPU-tillstånd";
    ls->dlgLoadVideoCapture     = "Ladda videoinspelning";      
    ls->dlgSaveState            = "Spara CPU-tillstånd";
    ls->dlgSaveCassette         = "blueMSX - Spara Kassett";
    ls->dlgSaveVideoClipAs      = "Spara videoinspeling som...";      
    ls->dlgAmountCompleted      = "Andel färdigt:";          
    ls->dlgInsertRom1           = "Sätt in ROM cartridge i slot 1";
    ls->dlgInsertRom2           = "Sätt in ROM cartridge i slot 2";
    ls->dlgInsertDiskA          = "Sätt in diskett i diskettstation A";
    ls->dlgInsertDiskB          = "Sätt in diskett i diskettstation B";
    ls->dlgInsertHarddisk       = "Sätt in hårddisk";
    ls->dlgInsertCas            = "Sätt in kassettband";
    ls->dlgRomType              = "Rom Typ:";
    ls->dlgDiskSize             = "Diskettstorlek:";             

    ls->dlgTapeTitle            = "blueMSX - Kassettposition";
    ls->dlgTapeFrameText        = "Kassettposition";
    ls->dlgTapeCurrentPos       = "Aktuell position";
    ls->dlgTapeTotalTime        = "Total längd";
    ls->dlgTapeSetPosText       = "Kassettposition:";
    ls->dlgTapeCustom           = "Visa Specialfiler";
    ls->dlgTabPosition          = "Position";
    ls->dlgTabType              = "Typ";
    ls->dlgTabFilename          = "Filnamn";
    ls->dlgZipReset             = "Starta om efter insättning";
    
    ls->dlgAboutTitle           = "blueMSX - Information";

    ls->dlgLangLangText         = "Välj språk som blueMSX ska använda";
    ls->dlgLangLangTitle        = "blueMSX - Språk";

    ls->dlgAboutAbout           = "INFORMATION\r\n========";
    ls->dlgAboutVersion         = "Version:";
    ls->dlgAboutBuildNumber     = "Bygge:";
    ls->dlgAboutBuildDate       = "Datum:";
    ls->dlgAboutCreat           = "Skapat av Daniel Vik";
    ls->dlgAboutDevel           = "UTVECKLARE\r\n========";
    ls->dlgAboutThanks          = "BIDRAGARE\r\n=======";       // New in 2.7 (retranslate, see english)
    ls->dlgAboutLisence         = "LICENS\r\n"
                                  "=====\r\n\r\n"
                                  "Denna programvara är erbjuden 'som den är', utan någon explicit eller "
                                  "implicit garanti. Inte av någon händelse kommer författaren/na att hållas "
                                  "ansvariga för några skador orsakade av detta program.\r\n\r\n"
                                  "Besök www.bluemsx.com for mer information.";

    ls->dlgSavePreview          = "Förvisning";
    ls->dlgSaveDate             = "Tid Sparad:";

    ls->dlgRenderVideoCapture   = "blueMSX - Generera videofil...";  


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - Egenskaper";
    ls->propEmulation           = "Emulering";
    ls->propVideo               = "Video";
    ls->propSound               = "Ljud";
    ls->propControls            = "Kontroller";
    ls->propPerformance         = "Prestanda";
    ls->propSettings            = "Inställningar";
    ls->propFile                = "Filer";
    ls->propDisk                = "Enheter";              // New in 2.7
    ls->propPorts               = "Portar";

    ls->propEmuGeneralGB        = "Allmänt ";
    ls->propEmuFamilyText       = "MSX familj:";
    ls->propEmuMemoryGB         = "Minne ";
    ls->propEmuRamSizeText      = "RAM storlek:";
    ls->propEmuVramSizeText     = "VRAM storlek:";
    ls->propEmuSpeedGB          = "Emuleringshastighet ";
    ls->propEmuSpeedText        = "Emuleringshastighet:";
    ls->propEmuFrontSwitchGB    = "Panasonicbrytare ";
    ls->propEmuFrontSwitch      = " Frontbrytare";
    ls->propEmuFdcTiming        = " Slå av diskettstationstiming";
    ls->propEmuReversePlay      = " Tillåt baklänges uppspelning";
    ls->propEmuPauseSwitch      = " Pausbrytare";
    ls->propEmuAudioSwitch      = " MSX-AUDIO cartridge switch";
    ls->propVideoFreqText       = "Videofrekvens:";
    ls->propVideoFreqAuto       = "Automatisk";
    ls->propSndOversampleText   = "Översampling:";
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 In ";              
    ls->propSndMidiInGB         = "MIDI In ";
    ls->propSndMidiOutGB        = "MIDI Ut ";
    ls->propSndMidiChannel      = "MIDI Channel:";                      
    ls->propSndMidiAll          = "Alla";                               

    ls->propMonMonGB            = "Monitor ";
    ls->propMonTypeText         = "Monitor typ:";
    ls->propMonEmuText          = "Monitoremulering:";
    ls->propVideoTypeText       = "Videotyp:";
    ls->propWindowSizeText      = "Fönsterstorlek:";
    ls->propMonHorizStretch     = " Horizontell utsträckning";
    ls->propMonVertStretch      = " Vertikal utsträckning";
    ls->propMonDeInterlace      = " De-interlace";
    ls->propBlendFrames         = " Blanda efterföljande bilder";           
    ls->propMonBrightness       = "Ljusstyrka:";
    ls->propMonContrast         = "Kontrast:";
    ls->propMonSaturation       = "Färgmättnad:";
    ls->propMonGamma            = "Gamma:";
    ls->propMonScanlines        = " Scanlinjer:";
    ls->propMonColorGhosting    = " RF-modulering:";
    ls->propMonEffectsGB        = "Effekter ";

    ls->propPerfVideoDrvGB      = "Videodriver ";
    ls->propPerfVideoDispDrvText= "Skärmdriver:";
    ls->propPerfFrameSkipText   = "Frame skipping:";
    ls->propPerfAudioDrvGB      = "Lkuddriver ";
    ls->propPerfAudioDrvText    = "Ljuddriver:";
    ls->propPerfAudioBufSzText  = "Storlek på ljudbuffer:";
    ls->propPerfEmuGB           = "Emulering ";
    ls->propPerfSyncModeText    = "Synkronisering:";
    ls->propFullscreenResText   = "Fullskärmsupplösning:";

    ls->propSndChipEmuGB        = "Emulering av ljudchip ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound        = " Moonsound";
    ls->propSndMt32ToGm         = " Konvertera MT-32 instrument till General MIDI";

    ls->propPortsLptGB          = "Parallellport ";
    ls->propPortsComGB          = "Serieportar ";
    ls->propPortsLptText        = "Port:";
    ls->propPortsCom1Text       = "Port 1:";
    ls->propPortsNone           = "Ingen";
    ls->propPortsSimplCovox     = "SiMPL / Covox DAC";
    ls->propPortsFile           = "Skriv till Fil";
    ls->propPortsComFile        = "Skicka till Fil";
    ls->propPortsOpenLogFile    = "Öppna Logfil";
    ls->propPortsEmulateMsxPrn  = "Emulering:";

    ls->propSetFileHistoryGB    = "Filhistoria ";
    ls->propSetFileHistorySize  = "Antal element i filhistorian:";
    ls->propSetFileHistoryClear = "Rensa historia";
    ls->propFileTypes           = " Registrera filtyper med blueMSX (.rom, .dsk, .cas, .sta)";
    ls->propWindowsEnvGB        = "Windows Miljö ";
    ls->propSetScreenSaver      = " Deaktivera skärmsläckare när blueMSX kör";
    ls->propDisableWinKeys      = " Avaktivera WIndows menyer när emulatorn kör";
    ls->propPriorityBoost       = " Höj prioriteten på blueMSX";
    ls->propScreenshotPng       = " Använd Portable Network Graphics (.png) skärmdump";
    ls->propEjectMediaOnExit    = " Ta ur media när blueMSX avslutas";
    ls->propClearHistory        = "Vill du verkligen radera filhistorien?";
    ls->propOpenRomGB           = "Öppna Rom Dialog ";
    ls->propDefaultRomType      = "Default Rom Typ:";
    ls->propGuessRomType        = "Gissa Rom Typ";

    ls->propSettDefSlotGB       = "Dra och Släpp ";
    ls->propSettDefSlots        = "Sätt in Rom i:";
    ls->propSettDefSlot         = " Slot";
    ls->propSettDefDrives       = "Sätt in Diskett i:";
    ls->propSettDefDrive        = " Drive";

    ls->propThemeGB             = "Tema ";
    ls->propTheme               = "Tema";

    ls->propCdromGB             = "CD-ROM ";         // New in 2.7
    ls->propCdromMethod         = "Åtkomstmetod:";   // New in 2.7
    ls->propCdromMethodNone     = "Ingen";           // New in 2.7
    ls->propCdromMethodIoctl    = "IOCTL";           // New in 2.7
    ls->propCdromMethodAspi     = "ASPI";            // New in 2.7
    ls->propCdromDrive          = "Enhet:";          // New in 2.7


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "Färg";
    ls->enumVideoMonGrey        = "Svartvit";
    ls->enumVideoMonGreen       = "Grön";
    ls->enumVideoMonAmber       = "Orange";

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "Ingen";
    ls->enumVideoEmuYc          = "Y/C kabel (skarp)";
    ls->enumVideoEmuMonitor     = "Monitor";
    ls->enumVideoEmuYcBlur      = "Brusig Y/C kabel (skarp)";
    ls->enumVideoEmuComp        = "Kompositkabel (suddig)";
    ls->enumVideoEmuCompBlur    = "Brusig komposit (suddig)";
    ls->enumVideoEmuScale2x     = "Scale 2x";
    ls->enumVideoEmuHq2x        = "Hq2x";

    ls->enumVideoSize1x         = "Normal - 320x200";
    ls->enumVideoSize2x         = "Dubbel - 640x400";
    ls->enumVideoSizeFullscreen = "Fullskärm";

    ls->enumVideoDrvDirectDrawHW= "DirectDraw HW accel.";
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "Ingen";
    ls->enumVideoFrameskip1     = "1 bild";
    ls->enumVideoFrameskip2     = "2 bilder";
    ls->enumVideoFrameskip3     = "3 bilder";
    ls->enumVideoFrameskip4     = "4 bilder";
    ls->enumVideoFrameskip5     = "5 bilder";

    ls->enumSoundDrvNone        = "Inget ljud";
    ls->enumSoundDrvWMM         = "WMM driver";
    ls->enumSoundDrvDirectX     = "DirectX driver";

    ls->enumEmuSync1ms          = "Synkronisera till MSX refresh";
    ls->enumEmuSyncVblank       = "Synkronisera till PC Vertikal Blank";
    ls->enumEmuAsyncVblank      = "Asynchronous PC Vblank";             
    ls->enumEmuSyncNone         = "Ingen";
    ls->enumEmuSyncAuto         = "Automatisk (snabb)";

    ls->enumControlsJoyNone     = "Ingen";
    ls->enumControlsJoyMouse    = "Mus";
    ls->enumControlsJoyTetris2Dongle = "Tetris 2 dosa";
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey dosa";
    ls->enumControlsJoy2Button = "2-knapps Styrspak";                   
    ls->enumControlsJoyGunstick  = "Gun Stick";                         
    ls->enumControlsJoyAsciiLaser="ASCII Plus-X Terminator Laser";      
    ls->enumControlsArkanoidPad  ="Arkanoid Pad";                   // New in 2.7.1
    ls->enumControlsJoyColeco = "ColecoVision Styrspak";                

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" Dubbelsidig, 9 Sektorer";     
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" Dubbelsidig, 8 Sektorer";     
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" Enkelsidig, 9 Sektorer";     
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" Enkelsidig, 8 Sektorer";     
    ls->enumDiskSvi525Dbl        = "SVI-328\" 5.25 Dubbelsidig";           
    ls->enumDiskSvi525Sgl        = "SVI-328\" 5.25 Enkelsidig";   
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" Enkelsidig";  


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle               = "blueMSX - Konfigureringsverktyg";
    ls->confConfigText          = "Konfigurering:";
    ls->confSlotLayout          = "Slot mappning";
    ls->confMemory              = "Minne";
    ls->confChipEmulation       = "Chipemulering";
    ls->confChipExtras          = "Extra";

    ls->confOpenRom             = "Öppna Rom fil";
    ls->confSaveTitle           = "blueMSX - Spara Konfigurering";
    ls->confSaveText            = "Vill du skriva över maskinkonfigureringen? :";
    ls->confSaveAsTitle         = "Spara Konfigurering Som...";
    ls->confSaveAsMachineName   = "Maskinnamn:";
    ls->confDiscardTitle        = "blueMSX - Konfigurering";
    ls->confExitSaveTitle       = "blueMSX - Avsluta Konfigureringsverktyg";
    ls->confExitSaveText        = "Vill du stanga konfigureringsverktyget utan att spara dina ändringar?";

    ls->confSlotLayoutGB        = "Slot Layout ";
    ls->confSlotExtSlotGB       = "Externa slots ";
    ls->confBoardGB             = "Board ";
    ls->confBoardText           = "Board Typ:";
    ls->confSlotPrimary         = "Primär";
    ls->confSlotExpanded        = "Expanderad (4 subslottar)";

    ls->confSlotCart            = "Cartridge:";
    ls->confSlot                = "Slot";
    ls->confSubslot             = "Subslot";

    ls->confMemAdd               = "Ny...";
    ls->confMemEdit              = "Ändra...";
    ls->confMemRemove            = "Ta Bort";
    ls->confMemSlot              = "Slot";
    ls->confMemAddresss          = "Adress";
    ls->confMemType              = "Typ";
    ls->confMemRomImage          = "Rom Fil";
    
    ls->confChipVideoGB          = "Video ";
    ls->confChipVideoChip        = "Video Chip:";
    ls->confChipVideoRam         = "Video RAM:";
    ls->confChipSoundGB          = "Audio ";
    ls->confChipPsgStereoText    = " PSG Stereo";

    ls->confCmosGB               = "CMOS ";
    ls->confCmosEnable           = " Använd CMOS";
    ls->confCmosBattery          = " Använd Laddat Batteri";

    ls->confCpuFreqGB            = "CPU Frekvens ";
    ls->confZ80FreqText          = "Z80 Frekvens:";
    ls->confR800FreqText         = "R800 Frekvens:";
    ls->confFdcGB                = "Kontroller för Diskettenhet ";
    ls->confCFdcNumDrivesText    = "Antal Diskettenheter:";

    ls->confEditMemTitle         = "blueMSX - Ändra Minnesmap";
    ls->confEditMemGB            = "Detailjer ";
    ls->confEditMemType          = "Typ:";
    ls->confEditMemFile          = "Fil:";
    ls->confEditMemAddress       = "Adress";
    ls->confEditMemSize          = "Storlek";
    ls->confEditMemSlot          = "Slot";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "Tangentkombination";
    ls->shortcutDescription     = "Kortkommando";

    ls->shortcutSaveConfig      = "blueMSX - Spara Konfigurering";
    ls->shortcutOverwriteConfig = "Vill du skriva över kortkommando konfigureringen:";
    ls->shortcutExitConfig      = "blueMSX - Avsluta Kortkommandoverktyget";
    ls->shortcutDiscardConfig   = "Vill du stanga konfigureringsverktyget utan att spara dina ändringar?";
    ls->shortcutSaveConfigAs    = "blueMSX - Spara Kortkommando Konfigurering Som...";
    ls->shortcutConfigName      = "Konfigurering:";
    ls->shortcutNewProfile      = "< Ny Profil >";
    ls->shortcutConfigTitle     = "blueMSX - Konfigurering av Kortkommandon";
    ls->shortcutAssign          = "Tilldela";
    ls->shortcutPressText       = "Tryck kortkommando:";
    ls->shortcutScheme          = "Kommandoschema:";
    ls->shortcutCartInsert1     = "Sätt in Cartridge 1";
    ls->shortcutCartRemove1     = "Ta ur Cartridge 1";
    ls->shortcutCartInsert2     = "Sätt in Cartridge 2";
    ls->shortcutCartRemove2     = "Ta ur Cartridge 2";
    ls->shortcutSpecialMenu1    = "Visa Specialrom meny för Cartridge 1";
    ls->shortcutSpecialMenu2    = "Visa Specialrom meny för Cartridge 2";
    ls->shortcutCartAutoReset   = "Starta om efter Insättning av Cartridge";
    ls->shortcutDiskInsertA     = "Sätt in Diskett A";
    ls->shortcutDiskDirInsertA  = "Sätt in Directory som Diskett A";
    ls->shortcutDiskRemoveA     = "Ta ur Diskett A";
    ls->shortcutDiskChangeA     = "Snabbbyt Diskett A";
    ls->shortcutDiskAutoResetA  = "Starta om efter Insättning av Diskett";
    ls->shortcutDiskInsertB     = "Sätt in Diskett B";
    ls->shortcutDiskDirInsertB  = "Sätt in Directory som Diskett B";
    ls->shortcutDiskRemoveB     = "Ta ur Diskett B";
    ls->shortcutCasInsert       = "Sätt in Kassett";
    ls->shortcutCasEject        = "Ta ur Kassett";
    ls->shortcutCasAutorewind   = "Slå Av/På Automatisk Tillbakaspolning";
    ls->shortcutCasReadOnly     = "Slå Av/På Skrivskydd på Kassettfiler";
    ls->shortcutCasSetPosition  = "Sätt Kassettposition";
    ls->shortcutCasRewind       = "Spola Tillabaka Kassett";
    ls->shortcutCasSave         = "Spara Kassett till fil";
    ls->shortcutPrnFormFeed     = "Form Feed på Skrivare";
    ls->shortcutCpuStateLoad    = "Ladda CPU-tillstånd";
    ls->shortcutCpuStateSave    = "Spara CPU-tillstånd";
    ls->shortcutCpuStateQload   = "Snabbladda CPU-tillstånd";
    ls->shortcutCpuStateQsave   = "Snabbspara CPU-tillstånd";
    ls->shortcutAudioCapture    = "Starta/Stanna Audioinspelning";
    ls->shortcutScreenshotOrig  = "Spar Skärmdump";
    ls->shortcutScreenshotSmall = "Spar Liten Ofiltrerad Skärmdump";
    ls->shortcutScreenshotLarge = "Spar Stor Ofiltrerad Skärmdump";
    ls->shortcutQuit            = "Avsluta blueMSX";
    ls->shortcutRunPause        = "Kör/Pause Emuleringen";
    ls->shortcutStop            = "Stanna Emuleringen";
    ls->shortcutResetHard       = "Hård Omstart";
    ls->shortcutResetSoft       = "Mjuk Omstart";
    ls->shortcutResetClean      = "Full Omstart";
    ls->shortcutSizeSmall       = "Växla till Liten Fönsterstorlek";
    ls->shortcutSizeNormal      = "Växla till Normal Fönsterstorlek";
    ls->shortcutSizeFullscreen  = "Växla till Fullskärm";
    ls->shortcutSizeMinimized   = "Minimera fönster";
    ls->shortcutToggleFullscren = "Växla till/från Fullskärm";
    ls->shortcutVolumeIncrease  = "Öka Volymen";
    ls->shortcutVolumeDecrease  = "Minska Volymen";
    ls->shortcutVolumeMute      = "Stäng av Volymen";
    ls->shortcutVolumeStereo    = "Växla mellan mono/stereo";
    ls->shortcutSwitchMsxAudio  = "Slå om MSX-AUDIO brytare";
    ls->shortcutSwitchFront     = "Slå om Panasonic Front brytare";
    ls->shortcutSwitchPause     = "Slå om Pausbrytare";
    ls->shortcutToggleMouseLock = "Slå om Muslås";
    ls->shortcutEmuSpeedMax     = "Maximal Emuleringshastighet";
    ls->shortcutEmuPlayReverse  = "Spola tillbaka";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "Växla Mellan Normal och Max Emuleringshastighet";
    ls->shortcutEmuSpeedNormal  = "Minska Emuleringhastigheten";
    ls->shortcutEmuSpeedInc     = "Öka Emuleringhastigheten";
    ls->shortcutEmuSpeedDec     = "Minska Emuleringhastigheten";
    ls->shortcutThemeSwitch     = "Växla fönstretema";
    ls->shortcutShowEmuProp     = "Öppna Emuleringsfönstret";
    ls->shortcutShowVideoProp   = "Öppna Videofönstret";
    ls->shortcutShowAudioProp   = "Öppna Audiofönstret";
    ls->shortcutShowCtrlProp    = "Öppna Kontrollfönstret";
    ls->shortcutShowPerfProp    = "Öppna Prestandafönstret";
    ls->shortcutShowSettProp    = "Öppna Inställningsfönstret";
    ls->shortcutShowPorts       = "Visa Portegenskaper";
    ls->shortcutShowLanguage    = "Öppna Språkfönstret";
    ls->shortcutShowMachines    = "Öppna Konfigureringsverktyget";
    ls->shortcutShowShortcuts   = "Öppna Kortkommandoverktyget";
    ls->shortcutShowKeyboard    = "Visa Tangentbordseditor";
    ls->shortcutShowDebugger    = "Visa Debugger";
    ls->shortcutShowTrainer     = "Visa Trainer";
    ls->shortcutShowMixer       = "Visa Mixer";
    ls->shortcutShowHelp        = "Öppna Hjälpfönstret";
    ls->shortcutShowAbout       = "Öppna \"Om blueMSX\"-fönstret";
    ls->shortcutShowFiles       = "Öppna Filerfönstret";
    ls->shortcutToggleSpriteEnable = "Visa/Dölj Sprites";
    ls->shortcutToggleFdcTiming = "Slå på/av Diskettstationstiming";
    ls->shortcutToggleCpuTrace  = "Slå på/av CPU trace";
    ls->shortcutVideoLoad       = "Ladda...";             
    ls->shortcutVideoPlay       = "Spela upp senaste";   
    ls->shortcutVideoRecord     = "Spela in";              
    ls->shortcutVideoStop       = "Stopp";                
    ls->shortcutVideoRender     = "Spara videofil";   


    //----------------------
    // Keyboard config lines
    //----------------------

    ls->keyconfigSelectedKey    = "Välj tangent:";
    ls->keyconfigMappedTo       = "Mappad till:";
    ls->keyconfigMappingScheme  = "Mapschema:";

    
    //----------------------
    // Rom type lines
    //----------------------

    ls->romTypeStandard         = "Standard";
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
    ls->romTypeF4deviceNormal   = "F4 Enhet Normal";
    ls->romTypeF4deviceInvert   = "F4 Enhet Inverted";
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

    ls->dbgMemVisible           = "Visible Memory";
    ls->dbgMemRamNormal         = "Normal";
    ls->dbgMemRamMapped         = "Mapped";
    ls->dbgMemYmf278            = "YMF278 Sample RAM";
    ls->dbgMemAy8950            = "AY8950 Sample RAM";
    ls->dbgMemScc               = "Memory";

    ls->dbgCallstack            = "Callstack";

    ls->dbgRegs                 = "Registers";
    ls->dbgRegsCpu              = "CPU Registers";
    ls->dbgRegsYmf262           = "YMF262 Registers";
    ls->dbgRegsYmf278           = "YMF278 Registers";
    ls->dbgRegsAy8950           = "AY8950 Registers";
    ls->dbgRegsYm2413           = "YM2413 Registers";

    ls->dbgDevRamMapper         = "RAM Mapper";
    ls->dbgDevRam               = "RAM";
    ls->dbgDevF4Device          = "F4 Enhet";
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
    ls->aboutScrollThanksTo     = "Speciellt tack till: ";
    ls->aboutScrollAndYou       = "och DIG !!!!";
};

#endif
