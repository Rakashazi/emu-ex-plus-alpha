/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageSpannish.h,v $
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
#ifndef LANGUAGE_SPANISH_H
#define LANGUAGE_SPANISH_H

#include "LanguageStrings.h"

void langInitSpanish(LanguageStrings* ls)
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalan";
    ls->langChineseSimplified   = "Chino simplificado";
    ls->langChineseTraditional  = "Chino tradicional";
    ls->langDutch               = "Holandés";
    ls->langEnglish             = "Inglés";
    ls->langFinnish             = "Finlandés";
    ls->langFrench              = "Francés";
    ls->langGerman              = "Alemán";
    ls->langItalian             = "Italiano";
    ls->langJapanese            = "Japonés";
    ls->langKorean              = "Coreano";
    ls->langPolish              = "Polaco";
    ls->langPortuguese          = "Portugués";
    ls->langRussian             = "Russian";            // v2.8
    ls->langSpanish             = "Español";
    ls->langSwedish             = "Sueco";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "Tipo:"; 
    ls->textFilename            = "Nombre:"; 
    ls->textFile                = "Archivo"; 
    ls->textNone                = "Ninguno"; 
    ls->textUnknown             = "Desconocido";                            


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle             = "blueMSX - Advertencia";
    ls->warningDiscardChanges   = "Quieres descartar cambios de la configuración actual?"; 
    ls->warningOverwriteFile    = "Usted desea sobreescribir este archivo?:"; 
    ls->errorTitle              = "blueMSX - Error";
    ls->errorEnterFullscreen    = "Error al intentar Modo Pantalla Completa             \n";
    ls->errorDirectXFailed      = "Error al crear objetos DirectX.           \n.\nComprueba configuración de Vídeo.";
    ls->errorNoRomInZip         = "No hay archivo .rom en el archivo zip.";
    ls->errorNoDskInZip         = "No hay archivo .dsk en el archivo zip.";
    ls->errorNoCasInZip         = "No hay archivo .cas en el archivo zip.";
    ls->errorNoHelp             = "Imposible encontrar archivo de ayuda de BlueMSX.";
    ls->errorStartEmu           = "Error al iniciar MSX emulator.";
    ls->errorPortableReadonly   = "El dispositivo portable es sólo lectura";        


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "ROM image"; 
    ls->fileAll                 = "Todos los archivos"; 
    ls->fileCpuState            = "Carga CPU"; 
    ls->fileVideoCapture        = "Captura video"; 
    ls->fileDisk                = "Disk Image"; 
    ls->fileCas                 = "Tape Image"; 
    ls->fileAvi                 = "Video Clip";    


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- No hay archivos recientes -";
    ls->menuInsert              = "Insertar";
    ls->menuEject               = "Sacar";

    ls->menuCartGameReader      = "Game Reader";                        
    ls->menuCartIde             = "IDE";                                
    ls->menuCartBeerIde         = "Beer";                               
    ls->menuCartGIde            = "GIDE";                               
    ls->menuCartSunriseIde      = "Sunrise";                              
    ls->menuCartScsi            = "SCSI";                // New in 2.7
    ls->menuCartMegaSCSI        = "MEGA-SCSI";           // New in 2.7
    ls->menuCartWaveSCSI        = "WAVE-SCSI";           // New in 2.7
    ls->menuCartGoudaSCSI       = "Gouda SCSI";          // New in 2.7
    ls->menuJoyrexPsg           = "Cartucho Joyrex PSG"; // New in 2.9
    ls->menuCartSCCPlus         = "Cartucho SCC-I";
    ls->menuCartSCC             = "Cartucho SCC";
    ls->menuCartFMPac           = "Cartucho FM-PAC";
    ls->menuCartPac             = "Cartucho PAC";
    ls->menuCartHBI55           = "Cartucho Sony HBI-55"; 
    ls->menuCartInsertSpecial   = "Insertar Especial";                     
    ls->menuCartMegaRam         = "MegaRAM";                            
    ls->menuCartExternalRam     = "RAM externo"; 
    ls->menuCartEseRam          = "Ese-RAM";             // New in 2.7
    ls->menuCartEseSCC          = "Ese-SCC";             // New in 2.7
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";      // New in 2.7

    ls->menuDiskInsertNew       = "Insertar nueva imagen de Disco";              
    ls->menuDiskInsertCdrom     = "Insertar CD-Rom";       // New in 2.7
    ls->menuDiskDirInsert       = "Insertar directorio"; 
    ls->menuDiskAutoStart       = "Reinicio Tras Insertar";
    ls->menuCartAutoReset       = "Reinicio Tras Insertar/Sacar";

    ls->menuCasRewindAfterInsert = "Rebobinar tras Insertar";
    ls->menuCasUseReadOnly       = "Usar Imagen de Cassette Sólo Lectura";
    ls->lmenuCasSaveAs           = "Salvar Imagen de Cassette Como...";
    ls->menuCasSetPosition      = "Posicionar";
    ls->menuCasRewind           = "Rebobinar";

    ls->menuVideoLoad           = "Cargar...";             
    ls->menuVideoPlay           = "Ver la más reciente captura";   
    ls->menuVideoRecord         = "Grabar";              
    ls->menuVideoRecording      = "Grabar en curso";           
    ls->menuVideoRecAppend      = "Grabar (añadir)";     
    ls->menuVideoStop           = "Parada";                
    ls->menuVideoRender         = "Crear video clip";   

    ls->menuPrnFormfeed         = "Página siguiente"; 

    ls->menuZoomNormal          = "Tamaño Pequeña";
    ls->menuZoomDouble          = "Tamaño Normal";
    ls->menuZoomFullscreen      = "Pantalla Completa";

    ls->menuPropsEmulation      = "Emulación";
    ls->menuPropsVideo          = "Vídeo";
    ls->menuPropsSound          = "Sonido";
    ls->menuPropsControls       = "Controles";
    ls->menuPropsPerformance    = "Rendimiento";
    ls->menuPropsSettings        = "Configuraciones";
    ls->menuPropsFile           = "Archivo";
    ls->menuPropsDisk           = "Discos";               // New in 2.7
    ls->menuPropsLanguage       = "Idioma";
    ls->menuPropsPorts          = "Puertos"; 

    ls->menuVideoSource         = "Salida Vídeo";                   
    ls->menuVideoSourceDefault  = "Salida Vídeo no conectada";      
    ls->menuVideoChipAutodetect = "Detección automática";     
    ls->menuVideoInSource       = "Entrada vídeo";                    
    ls->menuVideoInBitmap       = "Archivo Bitmap";                        
    
    ls->menuEthInterface        = "Interfaz De Ethernet"; 

    ls->menuHelpHelp            = "Ayuda";
    ls->menuHelpAbout           = "Acerca De blueMSX";

    ls->menuFileCart            = "Cartucho Slot";
    ls->menuFileDisk            = "Unidad Disco";
    ls->menuFileCas             = "Cassette";
    ls->menuFilePrn             = "Impresora"; 
    ls->menuFileLoadState       = "Estado Carga CPU";
    ls->menuFileSaveState       = "Grabar Estado CPU";
    ls->menuFileQLoadState      = "Carga rápida Estado";
    ls->menuFileQSaveState      = "Grabación Rápida Estado";
    ls->menuFileCaptureAudio    = "Captura Audio";
    ls->menuFileCaptureVideo    = "Captura Video"; 
    ls->menuFileScreenShot      = "Grabar Pantalla";
    ls->menuFileExit            = "Salir";

    ls->menuFileHarddisk        = "Disco duro";                          
    ls->menuFileHarddiskNoPesent= "No hay controladores";             
    ls->menuFileHarddiskRemoveAll= "Sacar todos los discos duros";    // New in 2.7

    ls->menuRunRun              = "Ejecutar";
    ls->menuRunPause            = "Pausar";
    ls->menuRunStop             = "Parada";
    ls->menuRunSoftReset        = "Reinicio Software";
    ls->menuRunHardReset        = "Reinicio Hardware";
    ls->menuRunCleanReset       = "Reinicio Completo";

    ls->menuToolsMachine        = "Editor de Máquina";
    ls->menuToolsShortcuts      = "Editor de Atajos";
    ls->menuToolsCtrlEditor     = "Editor de controladores/teclado";  
    ls->menuToolsMixer          = "Mezclador de Audio"; 
    ls->menuToolsDebugger       = "Debugger";               
    ls->menuToolsTrainer        = "Trainer";                
    ls->menuToolsTraceLogger    = "Trace Logger";           

    ls->menuFile                = "Archivo";
    ls->menuRun                 = "Emulación";
    ls->menuWindow              = "Ventana";
    ls->menuOptions             = "Opciones";
    ls->menuTools                = "Herramientas";
    ls->menuHelp                = "Ayuda";


    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "OK";
    ls->dlgOpen                 = "Open";
    ls->dlgCancel               = "Cancelar";
    ls->dlgSave                 = "Guardar";
    ls->dlgSaveAs               = "Guardar Como...";
    ls->dlgRun                  = "Ejecutar";
    ls->dlgClose                = "Cerrar";

    ls->dlgLoadRom              = "blueMSX - Seleccionar imagen Rom a cargar";
    ls->dlgLoadDsk              = "blueMSX - Seleccionar imagen Dsk a cargar";
    ls->dlgLoadCas              = "blueMSX - Seleccionar imagen Cas a cargar";
    ls->dlgLoadRomDskCas        = "blueMSX - Seleccionar un archivo Rom, Dsk, or Cas a cargar";
    ls->dlgLoadRomDesc          = "Seleccionar imagen Rom a cargar:";
    ls->dlgLoadDskDesc          = "Seleccionar imagen de disco a cargar:";
    ls->dlgLoadCasDesc          = "Seleccionar imagen de cinta a cargar:";
    ls->dlgLoadRomDskCasDesc    = "Seleccionar imagen Rom, Disco, o Cinta a cargar:";
    ls->dlgLoadState            = "Cargar Estado CPU";
    ls->dlgLoadVideoCapture     = "Cargar captura video";      
    ls->dlgSaveState            = "Salvar Estado CPU";
    ls->dlgSaveCassette          = "blueMSX - Salvar Imagen de Cinta";
    ls->dlgSaveVideoClipAs      = "Guardar video clip como...";      
    ls->dlgAmountCompleted      = "Guardar en curso:";          
    ls->dlgInsertRom1           = "Insertar Cartucho ROM en slot 1";
    ls->dlgInsertRom2           = "Insertar Cartucho ROM en slot 2";
    ls->dlgInsertDiskA          = "Insertar imagen de Disco en Unidad A";
    ls->dlgInsertDiskB          = "Insertar imagen de Disco en Unidad B";
    ls->dlgInsertHarddisk       = "Insertar disco duro";                   
    ls->dlgInsertCas            = "Insertar Cinta de cassette";
    ls->dlgRomType              = "Tipo rom:"; 
    ls->dlgDiskSize             = "Tamaño del disco:";             

    ls->dlgTapeTitle            = "blueMSX - Posición de la Cinta";
    ls->dlgTapeFrameText        = "Posición de la Cinta";
    ls->dlgTapeCurrentPos       = "Posición Actual";
    ls->dlgTapeTotalTime        = "Tiempo Total";
    ls->dlgTapeSetPosText        = "Posición de la Cinta:";
    ls->dlgTapeCustom            = "Mostrar Archivos Personalizados";
    ls->dlgTabPosition           = "Posición";
    ls->dlgTabType               = "Tipo";
    ls->dlgTabFilename           = "Nombre Archivo";
    ls->dlgZipReset             = "Reiniciar Tras Insertar";

    ls->dlgAboutTitle           = "blueMSX - Acerca de";

    ls->dlgLangLangText         = "Seleccionar Idioma";
    ls->dlgLangLangTitle        = "blueMSX - Idioma";

    ls->dlgAboutAbout           = "Acerca de\r\n====";
    ls->dlgAboutVersion         = "Versión:";
    ls->dlgAboutBuildNumber     = "Compilación:";
    ls->dlgAboutBuildDate       = "Fecha:";
    ls->dlgAboutCreat           = "Desarrollado por Daniel Vik";
    ls->dlgAboutDevel           = "PROGRAMADORES\r\n========";
    ls->dlgAboutThanks          = "CONTRIBUIDORES\r\n============";       // New in 2.7 (retranslate, see english)
    ls->dlgAboutLisence         = "LICENCIA\r\n"
                                  "======\r\n\r\n"
                                  "Este software se proporciona tal y como es, sin ninguna garantía."
                                  "En ningún caso el/los autores serán responsablesny de posibles daños "
                                  "producidos por el uso de este software.\r\n\r\n"
                                  "Para más detalles, visita la web www.bluemsx.com.";

    ls->dlgSavePreview          = "Imaginar"; 
    ls->dlgSaveDate             = "Fecha:"; 

    ls->dlgRenderVideoCapture   = "blueMSX - Crear video clip...";  


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - Propiedades";
    ls->propEmulation           = "Emulación";
    ls->propVideo               = "Video";
    ls->propSound               = "Sonido";
    ls->propControls            = "Controles";
    ls->propPerformance         = "Rendimiento";
    ls->propSettings             = "Configuraciones";
    ls->propFile                = "Archivo"; 
    ls->propDisk                = "Discos";              // New in 2.7
    ls->propPorts               = "Puertos";

    ls->propEmuGeneralGB        = "General ";
    ls->propEmuFamilyText       = "Familia MSX:";
    ls->propEmuMemoryGB         = "Memoria ";
    ls->propEmuRamSizeText      = "Tamaño RAM:";
    ls->propEmuVramSizeText     = "Tamaño VRAM:";
    ls->propEmuSpeedGB          = "Velocidad de la Emulación ";
    ls->propEmuSpeedText        = "Velocidad de la Emulación:";
    ls->propEmuFrontSwitchGB     = "Botones Panasonic "; 
    ls->propEmuFrontSwitch       = " Botón Frontal"; 
    ls->propEmuFdcTiming        = " No sincronizar unidad de disco"; 
    ls->propEmuReversePlay      = " Enable reverse playback"; // New in 2.8.3
    ls->propEmuPauseSwitch      = " Botón Pause"; 
    ls->propEmuAudioSwitch       = " Botón cartucho MSX-AUDIO"; 
    ls->propVideoFreqText       = "Frecuencia video:"; 
    ls->propVideoFreqAuto       = "Auto"; 
    ls->propSndOversampleText   = "Oversample:"; 
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 In ";                
    ls->propSndMidiInGB         = "MIDI In "; 
    ls->propSndMidiOutGB        = "MIDI Out "; 
    ls->propSndMidiChannel      = "Canal MIDI:";                      
    ls->propSndMidiAll          = "Todos";                                

    ls->propMonMonGB            = "Monitor ";
    ls->propMonTypeText         = "Tipo de Monitor:";
    ls->propMonEmuText          = "Emulación del Monitor:";
    ls->propVideoTypeText       = "Tipo de Video:";
    ls->propWindowSizeText      = "Tamaño de Ventana:";
    ls->propMonHorizStretch      = " Estiramiento Horizontal";
    ls->propMonVertStretch       = " Estiramiento Vertical";
    ls->propMonDeInterlace      = " De-entrelace";
    ls->propBlendFrames         = " Mezclar frames consecutivas";           
    ls->propMonBrightness       = "Brillo:";
    ls->propMonContrast         = "Contraste:";
    ls->propMonSaturation       = "Saturación:";
    ls->propMonGamma            = "Gamma:";
    ls->propMonScanlines        = " Scanlines:";
    ls->propMonColorGhosting    = " Modulador RF:"; 
    ls->propMonEffectsGB        = "Efectos  "; 

    ls->propPerfVideoDrvGB      = "Controlador de Video ";
    ls->propPerfVideoDispDrvText= "Controlador de Pantalla:";
    ls->propPerfFrameSkipText   = "Omisión de Frames:";
    ls->propPerfAudioDrvGB      = "Controlador de Audio ";
    ls->propPerfAudioDrvText    = "Controlador de Sonido:";
    ls->propPerfAudioBufSzText  = "Tamaño del Buffer de sonido:";
    ls->propPerfEmuGB           = "Emulación ";
    ls->propPerfSyncModeText    = "Modo SYNC:";
    ls->propFullscreenResText   = "Resolución pantalla completa:"; 

    ls->propSndChipEmuGB        = "Emulación Chip de Sonido ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound         = " Moonsound";
    ls->propSndMt32ToGm         = " Mapa instrumentos MT-32 a General MIDI"; 

    ls->propPortsLptGB          = "Puerto paralelo "; 
    ls->propPortsComGB          = "Puertos seriales "; 
    ls->propPortsLptText        = "Puerto:"; 
    ls->propPortsCom1Text       = "Puerto 1:"; 
    ls->propPortsNone           = "Ninguno";
    ls->propPortsSimplCovox     = "SiMPL / Covox DAC"; 
    ls->propPortsFile           = "Imprimir en archivo"; 
    ls->propPortsComFile        = "Enviar hacia archivo";
    ls->propPortsOpenLogFile    = "Abrir un archivo de datos"; 
    ls->propPortsEmulateMsxPrn  = "Emulación:"; 

    ls->propSetFileHistoryGB     = "Histórico de Archivos ";
    ls->propSetFileHistorySize   = "Número de items en Histórico de Archivos:";
    ls->propSetFileHistoryClear  = "Borrar Hostórico";
    ls->propFileTypes            = " Asociar algunos tipos de archivo con blueMSX";
    ls->propWindowsEnvGB         = "Ambiente Windows ";
    ls->propSetScreenSaver       = " Deshabilitar Salvapantallas mientras blueMSX esté en ejecución";
    ls->propDisableWinKeys       = " Función MSX automática para las teclas Windows"; 
    ls->propPriorityBoost       = " Dar a blueMSX una elevada prioridad";
    ls->propScreenshotPng       = " Utilizar el formato PNG para las capturas de pantalla";  
    ls->propEjectMediaOnExit    = " Eject media when blueMSX exits";        // New in 2.8
    ls->propClearHistory         = "¿Desean realmente borrar hostórico?";
    ls->propOpenRomGB           = "Abrir archivo rom "; 
    ls->propDefaultRomType      = "Tipo por defecto:"; 
    ls->propGuessRomType        = "Conjeturar tipo"; 

    ls->propSettDefSlotGB       = "Deslizar e depositar "; 
    ls->propSettDefSlots        = "Insertar cartucho en:"; 
    ls->propSettDefSlot         = " Slot"; 
    ls->propSettDefDrives       = "Insertar disco en:"; 
    ls->propSettDefDrive        = " Unidad"; 

    ls->propThemeGB             = "Tema ";
    ls->propTheme               = "Tema";

    ls->propCdromGB             = "CD-ROM ";         // New in 2.7
    ls->propCdromMethod         = "Método de acceso:";  // New in 2.7
    ls->propCdromMethodNone     = "Ninguno";            // New in 2.7
    ls->propCdromMethodIoctl    = "IOCTL";           // New in 2.7
    ls->propCdromMethodAspi     = "ASPI";            // New in 2.7
    ls->propCdromDrive          = "Unidad:";          // New in 2.7


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "Color";
    ls->enumVideoMonGrey        = "Blanco y Negro";
    ls->enumVideoMonGreen       = "Verde";
    ls->enumVideoMonAmber       = "Ambar"; 

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "Ninguno";
    ls->enumVideoEmuYc          = "Cable Y/C";
    ls->enumVideoEmuMonitor     = "Monitor"; 
    ls->enumVideoEmuYcBlur      = "Cable Y/C ruidoso";
    ls->enumVideoEmuComp        = "Compuesto";
    ls->enumVideoEmuCompBlur    = "Compuesto Ruidoso";
    ls->enumVideoEmuScale2x     = "Escala 2x";
    ls->enumVideoEmuHq2x        = "Hq2x"; 

    ls->enumVideoSize1x         = "Normal - 320x200";
    ls->enumVideoSize2x         = "Doble - 640x400";
    ls->enumVideoSizeFullscreen = "Pantalla Completa";

    ls->enumVideoDrvDirectDrawHW = "DirectDraw HW acel.";
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "Ninguno";
    ls->enumVideoFrameskip1     = "1 frame";
    ls->enumVideoFrameskip2     = "2 frames";
    ls->enumVideoFrameskip3     = "3 frames";
    ls->enumVideoFrameskip4     = "4 frames";
    ls->enumVideoFrameskip5     = "5 frames";

    ls->enumSoundDrvNone        = "Sin Sonido";
    ls->enumSoundDrvWMM         = "Controlador WMM";
    ls->enumSoundDrvDirectX     = "Controlador DirectX";

    ls->enumEmuSync1ms          = "Sinc sobre MSX refresh"; 
    ls->enumEmuSyncAuto         = "Auto (rápido)"; 
    ls->enumEmuSyncNone         = "Ninguno"; 
    ls->enumEmuSyncVblank       = "Sinc sobre PC Vertical Blank"; 
    ls->enumEmuAsyncVblank      = "Asincrónico PC Vblank";             

    ls->enumControlsJoyNone     = "Ninguno";
    ls->enumControlsJoyMouse    = "Ratón";
    ls->enumControlsJoyTetris2Dongle = "Tetris 2 Dongle"; 
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey Dongle";             
    ls->enumControlsJoy2Button = "2-button Joystick";                   
    ls->enumControlsJoyGunstick  = "Gun Stick";                         
    ls->enumControlsJoyAsciiLaser="ASCII Plus-X Terminator Laser";      
    ls->enumControlsArkanoidPad  ="Arkanoid Pad";                   // New in 2.7.1
    ls->enumControlsJoyColeco = "ColecoVision Joystick";                

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" doble cara, 9 sectores";     
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" doble cara, 8 sectores";     
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" simple cara, 9 sectores";     
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" simple cara, 8 sectores";     
    ls->enumDiskSvi525Dbl        = "SVI-328 5.25\" doble cara";           
    ls->enumDiskSvi525Sgl        = "SVI-328 5.25\" simple cara";  
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" simple cara";           


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle                = "blueMSX - Editor de Configuración de Máquina";
    ls->confConfigText           = "Configuración";
    ls->confSlotLayout           = "Esquema del Slot";
    ls->confMemory               = "Memoria";
    ls->confChipEmulation        = "Emulación de Chip";
    ls->confChipExtras          = "Extras"; 

    ls->confOpenRom             = "Abrirse ROM image"; 
    ls->confSaveTitle            = "blueMSX - Guardar Configuración";
    ls->confSaveText             = "Usted desea sobreescribir la configuración de máquina?:";
    ls->confSaveAsTitle         = "Guardar Como..."; 
    ls->confSaveAsMachineName    = "Nombre de Máquina:";
    ls->confDiscardTitle         = "blueMSX - Configuración";
    ls->confExitSaveTitle        = "blueMSX - Salir del Editor de Configuración";
    ls->confExitSaveText         = "Quieres descartar cambios de la configuración actual?";

    ls->confSlotLayoutGB         = "Esquema del Slot ";
    ls->confSlotExtSlotGB        = "Slots Externos ";
    ls->confBoardGB             = "Sistema "; 
    ls->confBoardText           = "Sistema tipo:"; 
    ls->confSlotPrimary          = "Primario";
    ls->confSlotExpanded         = "Expandido (cuator subslots)";

    ls->confSlotCart             = "Cartucho";
    ls->confSlot                = "Slot"; 
    ls->confSubslot             = "Subslot"; 

    ls->confMemAdd               = "Añadir...";
    ls->confMemEdit              = "Editar...";
    ls->confMemRemove            = "Borrar";
    ls->confMemSlot              = "Slot";
    ls->confMemAddresss          = "Dirección";
    ls->confMemType              = "Tipo";
    ls->confMemRomImage          = "Imagen Rom";
    
    ls->confChipVideoGB          = "Video ";
    ls->confChipVideoChip        = "Chip Video:";
    ls->confChipVideoRam         = "RAM Video:";
    ls->confChipSoundGB          = "Sonido ";
    ls->confChipPsgStereoText    = " PSG Stereo";

    ls->confCmosGB               = "CMOS "; 
    ls->confCmosEnable           = " Activar el CMOS"; 
    ls->confCmosBattery          = " Utilizar una Batería Cargada";

    ls->confCpuFreqGB            = "Frecuencia CPU "; 
    ls->confZ80FreqText          = "Frecuencia Z80:"; 
    ls->confR800FreqText         = "Frecuencia R800:"; 
    ls->confFdcGB                = "Regulador De Diskette "; 
    ls->confCFdcNumDrivesText    = "Número de unidades:"; 

    ls->confEditMemTitle         = "blueMSX - Editar Mapa";
    ls->confEditMemGB            = "Detalles Mapa ";
    ls->confEditMemType          = "Tipo:";
    ls->confEditMemFile          = "Archivo:";
    ls->confEditMemAddress       = "Dirección";
    ls->confEditMemSize          = "Tamaño";
    ls->confEditMemSlot          = "Slot";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "Tecla rápida"; 
    ls->shortcutDescription     = "Atajo"; 

    ls->shortcutSaveConfig      = "blueMSX -  - Guardar Configuración";
    ls->shortcutOverwriteConfig = "Usted desea sobreescribir la configuración del atajo?:";
    ls->shortcutExitConfig      = "blueMSX - Salir del Editor de Atajos";
    ls->shortcutDiscardConfig   = "Quieres descartar cambios de la configuración actual?";
    ls->shortcutSaveConfigAs    = "blueMSX - Guardar Configuración de Atajos como...";
    ls->shortcutConfigName      = "Nombre de la configuración:";
    ls->shortcutNewProfile      = "< Nuevo Perfil >";
    ls->shortcutConfigTitle     = "blueMSX - Editor de la tarjeta de los atajos";
    ls->shortcutAssign          = "Asigne";
    ls->shortcutPressText       = "Apoyar en la(s) tecla(s) del atajo";
    ls->shortcutScheme          = "Disposición:";
    ls->shortcutCartInsert1     = "Insertar Cartucho ROM en slot 1";
    ls->shortcutCartRemove1     = "Sacar Cartucho ROM en slot 1";
    ls->shortcutCartInsert2     = "Insertar Cartucho ROM en slot 2";
    ls->shortcutCartRemove2     = "Sacar Cartucho ROM en slot 2";
    ls->shortcutSpecialMenu1    = "Mostrar el menú especial para cartucho 1 ROM en slot 1";
    ls->shortcutSpecialMenu2    = "Mostrar el menú especial para cartucho 1 ROM en slot 2";
    ls->shortcutCartAutoReset   = "Reiniciar Tras Insertar Cartucho ROM";
    ls->shortcutDiskInsertA     = "Insertar imagen de Disco en Unidad A";
    ls->shortcutDiskDirInsertA  = "Insertar un directorio como disco A"; 
    ls->shortcutDiskRemoveA     = "Sacar imagen de Disco en Unidad A";
    ls->shortcutDiskChangeA     = "Cambiar rápidamente de Disco en Unidad A";
    ls->shortcutDiskAutoResetA  = "Reiniciar Tras Insertar Disco en Unidad A";
    ls->shortcutDiskInsertB     = "Insertar imagen de Disco en Unidad B";
    ls->shortcutDiskDirInsertB  = "Insertar un directorio como disco B";
    ls->shortcutDiskRemoveB     = "Sacar imagen de Disco en Unidad B";
    ls->shortcutCasInsert       = "Insertar Cinsta de cassette";
    ls->shortcutCasEject        = "Sacar Cinsta de cassette";
    ls->shortcutCasAutorewind   = "Rebobinado Automático o no de Cassette";
    ls->shortcutCasReadOnly     = "Cassette en método sólo Lectura o no";
    ls->shortcutCasSetPosition  = "Posicionar Cassette";
    ls->shortcutCasRewind       = "Rebobinar Cassette";
    ls->shortcutCasSave         = "Salvar Imagen de Cassette";
    ls->shortcutPrnFormFeed     = "Paso a la página siguiente"; 
    ls->shortcutCpuStateLoad    = "Cargar Estado CPU";
    ls->shortcutCpuStateSave    = "Grabar Estado CPU";
    ls->shortcutCpuStateQload   = "Cargar rápida Estado CPU";
    ls->shortcutCpuStateQsave   = "Grabar rápida Estado CPU";
    ls->shortcutAudioCapture    = "Iniciar/Pausar captura audio";
    ls->shortcutScreenshotOrig  = "Grabar Pantalla";
    ls->shortcutScreenshotSmall = "Pequeña captura de la pantalla sin filtro";
    ls->shortcutScreenshotLarge = "Gran captura de la pantalla sin filtro";
    ls->shortcutQuit            = "Salir blueMSX";
    ls->shortcutRunPause        = "Ejecutar/Pausar emulación";
    ls->shortcutStop            = "Parada emulación";
    ls->shortcutResetHard       = "Reinicio Hardware";
    ls->shortcutResetSoft       = "Reinicio Software";
    ls->shortcutResetClean      = "Reinicio Completo";
    ls->shortcutSizeSmall       = "Pasar en método ventana tamaño pequeña";
    ls->shortcutSizeNormal      = "Pasar en método ventana tamaño normal";
    ls->shortcutSizeFullscreen  = "Pasar en método pantalla completa";
    ls->shortcutSizeMinimized   = "Reducir la ventana"; 
    ls->shortcutToggleFullscren = "Balanza método ventana/método pantalla completa";
    ls->shortcutVolumeIncrease  = "Aumentar el volumen sonoro";
    ls->shortcutVolumeDecrease  = "Disminuir el volumen sonoro";
    ls->shortcutVolumeMute      = "Parada el sonido";
    ls->shortcutVolumeStereo    = "Balanza mono/stereo";
    ls->shortcutSwitchMsxAudio  = "Impulsar el botón cartucho MSX-AUDIO";
    ls->shortcutSwitchFront     = "Impulsar el botón frontal Panasonic";
    ls->shortcutSwitchPause     = "Impulsar el botón Pause"; 
    ls->shortcutToggleMouseLock = "Activar/desactivar el bloqueo del ratón";
    ls->shortcutEmuSpeedMax     = "Velocidad máxima de la emulación";
    ls->shortcutEmuPlayReverse  = "Rewind emulation";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "Balanza velocitad máxima de la emulación"; 
    ls->shortcutEmuSpeedNormal  = "Velocidad normal de la emulación";
    ls->shortcutEmuSpeedInc     = "Aumentar la velocidad de la emulación";
    ls->shortcutEmuSpeedDec     = "Disminuir la velocidad de la emulación";
    ls->shortcutThemeSwitch     = "Cambiar de tema";
    ls->shortcutShowEmuProp     = "Mostrar las propiedades de la emulación";
    ls->shortcutShowVideoProp   = "Mostraz las propiedades video";
    ls->shortcutShowAudioProp   = "Mostrar las propiedades de sonido";
    ls->shortcutShowCtrlProp    = "Mostrar las propiedades de los controles";
    ls->shortcutShowPerfProp    = "Mostrar las propiedades de redimiento";
    ls->shortcutShowSettProp    = "Mostrar las propiedades de los ajustes";
    ls->shortcutShowPorts       = "Mostrar las propriedades de los puertos";
    ls->shortcutShowLanguage    = "Mostrar diálogo de la idioma";
    ls->shortcutShowMachines    = "Mostrar editor de la máquina";
    ls->shortcutShowShortcuts   = "Mostrar editor de atajos";
    ls->shortcutShowKeyboard    = "Mostrar editor de controladores/teclado"; 
    ls->shortcutShowMixer       = "Mostrar Mezclador de Audio"; 
    ls->shortcutShowDebugger    = "Mostrar Debugger"; 
    ls->shortcutShowTrainer     = "Mostrar Trainer"; 
    ls->shortcutShowHelp        = "Ver la ayuda";
    ls->shortcutShowAbout       = "Ver la rúbrica acerca de blueMSX";
    ls->shortcutShowFiles       = "Mostrar las propiedades de los archivos";
    ls->shortcutToggleSpriteEnable = "Mostrar/ocultar los sprites";
    ls->shortcutToggleFdcTiming = "Sincronizar o no unidade disco"; 
    ls->shortcutToggleCpuTrace  = "Activar/Desactivar Rastro de la CPU"; 
    ls->shortcutVideoLoad       = "Cargar captura video";             
    ls->shortcutVideoPlay       = "Ver la más reciente captura video";   
    ls->shortcutVideoRecord     = "Grabar captura video";              
    ls->shortcutVideoStop       = "Parada captura video";                
    ls->shortcutVideoRender     = "Crear video clip";   


    //----------------------
    // Keyboard config lines
    //----------------------

    ls->keyconfigSelectedKey    = "Tecla MSX:"; 
    ls->keyconfigMappedTo       = "Tecla PC :"; 
    ls->keyconfigMappingScheme  = "Configuración del teclado:"; 

    
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

    ls->aboutScrollThanksTo     = "Gracias especiales a: ";
    ls->aboutScrollAndYou       = "y USTED !!!!";
};

#endif
