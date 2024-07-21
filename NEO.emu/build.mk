ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc \
main/input.cc \
main/options.cc \
main/state.cc \
main/unzip.cc \
main/EmuMenuViews.cc

CPPFLAGS += -I$(projectPath)/src \
-DHAVE_CONFIG_H -DLSB_FIRST

CFLAGS_WARN += -Wno-sign-compare -Wno-unused-parameter -Wno-unused-function

ifeq ($(config_compiler),clang)
 # needed for Z80CPU::makeFlagTables()
 CFLAGS_CODEGEN += -fconstexpr-steps=10000000
endif

GEO := gngeo

Z80_PATH = $(EMUFRAMEWORK_PATH)/../MD.emu/src/genplus-gx/z80
CPPFLAGS += -I$(Z80_PATH)
VPATH +=  $(Z80_PATH)
SRC += z80.cc \
$(GEO)/mamez80_interf.cc

SRC += $(GEO)/ym2610/2610intf.c \
$(GEO)/ym2610/ym2610.c

SRC += $(GEO)/debug.c \
$(GEO)/emu.c \
$(GEO)/fileio.c \
$(GEO)/mame_layer.c \
$(GEO)/memory.c \
$(GEO)/neoboot.c \
$(GEO)/neocrypt.c \
$(GEO)/pd4990a.c \
$(GEO)/roms.c \
$(GEO)/timer.c \
$(GEO)/video.c

ifeq ($(ENV), webos)
 LDLIBS += -lpthread
endif

ifeq ($(ARCH), arm)
 ifeq ($(ENV), ios)
  SRC += $(GEO)/cyclone_interf.c \
  $(GEO)/cyclone/Cyclone-apple.s
  #LDFLAGS += -Wl,-no_pie
 else
  SRC += $(GEO)/video_arm.s
  SRC += $(GEO)/cyclone_interf.c \
  $(GEO)/cyclone/Cyclone.s
 endif
else
 M68K_PATH = $(EMUFRAMEWORK_PATH)/../MD.emu/src/genplus-gx/m68k
 CPPFLAGS += -I$(M68K_PATH)
 VPATH +=  $(M68K_PATH)
 SRC += musashi/m68kcpu.cc \
 $(GEO)/musashi_interf.cc
endif

configInc += <gngeo-config.h>

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
