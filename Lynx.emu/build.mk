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
lynx/c65c02.cpp \
lynx/cart.cpp \
lynx/lynxdec.cpp \
lynx/memmap.cpp \
lynx/mikie.cpp \
lynx/ram.cpp \
lynx/rom.cpp \
lynx/susie.cpp \
lynx/system.cpp

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif