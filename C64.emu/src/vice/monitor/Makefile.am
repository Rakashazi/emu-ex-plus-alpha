INCLUDES = \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/arch/unix/readline \
	-I$(top_srcdir)/src/drive \
	-I$(top_srcdir)/src/imagecontents \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/lib/p64

AM_YFLAGS = -dt

noinst_LIBRARIES = libmonitor.a

noinst_HEADERS = \
	mon_parse.h

libmonitor_a_SOURCES = \
	asm6502.c \
	asm6502dtv.c \
	asm6809.c \
	asmR65C02.c \
	asm65816.c \
	asmz80.c \
	asm.h \
	mon_assemble6502.c \
	mon_assembleR65C02.c \
	mon_assemble65816.c \
	mon_assemble6809.c \
	mon_assemblez80.c \
	mon_assemble.h \
	mon_breakpoint.c \
	mon_breakpoint.h \
	mon_command.c \
	mon_command.h \
	mon_disassemble.c \
	mon_disassemble.h \
	mon_drive.c \
	mon_drive.h \
	mon_file.c \
	mon_file.h \
	mon_memory.c \
	mon_memory.h \
	mon_register6502.c \
	mon_register6502dtv.c \
	mon_register6809.c \
	mon_registerR65C02.c \
	mon_register65816.c \
	mon_registerz80.c \
	mon_register.h \
	mon_ui.c \
	mon_ui.h \
	mon_util.c \
	mon_util.h \
	mon_lex.l \
	mon_parse.y \
	monitor.c \
	monitor_network.c \
	monitor_network.h \
	montypes.h

BUILT_SOURCES = mon_parse.c
