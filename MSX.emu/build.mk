ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

# TODO: fix pointer type casts
CFLAGS_LANG += -Werror=implicit-function-declaration -Wno-incompatible-pointer-types
CFLAGS_WARN += -Wno-sign-compare -Wno-switch -Wno-implicit-fallthrough -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable

BMSX := blueMSX

CPPFLAGS += -DLSB_FIRST \
-DNO_ASM \
-DNO_EMBEDDED_SAMPLES \
-I$(projectPath)/src/$(BMSX)

# BlueMSX includes
CPPFLAGS += -I$(projectPath)/src \
-I$(projectPath)/src/$(BMSX)/SoundChips \
-I$(projectPath)/src/$(BMSX)/VideoChips \
-I$(projectPath)/src/$(BMSX)/Z80 \
-I$(projectPath)/src/$(BMSX)/Media \
-I$(projectPath)/src/$(BMSX)/Common \
-I$(projectPath)/src/$(BMSX)/TinyXML \
-I$(projectPath)/src/$(BMSX)/VideoRender \
-I$(projectPath)/src/$(BMSX)/Board \
-I$(projectPath)/src/$(BMSX)/Arch \
-I$(projectPath)/src/$(BMSX)/Memory \
-I$(projectPath)/src/$(BMSX)/Emulator \
-I$(projectPath)/src/$(BMSX)/Input \
-I$(projectPath)/src/$(BMSX)/Utils \
-I$(projectPath)/src/$(BMSX)/Language \
-I$(projectPath)/src/$(BMSX)/IoDevice \
-I$(projectPath)/src/$(BMSX)/Debugger

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
main/BlueMSXApi.cc \
main/Board.cc \
main/ziphelper.cc \
main/RomLoader.cc \
main/FrameBuffer.cc

# BlueMSX sources

# Sound Chips
SRC += $(BMSX)/SoundChips/AudioMixer.c \
$(BMSX)/SoundChips/YM2413.cpp \
$(BMSX)/SoundChips/SCC.c \
$(BMSX)/SoundChips/MsxPsg.c \
$(BMSX)/SoundChips/KeyClick.c \
$(BMSX)/SoundChips/DAC.c \
$(BMSX)/SoundChips/AY8910.c \
$(BMSX)/SoundChips/OpenMsxYM2413_2.cpp \
$(BMSX)/SoundChips/Y8950.c \
$(BMSX)/SoundChips/Fmopl.c \
$(BMSX)/SoundChips/Ymdeltat.c \
$(BMSX)/SoundChips/Moonsound.cpp \
$(BMSX)/SoundChips/OpenMsxYMF262.cpp \
$(BMSX)/SoundChips/OpenMsxYMF278.cpp \
$(BMSX)/SoundChips/VLM5030.c \
$(BMSX)/SoundChips/ym2151.c \
$(BMSX)/SoundChips/MameVLM5030.c \
$(BMSX)/SoundChips/MameYM2151.c

# Coleco/SG1000 Sound Chips
SRC += $(BMSX)/SoundChips/SN76489.c

# Video Chips
SRC += $(BMSX)/VideoChips/VideoManager.c \
$(BMSX)/VideoChips/VDP.c \
$(BMSX)/VideoChips/V9938.c \
$(BMSX)/VideoChips/CRTC6845.c

# Io Device
SRC += $(BMSX)/IoDevice/Casette.c \
$(BMSX)/IoDevice/Led.c \
$(BMSX)/IoDevice/Disk.c \
$(BMSX)/IoDevice/ScsiDevice.c \
$(BMSX)/IoDevice/Switches.c \
$(BMSX)/IoDevice/I8255.c \
$(BMSX)/IoDevice/SunriseIDE.c \
$(BMSX)/IoDevice/RTC.c \
$(BMSX)/IoDevice/MsxPPI.c \
$(BMSX)/IoDevice/WD2793.c \
$(BMSX)/IoDevice/FdcAudio.c \
$(BMSX)/IoDevice/MidiIO.c \
$(BMSX)/IoDevice/TC8566AF.c \
$(BMSX)/IoDevice/PrinterIO.c \
$(BMSX)/IoDevice/TurboRIO.c \
$(BMSX)/IoDevice/MSXMidi.c \
$(BMSX)/IoDevice/I8251.c \
$(BMSX)/IoDevice/I8254.c \
$(BMSX)/IoDevice/Z8530.c \
$(BMSX)/IoDevice/HarddiskIDE.c \
$(BMSX)/IoDevice/DirAsDisk.c \

# Utils
SRC += $(BMSX)/Utils/SaveState.c \
$(BMSX)/Utils/TokenExtract.c \
$(BMSX)/Utils/IniFileParser.c \
$(BMSX)/Utils/IsFileExtension.c

# Input
SRC += $(BMSX)/Input/JoystickPort.c \
$(BMSX)/Input/InputEvent.c \
$(BMSX)/Input/CoinDevice.c \
$(BMSX)/Input/MsxJoystick.c \
$(BMSX)/Input/MsxMouse.c

# Coleco Input
SRC += $(BMSX)/Input/ColecoJoystick.c \
$(BMSX)/Input/ColecoSteeringWheel.c \
$(BMSX)/Input/ColecoSuperAction.c

# Board
SRC += $(BMSX)/Board/Machine.c \
$(BMSX)/Board/MSX.c \
$(BMSX)/Board/Coleco.c

# Memory
# $(BMSX)/Memory/SlotManager.c
SRC += $(BMSX)/Memory/ram1kBMirrored.c \
$(BMSX)/Memory/ramNormal.c \
$(BMSX)/Debugger/DebugDeviceManager.c \
$(BMSX)/Memory/DeviceManager.c \
$(BMSX)/Memory/SlotManager.c \
$(BMSX)/Memory/ramMapper.c \
$(BMSX)/Memory/ramMapperIo.c \
$(BMSX)/Memory/IoPort.c \
$(BMSX)/Memory/romMapperDRAM.c \
$(BMSX)/Memory/MegaromCartridge.c \
$(BMSX)/Memory/romMapperFMPAC.c \
$(BMSX)/Memory/sramLoader.c \
$(BMSX)/Memory/romMapperPAC.c \
$(BMSX)/Memory/AmdFlash.c \
$(BMSX)/Memory/romMapperMegaFlashRomScc.c \
$(BMSX)/Memory/romMapperSCCplus.c \
$(BMSX)/Memory/romMapperNormal.c \
$(BMSX)/Memory/romMapperPlain.c \
$(BMSX)/Memory/romMapperBasic.c \
$(BMSX)/Memory/romMapperMegaRAM.c \
$(BMSX)/Memory/romMapperStandard.c \
$(BMSX)/Memory/romMapperMsxDos2.c \
$(BMSX)/Memory/romMapperKonami5.c \
$(BMSX)/Memory/romMapperKonami4.c \
$(BMSX)/Memory/romMapperMsxMusic.c \
$(BMSX)/Memory/romMapperMsxAudio.c \
$(BMSX)/Memory/romMapperPhilipsFdc.c \
$(BMSX)/Memory/romMapperASCII8.c \
$(BMSX)/Memory/romMapperASCII16.c \
$(BMSX)/Memory/romMapperF4device.c \
$(BMSX)/Memory/romMapperKanji.c \
$(BMSX)/Memory/romMapperMsxPrn.c \
$(BMSX)/Memory/romMapperCasette.c \
$(BMSX)/Memory/romMapperTC8566AF.c \
$(BMSX)/Memory/romMapperMoonsound.c \
$(BMSX)/Memory/sramMapperS1985.c \
$(BMSX)/Memory/romMapperS1990.c \
$(BMSX)/Memory/romMapperTurboRTimer.c \
$(BMSX)/Memory/romMapperTurboRPcm.c \
$(BMSX)/Memory/romMapperNms1210Rs232.c \
$(BMSX)/Memory/romMapperSonyHBI55.c \
$(BMSX)/Memory/romMapperASCII8sram.c \
$(BMSX)/Memory/romMapperASCII16sram.c\
$(BMSX)/Memory/romMapperRType.c \
$(BMSX)/Memory/romMapperCrossBlaim.c \
$(BMSX)/Memory/romMapperHarryFox.c \
$(BMSX)/Memory/romMapperKorean80.c \
$(BMSX)/Memory/romMapperKorean126.c \
$(BMSX)/Memory/romMapperKonami4nf.c \
$(BMSX)/Memory/romMapperASCII16nf.c \
$(BMSX)/Memory/romMapperHolyQuran.c \
$(BMSX)/Memory/romMapperLodeRunner.c \
$(BMSX)/Memory/romMapperKorean90.c \
$(BMSX)/Memory/romMapperKonamiSynth.c \
$(BMSX)/Memory/romMapperMajutsushi.c \
$(BMSX)/Memory/romMapperDisk.c \
$(BMSX)/Memory/romMapperKanji12.c\
$(BMSX)/Memory/romMapperHalnote.c \
$(BMSX)/Memory/romMapperMicrosol.c \
$(BMSX)/Memory/romMapperCvMegaCart.c \
$(BMSX)/Memory/romMapperGameMaster2.c \
$(BMSX)/Memory/romMapperPlayBall.c \
$(BMSX)/Memory/romMapperKonamiWordPro.c \
$(BMSX)/Memory/romMapperKonamiKeyboardMaster.c \
$(BMSX)/Memory/romMapperForteII.c \
$(BMSX)/Memory/romMapperKoei.c \
$(BMSX)/Memory/romMapperArc.c \
$(BMSX)/Memory/romMapperMicrosolVmx80.c \
$(BMSX)/Memory/romMapperSonyHBIV1.c \
$(BMSX)/Memory/romMapperNationalFdc.c \
$(BMSX)/Memory/romMapperMatraINK.c \
$(BMSX)/Memory/romMapperFMPAK.c \
$(BMSX)/Memory/romMapperNettouYakyuu.c \
$(BMSX)/Memory/romMapperFmDas.c \
$(BMSX)/Memory/romMapperSfg05.c \
$(BMSX)/Memory/romMapperGIDE.c \
$(BMSX)/Memory/sramMapperMatsuchita.c \
$(BMSX)/Memory/romMapperNational.c \
$(BMSX)/Memory/romMapperBunsetu.c \
$(BMSX)/Memory/romMapperSunriseIDE.c \
$(BMSX)/Memory/romMapperNet.c

# Z80
SRC += $(BMSX)/Z80/R800.c \
$(BMSX)/Z80/R800SaveState.c

useXMLDB := 0

ifeq ($(useXMLDB), 1)
 # TinyXML
 SRC += $(BMSX)/TinyXML/tinystr.cpp \
 $(BMSX)/TinyXML/tinyxml.cpp \
 $(BMSX)/TinyXML/tinyxmlerror.cpp \
 $(BMSX)/TinyXML/tinyxmlparser.cpp

 # Media
 SRC += $(BMSX)/Media/MediaDb.cpp \
 $(BMSX)/Media/Sha1.cpp \
 $(BMSX)/Media/Crc32Calc.c
else
 SRC += main/MediaDb.cc
endif

# Others 
SRC +=  $(BMSX)/Bios/Patch.c \
$(BMSX)/Language/LanguageMinimal.c

include $(IMAGINE_PATH)/make/package/zlib.mk
include $(EMUFRAMEWORK_PATH)/package/emuframework.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
