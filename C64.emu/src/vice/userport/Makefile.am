INCLUDES = \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/c64/cart \
	-I$(top_srcdir)/src/lib/p64 \
	-I$(top_srcdir)/src/rtc

noinst_LIBRARIES = libuserport.a

libuserport_a_SOURCES = \
	userport_dac.c \
	userport_dac.h \
	userport_digimax.c \
	userport_digimax.h \
	userport_joystick.c \
	userport_joystick.h \
	userport_rtc.c \
	userport_rtc.h
