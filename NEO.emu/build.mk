ifndef inc_main
inc_main := 1

# -O3 is faster with NEO.emu
HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/EmuControls.cc

CPPFLAGS += -I$(projectPath)/src -DSysDDec=float -DHAVE_CONFIG_H

GEO := gngeo

SRC += $(GEO)/mamez80/z80.c $(GEO)/mamez80_interf.c
#SRC += $(GEO)/z80/z80.cc $(GEO)/z80_interf.cc

SRC += $(GEO)/ym2610/2610intf.c $(GEO)/ym2610/ym2610.c

SRC += $(GEO)/debug.c $(GEO)/emu.c $(GEO)/fileio.c $(GEO)/mame_layer.c \
$(GEO)/memory.c $(GEO)/neoboot.c $(GEO)/neocrypt.c $(GEO)/pd4990a.c $(GEO)/roms.c $(GEO)/state.c \
$(GEO)/timer.c $(GEO)/video.c $(GEO)/unzip.c

ifeq ($(ENV), webos)
 LDLIBS += -lpthread
endif

ifeq ($(ARCH), arm)
 ifeq ($(ENV), ios)
  SRC += $(GEO)/cyclone_interf.c $(GEO)/cyclone/Cyclone-apple.s
  #LDFLAGS += -Wl,-no_pie
 else
  SRC += $(GEO)/video_arm.s
  SRC += $(GEO)/cyclone_interf.c $(GEO)/cyclone/Cyclone.s
 endif
else
 SRC += $(GEO)/generator68k_interf.c \
 $(GEO)/generator68k/cpu68k.c $(GEO)/generator68k/reg68k.c $(GEO)/generator68k/diss68k.c $(GEO)/generator68k/tab68k.c \
 $(GEO)/generator68k/cpu68k-0.c $(GEO)/generator68k/cpu68k-1.c $(GEO)/generator68k/cpu68k-2.c $(GEO)/generator68k/cpu68k-3.c \
 $(GEO)/generator68k/cpu68k-4.c $(GEO)/generator68k/cpu68k-5.c $(GEO)/generator68k/cpu68k-6.c $(GEO)/generator68k/cpu68k-7.c \
 $(GEO)/generator68k/cpu68k-8.c $(GEO)/generator68k/cpu68k-9.c $(GEO)/generator68k/cpu68k-a.c $(GEO)/generator68k/cpu68k-b.c \
 $(GEO)/generator68k/cpu68k-c.c $(GEO)/generator68k/cpu68k-d.c $(GEO)/generator68k/cpu68k-e.c $(GEO)/generator68k/cpu68k-f.c
endif

configInc += <gngeo-config.h>

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
