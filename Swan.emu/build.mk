ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk
include $(EMUFRAMEWORK_PATH)/make/mednafenCommon.mk

VPATH += $(EMUFRAMEWORK_PATH)/src/shared

CPPFLAGS += $(MDFN_COMMON_CPPFLAGS) \
 -I$(projectPath)/src

CXXFLAGS_WARN += -Wno-missing-field-initializers -Wno-unused-parameter

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
$(MDFN_COMMON_SRC) \
wswan/comm.cpp \
wswan/gfx.cpp \
wswan/memory.cpp \
wswan/tcache.cpp \
wswan/interrupt.cpp \
wswan/rtc.cpp \
wswan/v30mz.cpp \
wswan/eeprom.cpp \
wswan/main.cpp \
wswan/sound.cpp

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif