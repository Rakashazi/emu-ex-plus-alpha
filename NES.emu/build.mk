ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

emuFramework_cheats := 1
include $(EMUFRAMEWORK_PATH)/common.mk

SRC += main/Main.cc main/EmuControls.cc main/FceuApi.cc main/Cheats.cc

CPPFLAGS += -DHAVE_ASPRINTF -DPSS_STYLE=1 -DLSB_FIRST -DFRAMESKIP -DFCEU_NO_HQ_SOUND -DSysDDec=float -DSysLDDec=float -DUSE_PIX_RGB565 -I$(projectPath)/src/fceu
# fceux sources
FCEUX_SRC := fceu/cart.cpp fceu/cheat.cpp fceu/emufile.cpp fceu/fceu.cpp fceu/file.cpp fceu/filter.cpp \
fceu/ines.cpp fceu/input.cpp fceu/palette.cpp fceu/ppu.cpp fceu/sound.cpp fceu/state.cpp fceu/unif.cpp fceu/vsuni.cpp \
fceu/x6502.cpp fceu/movie.cpp fceu/fds.cpp \
fceu/utils/crc32.cpp fceu/utils/md5.cpp fceu/utils/memory.cpp fceu/utils/xstring.cpp \
fceu/utils/endian.cpp fceu/utils/general.cpp fceu/utils/guid.cpp \
fceu/input/mouse.cpp fceu/input/oekakids.cpp fceu/input/powerpad.cpp fceu/input/quiz.cpp \
fceu/input/shadow.cpp fceu/input/suborkb.cpp fceu/input/toprider.cpp fceu/input/zapper.cpp \
fceu/input/bworld.cpp fceu/input/arkanoid.cpp fceu/input/mahjong.cpp fceu/input/fkb.cpp \
fceu/input/ftrainer.cpp fceu/input/hypershot.cpp fceu/input/cursor.cpp
BOARDS_SRC := \
01-222.cpp   208.cpp       67.cpp             dance2000.cpp          mmc2and4.cpp \
09-034a.cpp  222.cpp       68.cpp             datalatch.cpp          mmc3.cpp \
103.cpp      225.cpp       69.cpp             dream.cpp              mmc5.cpp \
106.cpp      228.cpp       71.cpp             dsoundv1.cpp           n106.cpp \
108.cpp      230.cpp       72.cpp             __dummy_mapper.cpp     n625092.cpp \
112.cpp      232.cpp       77.cpp             edu2000.cpp            novel.cpp \
116.cpp      234.cpp       79.cpp             emu2413.cpp            onebus.cpp \
117.cpp      235.cpp       80.cpp             famicombox.cpp         pec-586.cpp \
120.cpp      244.cpp       8157.cpp           ffe.cpp                sa-9602b.cpp \
121.cpp      246.cpp       8237.cpp           fk23c.cpp              sachen.cpp \
12in1.cpp    252.cpp       82.cpp             ghostbusters63in1.cpp  sc-127.cpp \
151.cpp      253.cpp       830118C.cpp        gs-2004.cpp            sheroes.cpp \
156.cpp      28.cpp        88.cpp             gs-2013.cpp            sl1632.cpp \
15.cpp       32.cpp        90.cpp             h2288.cpp              subor.cpp \
164.cpp      33.cpp        91.cpp             karaoke.cpp            super24.cpp \
168.cpp      34.cpp        96.cpp             kof97.cpp              supervision.cpp \
170.cpp      36.cpp        99.cpp             ks7012.cpp             t-227-1.cpp \
175.cpp      3d-block.cpp  a9746.cpp          ks7013.cpp             t-262.cpp \
176.cpp      40.cpp        ac-08.cpp          ks7017.cpp             tengen.cpp \
177.cpp      411120-c.cpp  addrlatch.cpp      ks7030.cpp             tf-1201.cpp \
178.cpp      41.cpp        ax5705.cpp         ks7031.cpp             transformer.cpp \
183.cpp      42.cpp        bandai.cpp         ks7032.cpp             vrc1.cpp \
185.cpp      43.cpp        bb.cpp             ks7037.cpp             vrc2and4.cpp \
186.cpp      46.cpp        bmc13in1jy110.cpp  ks7057.cpp             vrc3.cpp \
187.cpp      50.cpp        bmc42in1r.cpp      le05.cpp               vrc5.cpp \
189.cpp      51.cpp        bmc64in1nr.cpp     lh32.cpp               vrc6.cpp \
18.cpp       57.cpp        bmc70in1.cpp       lh53.cpp               vrc7.cpp \
193.cpp      603-5052.cpp  bonza.cpp          malee.cpp              vrc7p.cpp \
199.cpp      62.cpp        bs-5.cpp           mihunche.cpp           yoko.cpp \
206.cpp      65.cpp        cityfighter.cpp    mmc1.cpp
FCEUX_SRC += $(addprefix fceu/boards/,$(BOARDS_SRC))
FCEUX_OBJ := $(addprefix $(objDir)/,$(FCEUX_SRC:.cpp=.o))
SRC += $(FCEUX_SRC)

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/zlib.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
