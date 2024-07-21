ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk
include $(EMUFRAMEWORK_PATH)/make/mednafenCommon.mk

VPATH += $(EMUFRAMEWORK_PATH)/src/shared

CPPFLAGS += $(MDFN_COMMON_CPPFLAGS) \
 $(MDFN_CDROM_CPPFLAGS) \
 -I$(projectPath)/src

CXXFLAGS_WARN += -Wno-missing-field-initializers -Wno-implicit-fallthrough -Wno-deprecated-enum-enum-conversion -Wno-unused-parameter -Wno-unused-function

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuMenuViews.cc \
mednafen/hash/sha256.cpp \
mednafen/resampler/resample.c \
$(MDFN_COMMON_SRC) \
$(MDFN_CDROM_SRC) \
ss/ak93c45.cpp \
ss/cdb.cpp \
ss/scu_dsp_gen.cpp \
ss/scu_dsp_misc.cpp \
ss/smpc.cpp \
ss/ss.cpp \
ss/stvio.cpp \
ss/vdp1_line.cpp \
ss/vdp1_sprite.cpp \
ss/vdp2_render.cpp \
ss/cart.cpp \
ss/db.cpp \
ss/scu_dsp_jmp.cpp \
ss/scu_dsp_mvi.cpp \
ss/sound.cpp \
ss/vdp1.cpp \
ss/vdp1_poly.cpp \
ss/vdp2.cpp \
ss/cart/ar4mp.cpp \
ss/cart/backup.cpp \
ss/cart/cs1ram.cpp \
ss/cart/bootrom.cpp \
ss/cart/extram.cpp \
ss/cart/rom.cpp \
ss/cart/stv.cpp \
ss/input/3dpad.cpp \
ss/input/gamepad.cpp \
ss/input/gun.cpp \
ss/input/jpkeyboard.cpp \
ss/input/keyboard.cpp \
ss/input/mission.cpp \
ss/input/mouse.cpp \
ss/input/multitap.cpp \
ss/input/wheel.cpp \
mednafen/hw_cpu/m68k/m68k.cpp

%/mednafen/sound/DSPUtility.o : CFLAGS_OPTIMIZE += -fno-fast-math

ifneq ($(compiler_sanitizeMode),)
# disable sanitizers for these files due to long compile times and runtime performance loss
%/ss/scu_dsp_gen.o : CFLAGS_CODEGEN += -fno-sanitize=address,undefined
%/ss/vdp2_render.o : CFLAGS_CODEGEN += -fno-sanitize=address,undefined
%/ss/ss.o : CFLAGS_CODEGEN += -fno-sanitize=address,undefined
%/mednafen/hw_cpu/m68k/m68k.o : CFLAGS_CODEGEN += -fno-sanitize=address,undefined
endif

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/flac.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
