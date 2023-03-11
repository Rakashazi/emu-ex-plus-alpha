AM_CPPFLAGS = \
	@VICE_CPPFLAGS@ \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/rtc \
	-I$(top_srcdir)/src/vicii \
	-I$(top_srcdir)/src/raster \
	-I$(top_srcdir)/src/c128 \
	-I$(top_srcdir)/src/c128/cart \
	-I$(top_srcdir)/src/c64 \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/arch/shared

AM_CFLAGS = @VICE_CFLAGS@

AM_CXXFLAGS = @VICE_CXXFLAGS@

AM_LDFLAGS = @VICE_LDFLAGS@


noinst_LIBRARIES = libc128cartsystem.a

libc128cartsystem_a_SOURCES = \
	c128cart.c \
	comal80.c \
	comal80.h \
	generic.c \
	generic.h \
	gmod2c128.c \
	gmod2c128.h \
	magicdesk128.c \
	magicdesk128.h \
	partner128.c \
	partner128.h \
	warpspeed128.c \
	warpspeed128.h
