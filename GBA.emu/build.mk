ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/VbamApi.cc

include ../EmuFramework/common.mk

ifneq ($(config_compiler),clang)
 OPTIMIZE_CFLAGS += --param inline-unit-growth=2000
endif

CPPFLAGS += -DHAVE_ZLIB_H -DFINAL_VERSION -DC_CORE -DNO_PNG -DNO_LINK -DNO_DEBUGGER -DBLIP_BUFFER_FAST=1 \
-DSysDecimal=float -Isrc/vbam

# vba-m sources
SRC += vbam/gba/GBA-thumb.cpp \
vbam/gba/bios.cpp vbam/gba/Globals.cpp \
vbam/gba/Cheats.cpp vbam/gba/Mode0.cpp \
vbam/gba/CheatSearch.cpp vbam/gba/Mode1.cpp \
vbam/gba/EEprom.cpp vbam/gba/Mode2.cpp \
vbam/gba/Mode3.cpp \
vbam/gba/Flash.cpp vbam/gba/Mode4.cpp \
vbam/gba/GBA-arm.cpp vbam/gba/Mode5.cpp \
vbam/gba/GBA.cpp \
vbam/gba/gbafilter.cpp vbam/gba/RTC.cpp \
vbam/gba/Sound.cpp \
vbam/gba/Sram.cpp vbam/common/memgzio.c vbam/Util.cpp
#vbam/gba/remote.cpp vbam/gba/GBASockClient.cpp vbam/gba/GBALink.cpp vbam/gba/agbprint.cpp
# vbam/7z_C/7zHeader.c vbam/7z_C/7zItem.c vbam/gba/armdis.cpp vbam/gba/elf.cpp

SRC += vbam/apu/Gb_Apu.cpp vbam/apu/Gb_Oscs.cpp \
vbam/apu/Blip_Buffer.cpp vbam/apu/Multi_Buffer.cpp \
vbam/apu/Gb_Apu_State.cpp

SRC += vbam/fex/Binary_Extractor.cpp  vbam/fex/Gzip_Extractor.cpp \
vbam/fex/blargg_common.cpp     vbam/fex/Gzip_Reader.cpp \
vbam/fex/blargg_errors.cpp     vbam/fex/Rar_Extractor.cpp \
vbam/fex/Data_Reader.cpp       vbam/fex/Zip7_Extractor.cpp \
vbam/fex/fex.cpp               vbam/fex/Zip_Extractor.cpp \
vbam/fex/File_Extractor.cpp    vbam/fex/Zlib_Inflater.cpp

SRC += vbam/7z_C/7zAlloc.c    vbam/7z_C/Bra.c \
vbam/7z_C/7zBuf.c      vbam/7z_C/7zIn.c      vbam/7z_C/CpuArch.c \
vbam/7z_C/7zCrc.c      vbam/7z_C/Lzma2Dec.c \
vbam/7z_C/7zCrcOpt.c   vbam/7z_C/7zStream.c  vbam/7z_C/LzmaDec.c \
vbam/7z_C/7zDecode.c   vbam/7z_C/Bcj2.c \
vbam/7z_C/7zExtract.c  vbam/7z_C/Bra86.c 

include $(IMAGINE_PATH)/make/package/stdc++.mk

ifndef target
 target := gbaemu
endif

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
