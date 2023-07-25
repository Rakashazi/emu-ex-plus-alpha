ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -DHAVE_ZLIB_H \
-DFINAL_VERSION \
-DC_CORE \
-DNO_PNG \
-DNO_LINK \
-DNO_DEBUGGER \
-I$(projectPath)/src \
-I$(projectPath)/src/vbam

vbamSrc := gba/GBA-thumb.cpp \
gba/bios.cpp \
gba/Globals.cpp \
gba/Cheats.cpp \
gba/Mode0.cpp \
gba/CheatSearch.cpp \
gba/Mode1.cpp \
gba/Mode2.cpp \
gba/Mode3.cpp \
gba/Mode4.cpp \
gba/Mode5.cpp \
gba/EEprom.cpp \
gba/Flash.cpp \
gba/GBA-arm.cpp \
gba/GBA.cpp \
gba/gbafilter.cpp \
gba/RTC.cpp \
gba/Sound.cpp \
gba/Sram.cpp \
common/memgzio.c \
common/Patch.cpp \
Util.cpp
#gba/remote.cpp gba/GBASockClient.cpp gba/GBALink.cpp gba/agbprint.cpp
#gba/armdis.cpp gba/elf.cpp

vbamSrc += apu/Gb_Apu.cpp \
apu/Gb_Oscs.cpp \
apu/Blip_Buffer.cpp \
apu/Multi_Buffer.cpp \
apu/Gb_Apu_State.cpp

vbamPath := vbam
SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
main/VbamApi.cc \
main/Cheats.cc \
$(addprefix $(vbamPath)/,$(vbamSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
