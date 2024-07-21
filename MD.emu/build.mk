ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

gplusPath := genplus-gx

CPPFLAGS += -DLSB_FIRST \
 -DNO_SYSTEM_PICO
# -DNO_SVP -DNO_SYSTEM_PBC

CFLAGS_WARN += -Wno-missing-field-initializers -Wno-unused-parameter -Wno-unused-function

ifeq ($(config_compiler),clang)
 # needed for Z80CPU::makeFlagTables()
 CFLAGS_CODEGEN += -fconstexpr-steps=10000000
endif

# Genesis Plus includes
CPPFLAGS += -I$(projectPath)/src \
-I$(projectPath)/src/$(gplusPath) \
-I$(projectPath)/src/$(gplusPath)/m68k \
-I$(projectPath)/src/$(gplusPath)/z80 \
-I$(projectPath)/src/$(gplusPath)/input_hw \
-I$(projectPath)/src/$(gplusPath)/sound \
-I$(projectPath)/src/$(gplusPath)/cart_hw \
-I$(projectPath)/src/$(gplusPath)/cart_hw/svp

# Genesis Plus sources
gplusSrc += system.cc \
genesis.cc \
io_ctrl.cc \
loadrom.cc \
mem68k.cc \
membnk.cc \
memz80.cc \
state.cc \
vdp_ctrl.cc \
vdp_render.cc

ifeq ($(ENV), android)
 gplusSrc += m68k/musashi/m68kcpu.cc
else
 gplusSrc += m68k/musashi/m68kcpu.cc
endif

gplusSrc += z80/z80.cc

gplusSrc += sound/sound.cc \
sound/ym2612.cc \
sound/Fir_Resampler.cc \
sound/ym2413.cc \
sound/sn76489.cc \
sound/blip.cc
#sound/eq.c

gplusSrc += cart_hw/eeprom.cc \
cart_hw/areplay.cc \
cart_hw/ggenie.cc \
cart_hw/md_cart.cc \
cart_hw/sram.cc \
cart_hw/svp/svp.cc \
cart_hw/svp/ssp16.cc \
cart_hw/sms_cart.cc

gplusSrc += input_hw/input.cc \
input_hw/activator.cc \
input_hw/gamepad.cc \
input_hw/lightgun.cc \
input_hw/mouse.cc \
input_hw/teamplayer.cc \
input_hw/xe_a1p.cc \
input_hw/sportspad.cc \
input_hw/paddle.cc

# Sega CD support

hasSCD := 1

ifdef hasSCD
 include $(EMUFRAMEWORK_PATH)/make/mednafenCommon.mk

 SRC += scd/scd.cc \
 scd/LC89510.cc \
 scd/cd_sys.cc \
 scd/gfx_cd.cc \
 scd/pcm.cc \
 scd/cd_file.cc \
 scd/memMain.cc \
 scd/memSub.cc \
 $(MDFN_CDROM_STANDALONE_SRC)

 VPATH += $(EMUFRAMEWORK_PATH)/src/shared

 CPPFLAGS += $(MDFN_COMMON_CPPFLAGS) \
  $(MDFN_CDROM_CPPFLAGS) \
  -DMDFN_CD_NO_CCD

 include $(IMAGINE_PATH)/make/package/libvorbis.mk
 include $(IMAGINE_PATH)/make/package/flac.mk
else
 CPPFLAGS += -DNO_SCD
endif

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
main/Cheats.cc \
$(addprefix $(gplusPath)/,$(gplusSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
