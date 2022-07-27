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

CXXFLAGS_WARN += -Wno-missing-field-initializers

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/PCEFast.cc \
$(MDFN_COMMON_SRC) \
$(MDFN_CDROM_SRC)

MDFN_SRC := pce_fast/input.cpp \
pce_fast/vdc.cpp \
pce_fast/huc6280.cpp \
pce_fast/pce.cpp \
pce_fast/huc.cpp \
pce_fast/pcecd.cpp \
pce_fast/pcecd_drive.cpp \
pce_fast/psg.cpp \
cputest/cputest.c \
sound/okiadpcm.cpp \
hw_misc/arcade_card/arcade_card.cpp \
hw_sound/pce_psg/pce_psg.cpp

SRC += $(addprefix mednafen/,$(MDFN_SRC))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/flac.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
