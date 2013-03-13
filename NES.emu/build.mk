ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/EmuControls.cc main/FceuApi.cc main/Cheats.cc

emuFramework_cheats := 1
include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_ASPRINTF -DPSS_STYLE=1 -DLSB_FIRST -DFRAMESKIP -DFCEU_NO_HQ_SOUND -DSysDDec=float -DSysLDDec=float -DUSE_PIX_RGB565 -Isrc/fceu
# fceux sources
FCEUX_SRC := fceu/cart.cpp fceu/cheat.cpp fceu/emufile.cpp fceu/fceu.cpp fceu/file.cpp fceu/filter.cpp \
fceu/ines.cpp fceu/input.cpp fceu/palette.cpp fceu/ppu.cpp fceu/sound.cpp fceu/state.cpp fceu/unif.cpp fceu/vsuni.cpp \
fceu/x6502.cpp fceu/movie.cpp fceu/fds.cpp \
fceu/utils/crc32.cpp fceu/utils/md5.cpp fceu/utils/memory.cpp fceu/utils/xstring.cpp \
fceu/utils/endian.cpp fceu/utils/general.cpp fceu/utils/guid.cpp \
fceu/input/mouse.cpp fceu/input/oekakids.cpp fceu/input/powerpad.cpp fceu/input/quiz.cpp \
fceu/input/shadow.cpp fceu/input/suborkb.cpp fceu/input/toprider.cpp fceu/input/zapper.cpp \
fceu/input/bworld.cpp fceu/input/arkanoid.cpp fceu/input/mahjong.cpp fceu/input/fkb.cpp \
fceu/input/ftrainer.cpp fceu/input/hypershot.cpp fceu/input/cursor.cpp \
$(patsubst src/%,%,$(wildcard src/fceu/boards/*.cpp))
FCEUX_OBJ := $(addprefix $(objDir)/,$(FCEUX_SRC:.cpp=.o))
SRC += $(FCEUX_SRC)
#SRC += fceu/drivers/common/vidblit.cpp fceu/drivers/common/nes_ntsc.cpp fceu/drivers/common/hq2x.cpp fceu/drivers/common/hq3x.cpp fceu/drivers/common/scale2x.cpp fceu/drivers/common/scale3x.cpp fceu/drivers/common/scalebit.cpp
# fceu/drivers/common/args.cpp fceu/boards/__dummy_mapper.cpp fceu/drawing.cpp fceu/nsf.cpp fceu/oldmovie.cpp
# fceu/drivers/common/configSys.cpp fceu/conddebug.cpp fceu/debug.cpp fceu/asm.cpp
# fceu/drivers/common/cheat.cpp fceu/drivers/common/config.cpp fceu/netplay.cpp
# fceu/lua-engine.cpp fceu/config.cpp fceu/wave.cpp
# src/lua/src/lapi.o src/lua/src/lauxlib.o src/lua/src/lbaselib.o src/lua/src/lcode.o
# src/lua/src/ldblib.o src/lua/src/ldebug.o src/lua/src/ldo.o src/lua/src/ldump.o src/lua/src/lfunc.o src/lua/src/lgc.o src/lua/src/linit.o src/lua/src/liolib.o
# src/lua/src/llex.o src/lua/src/lmathlib.o src/lua/src/lmem.o src/lua/src/loadlib.o src/lua/src/lobject.o src/lua/src/lopcodes.o src/lua/src/loslib.o
# src/lua/src/lparser.o src/lua/src/lstate.o src/lua/src/lstring.o src/lua/src/lstrlib.o src/lua/src/ltable.o src/lua/src/ltablib.o src/lua/src/ltm.o
# src/lua/src/lundump.o src/lua/src/lvm.o src/lua/src/lzio.o src/lua/src/print.o src/drivers/videolog/nesvideos-piece.o src/drivers/videolog/rgbtorgb.o
# src/drivers/sdl/input.o src/drivers/sdl/config.o src/drivers/sdl/sdl.o src/drivers/sdl/sdl-joystick.o src/drivers/sdl/sdl-sound.o
# src/drivers/sdl/sdl-throttle.o src/drivers/sdl/sdl-video.o src/drivers/sdl/unix-netplay.o src/drivers/sdl/sdl-opengl.o

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
