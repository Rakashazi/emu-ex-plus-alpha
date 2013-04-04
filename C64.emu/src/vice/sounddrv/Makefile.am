# Makefile.am for `libsounddrv.a'.

INCLUDES = \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

noinst_LIBRARIES = libsounddrv.a

# These sources are linked only under certain circumstances.
EXTRA_libsounddrv_a_SOURCES = \
	soundahi.c \
	soundaix.c \
	soundallegro.c \
	soundalsa.c \
	soundarts.c \
	soundbeos.cc \
	soundcoreaudio.c \
	sounddart.c \
	sounddx.c \
	soundhpux.c \
	soundmidas.c \
	soundmmos2.c \
	soundmp3.c \
	soundpulse.c \
	soundsdl.c \
	soundsgi.c \
	soundsun.c \
	sounduss.c \
	soundwmm.c \
	lamelib.c

# These sources are always built.
libsounddrv_a_SOURCES = \
	soundaiff.c \
	sounddummy.c \
	sounddump.c \
	soundfs.c \
	soundiff.c \
	soundvoc.c \
	soundwav.c \
	soundmovie.c

noinst_HEADERS = \
  soundmovie.h

libsounddrv_a_DEPENDENCIES = \
	@SOUND_DRIVERS@ \
	soundaiff.o \
	sounddummy.o \
	sounddump.o \
	soundfs.o \
	soundiff.o \
	soundvoc.o \
	soundwav.o

libsounddrv_a_LIBADD = @SOUND_DRIVERS@

EXTRA_DIST = \
	lamelib.h
