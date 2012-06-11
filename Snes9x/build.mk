ifndef inc_main
inc_main := 1

# -O3 is faster with Snes9x
HIGH_OPTIMIZE_CFLAGS := -O3 $(NORMAL_OPTIMIZE_CFLAGS_MISC) -funsafe-loop-optimizations -Wunsafe-loop-optimizations

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/S9XApi.cc

include ../EmuFramework/common.mk

SNES9X := snes9x-1.43
CPPFLAGS += -Isrc/snes9x-1.43 -DHAVE_STRINGS_H -DVAR_CYCLES -DRIGHTSHIFT_IS_SAR -DZLIB -DUNZIP_SUPPORT -DSysDDec=float \
-DUSE_OPENGL -DCPU_SHUTDOWN -DSPC700_SHUTDOWN -DEXECUTE_SUPERFX_PER_LINE -DSPC700_C -DSDD1_DECOMP -DNOASM -DPIXEL_FORMAT=RGB565
# -DNO_INLINE_SET_GET

# snes9x sources
SNES9X_SRC += $(SNES9X)/c4.cpp $(SNES9X)/c4emu.cpp $(SNES9X)/cheats.cpp
SNES9X_SRC += $(SNES9X)/cheats2.cpp $(SNES9X)/clip.cpp
SNES9X_SRC += $(SNES9X)/cpu.cpp $(SNES9X)/cpuexec.cpp $(SNES9X)/cpuops.cpp
SNES9X_SRC += $(SNES9X)/dma.cpp $(SNES9X)/dsp1.cpp $(SNES9X)/data.cpp
SNES9X_SRC += $(SNES9X)/fxemu.cpp $(SNES9X)/fxinst.cpp $(SNES9X)/gfx.cpp
SNES9X_SRC += $(SNES9X)/globals.cpp $(SNES9X)/loadzip.cpp $(SNES9X)/memmap.cpp
SNES9X_SRC += $(SNES9X)/movie.cpp $(SNES9X)/obc1.cpp $(SNES9X)/ppu.cpp
SNES9X_SRC += $(SNES9X)/sa1.cpp $(SNES9X)/sa1cpu.cpp $(SNES9X)/sdd1.cpp
SNES9X_SRC += $(SNES9X)/sdd1emu.cpp $(SNES9X)/seta.cpp $(SNES9X)/seta010.cpp $(SNES9X)/seta011.cpp
SNES9X_SRC += $(SNES9X)/seta018.cpp $(SNES9X)/snapshot.cpp $(SNES9X)/spc7110.cpp
SNES9X_SRC += $(SNES9X)/srtc.cpp $(SNES9X)/tile.cpp
SNES9X_SRC += $(SNES9X)/spc700.cpp $(SNES9X)/soundux.cpp $(SNES9X)/apu.cpp
SRC += $(SNES9X_SRC)

#SRC += $(SNES9X)/jma/7zlzma.cpp $(SNES9X)/unzip/crc32.cpp $(SNES9X)/unzip/iiostrm.cpp
#SRC += $(SNES9X)/jma/inbyte.cpp $(SNES9X)/unzip/jma.cpp $(SNES9X)/unzip/lzma.cpp
#SRC += $(SNES9X)/jma/lzmadec.cpp $(SNES9X)/unzip/s9x-jma.cpp $(SNES9X)/unzip/winout.cpp

include $(IMAGINE_PATH)/make/package/unzip.mk

ifndef target
target := s9x
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
