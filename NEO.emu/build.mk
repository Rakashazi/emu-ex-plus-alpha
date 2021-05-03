ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc \
main/input.cc \
main/options.cc \
main/unzip.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc

CPPFLAGS += -I$(projectPath)/src \
-DHAVE_CONFIG_H

CFLAGS_WARN += -Wno-sign-compare

GEO := gngeo

SRC += $(GEO)/mamez80/z80.c \
$(GEO)/mamez80_interf.c

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
$(GEO)/state.c \
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
