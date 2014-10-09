ifndef inc_main
inc_main := 1

# -O3 is faster with MD.emu
HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)

include $(IMAGINE_PATH)/make/imagineAppBase.mk

gplusPath := genplus-gx

CPPFLAGS += -DSUPPORT_16BPP_RENDER -DLSB_FIRST \
-DSysDDec=float -DSysLDDec=float -DNO_SYSTEM_PICO
# -DNO_SVP -DNO_SYSTEM_PBC

# Genesis Plus includes
CPPFLAGS += -I$(projectPath)/src -I$(projectPath)/src/$(gplusPath) -I$(projectPath)/src/$(gplusPath)/m68k \
-I$(projectPath)/src/$(gplusPath)/z80 -I$(projectPath)/src/$(gplusPath)/input_hw \
-I$(projectPath)/src/$(gplusPath)/sound -I$(projectPath)/src/$(gplusPath)/cart_hw \
-I$(projectPath)/src/$(gplusPath)/cart_hw/svp

# Genesis Plus sources
gplusSrc += system.cc genesis.cc io_ctrl.cc loadrom.cc \
mem68k.cc membnk.cc memz80.cc state.cc vdp_ctrl.cc vdp_render.cc

ifeq ($(ENV), android)
 #GPLUS_SRC += m68k/cyclone/Cyclone.s $(GPLUS)/m68k/cyclone/m68k.cc
 gplusSrc += m68k/musashi/m68kcpu.cc
else
 gplusSrc += m68k/musashi/m68kcpu.cc
endif

gplusSrc += z80/z80.cc

gplusSrc += sound/sound.cc sound/ym2612.cc sound/Fir_Resampler.cc \
sound/ym2413.cc sound/sn76489.cc sound/blip.cc #sound/eq.c

gplusSrc += cart_hw/eeprom.cc cart_hw/areplay.cc cart_hw/ggenie.cc \
cart_hw/md_cart.cc cart_hw/sram.cc cart_hw/svp/svp.cc \
cart_hw/svp/ssp16.cc cart_hw/sms_cart.cc

gplusSrc += input_hw/input.cc input_hw/activator.cc input_hw/gamepad.cc \
input_hw/lightgun.cc input_hw/mouse.cc input_hw/teamplayer.cc \
input_hw/xe_a1p.cc input_hw/sportspad.cc input_hw/paddle.cc

# Sega CD support

ifneq ($(SUBARCH), armv6)
 hasSCD := 1
endif

ifdef hasSCD
 SRC += scd/scd.cc scd/LC89510.cc scd/cd_sys.cc scd/gfx_cd.cc scd/pcm.cc scd/cd_file.cc \
 scd/memMain.cc scd/memSub.cc

 CPPFLAGS += -I$(EMUFRAMEWORK_PATH)/../PCE.emu/src/include -I$(EMUFRAMEWORK_PATH)/../PCE.emu/src
 VPATH += $(EMUFRAMEWORK_PATH)/../PCE.emu/src/mednafen $(EMUFRAMEWORK_PATH)/../PCE.emu/src/common
 CPPFLAGS += -DHAVE_MKDIR -DHAVE_CONFIG_H -DMDFN_CD_SUPPORTS_BINARY_IMAGES -DMDFN_CD_NO_CCD \
 -DHAVE_LIBSNDFILE -DPSS_STYLE=1
 SRC += MDFNApi.cc error.cpp endian.cpp general.cpp \
  cdrom/audioreader.cpp cdrom/lec.cpp cdrom/recover-raw.cpp \
  cdrom/galois.cpp cdrom/crc32.cpp cdrom/l-ec.cpp cdrom/CDUtility.cpp \
  cdrom/CDAccess_Image.cpp cdrom/CDAccess.cpp string/trim.cpp

 cxxExceptions := 1
 include $(IMAGINE_PATH)/make/package/libvorbis.mk
 include $(IMAGINE_PATH)/make/package/libsndfile.mk
 include $(IMAGINE_PATH)/make/package/stdc++.mk
else
 CPPFLAGS += -DNO_SCD
endif

SRC += main/Main.cc main/EmuControls.cc main/Cheats.cc fileio/fileio.cc $(addprefix $(gplusPath)/,$(gplusSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
