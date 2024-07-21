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
ngp/bios.cpp \
ngp/biosHLE.cpp \
ngp/dma.cpp \
ngp/flash.cpp \
ngp/gfx.cpp \
ngp/T6W28_Apu.cpp \
ngp/gfx_scanline_mono.cpp \
ngp/gfx_scanline_colour.cpp \
ngp/interrupt.cpp \
ngp/mem.cpp \
ngp/neopop.cpp \
ngp/rom.cpp \
ngp/rtc.cpp \
ngp/sound.cpp \
ngp/Z80_interface.cpp \
ngp/TLCS-900h/TLCS900h_interpret_single.cpp \
ngp/TLCS-900h/TLCS900h_interpret.cpp \
ngp/TLCS-900h/TLCS900h_registers.cpp \
ngp/TLCS-900h/TLCS900h_interpret_reg.cpp \
ngp/TLCS-900h/TLCS900h_interpret_src.cpp \
ngp/TLCS-900h/TLCS900h_interpret_dst.cpp \
mednafen/hw_cpu/z80-fuse/z80.cpp \
mednafen/hw_cpu/z80-fuse/z80_ops.cpp

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif