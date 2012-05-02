/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Language/LanguageChineseSimplified.h,v $
**
** $Revision: 1.53 $
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
#ifndef LANGUAGE_CHINESE_SIMPLIFIED_H
#define LANGUAGE_CHINESE_SIMPLIFIED_H

#include "LanguageStrings.h"
 
void langInitChineseSimplified(LanguageStrings* ls) 
{
    //----------------------
    // Language lines
    //----------------------

    ls->langCatalan             = "Catalan";
    ls->langChineseSimplified   = "简体中文";
    ls->langChineseTraditional  = "繁体中文";
    ls->langDutch               = "荷兰语";
    ls->langEnglish             = "英语";
    ls->langFinnish             = "芬兰语";
    ls->langFrench              = "法语";
    ls->langGerman              = "德语";
    ls->langItalian             = "意大利语";
    ls->langJapanese            = "日语";
    ls->langKorean              = "朝鲜语";
    ls->langPolish              = "波兰语";
    ls->langPortuguese          = "葡萄牙语";
    ls->langRussian             = "Russian";            // v2.8
    ls->langSpanish             = "西班牙语";
    ls->langSwedish             = "瑞典语";


    //----------------------
    // Generic lines
    //----------------------

    ls->textDevice              = "设备:";
    ls->textFilename            = "文件名:";
    ls->textFile                = "文件";
    ls->textNone                = "无";
    ls->textUnknown             = "未知";                            


    //----------------------
    // Warning and Error lines
    //----------------------

    ls->warningTitle             = "blueMSX - 警告";
    ls->warningDiscardChanges   = "要放弃修改吗？";
    ls->warningOverwriteFile    = "要覆盖原有文件吗:"; 
    ls->errorTitle              = "blueMSX - 错误";
    ls->errorEnterFullscreen    = "无法进入全屏模式。           \n";
    ls->errorDirectXFailed      = "无法创建 DirectX 对象。           \n改为使用GDI。\n请检查显卡设置。";
    ls->errorNoRomInZip         = "无法在 zip 压缩包内找到 .rom 文件。";
    ls->errorNoDskInZip         = "无法在 zip 压缩包内找到 .dsk 文件。";
    ls->errorNoCasInZip         = "无法在 zip 压缩包内找到 .cas 文件。";
    ls->errorNoHelp             = "无法找到 blueMSX 帮助文件。";
    ls->errorStartEmu           = "无法启动 MSX 模拟器。";
    ls->errorPortableReadonly   = "便携设备为只读";        


    //----------------------
    // File related lines
    //----------------------

    ls->fileRom                 = "ROM 映像";
    ls->fileAll                 = "所有文件";
    ls->fileCpuState            = "CPU 状态";
    ls->fileVideoCapture        = "截取视频"; 
    ls->fileDisk                = "软盘映像";
    ls->fileCas                 = "磁带映像";
    ls->fileAvi                 = "剪辑视频";    


    //----------------------
    // Menu related lines
    //----------------------

    ls->menuNoRecentFiles       = "- 无最近的文件 -";
    ls->menuInsert              = "插入";
    ls->menuEject               = "弹出";
    
    ls->menuCartGameReader      = "Game Reader";                        
    ls->menuCartIde             = "IDE";                                
    ls->menuCartBeerIde         = "Beer";                               
    ls->menuCartGIde            = "GIDE";                               
    ls->menuCartSunriseIde      = "Sunrise";                            
    ls->menuCartScsi            = "SCSI";                // New in 2.7
    ls->menuCartMegaSCSI        = "MEGA-SCSI";           // New in 2.7
    ls->menuCartWaveSCSI        = "WAVE-SCSI";           // New in 2.7
    ls->menuCartGoudaSCSI       = "Gouda SCSI";          // New in 2.7
    ls->menuJoyrexPsg           = "Joyrex PSG 卡带"; // New in 2.9
    ls->menuCartSCCPlus         = "SCC-I 卡带";
    ls->menuCartSCC             = "SCC 卡带";
    ls->menuCartFMPac           = "FM-PAC 卡带";
    ls->menuCartPac             = "PAC 卡带";
    ls->menuCartHBI55           = "Sony HBI-55 卡带";
    ls->menuCartInsertSpecial   = "插入特殊";                     
    ls->menuCartMegaRam         = "MegaRAM";                            
    ls->menuCartExternalRam     = "扩展内存";
    ls->menuCartEseRam          = "Ese-RAM";             // New in 2.7
    ls->menuCartEseSCC          = "Ese-SCC";             // New in 2.7
    ls->menuCartMegaFlashRom    = "Mega Flash ROM";      // New in 2.7

    ls->menuDiskInsertNew       = "插入新的软盘映像";              
    ls->menuDiskInsertCdrom     = "Insert CD-Rom";       // New in 2.7
    ls->menuDiskDirInsert       = "插入目录";
    ls->menuDiskAutoStart       = "插入后重置";
    ls->menuCartAutoReset       = "插入/移除后重置";

    ls->menuCasRewindAfterInsert = "插入后倒带";
    ls->menuCasUseReadOnly       = "使用磁带时只读";
    ls->lmenuCasSaveAs           = "另存为...";
    ls->menuCasSetPosition      = "磁带位置设置";
    ls->menuCasRewind           = "倒带";

    ls->menuVideoLoad           = "读取...";             
    ls->menuVideoPlay           = "播放上一个截取视频";   
    ls->menuVideoRecord         = "录制";              
    ls->menuVideoRecording      = "正在录制";           
    ls->menuVideoRecAppend      = "录制 （附加）";     
    ls->menuVideoStop           = "停止";                
    ls->menuVideoRender         = "渲染视频文件";   

    ls->menuZoomNormal          = "标准尺寸";
    ls->menuZoomDouble          = "两倍尺寸";
    ls->menuZoomFullscreen      = "全屏幕";
    
    ls->menuPrnFormfeed         = "换页";
    
    ls->menuPropsEmulation      = "模拟";
    ls->menuPropsVideo          = "视频";
    ls->menuPropsSound          = "声音";
    ls->menuPropsControls       = "控制器";
    ls->menuPropsPerformance    = "性能";
    ls->menuPropsSettings        = "设定";
    ls->menuPropsFile           = "文件";
    ls->menuPropsDisk           = "Disks";               // New in 2.7
    ls->menuPropsLanguage       = "语言";
    ls->menuPropsPorts          = "端口";
    
    ls->menuVideoSource         = "视频输出源";                   
    ls->menuVideoSourceDefault  = "未连接视频输出源";      
    ls->menuVideoChipAutodetect = "自动检测显示芯片";
    ls->menuVideoInSource       = "视频输入源";                    
    ls->menuVideoInBitmap       = "位图文件";                        
    
    ls->menuEthInterface        = "Ethernet"; 

    ls->menuHelpHelp            = "帮助主题";
    ls->menuHelpAbout           = "关于 blueMSX";

    ls->menuFileCart            = "卡带插槽";
    ls->menuFileDisk            = "软盘驱动器";
    ls->menuFileCas             = "磁带机";
    ls->menuFilePrn             = "打印机";
    ls->menuFileLoadState       = "读取 CPU 状态";
    ls->menuFileSaveState       = "保存 CPU 状态";
    ls->menuFileQLoadState      = "快速读取";
    ls->menuFileQSaveState      = "快速保存";
    ls->menuFileCaptureAudio    = "截取音频";
    ls->menuFileCaptureVideo    = "截取视频"; 
    ls->menuFileScreenShot      = "截图";
    ls->menuFileExit            = "退出";

    ls->menuFileHarddisk        = "硬盘";                          
    ls->menuFileHarddiskNoPesent= "当前无控制器";             
    ls->menuFileHarddiskRemoveAll= "Eject All Harddisk";    // New in 2.7

    ls->menuRunRun              = "运行";
    ls->menuRunPause            = "暂停";
    ls->menuRunStop             = "停止";
    ls->menuRunSoftReset        = "软件重置";
    ls->menuRunHardReset        = "硬件重置";
    ls->menuRunCleanReset       = "常规重置";

    ls->menuToolsMachine         = "机型资料编辑";
    ls->menuToolsShortcuts      = "快捷键编辑工具";
    ls->menuToolsCtrlEditor     = "控制器 / 键盘编辑器"; 
    ls->menuToolsMixer          = "混音器";
    ls->menuToolsDebugger       = "调试工具";               
    ls->menuToolsTrainer        = "作弊工具";                
    ls->menuToolsTraceLogger    = "追踪记录工具";           

    ls->menuFile                = "文件";
    ls->menuRun                 = "运行";
    ls->menuWindow              = "窗口";
    ls->menuOptions             = "选项";
    ls->menuTools                = "工具";
    ls->menuHelp                = "帮助";


    //----------------------
    // Dialog related lines
    //----------------------

    ls->dlgOK                   = "确定";
    ls->dlgOpen                 = "打开";
    ls->dlgCancel               = "取消";
    ls->dlgSave                 = "保存";
    ls->dlgSaveAs               = "另存为...";
    ls->dlgRun                  = "运行";
    ls->dlgClose                = "关闭";

    ls->dlgLoadRom              = "blueMSX - 请选择卡带映像";
    ls->dlgLoadDsk              = "blueMSX - 请选择软盘映像";
    ls->dlgLoadCas              = "blueMSX - 请选择磁带映像";
    ls->dlgLoadRomDskCas        = "blueMSX - 请选择卡带、软盘或磁带映像";
    ls->dlgLoadRomDesc          = "请选择要读取的卡带映像:";
    ls->dlgLoadDskDesc          = "请选择要读取的软盘映像:";
    ls->dlgLoadCasDesc          = "请选择要读取的磁带映像:";
    ls->dlgLoadRomDskCasDesc    = "请选择要读取的卡带、软盘或磁带映像:";
    ls->dlgLoadState            = "读取 CPU 状态";
    ls->dlgLoadVideoCapture     = "读取截取视频";      
    ls->dlgSaveState            = "保存 CPU 状态";
    ls->dlgSaveCassette          = "blueMSX - 保存磁带映像";
    ls->dlgSaveVideoClipAs      = "另存剪裁视频为...";     
    ls->dlgAmountCompleted      = "总计完成:";          
    ls->dlgInsertRom1           = "请在插槽 1 插入 ROM 卡带";
    ls->dlgInsertRom2           = "请在插槽 2 插入 ROM 卡带";
    ls->dlgInsertDiskA          = "请在驱动器 A 插入软盘映像";
    ls->dlgInsertDiskB          = "请在驱动器 B 插入软盘映像";
    ls->dlgInsertHarddisk       = "请插入硬盘";                   
    ls->dlgInsertCas            = "请插入磁带";
    ls->dlgRomType              = "Rom 类型:";
    ls->dlgDiskSize             = "软盘大小:";             

    ls->dlgTapeTitle            = "blueMSX - 磁带";
    ls->dlgTapeFrameText        = "磁带位置";
    ls->dlgTapeCurrentPos       = "当前位置";
    ls->dlgTapeTotalTime        = "总时间";
    ls->dlgTapeCustom            = "显示自定义文件";
    ls->dlgTapeSetPosText        = "磁带位置:";
    ls->dlgTabPosition           = "位置";
    ls->dlgTabType               = "类型";
    ls->dlgTabFilename           = "文件名";
    ls->dlgZipReset             = "插入后重置";

    ls->dlgAboutTitle           = "blueMSX - 关于";

    ls->dlgLangLangText         = "请选择 blueMSX 所使用语言";
    ls->dlgLangLangTitle        = "blueMSX - 语言";

    ls->dlgAboutAbout           = "关于\r\n====";
    ls->dlgAboutVersion         = "版本:";
    ls->dlgAboutBuildNumber     = "创建:";
    ls->dlgAboutBuildDate       = "日期:";
    ls->dlgAboutCreat           = "制作:	Daniel Vik";
    ls->dlgAboutDevel           = "各位协力的开发者们\r\n========";
    ls->dlgAboutThanks          = "特别鸣谢\r\n============";       // New in 2.7 (retranslate, see english)
    ls->dlgAboutLisence         = "授权\r\n"
                                  "======\r\n\r\n"
                                  "本软件是根据目前状况而发布的，没有任何明确或默许的授权。 "
                                  "在任何情况下，使用本软件所造成的后果需由用户自己承担， "
                                  "作者将不承担任何责任。\r\n\r\n"
                                  "更多详情请登陆: www.bluemsx.com ";

    ls->dlgSavePreview          = "显示预览";
    ls->dlgSaveDate             = "保存时间:";

    ls->dlgRenderVideoCapture   = "blueMSX - 正在渲染截取的视频...";  


    //----------------------
    // Properties related lines
    //----------------------

    ls->propTitle               = "blueMSX - 属性";
    ls->propEmulation           = "模拟";
    ls->propVideo               = "视频";
    ls->propSound               = "声音";
    ls->propControls            = "控制器";
    ls->propPerformance         = "性能";
    ls->propSettings             = "其他";
    ls->propFile                = "文件";
    ls->propDisk                = "Disks";              // New in 2.7
    ls->propPorts               = "端口";
    
    ls->propEmuGeneralGB        = "常规 ";
    ls->propEmuFamilyText       = "MSX 机型:";
    ls->propEmuMemoryGB         = "内存 ";
    ls->propEmuRamSizeText      = "主内存:";
    ls->propEmuVramSizeText     = "显存:";
    ls->propEmuSpeedGB          = "模拟速度 ";
    ls->propEmuSpeedText        = "模拟速度:";
    ls->propEmuFrontSwitchGB     = "Panasonic 开关 ";
    ls->propEmuFrontSwitch       = " 前端开关";
    ls->propEmuFdcTiming        = " 禁用软盘驱动器计时";
    ls->propEmuReversePlay      = " Enable reverse playback"; // New in 2.8.3
    ls->propEmuPauseSwitch      = " 暂停开关";
    ls->propEmuAudioSwitch       = " MSX-AUDIO 卡开关";
    ls->propVideoFreqText       = "视频频率:";
    ls->propVideoFreqAuto       = "自动";
    ls->propSndOversampleText   = "过采样:";
    ls->propSndYkInGB           = "YK-01/YK-10/YK-20 In ";                
    ls->propSndMidiInGB         = "MIDI 输入 ";
    ls->propSndMidiOutGB        = "MIDI 输出 ";
    ls->propSndMidiChannel      = "MIDI 声道:";                      
    ls->propSndMidiAll          = "全部";                                

    ls->propMonMonGB            = "显示器 ";
    ls->propMonTypeText         = "显示器类型:";
    ls->propMonEmuText          = "显示器模拟:";
    ls->propVideoTypeText       = "视频类型:";
    ls->propWindowSizeText      = "窗口大小:";
    ls->propMonHorizStretch      = " 水平拉伸";
    ls->propMonVertStretch       = " 垂直拉伸";
    ls->propMonDeInterlace      = " 高画质除网格技术";
    ls->propBlendFrames         = " 混合连续帧";           
    ls->propMonBrightness       = "亮度:";
    ls->propMonContrast         = "对比度:";
    ls->propMonSaturation       = "饱和度:";
    ls->propMonGamma            = "伽玛值:";
    ls->propMonScanlines        = " 扫描线:";
    ls->propMonColorGhosting    = " RF-调制器:";
    ls->propMonEffectsGB        = "特效 ";

    ls->propPerfVideoDrvGB      = "视频驱动 ";
    ls->propPerfVideoDispDrvText= "显示驱动:";
    ls->propPerfFrameSkipText   = "跳帧:";
    ls->propPerfAudioDrvGB      = "音频驱动 ";
    ls->propPerfAudioDrvText    = "声音驱动:";
    ls->propPerfAudioBufSzText  = "声音缓冲:";
    ls->propPerfEmuGB           = "模拟 ";
    ls->propPerfSyncModeText    = "同步模式:";
    ls->propFullscreenResText   = "全屏幕分辨率:";

    ls->propSndChipEmuGB        = "声音芯片模拟 ";
    ls->propSndMsxMusic         = " MSX-MUSIC";
    ls->propSndMsxAudio         = " MSX-AUDIO";
    ls->propSndMoonsound         = " Moonsound";
    ls->propSndMt32ToGm         = " 映射 MT-32 乐器为一般 MIDI 设备";

    ls->propPortsLptGB          = "并行端口 ";
    ls->propPortsComGB          = "串行端口 ";
    ls->propPortsLptText        = "端口:";
    ls->propPortsCom1Text       = "端口 1:";
    ls->propPortsNone           = "无";
    ls->propPortsSimplCovox     = "SiMPL/Covox 数模转换器";
    ls->propPortsFile           = "打印到文件";
    ls->propPortsComFile        = "发送到文件";
    ls->propPortsOpenLogFile    = "打开纪录文件";
    ls->propPortsEmulateMsxPrn  = "模拟:";

    ls->propSetFileHistoryGB     = "历史文件 ";
    ls->propSetFileHistorySize   = "历史文件的数量:";
    ls->propSetFileHistoryClear  = "清除历史文件";
    ls->propFileTypes            = " 用 blueMSX 关联文件 ( .rom, .dsk, .cas, .sta)";
    ls->propWindowsEnvGB         = "Windows 环境设定 "; 
    ls->propSetScreenSaver       = " blueMSX 运行时关闭屏幕保护";
    ls->propDisableWinKeys       = " MSX 使用中周围的 Windows 按键无效"; 
    ls->propPriorityBoost       = " 提升 blueMSX 的优先级";
    ls->propScreenshotPng       = " 使用便携网络图像格式 (.png) 的屏幕截图";  
    ls->propEjectMediaOnExit    = " Eject media when blueMSX exits";        // New in 2.8
    ls->propClearHistory         = "确认要清除历史文件？";
    ls->propOpenRomGB           = "打开 Rom 对话框 ";
    ls->propDefaultRomType      = "默认 Rom 类型:";
    ls->propGuessRomType        = "猜测 Rom 类型";

    ls->propSettDefSlotGB       = "拖放 ";
    ls->propSettDefSlots        = "插入 Rom 到:";
    ls->propSettDefSlot         = " 插槽";
    ls->propSettDefDrives       = "插入软盘到:";
    ls->propSettDefDrive        = " 驱动器";

    ls->propThemeGB             = "布景主题 ";
    ls->propTheme               = "布景主题:";

    ls->propCdromGB             = "CD-ROM ";         // New in 2.7
    ls->propCdromMethod         = "Access Method:";  // New in 2.7
    ls->propCdromMethodNone     = "None";            // New in 2.7
    ls->propCdromMethodIoctl    = "IOCTL";           // New in 2.7
    ls->propCdromMethodAspi     = "ASPI";            // New in 2.7
    ls->propCdromDrive          = "Drive:";          // New in 2.7


    //----------------------
    // Dropdown related lines
    //----------------------

    ls->enumVideoMonColor       = "彩色";
    ls->enumVideoMonGrey        = "黑白";
    ls->enumVideoMonGreen       = "绿色";
    ls->enumVideoMonAmber       = "黄色";

    ls->enumVideoTypePAL        = "PAL";
    ls->enumVideoTypeNTSC       = "NTSC";

    ls->enumVideoEmuNone        = "无";
    ls->enumVideoEmuYc          = "Y/C 分离回路 (锐利)";
    ls->enumVideoEmuMonitor     = "显示器";
    ls->enumVideoEmuYcBlur      = "降噪 Y/C 分离回路 (锐利)";
    ls->enumVideoEmuComp        = "复合 (模糊)";
    ls->enumVideoEmuCompBlur    = "降噪复合 (模糊)";
    ls->enumVideoEmuScale2x     = "2 倍柔化";
    ls->enumVideoEmuHq2x        = "Hq2x";

    ls->enumVideoSize1x         = "标准 - 320x200";
    ls->enumVideoSize2x         = "两倍 - 640x400";
    ls->enumVideoSizeFullscreen = "全屏幕";

    ls->enumVideoDrvDirectDrawHW = "DirectDraw HW 加速"; 
    ls->enumVideoDrvDirectDraw  = "DirectDraw";
    ls->enumVideoDrvGDI         = "GDI";

    ls->enumVideoFrameskip0     = "无";
    ls->enumVideoFrameskip1     = "1个帧";
    ls->enumVideoFrameskip2     = "2个帧";
    ls->enumVideoFrameskip3     = "3个帧";
    ls->enumVideoFrameskip4     = "4个帧";
    ls->enumVideoFrameskip5     = "5个帧";

    ls->enumSoundDrvNone        = "无声";
    ls->enumSoundDrvWMM         = "WMM 驱动";
    ls->enumSoundDrvDirectX     = "DirectX 驱动";

    ls->enumEmuSync1ms          = "同步于 MSX 的刷新";
    ls->enumEmuSyncAuto         = "自动 (快速)";
    ls->enumEmuSyncNone         = "无";
    ls->enumEmuSyncVblank       = "同步于 PC 的垂直空白";
    ls->enumEmuAsyncVblank      = "异步于 PC 的垂直空白";             

    ls->enumControlsJoyNone     = "无";
    ls->enumControlsJoyMouse    = "鼠标";
    ls->enumControlsJoyTetris2Dongle = "俄罗斯方块 2 界面模组";
    ls->enumControlsJoyTMagicKeyDongle = "MagicKey 界面模组";             
    ls->enumControlsJoy2Button = "2键操纵杆";                   
    ls->enumControlsJoyGunstick  = "光枪操纵杆";                         
    ls->enumControlsJoyAsciiLaser="ASCII Plus-X 终结者镭射";      
    ls->enumControlsArkanoidPad  ="Arkanoid Pad";                   // New in 2.7.1
    ls->enumControlsJoyColeco = "ColecoVision 操纵杆";                

    ls->enumDiskMsx35Dbl9Sect    = "MSX 3.5\" 吋 双面, 9 个扇区";     
    ls->enumDiskMsx35Dbl8Sect    = "MSX 3.5\" 吋 双面, 8 个扇区";     
    ls->enumDiskMsx35Sgl9Sect    = "MSX 3.5\" 吋 单面, 9 个扇区";     
    ls->enumDiskMsx35Sgl8Sect    = "MSX 3.5\" 吋 单面, 8 个扇区";     
    ls->enumDiskSvi525Dbl        = "SVI-328 5.25\" 吋 双面";           
    ls->enumDiskSvi525Sgl        = "SVI-328 5.25\" 吋 单面";           
    ls->enumDiskSf3Sgl           = "Sega SF-7000 3\" 吋 单面";     


    //----------------------
    // Configuration related lines
    //----------------------

    ls->confTitle                = "blueMSX - 机型资料编辑工具";
    ls->confConfigText           = "设置";
    ls->confSlotLayout           = "插槽配线";
    ls->confMemory               = "内存";
    ls->confChipEmulation        = "芯片模拟";
    ls->confChipExtras          = "附加";

    ls->confOpenRom             = "打开 ROM 映像";
    ls->confSaveTitle            = "blueMSX - 保存设置";
    ls->confSaveText             = "要覆盖原有的机型资料吗:";
    ls->confSaveAsTitle         = "配置另存为...";
    ls->confSaveAsMachineName    = "机型类型:";
    ls->confDiscardTitle         = "blueMSX - 设置";
    ls->confExitSaveTitle        = "blueMSX - 退出编辑工具";
    ls->confExitSaveText         = "要放弃对当前文件的修改吗？";

    ls->confSlotLayoutGB         = "插槽配线 ";
    ls->confSlotExtSlotGB        = "扩展插槽 ";
    ls->confBoardGB             = "主版 ";
    ls->confBoardText           = "主版类型:";
    ls->confSlotPrimary          = "主要";
    ls->confSlotExpanded         = "扩展 （四个子插槽）";

    ls->confSlotCart             = "卡带";
    ls->confSlot                = "插槽";
    ls->confSubslot             = "子插槽";

    ls->confMemAdd               = "添加...";
    ls->confMemEdit              = "编辑...";
    ls->confMemRemove            = "删除";
    ls->confMemSlot              = "插槽";
    ls->confMemAddresss          = "地址";
    ls->confMemType              = "类型";
    ls->confMemRomImage          = "Rom 映像";
    
    ls->confChipVideoGB          = "视频 ";
    ls->confChipVideoChip        = "显示芯片:";
    ls->confChipVideoRam         = "显示内存:";
    ls->confChipSoundGB          = "声音 ";
    ls->confChipPsgStereoText    = " PSG 立体声";

    ls->confCmosGB                = "CMOS ";
    ls->confCmosEnable            = " 启用 CMOS";
    ls->confCmosBattery           = " 启用内置电池";

    ls->confCpuFreqGB            = "CPU 频率 ";
    ls->confZ80FreqText          = "Z80 频率:";
    ls->confR800FreqText         = "R800 频率:";
    ls->confFdcGB                = "软盘控制器 ";
    ls->confCFdcNumDrivesText    = "驱动器数量:";

    ls->confEditMemTitle         = "blueMSX - 编辑 Mapper";
    ls->confEditMemGB            = "Mapper 详细 ";
    ls->confEditMemType          = "类型:";
    ls->confEditMemFile          = "文件:";
    ls->confEditMemAddress       = "地址";
    ls->confEditMemSize          = "大小";
    ls->confEditMemSlot          = "插槽";


    //----------------------
    // Shortcut lines
    //----------------------

    ls->shortcutKey             = "热键";
    ls->shortcutDescription     = "快捷键";

    ls->shortcutSaveConfig      = "blueMSX - 保存设置";
    ls->shortcutOverwriteConfig = "要覆盖原有的快捷键设置吗:";
    ls->shortcutExitConfig      = "blueMSX - 退出快捷键编辑工具";
    ls->shortcutDiscardConfig   = "要放弃对当前文件的修改吗？";
    ls->shortcutSaveConfigAs    = "blueMSX - 快捷键另存为...";
    ls->shortcutConfigName      = "设置名称:";
    ls->shortcutNewProfile      = "< 新建档案 >";
    ls->shortcutConfigTitle     = "blueMSX - 快捷键方案编辑工具";
    ls->shortcutAssign          = "分配";
    ls->shortcutPressText       = "请输入快捷键:";
    ls->shortcutScheme          = "映射方案:";
    ls->shortcutCartInsert1     = "插入卡带 1";
    ls->shortcutCartRemove1     = "移除卡带 1";
    ls->shortcutCartInsert2     = "插入卡带 2";
    ls->shortcutCartRemove2     = "移除卡带 2";
    ls->shortcutSpecialMenu1    = "为卡带 1 显示特别 Rom 菜单";
    ls->shortcutSpecialMenu2    = "为卡带 2 显示特别 Rom 菜单";
    ls->shortcutCartAutoReset   = "卡带插入后重置模拟器";
    ls->shortcutDiskInsertA     = "插入软盘 A";
    ls->shortcutDiskDirInsertA  = "插入目录作为软盘A";
    ls->shortcutDiskRemoveA     = "弹出软盘 A";
    ls->shortcutDiskChangeA     = "快速切换软盘 A";
    ls->shortcutDiskAutoResetA  = "软盘 A 插入后重置模拟器";
    ls->shortcutDiskInsertB     = "插入软盘 B";
    ls->shortcutDiskDirInsertB  = "插入目录作为软盘B";
    ls->shortcutDiskRemoveB     = "弹出软盘 B";
    ls->shortcutCasInsert       = "插入磁带";
    ls->shortcutCasEject        = "弹出磁带";
    ls->shortcutCasAutorewind   = "锁定磁带自动倒带";
    ls->shortcutCasReadOnly     = "锁定磁带只读";
    ls->shortcutCasSetPosition  = "设定磁带位置";
    ls->shortcutCasRewind       = "倒带";
    ls->shortcutCasSave         = "保存磁带映像";
    ls->shortcutPrnFormFeed     = "打印机换页";
    ls->shortcutCpuStateLoad    = "读取 CPU 状态";
    ls->shortcutCpuStateSave    = "保存 CPU 状态";
    ls->shortcutCpuStateQload   = "快速读取 CPU 状态";
    ls->shortcutCpuStateQsave   = "快速保存 CPU 状态";
    ls->shortcutAudioCapture    = "开始/停止录音";
    ls->shortcutScreenshotOrig  = "屏幕截图 （原始）";
    ls->shortcutScreenshotSmall = "屏幕截图 （标准，未过滤）";
    ls->shortcutScreenshotLarge = "屏幕截图 （两倍，未过滤）";
    ls->shortcutQuit            = "退出 blueMSX";
    ls->shortcutRunPause        = "运行/暂停模拟";
    ls->shortcutStop            = "停止模拟";
    ls->shortcutResetHard       = "硬件重置";
    ls->shortcutResetSoft       = "软件重置";
    ls->shortcutResetClean      = "常规重置";
    ls->shortcutSizeSmall       = "设为标准窗口";
    ls->shortcutSizeNormal      = "设为两倍窗口";
    ls->shortcutSizeFullscreen  = "设为全屏幕";
    ls->shortcutSizeMinimized   = "最小化窗口";
    ls->shortcutToggleFullscren = "切换全屏幕";
    ls->shortcutVolumeIncrease  = "增大音量";
    ls->shortcutVolumeDecrease  = "减小音量";
    ls->shortcutVolumeMute      = "静音";
    ls->shortcutVolumeStereo    = "切换单声道/立体声";
    ls->shortcutSwitchMsxAudio  = "切换 MSX-AUDIO 卡开关";
    ls->shortcutSwitchFront     = "切换 Panasonic 前端开关";
    ls->shortcutSwitchPause     = "切换暂停开关";
    ls->shortcutToggleMouseLock = "切换鼠标锁定";
    ls->shortcutEmuSpeedMax     = "最高速模拟";
    ls->shortcutEmuPlayReverse  = "Rewind emulation";                     // New in 2.8.3
    ls->shortcutEmuSpeedToggle  = "锁定最高速模拟";
    ls->shortcutEmuSpeedNormal  = "正常模拟速度";
    ls->shortcutEmuSpeedInc     = "加快模拟速度";
    ls->shortcutEmuSpeedDec     = "降低模拟速度";
    ls->shortcutThemeSwitch     = "切换布景主题";
    ls->shortcutShowEmuProp     = "显示模拟属性";
    ls->shortcutShowVideoProp   = "显示视频属性";
    ls->shortcutShowAudioProp   = "显示音频属性";
    ls->shortcutShowCtrlProp    = "显示控制属性";
    ls->shortcutShowPerfProp    = "显示性能属性";
    ls->shortcutShowSettProp    = "显示其他属性";
    ls->shortcutShowPorts       = "显示端口设定";
    ls->shortcutShowLanguage    = "显示语言对话框";
    ls->shortcutShowMachines    = "显示机型资料编辑工具";
    ls->shortcutShowShortcuts   = "显示快捷键编辑工具";
    ls->shortcutShowKeyboard    = "显示键盘编辑工具";
    ls->shortcutShowDebugger    = "显示调试工具";
    ls->shortcutShowTrainer     = "显示修改工具";
    ls->shortcutShowMixer       = "显示混音器";
    ls->shortcutShowHelp        = "显示帮助对话框";
    ls->shortcutShowAbout       = "显示关于对话框";
    ls->shortcutShowFiles       = "显示文件设定";
    ls->shortcutToggleSpriteEnable = "显示/隐藏活动块";
    ls->shortcutToggleFdcTiming = "启用/禁用软盘驱动器计时";
    ls->shortcutToggleCpuTrace  = "启用/禁用 CPU 追踪";
    ls->shortcutVideoLoad       = "读取截取视频";        
    ls->shortcutVideoPlay       = "播放上一个截取视频";   
    ls->shortcutVideoRecord     = "录制截取视频";      
    ls->shortcutVideoStop       = "停止截取视频";        
    ls->shortcutVideoRender     = "渲染视频文件";         


    //----------------------
    // Keyboard config lines
    //----------------------
    
    ls->keyconfigSelectedKey    = "选中键:";
    ls->keyconfigMappedTo       = "映射到:";
    ls->keyconfigMappingScheme  = "映射方案:";

    
    //----------------------
    // Rom type lines
    //----------------------</p>

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

    ls->dbgMemVisible           = "可见内存";
    ls->dbgMemRamNormal         = "普通";
    ls->dbgMemRamMapped         = "映射";
    ls->dbgMemYmf278            = "YMF278 取样内存";
    ls->dbgMemAy8950            = "AY8950 取样内存";
    ls->dbgMemScc               = "内存";

    ls->dbgCallstack            = "调用栈";

    ls->dbgRegs                 = "寄存器";
    ls->dbgRegsCpu              = "CPU 寄存器";
    ls->dbgRegsYmf262           = "YMF262 寄存器";
    ls->dbgRegsYmf278           = "YMF278 寄存器";
    ls->dbgRegsAy8950           = "AY8950 寄存器";
    ls->dbgRegsYm2413           = "YM2413 寄存器";

    ls->dbgDevRamMapper         = "内存映射";
    ls->dbgDevRam               = "内存";
    ls->dbgDevF4Device          = "F4 设备";
    ls->dbgDevKorean80          = "Korean 80";
    ls->dbgDevKorean90          = "Korean 90";
    ls->dbgDevKorean128         = "Korean 128";
    ls->dbgDevFdcMicrosol       = "Microsol FDC";
    ls->dbgDevPrinter           = "打印机";
    ls->dbgDevSviFdc            = "SVI FDC";
    ls->dbgDevSviPrn            = "SVI 打印机";
    ls->dbgDevSvi80Col          = "SVI 80 栏";
    ls->dbgDevRtc               = "RTC";
    ls->dbgDevTrPause           = "TR 暂停";


    //----------------------
    // Debug type lines
    // Note: Can only be translated to european languages
    //----------------------

    ls->aboutScrollThanksTo     = "Special thanks to: ";
    ls->aboutScrollAndYou       = "and YOU !!!!";
};

#endif
