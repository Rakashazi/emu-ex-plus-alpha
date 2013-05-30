ifndef inc_main
inc_main := 1

ifeq ($(ARCH), arm)
 ifneq ($(config_compiler), clang)
  ifeq (4.7, $(findstring 4.7,$(gccVersion)))
   # -fexpensive-optimizations seems to cause miscompile on ARM GCC 4.7.x (Golden Sun), fixed in GCC 4.8
   HIGH_OPTIMIZE_CFLAGS := -O3 -fno-expensive-optimizations $(HIGH_OPTIMIZE_CFLAGS_MISC)
  endif
 endif
endif

ifeq ($(config_compiler), clang)
 HIGH_OPTIMIZE_CFLAGS = -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)
endif

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CPPFLAGS += -DHAVE_ZLIB_H -DFINAL_VERSION -DC_CORE -DNO_PNG -DNO_LINK -DNO_DEBUGGER -DBLIP_BUFFER_FAST=1 \
-DSysDecimal=float -Isrc/vbam

ifneq ($(config_compiler),clang)
 ifeq (4.7, $(findstring 4.7,$(gccVersion)))
  OPTIMIZE_CFLAGS += --param inline-unit-growth=2000
 endif
endif

emuFramework_cheats := 1
include ../EmuFramework/common.mk

vbamSrc := gba/GBA-thumb.cpp gba/bios.cpp gba/Globals.cpp \
gba/Cheats.cpp gba/Mode0.cpp gba/CheatSearch.cpp gba/Mode1.cpp \
gba/EEprom.cpp gba/Mode2.cpp gba/Mode3.cpp gba/Flash.cpp gba/Mode4.cpp \
gba/GBA-arm.cpp gba/Mode5.cpp gba/GBA.cpp gba/gbafilter.cpp gba/RTC.cpp \
gba/Sound.cpp gba/Sram.cpp common/memgzio.c common/Patch.cpp Util.cpp
#gba/remote.cpp gba/GBASockClient.cpp gba/GBALink.cpp gba/agbprint.cpp
# 7z_C/7zHeader.c 7z_C/7zItem.c gba/armdis.cpp gba/elf.cpp

vbamSrc += apu/Gb_Apu.cpp apu/Gb_Oscs.cpp apu/Blip_Buffer.cpp \
apu/Multi_Buffer.cpp apu/Gb_Apu_State.cpp

vbamSrc += fex/Binary_Extractor.cpp fex/Gzip_Extractor.cpp fex/blargg_common.cpp \
fex/Gzip_Reader.cpp fex/blargg_errors.cpp fex/Rar_Extractor.cpp \
fex/Data_Reader.cpp fex/Zip7_Extractor.cpp fex/fex.cpp fex/Zip_Extractor.cpp \
fex/File_Extractor.cpp fex/Zlib_Inflater.cpp

vbamSrc += 7z_C/7zAlloc.c 7z_C/Bra.c 7z_C/7zBuf.c 7z_C/CpuArch.c 7z_C/7zDec.c \
7z_C/7zCrc.c 7z_C/Lzma2Dec.c 7z_C/7zIn.c 7z_C/7zCrcOpt.c 7z_C/7zStream.c \
7z_C/LzmaDec.c 7z_C/Bcj2.c 7z_C/Bra86.c 7z_C/Ppmd7.c 7z_C/Ppmd7Dec.c

vbamPath := vbam
SRC += main/Main.cc main/EmuControls.cc main/VbamApi.cc main/Cheats.cc $(addprefix $(vbamPath)/,$(vbamSrc))

include $(IMAGINE_PATH)/make/package/zlib.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
