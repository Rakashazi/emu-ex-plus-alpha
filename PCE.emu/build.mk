ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk
include $(EMUFRAMEWORK_PATH)/make/mednafenCommon.mk

VPATH += $(EMUFRAMEWORK_PATH)/src/shared

CPPFLAGS += $(MDFN_COMMON_CPPFLAGS) \
 $(MDFN_CDROM_CPPFLAGS) \
 -I$(projectPath)/src \
 -I$(EMUFRAMEWORK_PATH)/src/shared/mednafen/hw_misc \
 -I$(EMUFRAMEWORK_PATH)/src/shared/mednafen/hw_sound

CXXFLAGS_WARN += -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
$(MDFN_COMMON_SRC) \
$(MDFN_CDROM_SRC) \
pce_fast/input.cpp \
pce_fast/vdc.cpp \
pce_fast/huc6280.cpp \
pce_fast/pce.cpp \
pce_fast/huc.cpp \
pce_fast/pcecd.cpp \
pce_fast/pcecd_drive.cpp \
pce_fast/psg.cpp \
pce/huc.cpp \
pce/huc6280.cpp \
pce/input.cpp \
pce/mcgenjin.cpp \
pce/pce.cpp \
pce/pcecd.cpp \
pce/tsushin.cpp \
pce/vce.cpp \
pce/input/gamepad.cpp \
pce/input/mouse.cpp \
pce/input/tsushinkb.cpp \
mednafen/cputest/cputest.c \
mednafen/sound/DSPUtility.cpp \
mednafen/sound/okiadpcm.cpp \
mednafen/sound/OwlResampler.cpp \
mednafen/hw_misc/arcade_card/arcade_card.cpp \
mednafen/hw_sound/pce_psg/pce_psg.cpp \
mednafen/hw_video/huc6270/vdc.cpp

%/mednafen/sound/DSPUtility.o : CFLAGS_OPTIMIZE += -fno-fast-math

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/flac.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
