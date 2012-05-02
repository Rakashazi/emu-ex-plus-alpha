/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageRussian.h,v $
**
** $Revision: 1.9 $
**
** $Date: 2009-04-04 20:57:19 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2008 Daniel Vik
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
#ifndef LANGUAGE_RUSSIAN_H
#define LANGUAGE_RUSSIAN_H

#include "LanguageStrings.h"
 
void langInitRussian(LanguageStrings* ls) 
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalan";
    ls->langChineseSimplified   = "Chinese Simplified";
    ls->langChineseTraditional  = "Chinese Traditional";
    ls->langDutch               = "Dutch";
    ls->langEnglish             = "English";
    ls->langFinnish             = "Finnish";
    ls->langFrench              = "French";
    ls->langGerman              = "German";
    ls->langItalian             = "Italian";
    ls->langJapanese            = "Japanese";
    ls->langKorean              = "Korean";
    ls->langPolish              = "Polish";
    ls->langPortuguese          = "Portuguese";
    ls->langRussian             = "Russian";
    ls->langSpanish             = "Spanish";
    ls->langSwedish             = "Swedish";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "Устройство:";
    ls->textFilename            = "Название файла:";
    ls->textFile                = "Файл";
    ls->textNone                = "Нет";
    ls->textUnknown             = "Неизвестно";


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle            = "blueMSX - Внимание";
    ls->warningDiscardChanges   = "Вы хотите отказаться от изменений?";
    ls->warningOverwriteFile    = "Вы хотите перезаписать файл:";
    ls->errorTitle              = "blueMSX - Ошибка";
    ls->errorEnterFullscreen    = "Не удалось перейти в полноэкранный режим.           \n";
    ls->errorDirectXFailed      = "Не удалось создать DirectX объекты.           \nUsing GDI instead.\nCheck Video properties.";
    ls->errorNoRomInZip         = "Не нашлось .rom файла в zip архиве.";
    ls->errorNoDskInZip         = "Не нашлось .dsk файла в zip архиве.";
    ls->errorNoCasInZip         = "Не нашлось .cas файла в zip архиве.";
    ls->errorNoHelp             = "Не нашлось файла помощи blueMSX.";
    ls->errorStartEmu           = "Не удалось начать эмуляцию.";
    ls->errorPortableReadonly   = "Портативное устройство только для чтения";


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "ROM образ";
    ls->fileAll                 = "Все файлы";
    ls->fileCpuState            = "Состояние CPU";
    ls->fileVideoCapture        = "Захват видео"; 
    ls->fileDisk                = "Образ дискеты";
    ls->fileCas                 = "Образ кассеты";
    ls->fileAvi                 = "Видео клип";    


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- нет текущих файлов -";
    ls->menuInsert              = "Вставить";
    ls->menuEject               = "Убрать";

    ls->menuCartGameReader      = "Game Reader";
    ls->menuCartIde             = "IDE";
    ls->menuCartBeerIde         = "Beer";
    ls->menuCartGIde            = "GIDE";
    ls->menuCartSunriseIde      = "Sunrise";  
    ls->menuCartScsi            = "SCSI";                // New in 2.7
    ls->menuCartMegaSCSI        = "MEGA-SCSI";           // New in 2.7
    ls->menuCartWaveSCSI        = "WAVE-SCSI";           // New in 2.7
    ls->menuCartGoudaSCSI       = "Gouda SCSI";          // New in 2.7
    ls->menuJoyrexPsg           = "Joyrex PSG картридж"; // New in 2.9
    ls->menuCartSCC             = "SCC картридж";
    ls->menuCartSCCPlus         = "SCC-I картридж";
    ls->menuCartFMPac           = "FM-PAC картридж";
    ls->menuCartPac             = "PAC Cartridge";
    ls->menuCartHBI55           = "Sony HBI-55 Cartridge";
    ls->menuCartInsertSpecial   = "Вставить другое";
    ls->menuCartMegaRam         = "MegaRAM";
    ls->menuCartExternalRam     = "Внешняя RAM";
    ls->menuCartEseRam          = "Ese-RAM";             // New in 2.7
    ls->menuCartEseSCC          = "Ese-SCC";             // New in 2.7
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";      // New in 2.7

    ls->menuDiskInsertNew       = "Вставить новый образ дискеты";
    ls->menuDiskInsertCdrom     = "Вставить CD-Rom диск";       // New in 2.7
    ls->menuDiskDirInsert       = "Вставить папку";
    ls->menuDiskAutoStart       = "Сбросить после загрузки дискеты";
    ls->menuCartAutoReset       = "Сбросить после загрузки/удаления";

    ls->menuCasRewindAfterInsert= "Промотать после загрузки";
    ls->menuCasUseReadOnly      = "Испольовать образ кассеты только для чтения";
    ls->lmenuCasSaveAs          = "Сохранить образ кассеты как...";
    ls->menuCasSetPosition      = "Задать позицию";
    ls->menuCasRewind           = "Промотать";

    ls->menuVideoLoad           = "Загрузить...";             
    ls->menuVideoPlay           = "Воспроизвести последнее видео";   
    ls->menuVideoRecord         = "Записать";              
    ls->menuVideoRecording      = "Запись";           
    ls->menuVideoRecAppend      = "Дописать";     
    ls->menuVideoStop           = "Остановить";                
    ls->menuVideoRender         = "Сохранить видео в файл";   

    ls->menuPrnFormfeed         = "Печать страницы";

    ls->menuZoomNormal          = "Маленькое окно";
    ls->menuZoomDouble          = "Обычное окно";
    ls->menuZoomFullscreen      = "На весь экран";
    
    ls->menuPropsEmulation      = "Эмуляция";
    ls->menuPropsVideo          = "Видео";
    ls->menuPropsSound          = "Звук";
    ls->menuPropsControls       = "Управление";
    ls->menuPropsPerformance    = "Быстродействие";
    ls->menuPropsSettings       = "Настройка";
    ls->menuPropsFile           = "Файлы";
    ls->menuPropsDisk           = "Дискеты";               // New in 2.7
    ls->menuPropsLanguage       = "Выбор языка";
    ls->menuPropsPorts          = "Порты";
    
    ls->menuVideoSource         = "Вывод видеоданных";
    ls->menuVideoSourceDefault  = "Не обнаружен видеоисточник";
    ls->menuVideoChipAutodetect = "Автоматически обнаруживать видеочип";
    ls->menuVideoInSource       = "Входной видеосигнал";
    ls->menuVideoInBitmap       = "Файл Bitmap";
    
    ls->menuEthInterface        = "Сетевой интерфейс"; 

    ls->menuHelpHelp            = "Помощь";
    ls->menuHelpAbout           = "О blueMSX";

    ls->menuFileCart            = "Слот для картриджа";
    ls->menuFileDisk            = "Дисковод";
    ls->menuFileCas             = "Кассета";
    ls->menuFilePrn             = "Принтер";
    ls->menuFileLoadState       = "Загрузить состояние CPU";
    ls->menuFileSaveState       = "Сохранить состояние CPU";
    ls->menuFileQLoadState      = "Быстрая загрузка";
    ls->menuFileQSaveState      = "Быстрое сохранение";
    ls->menuFileCaptureAudio    = "Захват звука";
    ls->menuFileCaptureVideo    = "Захват видео"; 
    ls->menuFileScreenShot      = "Сделать скриншот";
    ls->menuFileExit            = "Выход";

    ls->menuFileHarddisk        = "Жесткий диск";
    ls->menuFileHarddiskNoPesent= "Пока нет контроллера";
    ls->menuFileHarddiskRemoveAll= "Извлечь все жесткие диски";    // New in 2.7

    ls->menuRunRun              = "Пуск";
    ls->menuRunPause            = "Пауза";
    ls->menuRunStop             = "Остановить";
    ls->menuRunSoftReset        = "Программный сброс";
    ls->menuRunHardReset        = "Аппаратный сброс";
    ls->menuRunCleanReset       = "Полная перезагрузка";

    ls->menuToolsMachine        = "Настройка машин";
    ls->menuToolsShortcuts      = "Настройка быстрого вызова";
    ls->menuToolsCtrlEditor     = "Настройка контроллеров / клавиатуры"; 
    ls->menuToolsMixer          = "Эквалайзер";
    ls->menuToolsDebugger       = "Дебаггер";               
    ls->menuToolsTrainer        = "Коды";                
    ls->menuToolsTraceLogger    = "Логгер клавиатуры";           

    ls->menuFile                = "Файл";
    ls->menuRun                 = "Эмуляция";
    ls->menuWindow              = "Окно";
    ls->menuOptions             = "Опции";
    ls->menuTools               = "Инструменты";
    ls->menuHelp                = "Помощь";
    

    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "OK";
    ls->dlgOpen                 = "Открыть";
    ls->dlgCancel               = "Отмена";
    ls->dlgSave                 = "Сохранить";
    ls->dlgSaveAs               = "Сохранить как...";
    ls->dlgRun                  = "Пуск";
    ls->dlgClose                = "Закрыть";

    ls->dlgLoadRom              = "blueMSX - Выбрать rom образ для загрузки";
    ls->dlgLoadDsk              = "blueMSX - Выбрать dsk образ для загрузки";
    ls->dlgLoadCas              = "blueMSX - Выбрать cas образ для загрузки";
    ls->dlgLoadRomDskCas        = "blueMSX - Выбрать rom, dsk или cas файлы для загрузки";
    ls->dlgLoadRomDesc          = "Выберите rom образ для загрузки:";
    ls->dlgLoadDskDesc          = "Выберите disk образ для загрузки:";
    ls->dlgLoadCasDesc          = "Выберите tape образ для загрузки:";
    ls->dlgLoadRomDskCasDesc    = "Выберите rom, disk или tape образы для загрузки:";
    ls->dlgLoadState            = "Загрузить";
    ls->dlgLoadVideoCapture     = "Загрузить захваченное видео";      
    ls->dlgSaveState            = "Сохранить как...";
    ls->dlgSaveCassette         = "blueMSX - Сохранить образ кассеты";
    ls->dlgSaveVideoClipAs      = "Сохранить видеоклип как...";      
    ls->dlgAmountCompleted      = "Amount completed:";          
    ls->dlgInsertRom1           = "Вставить ROM картридж в слот 1";
    ls->dlgInsertRom2           = "Вставить ROM картридж в слот 2";
    ls->dlgInsertDiskA          = "Вставить образ дискеты в дисковод A";
    ls->dlgInsertDiskB          = "Вставить образ дискеты в дисковод B";
    ls->dlgInsertHarddisk       = "Вставить жесткий диск";
    ls->dlgInsertCas            = "Вставить кассету";
    ls->dlgRomType              = "Rom тип:";
    ls->dlgDiskSize             = "Емкость:";             

    ls->dlgTapeTitle            = "blueMSX - Позиция кассеты";
    ls->dlgTapeFrameText        = "Позиция кассеты";
    ls->dlgTapeCurrentPos       = "Текущая позиция";
    ls->dlgTapeTotalTime        = "Всего времени";
    ls->dlgTapeSetPosText       = "Позиция кассеты:";
    ls->dlgTapeCustom           = "Показать файлы пользователя";
    ls->dlgTabPosition          = "Позиция";
    ls->dlgTabType              = "Тип";
    ls->dlgTabFilename          = "Название";
    ls->dlgZipReset             = "Сбросить после загрузки";

    ls->dlgAboutTitle           = "blueMSX - О blueMSX";

    ls->dlgLangLangText         = "Выберите язык для blueMSX";
    ls->dlgLangLangTitle        = "blueMSX - Язык";

    ls->dlgAboutAbout           = "О blueMSX\r\n====";
    ls->dlgAboutVersion         = "Версия:";
    ls->dlgAboutBuildNumber     = "Сборка:";
    ls->dlgAboutBuildDate       = "Дата:";
    ls->dlgAboutCreat           = "Создан Daniel Vik";
    ls->dlgAboutDevel           = "Разработчики\r\n========";
    ls->dlgAboutThanks          = "Спонсоры\r\n==========";       // New in 2.7 (retranslate, see english)
    ls->dlgAboutLisence         = "Лицензия\r\n"
                                  "======\r\n\r\n"
                                  "Эта программа распространяется \"как есть\", без всякой гарантии. "
                                  "Автор не несет никакой ответственности за любые убытки, являющиеся "
                                  "результатом действия этой программы.\r\n\r\n"
                                  "Посетите сайт www.bluemsx.com для большей информации.";

    ls->dlgSavePreview          = "Показать превью";
    ls->dlgSaveDate             = "Время:";

    ls->dlgRenderVideoCapture   = "blueMSX - Сжатие захваченного видео...";  


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - Настройки";
    ls->propEmulation           = "Эмуляция";
    ls->propVideo               = "Видео";
    ls->propSound               = "Звук";
    ls->propControls            = "Управление";
    ls->propPerformance         = "Быстродействие";
    ls->propSettings            = "Опции";
    ls->propFile                = "Файлы";
    ls->propDisk                = "Диски";              // New in 2.7
    ls->propPorts               = "Порты";
    
    ls->propEmuGeneralGB        = "Основное ";
    ls->propEmuFamilyText       = "Машина MSX:";
    ls->propEmuMemoryGB         = "Память ";
    ls->propEmuRamSizeText      = "Объем RAM:";
    ls->propEmuVramSizeText     = "Объем VRAM:";
    ls->propEmuSpeedGB          = "Скорость эмуляции ";
    ls->propEmuSpeedText        = "Скорость эмуляции:";
    ls->propEmuFrontSwitchGB    = "Переключатели Panasonic ";
    ls->propEmuFrontSwitch      = " Передний выключатель";
    ls->propEmuFdcTiming        = " Disable Floppy Drive Timing";
    ls->propEmuReversePlay      = " Enable reverse playback"; // New in 2.8.3
    ls->propEmuPauseSwitch      = " Кнопка паузы";
    ls->propEmuAudioSwitch      = " Переключатель картриджа MSX-AUDIO";
    ls->propVideoFreqText       = "Частота экрана:";
    ls->propVideoFreqAuto       = "Авто";
    ls->propSndOversampleText   = "Супердискретизация:";
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 In ";
    ls->propSndMidiInGB         = "MIDI вход ";
    ls->propSndMidiOutGB        = "MIDI выход ";
    ls->propSndMidiChannel      = "MIDI канал:";
    ls->propSndMidiAll          = "Все";

    ls->propMonMonGB            = "Монитор ";
    ls->propMonTypeText         = "Тип монитора:";
    ls->propMonEmuText          = "Эмуляция монитора:";
    ls->propVideoTypeText       = "Тип видео:";
    ls->propWindowSizeText      = "Размер окна:";
    ls->propMonHorizStretch     = " Растянуть по горизонтали";
    ls->propMonVertStretch      = " Растянуть по вертикали";
    ls->propMonDeInterlace      = " Убрать черезполосицу";
    ls->propBlendFrames         = " Смешивать последовательные кадры";
    ls->propMonBrightness       = "Яркость:";
    ls->propMonContrast         = "Контраст:";
    ls->propMonSaturation       = "Насыщенность:";
    ls->propMonGamma            = "Гамма:";
    ls->propMonScanlines        = " Черезполосица:";
    ls->propMonColorGhosting    = " RF-модулятор:";
    ls->propMonEffectsGB        = "Эффекты ";

    ls->propPerfVideoDrvGB      = "Видеодрайвер ";
    ls->propPerfVideoDispDrvText= "Драйвер монитора:";
    ls->propPerfFrameSkipText   = "Пропуск кадров:";
    ls->propPerfAudioDrvGB      = "Аудио драйвер ";
    ls->propPerfAudioDrvText    = "Аудио драйвер:";
    ls->propPerfAudioBufSzText  = "Размер буфера аудио:";
    ls->propPerfEmuGB           = "Эмуляция ";
    ls->propPerfSyncModeText    = "Синхронизация:";
    ls->propFullscreenResText   = "Полноэкранное разрешение:";

    ls->propSndChipEmuGB        = "Эмуляция звукового чипа ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound        = " Moonsound";
    ls->propSndMt32ToGm         = " Map MT-32 инструменты для основного MIDI";

    ls->propPortsLptGB          = "Параллельный порт ";
    ls->propPortsComGB          = "Последовательные порты ";
    ls->propPortsLptText        = "Порт:";
    ls->propPortsCom1Text       = "Портt 1:";
    ls->propPortsNone           = "Нет";
    ls->propPortsSimplCovox     = "SiMPL / Covox DAC";
    ls->propPortsFile           = "Печатать в файл";
    ls->propPortsComFile        = "Отправить в файл";
    ls->propPortsOpenLogFile    = "Открыть лог-файл";
    ls->propPortsEmulateMsxPrn  = "Эмуляция:";

    ls->propSetFileHistoryGB    = "История файлов ";
    ls->propSetFileHistorySize  = "Количество файлов в истории:";
    ls->propSetFileHistoryClear = "Очистить историю";
    ls->propFileTypes           = " Зарегистрировать типы файлов с blueMSX (.rom, .dsk, .cas, .sta)";
    ls->propWindowsEnvGB        = "Настройки Windows "; 
    ls->propSetScreenSaver      = " Отключить скринсейвер когда blueMSX работает";
    ls->propDisableWinKeys      = " Отключение кнопок Windows для работы MSX"; 
    ls->propPriorityBoost       = " Повышенный приоритет для blueMSX";
    ls->propScreenshotPng       = " Использовать тип PNG файлов для скриншотов";
    ls->propEjectMediaOnExit    = " Eject media when blueMSX exits";        // New in 2.8
    ls->propClearHistory        = "Вы точно хотите очистить файл истории?";
    ls->propOpenRomGB           = "Диалог открытия Rom файла ";
    ls->propDefaultRomType      = "Тип файла Rom по умолчанию:";
    ls->propGuessRomType        = "Любой тип Rom";

    ls->propSettDefSlotGB       = "Перетаскивание ";
    ls->propSettDefSlots        = "Вставить Rom в:";
    ls->propSettDefSlot         = " слот";
    ls->propSettDefDrives       = "Вставить дискету в:";
    ls->propSettDefDrive        = " дисковод";

    ls->propThemeGB             = "Тема ";
    ls->propTheme               = "Тема:";

    ls->propCdromGB             = "CD-ROM ";         // New in 2.7
    ls->propCdromMethod         = "Метод доступа:";  // New in 2.7
    ls->propCdromMethodNone     = "Нет";            // New in 2.7
    ls->propCdromMethodIoctl    = "IOCTL";           // New in 2.7
    ls->propCdromMethodAspi     = "ASPI";            // New in 2.7
    ls->propCdromDrive          = "Привод:";          // New in 2.7


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "Цветной";
    ls->enumVideoMonGrey        = "Черно-белый";
    ls->enumVideoMonGreen       = "Зеленый";
    ls->enumVideoMonAmber       = "Монохромный";

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "Нет";
    ls->enumVideoEmuYc          = "Y/C cable (sharp)";
    ls->enumVideoEmuMonitor     = "Монитор";
    ls->enumVideoEmuYcBlur      = "Noisy Y/C cable (sharp)";
    ls->enumVideoEmuComp        = "Composite (blurry)";
    ls->enumVideoEmuCompBlur    = "Noisy Composite (blurry)";
    ls->enumVideoEmuScale2x     = "Scale 2x";
    ls->enumVideoEmuHq2x        = "Hq2x";

    ls->enumVideoSize1x         = "Нормальное - 320x200";
    ls->enumVideoSize2x         = "Удвоенное - 640x400";
    ls->enumVideoSizeFullscreen = "Во весь экран";

    ls->enumVideoDrvDirectDrawHW= "DirectDraw аппаратно"; 
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "Нет";
    ls->enumVideoFrameskip1     = "1 кадр";
    ls->enumVideoFrameskip2     = "2 кадра";
    ls->enumVideoFrameskip3     = "3 кадра";
    ls->enumVideoFrameskip4     = "4 кадра";
    ls->enumVideoFrameskip5     = "5 кадров";

    ls->enumSoundDrvNone        = "Нет звука";
    ls->enumSoundDrvWMM         = "Драйвер WMM";
    ls->enumSoundDrvDirectX     = "DirectX драйвер";

    ls->enumEmuSync1ms          = "Синхрон. с MSX";
    ls->enumEmuSyncAuto         = "Авто (быстр.)";
    ls->enumEmuSyncNone         = "Нет";
    ls->enumEmuSyncVblank       = "Вертикальная синхронизация";
    ls->enumEmuAsyncVblank      = "Асинхронная верт. синхрон.";             

    ls->enumControlsJoyNone     = "Нет";
    ls->enumControlsJoyMouse    = "Мышка";
    ls->enumControlsJoyTetris2Dongle = "Tetris 2 Dongle";
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey Dongle";
    ls->enumControlsJoy2Button = "2-кнопочный джойстик";                   
    ls->enumControlsJoyGunstick  = "Пистолет";                         
    ls->enumControlsJoyAsciiLaser="ASCII Plus-X Terminator Laser";      
    ls->enumControlsArkanoidPad  ="Arkanoid Pad";                   // New in 2.7.1
    ls->enumControlsJoyColeco = "Джойстик от ColecoVision";                

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" двухсторонний, 9 секторов";     
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" двухсторонний, 8 секторов";     
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" односторонний, 9 секторов";     
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" односторонний, 8 секторов";     
    ls->enumDiskSvi525Dbl        = "SVI-328 5.25\" двухсторонний";           
    ls->enumDiskSvi525Sgl        = "SVI-328 5.25\" односторонний"; 
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" односторонний";            


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle               = "blueMSX - Конфигурация машин";
    ls->confConfigText          = "Название профиля";
    ls->confSlotLayout          = "Расположение слота";
    ls->confMemory              = "Память";
    ls->confChipEmulation       = "Эмуляция чипа";
    ls->confChipExtras          = "Дополнительно";

    ls->confOpenRom             = "Открыть ROM образ";
    ls->confSaveTitle           = "blueMSX - Сохранить конфигурацию";
    ls->confSaveText            = "Вы хотите перезаписать конфигурацию:";
    ls->confSaveAsTitle         = "Сохранить конфигурацию как...";
    ls->confSaveAsMachineName   = "Имя машины:";
    ls->confDiscardTitle        = "blueMSX - Конфигурация";
    ls->confExitSaveTitle       = "blueMSX - Выход из редактора конфигураций";
    ls->confExitSaveText        = "Вы хотите отменить все изменения текущей конфигурации?";

    ls->confSlotLayoutGB        = "Расположение слота ";
    ls->confSlotExtSlotGB       = "Внешние слоты ";
    ls->confBoardGB             = "Плата ";
    ls->confBoardText           = "Тип платы:";
    ls->confSlotPrimary         = "Основной";
    ls->confSlotExpanded        = "Расширенный (4 субслота)";

    ls->confSlotCart            = "Картридж";
    ls->confSlot                = "Слот";
    ls->confSubslot             = "Субслот";

    ls->confMemAdd              = "Добавить...";
    ls->confMemEdit             = "Изменить...";
    ls->confMemRemove           = "Убрать";
    ls->confMemSlot             = "Слот";
    ls->confMemAddresss         = "Адрес";
    ls->confMemType             = "Тип";
    ls->confMemRomImage         = "Rom образ";

    ls->confChipVideoGB          = "Видео ";
    ls->confChipVideoChip        = "Видеочип:";
    ls->confChipVideoRam         = "Видеопамять:";
    ls->confChipSoundGB          = "Звук ";
    ls->confChipPsgStereoText    = " PSG стерео";

    ls->confCmosGB               = "CMOS ";
    ls->confCmosEnable           = " Включить CMOS";
    ls->confCmosBattery          = " Использовать заряженную батарейку";

    ls->confCpuFreqGB            = "Частота CPU ";
    ls->confZ80FreqText          = "Z80 частота:";
    ls->confR800FreqText         = "R800 частота:";
    ls->confFdcGB                = "Контроллер дисковода ";
    ls->confCFdcNumDrivesText    = "Количество дисководов:";

    ls->confEditMemTitle         = "blueMSX - Изменение маппера";
    ls->confEditMemGB            = "Подробности маппера ";
    ls->confEditMemType          = "Тип:";
    ls->confEditMemFile          = "Файл:";
    ls->confEditMemAddress       = "Адрес";
    ls->confEditMemSize          = "Размер";
    ls->confEditMemSlot          = "Слот";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "Действие";
    ls->shortcutDescription     = "Кнопки";

    ls->shortcutSaveConfig      = "blueMSX - Сохранить конфигурацию";
    ls->shortcutOverwriteConfig = "Вы хотите перезаписать конфигурацию кнопок:";
    ls->shortcutExitConfig      = "blueMSX - Выход из редактора кнопок";
    ls->shortcutDiscardConfig   = "Вы хотите отменить все текущие изменени?";
    ls->shortcutSaveConfigAs    = "blueMSX - Сохранить конфигурацию как...";
    ls->shortcutConfigName      = "Имя конфига:";
    ls->shortcutNewProfile      = "< Новый профайл >";
    ls->shortcutConfigTitle     = "blueMSX - Редактор кнопок";
    ls->shortcutAssign          = "Назначить";
    ls->shortcutPressText       = "Нажмите кнопки:";
    ls->shortcutScheme          = "Схема:";
    ls->shortcutCartInsert1     = "Вставьте картридж 1";
    ls->shortcutCartRemove1     = "Убрать картридж 1";
    ls->shortcutCartInsert2     = "Вставьте картридж 2";
    ls->shortcutCartRemove2     = "Убрать картридж 2";
    ls->shortcutSpecialMenu1    = "Показать специальное меню картриджа 1";
    ls->shortcutSpecialMenu2    = "Показать специальное меню картриджа 2";
    ls->shortcutCartAutoReset   = "Сбросить эмулятор когда вставится картридж";
    ls->shortcutDiskInsertA     = "Вставить дискету A";
    ls->shortcutDiskDirInsertA  = "Вставить папку как дискету A";
    ls->shortcutDiskRemoveA     = "Убрать дискету A";
    ls->shortcutDiskChangeA     = "Быстро сменить дискету A";
    ls->shortcutDiskAutoResetA  = "Сбросить эмулятор когда вставится дискета A";
    ls->shortcutDiskInsertB     = "Вставить дискету B";
    ls->shortcutDiskDirInsertB  = "Вставить папку как дискету B";
    ls->shortcutDiskRemoveB     = "Убрать дискету B";
    ls->shortcutCasInsert       = "Вставить кассету";
    ls->shortcutCasEject        = "Убрать кассету";
    ls->shortcutCasAutorewind   = "Включить автопромотку кассеты";
    ls->shortcutCasReadOnly     = "Включить только чтение кассеты";
    ls->shortcutCasSetPosition  = "Установить позицию кассеты";
    ls->shortcutCasRewind       = "Перемотать кассету";
    ls->shortcutCasSave         = "Сохранить образ кассеты";
    ls->shortcutPrnFormFeed     = "Распечатать";
    ls->shortcutCpuStateLoad    = "Загрузить состояние CPU";
    ls->shortcutCpuStateSave    = "Сохранить состояние CPU";
    ls->shortcutCpuStateQload   = "Быстро загрузить";
    ls->shortcutCpuStateQsave   = "Быстро сохранить";
    ls->shortcutAudioCapture    = "Старт/стоп захват аудио";
    ls->shortcutScreenshotOrig  = "Снять скриншот";
    ls->shortcutScreenshotSmall = "Небольшой скриншот без фильтра";
    ls->shortcutScreenshotLarge = "Большой скриншот без фильтра";
    ls->shortcutQuit            = "Выйти из blueMSX";
    ls->shortcutRunPause        = "Пуск/Пауза эмуляции";
    ls->shortcutStop            = "Остановить эмуляцию";
    ls->shortcutResetHard       = "Аппаратный сброс";
    ls->shortcutResetSoft       = "Программный сброс";
    ls->shortcutResetClean      = "Полный сброс";
    ls->shortcutSizeSmall       = "Установить малый размер окна";
    ls->shortcutSizeNormal      = "Установить обычный размер окна";
    ls->shortcutSizeFullscreen  = "Установить на полный экран";
    ls->shortcutSizeMinimized   = "Свернуть";
    ls->shortcutToggleFullscren = "Во весь экран";
    ls->shortcutVolumeIncrease  = "Увеличить громкость";
    ls->shortcutVolumeDecrease  = "Уменьшить громкость";
    ls->shortcutVolumeMute      = "Убрать звук";
    ls->shortcutVolumeStereo    = "Включить моно/стерео";
    ls->shortcutSwitchMsxAudio  = "Включить MSX-AUDIO";
    ls->shortcutSwitchFront     = "Включить переключатель Panasonic";
    ls->shortcutSwitchPause     = "Включить переключатель паузы";
    ls->shortcutToggleMouseLock = "Включить блокировку мышки";
    ls->shortcutEmuSpeedMax     = "Максимальная скорость эмуляции";
    ls->shortcutEmuPlayReverse  = "Rewind emulation";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "Включить максимальную скорость эмуляции";
    ls->shortcutEmuSpeedNormal  = "Обыная скорость эмуляции";
    ls->shortcutEmuSpeedInc     = "Увеличить скорость эмуляции";
    ls->shortcutEmuSpeedDec     = "Уменьшить скорость эмуляции";
    ls->shortcutThemeSwitch     = "Сменить тему";
    ls->shortcutShowEmuProp     = "Показать настройки эмуляции";
    ls->shortcutShowVideoProp   = "Показать настройки видео";
    ls->shortcutShowAudioProp   = "Показать настройки аудио";
    ls->shortcutShowCtrlProp    = "Показать настройки управления";
    ls->shortcutShowPerfProp    = "Показать настройки быстродействия";
    ls->shortcutShowSettProp    = "Показать настройки";
    ls->shortcutShowPorts       = "Показать настройки портов";
    ls->shortcutShowLanguage    = "Показать выбор языка";
    ls->shortcutShowMachines    = "Показать редактор машин";
    ls->shortcutShowShortcuts   = "Показать редактор быстрого вызова";
    ls->shortcutShowKeyboard    = "Показать редактор контроллеров / клавиатуры";
    ls->shortcutShowMixer       = "Показать эквалайзер";
    ls->shortcutShowDebugger    = "Показать дебаггер";
    ls->shortcutShowTrainer     = "Показать коды";
    ls->shortcutShowHelp        = "Показать помощь";
    ls->shortcutShowAbout       = "Показать об эмуляторе";    
    ls->shortcutShowFiles       = "Показать настройки файлов";
    ls->shortcutToggleSpriteEnable = "показать/спрятать спрайты";
    ls->shortcutToggleFdcTiming = "Включить/выключить таймнг дисковода";
    ls->shortcutToggleCpuTrace  = "Включить/выключить CPU Trace";
    ls->shortcutVideoLoad       = "Загрузить видеозахват";             
    ls->shortcutVideoPlay       = "Воспроизвести видеозахват";   
    ls->shortcutVideoRecord     = "Record Video Capture";              
    ls->shortcutVideoStop       = "Остановить видеозахват";                
    ls->shortcutVideoRender     = "Сжать видеозахват";   


    //----------------------
    // Keyboard config lines
    //----------------------    
 
    ls->keyconfigSelectedKey    = "Выберите кнопку:";
    ls->keyconfigMappedTo       = "Mapped To:";
    ls->keyconfigMappingScheme  = "Схема:";

    
    //----------------------
    // Rom type lines
    //----------------------

    ls->romTypeStandard         = "Стандарт";
    ls->romTypeZenima80         = "Zemina 80 in 1";
    ls->romTypeZenima90         = "Zemina 90 in 1";
    ls->romTypeZenima126        = "Zemina 126 in 1";
    ls->romTypeSccMirrored      = "SCC mirrored";
    ls->romTypeSccExtended      = "SCC extended";
    ls->romTypeKonamiGeneric    = "Konami Generic";
    ls->romTypeMirrored         = "Mirrored ROM";
    ls->romTypeNormal           = "Обычный ROM";
    ls->romTypeDiskPatch        = "Обычный + патч дискеты";
    ls->romTypeCasPatch         = "Обычный + патч кассеты";
    ls->romTypeTc8566afFdc      = "TC8566AF контроллер дисковода";
    ls->romTypeTc8566afTrFdc    = "TC8566AF Turbo-R контроллер дисковода";
    ls->romTypeMicrosolFdc      = "Microsol контроллер дисковода";
    ls->romTypeNationalFdc      = "National контроллер дисковода";
    ls->romTypePhilipsFdc       = "Philips контроллер дисковода";
    ls->romTypeSvi738Fdc        = "SVI-738 контроллер дисковода";
    ls->romTypeMappedRam        = "Mapped RAM";
    ls->romTypeMirroredRam1k    = "1kB Mirrored RAM";
    ls->romTypeMirroredRam2k    = "2kB Mirrored RAM";
    ls->romTypeNormalRam        = "Обычная RAM";
    ls->romTypeTurborPause      = "Turbo-R Pause";
    ls->romTypeF4deviceNormal   = "F4 Device Normal";
    ls->romTypeF4deviceInvert   = "F4 Device Inverted";
    ls->romTypeTurborTimer      = "Turbo-R Timer";
    ls->romTypeNormal4000       = "Normal 4000h";
    ls->romTypeNormalC000       = "Normal C000h";
    ls->romTypeExtRam           = "Внешняя RAM";
    ls->romTypeExtRam16         = "16kB внешняя RAM";
    ls->romTypeExtRam32         = "32kB внешняя RAM";
    ls->romTypeExtRam48         = "48kB внешняя RAM";
    ls->romTypeExtRam64         = "64kB внешняя RAM";
    ls->romTypeExtRam512        = "512kB внешняя RAM";
    ls->romTypeExtRam1mb        = "1MB внешняя RAM";
    ls->romTypeExtRam2mb        = "2MB внешняя RAM";
    ls->romTypeExtRam4mb        = "4MB внешняя RAM";
    ls->romTypeSvi328Cart       = "SVI-328 картридж";
    ls->romTypeSvi328Fdc        = "SVI-328 контроллер дисковода";
    ls->romTypeSvi328Prn        = "SVI-328 Принтер";
    ls->romTypeSvi328Uart       = "SVI-328 последовательный порт";
    ls->romTypeSvi328col80      = "SVI-328 80 Column Card";
    ls->romTypeSvi727col80      = "SVI-727 80 Column Card";
    ls->romTypeColecoCart       = "Картридж Coleco";
    ls->romTypeSg1000Cart       = "Картридж SG-1000";
    ls->romTypeSc3000Cart       = "Картридж SC-3000";
    ls->romTypeMsxPrinter       = "MSX принтер";
    ls->romTypeTurborPcm        = "Turbo-R PCM Chip";
    ls->romTypeNms8280Digitiz   = "Philips NMS-8280 Digitizer";
    ls->romTypeHbiV1Digitiz     = "Sony HBI-V1 Digitizer";
    
    
    //----------------------
    // Debug type lines
    // Note: Only needs translation if debugger is translated
    //----------------------

    ls->dbgMemVisible           = "Видимая память";
    ls->dbgMemRamNormal         = "Обычная";
    ls->dbgMemRamMapped         = "Отображаемая";
    ls->dbgMemYmf278            = "YMF278 Sample RAM";
    ls->dbgMemAy8950            = "AY8950 Sample RAM";
    ls->dbgMemScc               = "Память";

    ls->dbgCallstack            = "Стек вызовов";

    ls->dbgRegs                 = "Регистры";
    ls->dbgRegsCpu              = "CPU регистры";
    ls->dbgRegsYmf262           = "YMF262 регистры";
    ls->dbgRegsYmf278           = "YMF278 регистры";
    ls->dbgRegsAy8950           = "AY8950 регистры";
    ls->dbgRegsYm2413           = "YM2413 регистры";

    ls->dbgDevRamMapper         = "RAM Mapper";
    ls->dbgDevRam               = "RAM";
    ls->dbgDevF4Device          = "F4 Device";
    ls->dbgDevKorean80          = "Korean 80";
    ls->dbgDevKorean90          = "Korean 90";
    ls->dbgDevKorean128         = "Korean 128";
    ls->dbgDevFdcMicrosol       = "Microsol FDC";
    ls->dbgDevPrinter           = "Принтер";
    ls->dbgDevSviFdc            = "SVI FDC";
    ls->dbgDevSviPrn            = "SVI Принтер";
    ls->dbgDevSvi80Col          = "SVI 80 Column";
    ls->dbgDevRtc               = "RTC";
    ls->dbgDevTrPause           = "TR Pause";


    //----------------------
    // Debug type lines
    // Note: Can only be translated to european languages
    //----------------------

    ls->aboutScrollThanksTo     = "Special thanks to: ";
    ls->aboutScrollAndYou       = "and YOU !!!!";
};

#endif
