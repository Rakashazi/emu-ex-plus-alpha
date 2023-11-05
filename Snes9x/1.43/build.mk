ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

snes9xPath := snes9x-1.43
CPPFLAGS += \
-I$(projectPath)/src \
-I$(projectPath)/src/snes9x-1.43 \
-DHAVE_STRINGS_H \
-DVAR_CYCLES \
-DRIGHTSHIFT_IS_SAR \
-DUSE_OPENGL \
-DCPU_SHUTDOWN \
-DSPC700_SHUTDOWN \
-DSPC700_C \
-DSDD1_DECOMP \
-DNOASM \
-DPIXEL_FORMAT=RGB565 \
-DSNES9X_VERSION_1_4
# -DNO_INLINE_SET_GET

CXXFLAGS_WARN += -Wno-register -Wno-implicit-fallthrough

snes9xSrc := \
c4.cpp \
c4emu.cpp \
cheats.cpp \
cheats2.cpp \
clip.cpp \
cpu.cpp \
cpuexec.cpp \
cpuops.cpp \
dma.cpp \
dsp1.cpp \
data.cpp \
fxemu.cpp \
fxinst.cpp \
gfx.cpp \
globals.cpp \
loadzip.cpp \
memmap.cpp \
movie.cpp \
obc1.cpp \
ppu.cpp \
sa1.cpp \
sa1cpu.cpp \
sdd1.cpp \
sdd1emu.cpp \
seta.cpp \
seta010.cpp \
seta011.cpp \
seta018.cpp \
snapshot.cpp \
spc7110.cpp \
srtc.cpp \
tile.cpp \
spc700.cpp \
soundux.cpp \
apu.cpp

SRC += \
main/Main.cc \
main/input.cc \
main/options.cc \
main/S9XApi.cc \
main/EmuMenuViews.cc \
main/Cheats.cc \
$(addprefix $(snes9xPath)/,$(snes9xSrc))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
