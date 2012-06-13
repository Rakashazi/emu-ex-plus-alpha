ifndef inc_main
inc_main := 1

# -O3 is faster with PCE.emu
HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/MDFNApi.cc

include ../EmuFramework/common.mk

CPPFLAGS += -DHAVE_CONFIG_H -DSysDDec=float -Isrc/include -Isrc/mednafen/hw_misc -Isrc/mednafen/hw_sound

# mednafen sources
SRC += mednafen/pce_fast/input.cpp mednafen/pce_fast/vdc.cpp \
mednafen/pce_fast/huc6280.cpp mednafen/pce_fast/pce.cpp mednafen/pce_fast/huc.cpp \
mednafen/pce_fast/tsushin.cpp
MDFN_SRC += mednafen/endian.cpp mednafen/movie.cpp mednafen/state.cpp mednafen/file.cpp \
mednafen/md5.cpp mednafen/mempatcher.cpp mednafen/okiadpcm.cpp mednafen/cdrom/audioreader.cpp \
mednafen/cdrom/galois.cpp mednafen/cdrom/recover-raw.cpp mednafen/cdrom/CDAccess.cpp \
mednafen/cdrom/CDAccess_Image.cpp mednafen/FileWrapper.cpp mednafen/cdrom/CDUtility.cpp \
mednafen/cdrom/l-ec.cpp mednafen/cdrom/scsicd.cpp mednafen/cdrom/cdromif.cpp mednafen/cdrom/lec.cpp \
mednafen/cdrom/crc32.cpp mednafen/cdrom/pcecd.cpp mednafen/hw_misc/arcade_card/arcade_card.cpp \
mednafen/hw_sound/pce_psg/pce_psg.cpp mednafen/sound/Blip_Buffer.cpp mednafen/video/resize.cpp \
mednafen/video/surface.cpp mednafen/general.cpp mednafen/error.cpp mednafen/math_ops.cpp

COMPRESS_SRC := mednafen/compress/blz.c mednafen/compress/minilzo.c mednafen/compress/quicklz.c
SRC += $(MDFN_SRC) $(COMPRESS_SRC)
# mednafen/pce_fast/hes.cpp mednafen/memory.cpp mednafen/error.cpp mednafen/general.cpp
# mednafen/cdrom/cdromfile.cpp

include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/libsndfile.mk
include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

ifeq ($(ENV), linux)
 LDLIBS +=  /usr/lib/libcdio.so
 SRC += mednafen/cdrom/CDAccess_Physical.cpp
endif

ifneq ($(ENV), android)
 LDLIBS += -lpthread
endif

configInc += <mednafen-config.h>

ifndef target
target := pceemu
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
