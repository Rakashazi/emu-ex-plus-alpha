ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/FceuApi.cc

include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_ASPRINTF -DPSS_STYLE=1 -DLSB_FIRST -DFRAMESKIP -DFCEU_NO_HQ_SOUND -DSysDDec=float -DSysLDDec=float -DUSE_PIX_RGB565 -Isrc/fceu
# fceux sources
FCEUX_SRC += fceu/cart.cpp fceu/cheat.cpp fceu/emufile.cpp fceu/fceu.cpp fceu/file.cpp fceu/filter.cpp
FCEUX_SRC += fceu/ines.cpp fceu/input.cpp fceu/palette.cpp fceu/ppu.cpp fceu/sound.cpp fceu/state.cpp fceu/unif.cpp fceu/video.cpp fceu/vsuni.cpp
FCEUX_SRC += fceu/x6502.cpp fceu/movie.cpp fceu/fds.cpp
FCEUX_SRC += fceu/input/mouse.cpp fceu/input/oekakids.cpp fceu/input/powerpad.cpp fceu/input/quiz.cpp
FCEUX_SRC +=  fceu/input/shadow.cpp fceu/input/suborkb.cpp fceu/input/toprider.cpp fceu/input/zapper.cpp
FCEUX_SRC += fceu/boards/01-222.cpp fceu/boards/103.cpp fceu/boards/106.cpp fceu/boards/108.cpp fceu/boards/112.cpp
FCEUX_SRC += fceu/boards/117.cpp fceu/boards/120.cpp
FCEUX_SRC +=  fceu/boards/121.cpp fceu/boards/15.cpp fceu/boards/164.cpp fceu/boards/175.cpp fceu/boards/176.cpp fceu/boards/177.cpp fceu/boards/178.cpp
FCEUX_SRC += fceu/boards/179.cpp fceu/boards/183.cpp fceu/boards/185.cpp fceu/boards/186.cpp fceu/boards/187.cpp fceu/boards/189.cpp fceu/boards/199.cpp fceu/boards/208.cpp fceu/boards/222.cpp
FCEUX_SRC += fceu/boards/23.cpp fceu/boards/235.cpp fceu/boards/253.cpp
FCEUX_SRC += fceu/boards/3d-block.cpp fceu/boards/411120-c.cpp fceu/boards/43.cpp fceu/boards/57.cpp fceu/boards/603-5052.cpp
FCEUX_SRC += fceu/boards/68.cpp fceu/boards/8157.cpp fceu/boards/8237.cpp fceu/boards/830118C.cpp fceu/boards/88.cpp fceu/boards/90.cpp fceu/boards/95.cpp fceu/boards/a9711.cpp
FCEUX_SRC += fceu/boards/a9746.cpp fceu/boards/addrlatch.cpp fceu/boards/ax5705.cpp fceu/boards/bandai.cpp fceu/boards/bmc13in1jy110.cpp fceu/boards/bmc42in1r.cpp
FCEUX_SRC += fceu/boards/bmc64in1nr.cpp fceu/boards/bmc70in1.cpp fceu/boards/bonza.cpp fceu/boards/bs-5.cpp fceu/boards/onebus.cpp fceu/boards/datalatch.cpp
FCEUX_SRC += fceu/boards/deirom.cpp fceu/boards/dream.cpp fceu/boards/edu2000.cpp fceu/boards/fk23c.cpp fceu/boards/ghostbusters63in1.cpp
FCEUX_SRC += fceu/boards/gs-2004.cpp fceu/boards/gs-2013.cpp fceu/boards/h2288.cpp fceu/boards/karaoke.cpp fceu/boards/kof97.cpp fceu/boards/konami-qtai.cpp fceu/boards/ks7032.cpp
FCEUX_SRC += fceu/boards/malee.cpp fceu/boards/mmc1.cpp fceu/boards/mmc3.cpp fceu/boards/mmc5.cpp fceu/boards/n-c22m.cpp fceu/boards/n106.cpp fceu/boards/n625092.cpp fceu/boards/novel.cpp
FCEUX_SRC += fceu/boards/sachen.cpp fceu/boards/sc-127.cpp fceu/boards/sheroes.cpp fceu/boards/sl1632.cpp fceu/boards/smb2j.cpp fceu/boards/subor.cpp fceu/boards/super24.cpp
FCEUX_SRC += fceu/boards/supervision.cpp fceu/boards/t-227-1.cpp fceu/boards/t-262.cpp fceu/boards/tengen.cpp fceu/boards/tf-1201.cpp
FCEUX_SRC += fceu/input/arkanoid.cpp fceu/input/bworld.cpp fceu/input/cursor.cpp fceu/input/fkb.cpp fceu/input/ftrainer.cpp fceu/input/hypershot.cpp fceu/input/mahjong.cpp
FCEUX_SRC += fceu/utils/endian.cpp fceu/utils/memory.cpp fceu/utils/crc32.cpp fceu/utils/general.cpp
FCEUX_SRC += fceu/utils/guid.cpp fceu/utils/xstring.cpp fceu/utils/md5.cpp
FCEUX_SRC += fceu/mappers/151.cpp fceu/mappers/16.cpp fceu/mappers/17.cpp fceu/mappers/18.cpp fceu/mappers/193.cpp fceu/mappers/201.cpp fceu/mappers/202.cpp
FCEUX_SRC += fceu/mappers/203.cpp fceu/mappers/204.cpp fceu/mappers/212.cpp fceu/mappers/213.cpp fceu/mappers/214.cpp fceu/mappers/215.cpp fceu/mappers/217.cpp fceu/mappers/21.cpp
FCEUX_SRC += fceu/mappers/225.cpp fceu/mappers/227.cpp fceu/mappers/228.cpp fceu/mappers/229.cpp fceu/mappers/22.cpp fceu/mappers/230.cpp fceu/mappers/231.cpp fceu/mappers/232.cpp
FCEUX_SRC += fceu/mappers/234.cpp fceu/mappers/241.cpp fceu/mappers/242.cpp fceu/mappers/244.cpp fceu/mappers/246.cpp fceu/mappers/24and26.cpp fceu/mappers/255.cpp fceu/mappers/25.cpp
FCEUX_SRC += fceu/mappers/27.cpp fceu/mappers/32.cpp fceu/mappers/33.cpp fceu/mappers/40.cpp fceu/mappers/41.cpp fceu/mappers/42.cpp fceu/mappers/46.cpp fceu/mappers/50.cpp fceu/mappers/51.cpp
FCEUX_SRC += fceu/mappers/59.cpp fceu/mappers/60.cpp fceu/mappers/61.cpp fceu/mappers/62.cpp fceu/mappers/65.cpp fceu/mappers/67.cpp fceu/mappers/69.cpp fceu/mappers/6.cpp fceu/mappers/71.cpp
FCEUX_SRC += fceu/mappers/72.cpp fceu/mappers/73.cpp fceu/mappers/75.cpp fceu/mappers/76.cpp fceu/mappers/77.cpp fceu/mappers/79.cpp fceu/mappers/80.cpp fceu/mappers/82.cpp fceu/mappers/83.cpp
FCEUX_SRC += fceu/mappers/85.cpp fceu/mappers/86.cpp fceu/mappers/89.cpp fceu/mappers/8.cpp fceu/mappers/91.cpp fceu/mappers/92.cpp fceu/mappers/97.cpp fceu/mappers/99.cpp
FCEUX_SRC += fceu/mappers/mmc2and4.cpp fceu/mappers/simple.cpp
FCEUX_OBJ := $(addprefix $(objDir)/,$(FCEUX_SRC:.cpp=.o))
SRC += $(FCEUX_SRC) fceu/mappers/emu2413.cc
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

ifndef target
target := nesemu
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
