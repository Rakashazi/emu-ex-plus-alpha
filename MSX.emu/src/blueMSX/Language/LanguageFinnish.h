/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageFinnish.h,v $
**
** $Revision: 1.59 $
**
** $Date: 2009-04-04 20:57:19 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, NYYRIKKI
**
** Translated By : NYYRIKKI 2006
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
#ifndef LANGUAGE_FINNISH_H
#define LANGUAGE_FINNISH_H

#include "LanguageStrings.h"

void langInitFinnish(LanguageStrings* ls)
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalan";
    ls->langChineseSimplified   = "Kiina, yksinkertaistettu";
    ls->langChineseTraditional  = "Kiina, perinteinen";
    ls->langDutch               = "Hollanti";
    ls->langEnglish             = "Englanti";
    ls->langFinnish             = "Suomi";
    ls->langFrench              = "Ranska";
    ls->langGerman              = "Saksa";
    ls->langItalian             = "Italia";
    ls->langJapanese            = "Japani";
    ls->langKorean              = "Korea";
    ls->langPolish              = "Puola";
    ls->langPortuguese          = "Portugali";
    ls->langRussian             = "Russian";            // v2.8
    ls->langSpanish             = "Espanja";
    ls->langSwedish             = "Ruotsi";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "Laite:";
    ls->textFilename            = "Tiedostonimi:";
    ls->textFile                = "Tiedosto";
    ls->textNone                = "Ei käytössä";
    ls->textUnknown             = "Tuntematon";


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle            = "blueMSX - Varoitus";
    ls->warningDiscardChanges   = "Haluatko perua tekemäsi muutokset?";
    ls->warningOverwriteFile    = "Haluatko korvata tiedoston:";
    ls->errorTitle              = "blueMSX - Virhe";
    ls->errorEnterFullscreen    = "Siirtyminen kokoruututilaan epäonnistui.  \n";
    ls->errorDirectXFailed      = "DirectX objektien luonti epäonnistui.      \nDirectX on korvattu GDI:llä.\nTarkista video asetukset.";
    ls->errorNoRomInZip         = "Zip paketista ei löytynyt .ROM tiedostoa.";
    ls->errorNoDskInZip         = "Zip paketista ei löytynyt .DSK tiedostoa.";
    ls->errorNoCasInZip         = "Zip paketista ei löytynyt .CAS tiedostoa.";
    ls->errorNoHelp             = "blueMSX:n ohjetiedostoa ei löytynyt.";
    ls->errorStartEmu           = "MSX emulaattorin käynnistys epäonnistui.";
    ls->errorPortableReadonly   = "Kannettava laite tukee vain lukemista.";


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "ROM-tiedosto";
    ls->fileAll                 = "Kaikki tiedostot";
    ls->fileCpuState            = "Tilannetallenne";
    ls->fileVideoCapture        = "Videokaappaus"; 
    ls->fileDisk                = "Levytiedosto";
    ls->fileCas                 = "Kasettitiedosto";
    ls->fileAvi                 = "Videopätkä";    


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- ei viimeaikaisia tiedostoja -";
    ls->menuInsert              = "Aseta...";
    ls->menuEject               = "Poista";

    ls->menuCartGameReader      = "Game Reader";
    ls->menuCartIde             = "IDE";
    ls->menuCartBeerIde         = "Beer";
    ls->menuCartGIde            = "GIDE";
    ls->menuCartSunriseIde      = "Sunrise";  
    ls->menuCartScsi            = "SCSI";                // New in 2.7
    ls->menuCartMegaSCSI        = "MEGA-SCSI";           // New in 2.7
    ls->menuCartWaveSCSI        = "WAVE-SCSI";           // New in 2.7
    ls->menuCartGoudaSCSI       = "Gouda SCSI";          // New in 2.7
    ls->menuJoyrexPsg           = "Joyrex PSG Moduli"; // New in 2.9
    ls->menuCartSCC             = "SCC Moduli";
    ls->menuCartSCCPlus         = "SCC-I Moduli";
    ls->menuCartFMPac           = "FM-PAC Moduli";
    ls->menuCartPac             = "PAC Moduli";
    ls->menuCartHBI55           = "Sony HBI-55 Moduli";
    ls->menuCartInsertSpecial   = "Erikoismoduli";
    ls->menuCartMegaRam         = "MegaRAM";
    ls->menuCartExternalRam     = "Ulkoinen muisti";
    ls->menuCartEseRam          = "Ese-RAM";             // New in 2.7
    ls->menuCartEseSCC          = "Ese-SCC";             // New in 2.7
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";      // New in 2.7

    ls->menuDiskInsertNew       = "Uusi levytiedosto";
    ls->menuDiskInsertCdrom     = "Insert CD-Rom";       // New in 2.7
    ls->menuDiskDirInsert       = "Aseta hakemisto";
    ls->menuDiskAutoStart       = "Käynnistä uudelleen asettamisen jälkeen";
    ls->menuCartAutoReset       = "Käynnistä uudelleen muutettaessa";

    ls->menuCasRewindAfterInsert = "Kelaa alkuun asetettaessa";
    ls->menuCasUseReadOnly       = "Kirjoitussuojaa kasetti";
    ls->lmenuCasSaveAs           = "Tallenna kasetti nimellä";
    ls->menuCasSetPosition      = "Valitse kohta";
    ls->menuCasRewind           = "Kelaa alkuun";

    ls->menuVideoLoad           = "Lataa...";             
    ls->menuVideoPlay           = "Toista viimeisin kaappaus";   
    ls->menuVideoRecord         = "Nauhoita";              
    ls->menuVideoRecording      = "Nauhoittaa";           
    ls->menuVideoRecAppend      = "Jatka nauhoitusta";     
    ls->menuVideoStop           = "Pysäytä";                
    ls->menuVideoRender         = "Koosta videotiedosto";   

    ls->menuPrnFormfeed         = "Paperin vaihto";

    ls->menuZoomNormal          = "Pieni koko";
    ls->menuZoomDouble          = "Normaali koko";
    ls->menuZoomFullscreen      = "Kokoruututila";

    ls->menuPropsEmulation      = "Emulaatio";
    ls->menuPropsVideo          = "Video";
    ls->menuPropsSound          = "Ääni";
    ls->menuPropsControls       = "Kontrollit";
    ls->menuPropsPerformance    = "Suorituskyky";
    ls->menuPropsSettings       = "Asetukset";
    ls->menuPropsFile           = "Tiedostot";
    ls->menuPropsDisk           = "Levyt";               // New in 2.7
    ls->menuPropsLanguage       = "Kieli";
    ls->menuPropsPorts          = "Liitännät";
    
    ls->menuVideoSource         = "Videoulostulo";
    ls->menuVideoSourceDefault  = "Videoulostuloa ei ole kytketty.";
    ls->menuVideoChipAutodetect = "Tunnista videopiiri automaattisesti";
    ls->menuVideoInSource       = "Videosisääntulo";
    ls->menuVideoInBitmap       = "Bittikarttatiedosto";
    
    ls->menuEthInterface        = "Verkkokortti"; 

    ls->menuHelpHelp            = "Ohjeen aiheet";
    ls->menuHelpAbout           = "Tietoja blueMSX:stä";

    ls->menuFileCart            = "Moduliportti";
    ls->menuFileDisk            = "Levyasema";
    ls->menuFileCas             = "Kasetti";
    ls->menuFilePrn             = "Tulostin";
    ls->menuFileLoadState       = "Lataa tila...";
    ls->menuFileSaveState       = "Tallenna tila nimellä";
    ls->menuFileQLoadState      = "Tilan pikalataus";
    ls->menuFileQSaveState      = "Tilan pikatallennus";
    ls->menuFileCaptureAudio    = "Tallenna äänet";
    ls->menuFileCaptureVideo    = "Videokaappaus"; 
    ls->menuFileScreenShot      = "Tallenna kuvaruutukaappaus";
    ls->menuFileExit            = "Poistu";
    ls->menuFileHarddisk        = "Kovalevy";
    ls->menuFileHarddiskNoPesent= "Ohjaimia ei ole kytketty";
    ls->menuFileHarddiskRemoveAll= "Eject All Harddisk";    // New in 2.7

    ls->menuRunRun              = "Käynnistä";
    ls->menuRunPause            = "Keskeytä";
    ls->menuRunStop             = "Pysäytä";
    ls->menuRunSoftReset        = "Lämminkäynnistys";
    ls->menuRunHardReset        = "Kylmäkäynnistys";
    ls->menuRunCleanReset       = "Täydellinen uudelleenkäynnistys";

    ls->menuToolsMachine        = "Tietokonemuokkain";
    ls->menuToolsShortcuts      = "Pikanäppäinasetukset";
    ls->menuToolsCtrlEditor     = "Peliohjaimet / Näppäimistökartta"; 
    ls->menuToolsMixer          = "Mikseri";
    ls->menuToolsDebugger       = "Debugger";               
    ls->menuToolsTrainer        = "Trainer";                
    ls->menuToolsTraceLogger    = "Seurantalogi";         

    ls->menuFile                = "Tiedosto";
    ls->menuRun                 = "Emulaatio";
    ls->menuWindow              = "Ikkuna";
    ls->menuOptions             = "Asetukset";
    ls->menuTools                = "Työkalut";
    ls->menuHelp                = "Ohje";


    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "OK";
    ls->dlgOpen                 = "Avaa";
    ls->dlgCancel               = "Peruuta";
    ls->dlgSave                 = "Tallenna";
    ls->dlgSaveAs               = "Tallenna nimellä";
    ls->dlgRun                  = "Käynnistä";
    ls->dlgClose                = "Sulje";

    ls->dlgLoadRom              = "blueMSX - Lataa rom tiedosto";
    ls->dlgLoadDsk              = "blueMSX - Lataa dsk tiedosto";
    ls->dlgLoadCas              = "blueMSX - Lataa cas tiedosto";
    ls->dlgLoadRomDskCas        = "blueMSX - Valitse ladattava rom, dsk tai cas tiedosto";
    ls->dlgLoadRomDesc          = "Valitse ladattava ROM-tiedosto:";
    ls->dlgLoadDskDesc          = "Valitse ladattava levytiedosto:";
    ls->dlgLoadCasDesc          = "Valitse ladattava kasettitiedosto:";
    ls->dlgLoadRomDskCasDesc    = "Valitse ladattava ROM-, kasetti- tai levytiedosto:";
    ls->dlgLoadState            = "Lataa tila";
    ls->dlgLoadVideoCapture     = "Lataa videokaappaus";      
    ls->dlgSaveState            = "Tallenna tila nimellä";
    ls->dlgSaveCassette         = "blueMSX - Tallenna kasetti";
    ls->dlgSaveVideoClipAs      = "Tallenna videokaappaus nimellä";      
    ls->dlgAmountCompleted      = "Valmiina:";          
    ls->dlgInsertRom1           = "Aseta moduli porttiin 1";
    ls->dlgInsertRom2           = "Aseta moduli porttiin 2";
    ls->dlgInsertDiskA          = "Aseta levy asemaan A";
    ls->dlgInsertDiskB          = "Aseta levy asemaan B";
    ls->dlgInsertHarddisk       = "Aseta kovalevy";
    ls->dlgInsertCas            = "Aseta kasetti";
    ls->dlgRomType              = "ROM-malli:";
    ls->dlgDiskSize             = "Levyn koko:";             

    ls->dlgTapeTitle            = "blueMSX - Kasetin kohta";
    ls->dlgTapeFrameText        = "Kasetin kohta";
    ls->dlgTapeCurrentPos       = "Tämänhetkinen kohta";
    ls->dlgTapeTotalTime        = "Kokonaispituus";
    ls->dlgTapeSetPosText       = "Kasetin kohta:";
    ls->dlgTapeCustom           = "Näytä erilliset tiedostojen osat";
    ls->dlgTabPosition          = "Kohta";
    ls->dlgTabType              = "Tyyppi";
    ls->dlgTabFilename          = "Tiedostonimi";
    ls->dlgZipReset             = "Uudelleenkäynnistys asettamisen jälkeen";

    ls->dlgAboutTitle           = "Tietoja blueMSX:stä";

    ls->dlgLangLangText         = "Valitse käytettävä kieli";
    ls->dlgLangLangTitle        = "blueMSX - Kielivalinta";

    ls->dlgAboutAbout           = "TIEDOT\r\n====";
    ls->dlgAboutVersion         = "Versio:";
    ls->dlgAboutBuildNumber     = "Käännös:";
    ls->dlgAboutBuildDate       = "Päiväys:";
    ls->dlgAboutCreat           = "Tekijä: Daniel Vik";
    ls->dlgAboutDevel           = "KEHITTÄJÄT\r\n=======";
    ls->dlgAboutThanks          = "ERITYIS KIITOKSET\r\n===========";       // New in 2.7 (retranslate, see english)
    ls->dlgAboutLisence         = "LISENSSI\r\n"
                                  "=====\r\n\r\n"
                                  "Tämä ohjelmisto toimitetaan sellaisenaan ilman minkäänlaista takuuta. "
                                  "Kehittäjät eivät ota mitään vastuuta ohjelman kaytössä tai käytöstä "
                                  "aiheutuneista mahdollisista ongelmista tai vahingoista.\r\n\r\n"
                                  "Lisätietoja löydät osoitteesta www.bluemsx.com.";

    ls->dlgSavePreview          = "Näytä esikatselu";
    ls->dlgSaveDate             = "Tallennettu:";

    ls->dlgRenderVideoCapture   = "blueMSX - Videokaappauksen koostaminen...";  


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - Ominaisuudet";
    ls->propEmulation           = "Emulaatio";
    ls->propVideo               = "Video";
    ls->propSound               = "Ääni";
    ls->propControls            = "Kontrollit";
    ls->propPerformance         = "Suorituskyky";
    ls->propSettings            = "Asetukset";
    ls->propFile                = "Tiedostot";
    ls->propDisk                = "Disks";              // New in 2.7
    ls->propPorts               = "Liitännät";

    ls->propEmuGeneralGB        = "Yleistä ";
    ls->propEmuFamilyText       = "MSX malli:";
    ls->propEmuMemoryGB         = "Muisti ";
    ls->propEmuRamSizeText      = "RAM koko:";
    ls->propEmuVramSizeText     = "VRAM koko:";
    ls->propEmuSpeedGB          = "Emulaationopeus ";
    ls->propEmuSpeedText        = "Emulaationopeus:";
    ls->propEmuFrontSwitchGB     = "Panasonicin kytkimet ";
    ls->propEmuFrontSwitch       = " Aplikaatiokytkin";
    ls->propEmuFdcTiming        = " Poista käytöstä levyaseman ajoitus";
    ls->propEmuReversePlay      = " Salli taaksepäin suoritus"; // New in 2.8.3
    ls->propEmuPauseSwitch      = " PAUSE-näppäin";
    ls->propEmuAudioSwitch       = " MSX-AUDIO modulin kytkin";
    ls->propVideoFreqText       = "Videotaajuus:";
    ls->propVideoFreqAuto       = "Automaattinen";
    ls->propSndOversampleText   = "Ylinäytteistys:";
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 Sisään ";                
    ls->propSndMidiInGB         = "MIDI Sisään ";
    ls->propSndMidiOutGB        = "MIDI Ulos ";
    ls->propSndMidiChannel      = "MIDI-kanava:";                      
    ls->propSndMidiAll          = "Kaikki";                                

    ls->propMonMonGB            = "Monitori ";
    ls->propMonTypeText         = "Monitorin tyyppi:";
    ls->propMonEmuText          = "Monitorin emulointi:";
    ls->propVideoTypeText       = "Videotyyppi:";
    ls->propWindowSizeText      = "Ikkunan koko:";
    ls->propMonHorizStretch     = " Vaakavenytys";
    ls->propMonVertStretch      = " Pystyvenytys";
    ls->propMonDeInterlace      = " Lomituksen purku";
    ls->propBlendFrames         = " Yhdistä peräkkäiset kuvat";
    ls->propMonBrightness       = "Kirkkaus:";
    ls->propMonContrast         = "Kontrasti:";
    ls->propMonSaturation       = "Saturaatio:";
    ls->propMonGamma            = "Gamma:";
    ls->propMonScanlines        = " Juovat:";
    ls->propMonColorGhosting    = " Antennimuunnos:";
    ls->propMonEffectsGB        = "Tehosteet ";

    ls->propPerfVideoDrvGB      = "Videoajuri ";
    ls->propPerfVideoDispDrvText= "Näyttöajuri:";
    ls->propPerfFrameSkipText   = "Ruudunpäivitysten ohitus:";
    ls->propPerfAudioDrvGB      = "Ääniajuri ";
    ls->propPerfAudioDrvText    = "Ääniajuri:";
    ls->propPerfAudioBufSzText  = "Äänipuskurin koko:";
    ls->propPerfEmuGB           = "Emulaatio ";
    ls->propPerfSyncModeText    = "Tahdistutapa:";
    ls->propFullscreenResText   = "Resoluutio kokoruututilassa:";

    ls->propSndChipEmuGB        = "Äänipiirien emulointi ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound        = " Moonsound";
    ls->propSndMt32ToGm         = " Muunna MT-32 soittimet General MIDI soittimiksi";

    ls->propPortsLptGB          = "Rinnakkaisportti ";
    ls->propPortsComGB          = "Sarjaportit ";
    ls->propPortsLptText        = "Portti:";
    ls->propPortsCom1Text       = "Portti 1:";
    ls->propPortsNone           = "Ei mitään";
    ls->propPortsSimplCovox     = "SiMPL / Covox DAC";
    ls->propPortsFile           = "Tulosta tiedostoon";
    ls->propPortsComFile        = "Lähetys tiedostoon";
    ls->propPortsOpenLogFile    = "Avaa lokitiedosto";
    ls->propPortsEmulateMsxPrn  = "Emulaatio:";

    ls->propSetFileHistoryGB    = "Tiedostohistoria ";
    ls->propSetFileHistorySize  = "Tiedostojen lukumäärä:";
    ls->propSetFileHistoryClear = "Tyhjennä historia";
    ls->propFileTypes           = " Rekisteröi tiedostotyypit blueMSX:ään (.rom, .dsk, .cas, .sta)";
    ls->propWindowsEnvGB        = "Windows Ympäristö ";
    ls->propSetScreenSaver      = " Poista ruudunsäästäjä käytöstä, kun blueMSX on käynnissä";
    ls->propDisableWinKeys      = " Ota Windows-näppäimet MSX käyttöön";
    ls->propPriorityBoost       = " Nosta blueMSX:n prioriteettia";
    ls->propScreenshotPng       = " Tallenna kuvaruutukaappaukset PNG-muodossa";
    ls->propEjectMediaOnExit    = " Poista mediat kun ohjelma suljetaan";        // New in 2.8
    ls->propClearHistory        = "Oletko varma, että haluat poistaa kaikki tiedostohistorian tiedot?";
    ls->propOpenRomGB           = "ROM-tiedostojen avaus ";
    ls->propDefaultRomType      = "Oletus ROM tyyppi:";
    ls->propGuessRomType        = "Arvaa ROM tyyppi";

    ls->propSettDefSlotGB       = "Tiedostojen raahaaminen emulaattoriin ";
    ls->propSettDefSlots        = "Aseta ROM Moduliporttiin:";
    ls->propSettDefSlot         = " Moduliportti";
    ls->propSettDefDrives       = "Aseta levy asemaan:";
    ls->propSettDefDrive        = " Levysema";

    ls->propThemeGB             = "Teemat ";
    ls->propTheme               = "Teema:";

    ls->propCdromGB             = "CD-ROM ";         // New in 2.7
    ls->propCdromMethod         = "Käyttötapa:";  // New in 2.7
    ls->propCdromMethodNone     = "None";            // New in 2.7
    ls->propCdromMethodIoctl    = "IOCTL";           // New in 2.7
    ls->propCdromMethodAspi     = "ASPI";            // New in 2.7
    ls->propCdromDrive          = "Asema:";          // New in 2.7


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "Värillinen";
    ls->enumVideoMonGrey        = "Mustavalko";
    ls->enumVideoMonGreen       = "Vihermusta";
    ls->enumVideoMonAmber       = "Amber";

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "Ei käytössä";
    ls->enumVideoEmuYc          = "Y/C kaapeli (tarkka)";
    ls->enumVideoEmuMonitor     = "Monitori";
    ls->enumVideoEmuYcBlur      = "Kohiseva Y/C kaapeli (tarkka)";
    ls->enumVideoEmuComp        = "Komposiitti (samea)";
    ls->enumVideoEmuCompBlur    = "Kohiseva Komposiitti (samea)";
    ls->enumVideoEmuScale2x     = "Scale 2x";
    ls->enumVideoEmuHq2x        = "Hq2x";

    ls->enumVideoSize1x         = "Pieni - 320x200";
    ls->enumVideoSize2x         = "Normaali - 640x400";
    ls->enumVideoSizeFullscreen = "Kokoruututila";

    ls->enumVideoDrvDirectDrawHW= "Kiihdytetty DirectDraw";
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "Ei käytössä";
    ls->enumVideoFrameskip1     = "1 päivitys";
    ls->enumVideoFrameskip2     = "2 päivitystä";
    ls->enumVideoFrameskip3     = "3 päivitystä";
    ls->enumVideoFrameskip4     = "4 päivitystä";
    ls->enumVideoFrameskip5     = "5 päivitystä";

    ls->enumSoundDrvNone        = "Ei ääntä";
    ls->enumSoundDrvWMM         = "WMM-ajuri";
    ls->enumSoundDrvDirectX     = "DirectX-ajuri";

    ls->enumEmuSync1ms          = "Tahdistus MSX:n ruudunpäivitykseen";
    ls->enumEmuSyncAuto         = "Automaattinen (nopea)";
    ls->enumEmuSyncNone         = "Ei käytössä";
    ls->enumEmuSyncVblank       = "Tahdistus PC:n ruudunpäivitykseen";
    ls->enumEmuAsyncVblank      = "Tahdistamaton PC:n ruudunpäivitys";           

    ls->enumControlsJoyNone     = "Tyhjä";
    ls->enumControlsJoyMouse    = "Hiiri";
    ls->enumControlsJoyTetris2Dongle = "Tetris 2 Dongle";
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey Dongle";
    ls->enumControlsJoy2Button = "2-näppäiminen Joystick";                   
    ls->enumControlsJoyGunstick  = "Gun Stick-valopistooli";                      
    ls->enumControlsJoyAsciiLaser="ASCII Plus-X Terminator Laser";      
    ls->enumControlsArkanoidPad  ="Arkanoid Pad";                   // New in 2.7.1
    ls->enumControlsJoyColeco = "ColecoVision Joystick";                

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" Kaksipuolinen, 9 Sektoria";
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" Kaksipuolinen, 8 Sektoria";
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" Yksipuolinen, 9 Sektoria";
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" Yksipuolinen, 8 Sektoria";
    ls->enumDiskSvi525Dbl        = "SVI-328 5.25\" Kaksipuolinen";
    ls->enumDiskSvi525Sgl        = "SVI-328 5.25\" Yksipuolinen";
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" Yksipuolinen";  


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle               = "blueMSX - Tietokonemuokkain";
    ls->confConfigText          = "Tietokoneprofiili:";
    ls->confSlotLayout          = "Slottijärjestys";
    ls->confMemory              = "Muisti";
    ls->confChipEmulation       = "Piirien Emulointi";
    ls->confChipExtras          = "Muuta";

    ls->confOpenRom             = "Valitse ROM tiedosto";
    ls->confSaveTitle           = "blueMSX - Tallenna tietokoneprofiili";
    ls->confSaveText            = "Haluatko korvata tietokoneprofiilin:";
    ls->confSaveAsTitle         = "Tallenna tietokoneprofiili nimellä...";
    ls->confSaveAsMachineName   = "Tietokoneprofiili:";
    ls->confDiscardTitle        = "blueMSX - Tietokonemuokkain";
    ls->confExitSaveTitle       = "blueMSX - Poistu tietokonemuokkaimesta";
    ls->confExitSaveText        = "Haluatko hylätä tekemäsi muutokset nykyiseen tietokoneprofiiliin?";

    ls->confSlotLayoutGB        = "Slotti jako ";
    ls->confSlotExtSlotGB       = "Ulkoiset Slotit ";
    ls->confBoardGB             = "Emolevy ";
    ls->confBoardText           = "Emolevytyyppi:";
    ls->confSlotPrimary         = "Jakamaton";
    ls->confSlotExpanded        = "Jaettu (4 alislottia)";

    ls->confSlotCart            = "Moduliportti";
    ls->confSlot                = "Slotti";
    ls->confSubslot             = "-";

    ls->confMemAdd              = "Lisää...";
    ls->confMemEdit             = "Muuta...";
    ls->confMemRemove           = "Poista";
    ls->confMemSlot             = "Slotti";
    ls->confMemAddresss         = "Muistialue";
    ls->confMemType             = "Tyyppi";
    ls->confMemRomImage         = "ROM tiedosto";

    ls->confChipVideoGB          = "Video ";
    ls->confChipVideoChip        = "Videopiiri:";
    ls->confChipVideoRam         = "Videomuisti:";
    ls->confChipSoundGB          = "Ääni ";
    ls->confChipPsgStereoText    = " PSG Stereo";

    ls->confCmosGB               = "Kellopiiri ";
    ls->confCmosEnable           = " Ota kellopiiri käyttöön";
    ls->confCmosBattery          = " Käytä ladattua paristoa";

    ls->confCpuFreqGB            = "Prosessorikellotaajuudet ";
    ls->confZ80FreqText          = "Z80 Kellotaajuus:";
    ls->confR800FreqText         = "R800 Kellotaajuus:";
    ls->confFdcGB                = "Levyasemaohjain ";
    ls->confCFdcNumDrivesText    = "Levyasemien lukumäärä:";

    ls->confEditMemTitle         = "blueMSX - Muistin hallinta";
    ls->confEditMemGB            = "Muistin tiedot ";
    ls->confEditMemType          = "Tyyppi:";
    ls->confEditMemFile          = "Tiedosto:";
    ls->confEditMemAddress       = "Muistialue";
    ls->confEditMemSize          = "Koko";
    ls->confEditMemSlot          = "Slotti";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "Tapahtuma";
    ls->shortcutDescription     = "Pikanäppäin";

    ls->shortcutSaveConfig      = "blueMSX - Tallenna Asetukset";
    ls->shortcutOverwriteConfig = "Haluatko korvata pikanäppäinasetukset:";
    ls->shortcutExitConfig      = "blueMSX - Poistu pikanäppäinasetuksista";
    ls->shortcutDiscardConfig   = "Haluatko hylätä tekemäsi muutokset pikanäppäinasetuksiin?";
    ls->shortcutSaveConfigAs    = "blueMSX - Tallenna pikanäppäinasetukset";
    ls->shortcutConfigName      = "Asetusten nimi:";
    ls->shortcutNewProfile      = "< Uusi profiili >";
    ls->shortcutConfigTitle     = "blueMSX - Pikanäppäinasetukset";
    ls->shortcutAssign          = "Käytä";
    ls->shortcutPressText       = "Pikanäppäin (paina):";
    ls->shortcutScheme          = "Pikanäppäin profiili:";
    ls->shortcutCartInsert1     = "Aseta moduli 1";
    ls->shortcutCartRemove1     = "Poista moduli 1";
    ls->shortcutCartInsert2     = "Aseta moduli 2";
    ls->shortcutCartRemove2     = "Poista moduli 2";
    ls->shortcutSpecialMenu1    = "Näytä erikoismodulivalikko moduliportille 1";
    ls->shortcutSpecialMenu2    = "Näytä erikoismodulivalikko moduliportille 2";
    ls->shortcutCartAutoReset   = "Käynnistä kone uudelleen modulin asettamisen jälkeen";
    ls->shortcutDiskInsertA     = "Aseta levytiedosto asemaan A";
    ls->shortcutDiskDirInsertA  = "Aseta hakemisto levyasemaksi A";
    ls->shortcutDiskRemoveA     = "Poista levy levyasemasta A";
    ls->shortcutDiskChangeA     = "Pika vaihda levy A";
    ls->shortcutDiskAutoResetA  = "Käynnistä kone uudelleen levyn lisäämisen jälkeen asemaan A";
    ls->shortcutDiskInsertB     = "Aseta levytiedosto asemaan B";
    ls->shortcutDiskDirInsertB  = "Aseta hakemisto levyasemaksi B";
    ls->shortcutDiskRemoveB     = "Poista levy levyasemasta B";
    ls->shortcutCasInsert       = "Aseta kasetti";
    ls->shortcutCasEject        = "Poista kasetti";
    ls->shortcutCasAutorewind   = "Muuta kasetin automaattista alkuunkelaustoimintoa";
    ls->shortcutCasReadOnly     = "Muuta kasetin kirjoitussuojausta";
    ls->shortcutCasSetPosition  = "Aseta kasetin kohta";
    ls->shortcutCasRewind       = "Kelaa kasetti alkuun";
    ls->shortcutCasSave         = "Tallenna kasetti-image";
    ls->shortcutPrnFormFeed     = "Vaihda tulostimessa sivua";
    ls->shortcutCpuStateLoad    = "Lataa tila";
    ls->shortcutCpuStateSave    = "Tallenna tila nimellä";
    ls->shortcutCpuStateQload   = "Tilan pikalataus";
    ls->shortcutCpuStateQsave   = "Tilan pikatallennus";
    ls->shortcutAudioCapture    = "Käynnistä/Pysäytä äänen kaappaus";
    ls->shortcutScreenshotOrig  = "Ota ruudunkaappaus";
    ls->shortcutScreenshotSmall = "Pieni filtteröimätön ruudunkaappaus";
    ls->shortcutScreenshotLarge = "Suuri filtteröimätön ruudunkaappaus";
    ls->shortcutQuit            = "Poistu blueMSX:stä";
    ls->shortcutRunPause        = "Käynnistä/Keskeytä emulaatio";
    ls->shortcutStop            = "Pysäytä emulaatio";
    ls->shortcutResetHard       = "Kylmäkäynnistys";
    ls->shortcutResetSoft       = "Lämminkäynnistys";
    ls->shortcutResetClean      = "Täydellinen uudelleenkäynnistys";
    ls->shortcutSizeSmall       = "Aseta pieni ikkunan koko";
    ls->shortcutSizeNormal      = "Aseta normaali ikkunan koko";
    ls->shortcutSizeFullscreen  = "Aseta kokoruututila";
    ls->shortcutSizeMinimized   = "Pienennä ikkuna";
    ls->shortcutToggleFullscren = "Vaihda kokoruututilaa";
    ls->shortcutVolumeIncrease  = "Nosta äänenvoimakkuutta";
    ls->shortcutVolumeDecrease  = "Laske äänenvoimakkuutta";
    ls->shortcutVolumeMute      = "Hiljennä äänet";
    ls->shortcutVolumeStereo    = "Muuta mono/stereo";
    ls->shortcutSwitchMsxAudio  = "Muuta MSX-AUDIO kytkimen tila";
    ls->shortcutSwitchFront     = "Muuta Panasonic aplikaatiokytkimen tila";
    ls->shortcutSwitchPause     = "Muuta Pause-näppäimen tilaa";
    ls->shortcutToggleMouseLock = "Muuta hiirenkaappauksen tilaa";
    ls->shortcutEmuSpeedMax     = "Maksimi emulointinopeus";
    ls->shortcutEmuPlayReverse  = "Taaksepäin suoritus";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "Vaihda maksimi emulointinopeus";
    ls->shortcutEmuSpeedNormal  = "Normaali emulointinopeus";
    ls->shortcutEmuSpeedInc     = "Nopeuta emulointia";
    ls->shortcutEmuSpeedDec     = "Hidasta emulointia";
    ls->shortcutThemeSwitch     = "Vaihda teemaa";
    ls->shortcutShowEmuProp     = "Näytä emulaatioasetukset";
    ls->shortcutShowVideoProp   = "Näytä videoasetukset";
    ls->shortcutShowAudioProp   = "Näytä ääniasetukset";
    ls->shortcutShowCtrlProp    = "Näytä kontrolliasetukset";
    ls->shortcutShowPerfProp    = "Näytä suorituskykyasetukset";
    ls->shortcutShowSettProp    = "Näytä yleisasetukset";
    ls->shortcutShowPorts       = "Näytä liitäntäasetukset";
    ls->shortcutShowLanguage    = "Näytä kielivalinnat";
    ls->shortcutShowMachines    = "Näytä tietokonemuokkain";
    ls->shortcutShowShortcuts   = "Näytä pikanäppäinvalinnat";
    ls->shortcutShowKeyboard    = "Näytä Peliohjain / Näppäimistöasetukset";
    ls->shortcutShowMixer       = "Näytä Mixeri";
    ls->shortcutShowDebugger    = "Näytä Debugger";
    ls->shortcutShowTrainer     = "Näytä Trainer";
    ls->shortcutShowHelp        = "Näytä ohjeet";
    ls->shortcutShowAbout       = "Näytä Tietoja blueMSX:stä";
    ls->shortcutShowFiles       = "Näytä tiedosto asetukset";
    ls->shortcutToggleSpriteEnable = "Näytä/Piilota Spritet";
    ls->shortcutToggleFdcTiming = "Ota käyttöön/Poista käytöstä levyaseman ajoitus";
    ls->shortcutToggleCpuTrace  = "Ota käyttöön/Poista käytöstä Prosessorin jäljitystoiminto";
    ls->shortcutVideoLoad       = "Lataa Videokaappaus";             
    ls->shortcutVideoPlay       = "Toista viimeisin videokaappaus";   
    ls->shortcutVideoRecord     = "Tallenna videokaappaus";              
    ls->shortcutVideoStop       = "Pysäytä videokaappaus";                
    ls->shortcutVideoRender     = "Koosta videotiedosto";   


    //----------------------
    // Keyboard config lines
    //----------------------

    ls->keyconfigSelectedKey    = "Emuloitava näppäin:";
    ls->keyconfigMappedTo       = "PC-näppäin:";
    ls->keyconfigMappingScheme  = "Näppäinkarttaprofiili:";

    
    //----------------------
    // Rom type lines
    //----------------------

    ls->romTypeStandard         = "Standardi";

    ls->romTypeZenima80         = "Zemina 80 in 1";
    ls->romTypeZenima90         = "Zemina 90 in 1";
    ls->romTypeZenima126        = "Zemina 126 in 1";

    ls->romTypeSccMirrored      = "SCC peilattu";
    ls->romTypeSccExtended      = "SCC laajennettu";

    ls->romTypeKonamiGeneric    = "Konami Yleinen";

    ls->romTypeMirrored         = "Peilattu ROM";
    ls->romTypeNormal           = "Tavallinen ROM";
    ls->romTypeDiskPatch        = "Tavallinen + korjaus levyltä";
    ls->romTypeCasPatch         = "Tavallinen + korjaus kasetilta";
    ls->romTypeTc8566afFdc      = "TC8566AF Diskettiasemaohjain";
    ls->romTypeTc8566afTrFdc    = "TC8566AF Turbo-R Diskettiasemaohjain";
    ls->romTypeMicrosolFdc      = "Microsol Diskettiasemaohjain";
    ls->romTypeNationalFdc      = "National Diskettiasemaohjain";
    ls->romTypePhilipsFdc       = "Philips Diskettiasemaohjain";
    ls->romTypeSvi738Fdc        = "SVI-738 Diskettiasemaohjain";
    ls->romTypeMappedRam        = "Mapper RAM";
    ls->romTypeMirroredRam1k    = "1kB peilattu RAM";
    ls->romTypeMirroredRam2k    = "2kB peilattu RAM";
    ls->romTypeNormalRam        = "Tavallinen RAM";

    ls->romTypeTurborPause      = "Turbo-R Pause";
    ls->romTypeF4deviceNormal   = "Tavallinen F4 portti";
    ls->romTypeF4deviceInvert   = "Käännetty F4 portti";

    ls->romTypeTurborTimer      = "Turbo-R Ajastin";

    ls->romTypeNormal4000       = "Tavallinen #4000";
    ls->romTypeNormalC000       = "Tavallinen #C000";

    ls->romTypeExtRam           = "Ulkoinen RAM";
    ls->romTypeExtRam16         = "16kB Ulkoinen RAM";
    ls->romTypeExtRam32         = "32kB Ulkoinen RAM";
    ls->romTypeExtRam48         = "48kB Ulkoinen RAM";
    ls->romTypeExtRam64         = "64kB Ulkoinen RAM";
    ls->romTypeExtRam512        = "512kB Ulkoinen RAM";
    ls->romTypeExtRam1mb        = "1MB Ulkoinen RAM";
    ls->romTypeExtRam2mb        = "2MB Ulkoinen RAM";
    ls->romTypeExtRam4mb        = "4MB Ulkoinen RAM";

    ls->romTypeSvi328Cart       = "SVI-328 Moduli";
    ls->romTypeSvi328Fdc        = "SVI-328 Levyasemaohjain";
    ls->romTypeSvi328Prn        = "SVI-328 Tulostin";
    ls->romTypeSvi328Uart       = "SVI-328 Sarjaportti";
    ls->romTypeSvi328col80      = "SVI-328 80-merkin kortti";
    ls->romTypeSvi727col80      = "SVI-727 80-merkin kortti";
    ls->romTypeColecoCart       = "Coleco Moduli";
    ls->romTypeSg1000Cart       = "SG-1000 Moduli";
    ls->romTypeSc3000Cart       = "SC-3000 Moduli";

    ls->romTypeMsxPrinter       = "MSX tulostin";
    ls->romTypeTurborPcm        = "Turbo-R PCM piiri";
    
    ls->romTypeNms8280Digitiz   = "Philips NMS-8280 Digitoija";
    ls->romTypeHbiV1Digitiz     = "Sony HBI-V1 Digitoija";
    
    
    //----------------------
    // Debug type lines
    // Note: Only needs translation if debugger is translated
    //----------------------

    ls->dbgMemVisible           = "Näkyvä muisti";
    ls->dbgMemRamNormal         = "Normaali";
    ls->dbgMemRamMapped         = "Sivutettu";
    ls->dbgMemYmf278            = "YMF278 Sample RAM";
    ls->dbgMemAy8950            = "AY8950 Sample RAM";
    ls->dbgMemScc               = "Muisti";
    ls->dbgCallstack            = "Kutsupino";
    ls->dbgRegs                 = "Rekisterit";
    ls->dbgRegsCpu              = "CPU Rekisterit";
    ls->dbgRegsYmf262           = "YMF262 Rekisterit";
    ls->dbgRegsYmf278           = "YMF278 Rekisterit";
    ls->dbgRegsAy8950           = "AY8950 Rekisterit";
    ls->dbgRegsYm2413           = "YM2413 Rekisterit";
    ls->dbgDevRamMapper         = "RAM Mapper";
    ls->dbgDevRam               = "RAM";
    ls->dbgDevF4Device          = "F4 Portti";
    ls->dbgDevKorean80          = "Korea 80";
    ls->dbgDevKorean90          = "Korea 90";
    ls->dbgDevKorean128         = "Korea 128";
    ls->dbgDevFdcMicrosol       = "Microsol levyohjain";

    ls->dbgDevPrinter           = "Tulostin";

    ls->dbgDevSviFdc            = "SVI levyohjain";
    ls->dbgDevSviPrn            = "SVI Tulostin";
    ls->dbgDevSvi80Col          = "SVI 80-merkkiä";

    ls->dbgDevRtc               = "Kellopiiri";
    ls->dbgDevTrPause           = "tR Pause";


    //----------------------
    // Debug type lines
    // Note: Can only be translated to european languages
    //----------------------
   ls->aboutScrollThanksTo      = "Kiitoksen ansaitsevat : ";
   ls->aboutScrollAndYou        = "sekä SINÄ !!!!";
};

#endif
