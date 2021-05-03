ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
common/CDImpl.cc \
common/MDFNApi.cc \
common/MThreading.cc \
common/StreamImpl.cc \
main/PCEFast.cc \
main/MDFNFILE.cc

CPPFLAGS += -DHAVE_CONFIG_H \
-I$(projectPath)/src \
-I$(projectPath)/src/include \
-I$(projectPath)/src/mednafen/hw_misc \
-I$(projectPath)/src/mednafen/hw_sound

CXXFLAGS_WARN += -Wno-register -Wno-missing-field-initializers

# mednafen sources
SRC += mednafen/pce_fast/input.cpp \
mednafen/pce_fast/vdc.cpp \
mednafen/pce_fast/huc6280.cpp \
mednafen/pce_fast/pce.cpp \
mednafen/pce_fast/huc.cpp \
mednafen/pce_fast/pcecd.cpp \
mednafen/pce_fast/pcecd_drive.cpp \
mednafen/pce_fast/psg.cpp
MDFN_SRC += mednafen/endian.cpp \
mednafen/movie.cpp \
mednafen/state.cpp \
mednafen/file.cpp \
mednafen/mempatcher.cpp \
mednafen/general.cpp \
mednafen/error.cpp \
mednafen/MemoryStream.cpp \
mednafen/NativeVFS.cpp \
mednafen/Stream.cpp \
mednafen/memory.cpp \
mednafen/git.cpp \
mednafen/VirtualFS.cpp \
mednafen/cputest/cputest.c \
mednafen/sound/okiadpcm.cpp \
mednafen/sound/Blip_Buffer.cpp \
mednafen/cdrom/CDAFReader.cpp \
mednafen/cdrom/CDAFReader_SF.cpp \
mednafen/cdrom/CDAFReader_Vorbis.cpp \
mednafen/cdrom/galois.cpp \
mednafen/cdrom/recover-raw.cpp \
mednafen/cdrom/CDAccess.cpp \
mednafen/cdrom/CDAccess_Image.cpp \
mednafen/cdrom/CDAccess_CCD.cpp \
mednafen/cdrom/CDUtility.cpp \
mednafen/cdrom/l-ec.cpp \
mednafen/cdrom/scsicd.cpp \
mednafen/cdrom/CDInterface.cpp \
mednafen/cdrom/CDInterface_MT.cpp \
mednafen/cdrom/CDInterface_ST.cpp \
mednafen/cdrom/lec.cpp \
mednafen/cdrom/crc32.cpp \
mednafen/hw_misc/arcade_card/arcade_card.cpp \
mednafen/hw_sound/pce_psg/pce_psg.cpp \
mednafen/video/resize.cpp \
mednafen/video/surface.cpp \
mednafen/compress/GZFileStream.cpp \
mednafen/string/string.cpp \
mednafen/hash/crc.cpp \
mednafen/hash/md5.cpp

SRC += $(MDFN_SRC)

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/libvorbis.mk
include $(IMAGINE_PATH)/make/package/libsndfile.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
