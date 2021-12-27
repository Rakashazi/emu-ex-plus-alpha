ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

VPATH += $(EMUFRAMEWORK_PATH)/src/shared

CPPFLAGS += -DHAVE_CONFIG_H \
-I$(projectPath)/src \
-I$(EMUFRAMEWORK_PATH)/include/shared/mednafen \
-I$(EMUFRAMEWORK_PATH)/src/shared

CXXFLAGS_WARN += -Wno-register -Wno-missing-field-initializers

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/NGP.cc \
mednafen-emuex/MDFNApi.cc \
mednafen-emuex/MThreading.cc \
mednafen-emuex/StreamImpl.cc \
mednafen-emuex/VirtualFS.cpp \
mednafen-emuex/MDFNFILE.cc

MDFN_SRC := ngp/bios.cpp ngp/biosHLE.cpp ngp/dma.cpp ngp/flash.cpp ngp/gfx.cpp ngp/T6W28_Apu.cpp \
ngp/gfx_scanline_mono.cpp ngp/gfx_scanline_colour.cpp ngp/interrupt.cpp ngp/mem.cpp ngp/neopop.cpp \
ngp/rom.cpp ngp/rtc.cpp ngp/sound.cpp ngp/Z80_interface.cpp \
ngp/TLCS-900h/TLCS900h_interpret_single.cpp \
ngp/TLCS-900h/TLCS900h_interpret.cpp \
ngp/TLCS-900h/TLCS900h_registers.cpp \
ngp/TLCS-900h/TLCS900h_interpret_reg.cpp \
ngp/TLCS-900h/TLCS900h_interpret_src.cpp \
ngp/TLCS-900h/TLCS900h_interpret_dst.cpp \
hw_cpu/z80-fuse/z80.cpp hw_cpu/z80-fuse/z80_ops.cpp \
endian.cpp \
movie.cpp \
state.cpp \
file.cpp \
mempatcher.cpp \
error.cpp \
MemoryStream.cpp \
NativeVFS.cpp \
Stream.cpp \
memory.cpp \
git.cpp \
sound/Blip_Buffer.cpp \
sound/Stereo_Buffer.cpp \
video/resize.cpp \
video/surface.cpp \
compress/GZFileStream.cpp \
string/string.cpp \
hash/crc.cpp \
hash/md5.cpp

SRC += $(addprefix mednafen/,$(MDFN_SRC))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif