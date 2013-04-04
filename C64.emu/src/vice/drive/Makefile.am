SUBDIRS = \
	iec \
	iec128dcr \
	iecieee \
	ieee \
	tcbm

INCLUDES = \
	@ARCH_INCLUDES@ \
	-I$(top_builddir)/src \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/vdrive \
	-I$(top_srcdir)/src/rtc \
	-I$(top_srcdir)/src/lib/p64

noinst_LIBRARIES = libdrive.a

libdrive_a_SOURCES = \
	drive-check.c \
	drive-check.h \
	drive-cmdline-options.c \
	drive-cmdline-options.h \
	drive-overflow.c \
	drive-overflow.h \
	drive-resources.c \
	drive-resources.h \
	drive-snapshot.c \
	drive-snapshot.h \
	drive-writeprotect.c \
	drive-writeprotect.h \
	drive.c \
	drive.h \
	drivecpu.c \
	drivecpu.h \
	drivecpu65c02.c \
	drivecpu65c02.h \
	drivemem.c \
	driveimage.c \
	driveimage.h \
	drivemem.h \
	driverom.c \
	driverom.h \
	drivesync.c \
	drivesync.h \
	drivetypes.h \
	iec-c64exp.h \
	iec-plus4exp.h \
	iec.h \
	iec128dcr.h \
	iecieee.h \
	ieee.h \
	drive-sound.c \
	drive-sound.h \
	rotation.c \
	rotation.h \
	tcbm.h \
	viad.h

.PHONY: libdriveiec libdriveiec128dcr libdriveiecieee libdriveieee libdrivetcbm

libdriveiec:
	(cd iec; $(MAKE))
libdriveiec128dcr:
	(cd iec128dcr; $(MAKE))
libdriveiecieee:
	(cd iecieee; $(MAKE))
libdriveieee:
	(cd ieee; $(MAKE))
libdrivetcbm:
	(cd tcbm; $(MAKE))

