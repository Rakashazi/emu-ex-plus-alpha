ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

VPATH += $(EMUFRAMEWORK_PATH)/src/shared

CPPFLAGS += -DHAVE_CONFIG_H \
-I$(projectPath)/src \
-I$(EMUFRAMEWORK_PATH)/include/shared/mednafen \
-I$(EMUFRAMEWORK_PATH)/src/shared \
-I$(EMUFRAMEWORK_PATH)/src/shared/mednafen/hw_misc \
-I$(EMUFRAMEWORK_PATH)/src/shared/mednafen/hw_sound

CXXFLAGS_WARN += -Wno-register -Wno-missing-field-initializers

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/PCEFast.cc \
mednafen-emuex/CDImpl.cc \
mednafen-emuex/MDFNApi.cc \
mednafen-emuex/MThreading.cc \
mednafen-emuex/StreamImpl.cc \
mednafen-emuex/VirtualFS.cpp \
mednafen-emuex/MDFNFILE.cc

MDFN_SRC := pce_fast/input.cpp \
pce_fast/vdc.cpp \
pce_fast/huc6280.cpp \
pce_fast/pce.cpp \
pce_fast/huc.cpp \
pce_fast/pcecd.cpp \
pce_fast/pcecd_drive.cpp \
pce_fast/psg.cpp \
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
cputest/cputest.c \
sound/okiadpcm.cpp \
sound/Blip_Buffer.cpp \
cdrom/CDAFReader.cpp \
cdrom/CDAFReader_FLAC.cpp \
cdrom/CDAFReader_MPC.cpp \
cdrom/CDAFReader_PCM.cpp \
cdrom/CDAFReader_Vorbis.cpp \
cdrom/galois.cpp \
cdrom/recover-raw.cpp \
cdrom/CDAccess.cpp \
cdrom/CDAccess_Image.cpp \
cdrom/CDAccess_CCD.cpp \
cdrom/CDUtility.cpp \
cdrom/l-ec.cpp \
cdrom/scsicd.cpp \
cdrom/CDInterface.cpp \
cdrom/CDInterface_MT.cpp \
cdrom/CDInterface_ST.cpp \
cdrom/lec.cpp \
cdrom/crc32.cpp \
hw_misc/arcade_card/arcade_card.cpp \
hw_sound/pce_psg/pce_psg.cpp \
video/resize.cpp \
video/surface.cpp \
compress/GZFileStream.cpp \
string/string.cpp \
hash/crc.cpp \
hash/md5.cpp \
mpcdec/huffman.c \
mpcdec/mpc_decoder.c \
mpcdec/mpc_reader.c \
mpcdec/requant.c \
mpcdec/streaminfo.c \
mpcdec/synth_filter.c \
mpcdec/mpc_bits_reader.c \
mpcdec/mpc_demux.c \
mpcdec/crc32.c

SRC += $(addprefix mednafen/,$(MDFN_SRC))

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/flac.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
