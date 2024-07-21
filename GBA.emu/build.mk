ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -DHAVE_ZLIB_H \
-DFINAL_VERSION \
-DC_CORE \
-DNO_PNG \
-DNO_LINK \
-DNO_DEBUGGER \
-I$(projectPath)/src

CFLAGS_WARN += -Wno-unused-parameter -Wno-unused-function -Wno-unused-but-set-variable

vbamSrc := gba/gbaCpuThumb.cpp \
gba/internal/gbaBios.cpp \
gba/gbaCheats.cpp \
gba/gbaMode0.cpp \
gba/gbaMode1.cpp \
gba/gbaMode2.cpp \
gba/gbaMode3.cpp \
gba/gbaMode4.cpp \
gba/gbaMode5.cpp \
gba/gbaEeprom.cpp \
gba/gbaFlash.cpp \
gba/gbaCpuArm.cpp \
gba/gba.cpp \
gba/gbaRtc.cpp \
gba/gbaSound.cpp \
gba/internal/gbaSram.cpp \
base/patch.cpp

vbamSrc += apu/Gb_Apu.cpp \
apu/Gb_Oscs.cpp \
apu/Blip_Buffer.cpp \
apu/Multi_Buffer.cpp \
apu/Gb_Apu_State.cpp

vbamPath := core
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
