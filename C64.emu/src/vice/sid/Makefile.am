if HAVE_RESID
resid_libadd = resid.o
else
resid_libadd =
endif

if HAVE_RESID_FP
residfp_libadd = resid-fp.o
else
residfp_libadd =
endif

SUFFIXES = .def

INCLUDES = \
	@ARCH_INCLUDES@ \
	@RESID_INCLUDES@ \
	@RESID_FP_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src

noinst_LIBRARIES = libsid.a

libsid_a_SOURCES = \
	fastsid.c \
	fastsid.h \
	sid-cmdline-options.c \
	sid-cmdline-options.h \
	sid-resources.c \
	sid-resources.h \
	sid-snapshot.c \
	sid-snapshot.h \
	sid.c \
	sid.h \
	wave6581.h \
	wave8580.h

EXTRA_libsid_a_SOURCES = \
	resid.cc \
	resid.h \
	resid-fp.cc \
	resid-fp.h

EXTRA_DIST = resid-dtv.cc

libsid_a_LIBADD = $(resid_libadd) $(residfp_libadd)

siddir = $(top_srcdir)/src/sid
