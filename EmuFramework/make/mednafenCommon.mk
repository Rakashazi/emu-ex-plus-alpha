ifndef inc_mdfn_common
inc_mdfn_common := 1

MDFN_COMMON_CPPFLAGS = -DHAVE_CONFIG_H \
 -I$(EMUFRAMEWORK_PATH)/include/shared/mednafen \
 -I$(EMUFRAMEWORK_PATH)/src/shared

MDFN_COMMON_SRC := mednafen-emuex/MDFNApi.cc \
 mednafen-emuex/MThreading.cc \
 mednafen-emuex/StreamImpl.cc \
 mednafen-emuex/VirtualFS.cpp \
 mednafen-emuex/MDFNFILE.cc \
 mednafen/endian.cpp \
 mednafen/movie.cpp \
 mednafen/state.cpp \
 mednafen/file.cpp \
 mednafen/mempatcher.cpp \
 mednafen/error.cpp \
 mednafen/MemoryStream.cpp \
 mednafen/NativeVFS.cpp \
 mednafen/Stream.cpp \
 mednafen/memory.cpp \
 mednafen/git.cpp \
 mednafen/sound/Blip_Buffer.cpp \
 mednafen/sound/Stereo_Buffer.cpp \
 mednafen/video/resize.cpp \
 mednafen/video/surface.cpp \
 mednafen/compress/GZFileStream.cpp \
 mednafen/string/string.cpp \
 mednafen/hash/crc.cpp \
 mednafen/hash/md5.cpp

MDFN_CDROM_CPPFLAGS = -I$(EMUFRAMEWORK_PATH)/include/shared/lzma \
 -I$(EMUFRAMEWORK_PATH)/include/shared \

MDFN_CDROM_SRC := mednafen-emuex/CDImpl.cc \
 mednafen-emuex/ArchiveVFS.cc \
 mednafen/cdrom/CDAFReader.cpp \
 mednafen/cdrom/CDAFReader_FLAC.cpp \
 mednafen/cdrom/CDAFReader_MPC.cpp \
 mednafen/cdrom/CDAFReader_PCM.cpp \
 mednafen/cdrom/CDAFReader_Vorbis.cpp \
 mednafen/cdrom/galois.cpp \
 mednafen/cdrom/recover-raw.cpp \
 mednafen/cdrom/CDAccess.cpp \
 mednafen/cdrom/CDAccess_Image.cpp \
 mednafen/cdrom/CDAccess_CCD.cpp \
 mednafen/cdrom/CDAccess_CHD.cpp \
 mednafen/cdrom/CDUtility.cpp \
 mednafen/cdrom/l-ec.cpp \
 mednafen/cdrom/scsicd.cpp \
 mednafen/cdrom/CDInterface.cpp \
 mednafen/cdrom/CDInterface_MT.cpp \
 mednafen/cdrom/CDInterface_ST.cpp \
 mednafen/cdrom/lec.cpp \
 mednafen/cdrom/crc32.cpp \
 mednafen/mpcdec/huffman.c \
 mednafen/mpcdec/mpc_decoder.c \
 mednafen/mpcdec/mpc_reader.c \
 mednafen/mpcdec/requant.c \
 mednafen/mpcdec/streaminfo.c \
 mednafen/mpcdec/synth_filter.c \
 mednafen/mpcdec/mpc_bits_reader.c \
 mednafen/mpcdec/mpc_demux.c \
 mednafen/mpcdec/crc32.c \
 libchdr/libchdr_bitstream.c \
 libchdr/libchdr_cdrom.c \
 libchdr/libchdr_chd.c \
 libchdr/libchdr_flac.c \
 libchdr/libchdr_huffman.c \
 lzma/Alloc.c \
 lzma/BraIA64.c \
 lzma/Delta.c \
 lzma/Lzma86Dec.c \
 lzma/LzmaEnc.c \
 lzma/Bra86.c \
 lzma/CpuArch.c \
 lzma/LzFind.c \
 lzma/LzmaDec.c \
 lzma/Sort.c

MDFN_CDROM_STANDALONE_SRC := $(MDFN_CDROM_SRC) \
 mednafen-emuex/MDFNApi.cc \
 mednafen-emuex/StreamImpl.cc \
 mednafen-emuex/VirtualFS.cpp \
 mednafen-emuex/MDFNFILE.cc \
 mednafen/endian.cpp \
 mednafen/error.cpp \
 mednafen/MemoryStream.cpp \
 mednafen/NativeVFS.cpp \
 mednafen/Stream.cpp \
 mednafen/memory.cpp \
 mednafen/git.cpp \
 mednafen/string/string.cpp \
 mednafen/hash/crc.cpp
endif