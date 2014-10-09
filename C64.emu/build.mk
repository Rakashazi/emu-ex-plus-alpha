ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

SRC += main/Main.cc main/EmuControls.cc main/ViceApi.cc

CPPFLAGS += -DSysDecimal=float -I$(projectPath)/src -I$(projectPath)/src/config -I$(projectPath)/src/vice -I$(projectPath)/src/vice/c64 -I$(projectPath)/src/vice/c64/cart \
-I$(projectPath)/src/vice/drive -I$(projectPath)/src/vice/lib/p64 -I$(projectPath)/src/vice/sid -I$(projectPath)/src/vice/vicii -I$(projectPath)/src/vice/tape -I$(projectPath)/src/vice/userport \
-I$(projectPath)/src/vice/video -I$(projectPath)/src/vice/drive/iec/c64exp -I$(projectPath)/src/vice/core -I$(projectPath)/src/vice/rtc -I$(projectPath)/src/vice/vdrive \
-I$(projectPath)/src/vice/imagecontents -I$(projectPath)/src/vice/monitor -I$(projectPath)/src/vice/platform -I$(projectPath)/src/vice/raster -I$(projectPath)/src/vice/c64dtv \
-DSTDC_HEADERS=1 -DHAVE_SYS_TYPES_H=1 -DHAVE_SYS_STAT_H=1 -DHAVE_STDLIB_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 \
-DHAVE_STRINGS_H=1 -DHAVE_INTTYPES_H=1 -DHAVE_STDINT_H=1 -DHAVE_UNISTD_H=1

VPATH += $(projectPath)/src/vice
base_sources = \
	alarm.c \
	attach.c \
	autostart.c \
	autostart-prg.c \
	charset.c \
	clkguard.c \
	clipboard.c \
	cbmdos.c \
	cbmimage.c \
	crc32.c \
	datasette.c \
	debug.c \
	dma.c \
	event.c \
	fliplist.c \
	gcr.c \
	info.c \
	init.c \
	interrupt.c \
	ioutil.c \
	joystick.c \
	kbdbuf.c \
	keyboard.c \
	lib.c \
	libm_math.c \
	lightpen.c \
	machine-bus.c \
	machine.c \
	network.c \
	opencbmlib.c \
	palette.c \
	ram.c \
	rawfile.c \
	rawnet.c \
	resources.c \
	romset.c \
	snapshot.c \
	socket.c \
	sound.c \
	translate.c \
	traps.c \
	util.c \
	vsync.c \
	zfile.c \
	zipcode.c
	# log.c screenshot.c embedded.c sysfile.c findpath.c cmdline.c initcmdline.c main.c color.c

libc64_a_SOURCES = \
	c64-cmdline-options.c \
	c64-resources.c \
	c64-snapshot.c \
	c64.c \
	c64_256k.c \
	c64bus.c \
	c64cia1.c \
	c64cia2.c \
	c64datasette.c \
	c64drive.c \
	c64export.c \
	c64fastiec.c \
	c64gluelogic.c \
	c64iec.c \
	c64io.c \
	c64keyboard.c \
	c64mem.c \
	c64meminit.c \
	c64memlimit.c \
	c64memrom.c \
	c64memsnapshot.c \
	c64model.c \
	c64parallel.c \
	c64pla.c \
	c64printer.c \
	c64rom.c \
	c64romset.c \
	c64rsuser.c \
	c64sound.c \
	c64video.c \
	patchrom.c \
	plus256k.c \
	plus60k.c \
	psid.c \
	reloc65.c
	# c64embedded.c
libc64_a_SOURCES := $(addprefix c64/,$(libc64_a_SOURCES))

libc64cartsystem_a_SOURCES = \
	c64cart.c \
	c64carthooks.c \
	c64cartmem.c \
	crt.c
libc64cartsystem_a_SOURCES := $(addprefix c64/cart/,$(libc64cartsystem_a_SOURCES))

libc64cart_a_SOURCES = \
	actionreplay2.c \
	actionreplay3.c \
	actionreplay4.c \
	actionreplay.c \
	atomicpower.c \
	c64-generic.c \
	c64-midi.c \
	c64tpi.c \
	capture.c \
	comal80.c \
	delaep256.c \
	delaep64.c \
	delaep7x8.c \
	diashowmaker.c \
	dqbb.c \
	dinamic.c \
	easyflash.c \
	epyxfastload.c \
	exos.c \
	expert.c \
	final.c \
	final3.c \
	finalplus.c \
	formel64.c \
	freezeframe.c \
	freezemachine.c \
	funplay.c \
	gamekiller.c \
	gs.c \
	ide64.c \
	isepic.c \
	kcs.c \
	kingsoft.c \
	mach5.c \
	magicdesk.c \
	magicformel.c \
	magicvoice.c \
	mikroass.c \
	mmcreplay.c \
	mmc64.c \
	ocean.c \
	prophet64.c \
	pagefox.c \
	ramcart.c \
	retroreplay.c \
	reu.c \
	rexep256.c \
	rexutility.c \
	ross.c \
	silverrock128.c \
	simonsbasic.c \
	snapshot64.c \
	stardos.c \
	stb.c \
	superexplode5.c \
	supergames.c \
	supersnapshot.c \
	supersnapshot4.c \
	warpspeed.c \
	westermann.c \
	zaxxon.c
libc64cart_a_SOURCES := $(addprefix c64/cart/,$(libc64cart_a_SOURCES))

libc64commoncart_a_SOURCES = \
	c64acia1.c \
	digimax.c \
	ds12c887rtc.c \
	georam.c \
	sfx_soundexpander.c \
	sfx_soundsampler.c \
	tfe.c
libc64commoncart_a_SOURCES := $(addprefix c64/cart/,$(libc64commoncart_a_SOURCES))

libdrive_a_SOURCES = \
	drive-check.c \
	drive-cmdline-options.c \
	drive-overflow.c \
	drive-resources.c \
	drive-snapshot.c \
	drive-writeprotect.c \
	drive.c \
	drivecpu.c \
	drivecpu65c02.c \
	drivemem.c \
	driveimage.c \
	driverom.c \
	drivesync.c \
	drive-sound.c \
	rotation.c
libdrive_a_SOURCES := $(addprefix drive/,$(libdrive_a_SOURCES))

libdriveiec_a_SOURCES = \
	cia1571d.c \
	cia1581d.c \
	glue1571.c \
	iec-cmdline-options.c \
	iec-resources.c \
	iec.c \
	iecrom.c \
	memiec.c \
	via1d1541.c \
	wd1770.c \
	via4000.c \
	fdd.c \
	pc8477.c
libdriveiec_a_SOURCES := $(addprefix drive/iec/,$(libdriveiec_a_SOURCES))

libdriveiecc64exp_a_SOURCES = \
	c64exp-cmdline-options.c \
	c64exp-resources.c \
	iec-c64exp.c \
	dolphindos3.c \
	profdos.c \
	supercard.c
libdriveiecc64exp_a_SOURCES := $(addprefix drive/iec/c64exp/,$(libdriveiecc64exp_a_SOURCES))

libdriveiecplus4exp_a_SOURCES = \
	iec-plus4exp.c \
	plus4exp-cmdline-options.c \
	plus4exp-resources.c
libdriveiecplus4exp_a_SOURCES := $(addprefix drive/iec/plus4exp/,$(libdriveiecplus4exp_a_SOURCES))

libdriveiec128dcr_a_SOURCES = \
	iec128dcr-cmdline-options.c \
	iec128dcr-resources.c \
	iec128dcr.c \
	iec128dcrrom.c
libdriveiec128dcr_a_SOURCES := $(addprefix drive/iec128dcr/,$(libdriveiec128dcr_a_SOURCES))

libdriveiecieee_a_SOURCES = \
	iecieee.c \
	via2d.c
libdriveiecieee_a_SOURCES := $(addprefix drive/iecieee/,$(libdriveiecieee_a_SOURCES))

libdriveieee_a_SOURCES = \
	fdc.c \
	ieee-cmdline-options.c \
	ieee-resources.c \
	ieee.c \
	ieeerom.c \
	memieee.c \
	riot1d.c \
	riot2d.c \
	via1d2031.c
libdriveieee_a_SOURCES := $(addprefix drive/ieee/,$(libdriveieee_a_SOURCES))

# unused by C64
#libdrivetcbm_a_SOURCES = \
	glue1551.c \
	mem1551.c \
	tcbm-cmdline-options.c \
	tcbm-resources.c \
	tcbm.c \
	tcbmrom.c \
	tpid.c
#libdrivetcbm_a_SOURCES := $(addprefix drive/tcbm/,$(libdrivetcbm_a_SOURCES))

libp64_a_SOURCES = p64.c
libp64_a_SOURCES := $(addprefix lib/p64/,$(libp64_a_SOURCES))

libimagecontents_a_SOURCES = \
	diskcontents-block.c \
	diskcontents-iec.c \
	diskcontents.c \
	imagecontents.c \
	tapecontents.c
libimagecontents_a_SOURCES := $(addprefix imagecontents/,$(libimagecontents_a_SOURCES))

libmonitor_a_SOURCES = \
	asm6502.c \
	asm6502dtv.c \
	asm6809.c \
	asmR65C02.c \
	asmz80.c \
	mon_assemble6502.c \
	mon_assembleR65C02.c \
	mon_assemble6809.c \
	mon_assemblez80.c \
	mon_breakpoint.c \
	mon_command.c \
	mon_disassemble.c \
	mon_drive.c \
	mon_file.c \
	mon_memory.c \
	mon_register6502.c \
	mon_register6502dtv.c \
	mon_register6809.c \
	mon_registerR65C02.c \
	mon_registerz80.c \
	mon_ui.c \
	mon_util.c \
	monitor.c \
	monitor_network.c \
	mon_parse.c \
	mon_lex.c
libmonitor_a_SOURCES := $(addprefix monitor/,$(libmonitor_a_SOURCES))

libiecbus_a_SOURCES = \
	iecbus.c
libiecbus_a_SOURCES := $(addprefix iecbus/,$(libiecbus_a_SOURCES))

libparallel_a_SOURCES = \
	parallel-trap.c \
	parallel.c
libparallel_a_SOURCES := $(addprefix parallel/,$(libparallel_a_SOURCES))

libvdrive_a_SOURCES = \
	vdrive-bam.c \
	vdrive-command.c \
	vdrive-dir.c \
	vdrive-iec.c \
	vdrive-internal.c \
	vdrive-rel.c \
	vdrive-snapshot.c \
	vdrive.c
libvdrive_a_SOURCES := $(addprefix vdrive/,$(libvdrive_a_SOURCES))

libsid_a_SOURCES = \
	fastsid.c \
	sid-cmdline-options.c \
	sid-resources.c \
	sid-snapshot.c \
	sid.c
libsid_a_SOURCES := $(addprefix sid/,$(libsid_a_SOURCES))

EXTRA_libsid_a_SOURCES += resid.cc
libresid_a_SOURCES = sid.cc voice.cc wave.cc envelope.cc filter.cc dac.cc extfilt.cc pot.cc #version.cc
libresid_a_SOURCES := $(addprefix resid/,$(libresid_a_SOURCES))

EXTRA_libsid_a_SOURCES := $(addprefix sid/,$(EXTRA_libsid_a_SOURCES))

librtc_a_SOURCES = \
	bq4830y.c \
	ds12c887.c \
	ds1202_1302.c \
	ds1216e.c \
	rtc.c \
	rtc-58321a.c
librtc_a_SOURCES := $(addprefix rtc/,$(librtc_a_SOURCES))

libtape_a_SOURCES = \
	t64.c \
	tap.c \
	tape-internal.c \
	tape-snapshot.c \
	tape.c \
	tapeimage.c
libtape_a_SOURCES := $(addprefix tape/,$(libtape_a_SOURCES))

libcore_a_SOURCES = \
	ciacore.c \
	ciatimer.c \
	cs8900.c \
	flash040core.c \
	fmopl.c \
	mc6821core.c \
	riotcore.c \
	ser-eeprom.c \
	spi-sdcard.c \
	t6721.c \
	tpicore.c \
	viacore.c \
	ata.c
libcore_a_SOURCES := $(addprefix core/,$(libcore_a_SOURCES))

libuserport_a_SOURCES = \
	userport_dac.c \
	userport_digimax.c \
	userport_joystick.c \
	userport_rtc.c
libuserport_a_SOURCES := $(addprefix userport/,$(libuserport_a_SOURCES))

libvicii_a_SOURCES = \
	vicii-badline.c \
	vicii-clock-stretch.c \
	vicii-cmdline-options.c \
	vicii-color.c \
	vicii-draw.c \
	vicii-fetch.c \
	vicii-irq.c \
	vicii-mem.c \
	vicii-phi1.c \
	vicii-resources.c \
	vicii-snapshot.c \
	vicii-sprites.c \
	vicii-stubs.c \
	vicii-timing.c \
	vicii.c
libvicii_a_SOURCES := $(addprefix vicii/,$(libvicii_a_SOURCES))

libraster_a_SOURCES = \
	raster-cache.c \
	raster-canvas.c \
	raster-changes.c \
	raster-cmdline-options.c \
	raster-line-changes-sprite.c \
	raster-line-changes.c \
	raster-line.c \
	raster-modes.c \
	raster-resources.c \
	raster-sprite.c \
	raster-sprite-status.c \
	raster-sprite-cache.c \
	raster.c
libraster_a_SOURCES := $(addprefix raster/,$(libraster_a_SOURCES))

libvideo_a_SOURCES = \
	render1x1.c \
	render1x1pal.c \
	render1x1ntsc.c \
	video-canvas.c \
	video-cmdline-options.c \
	video-color.c \
	video-render-crt.c \
	video-render-pal.c \
	video-render.c \
	video-resources.c \
	video-sound.c \
	video-viewport.c
	# render1x2.c \
	render1x2crt.c \
	render2x2.c \
	render2x2crt.c \
	render2x2pal.c \
	render2x2ntsc.c \
	renderscale2x.c \
	render2x4.c \
	render2x4crt.c \
	renderyuv.c \
	video-render-1x2.c \
	video-render-2x2.c
libvideo_a_SOURCES := $(addprefix video/,$(libvideo_a_SOURCES))

libserial_a_SOURCES = \
	fsdrive.c \
	serial-device.c \
	serial-iec-bus.c \
	serial-iec-device.c \
	serial-iec-lib.c \
	serial-iec.c \
	serial-realdevice.c \
	serial-trap.c \
	serial.c
libserial_a_SOURCES := $(addprefix serial/,$(libserial_a_SOURCES))

libfsdevice_a_SOURCES = \
	fsdevice-close.c \
	fsdevice-cmdline-options.c \
	fsdevice-flush.c \
	fsdevice-open.c \
	fsdevice-read.c \
	fsdevice-resources.c \
	fsdevice-write.c \
	fsdevice.c
libfsdevice_a_SOURCES := $(addprefix fsdevice/,$(libfsdevice_a_SOURCES))

libprinterdrv_a_SOURCES = \
	driver-select.c \
	drv-ascii.c \
	drv-mps803.c \
	drv-nl10.c \
	drv-raw.c \
	interface-serial.c \
	interface-userport.c \
	output-graphics.c \
	output-select.c \
	output-text.c \
	printer-serial.c \
	printer-userport.c \
	printer.c
libprinterdrv_a_SOURCES := $(addprefix printerdrv/,$(libprinterdrv_a_SOURCES))

librs232drv_a_SOURCES = \
	rs232drv.c \
	rsuser.c
librs232drv_a_SOURCES := $(addprefix rs232drv/,$(librs232drv_a_SOURCES))

libdiskimage_a_SOURCES = \
	diskimage.c \
	fsimage-check.c \
	fsimage-create.c \
	fsimage-dxx.c \
	fsimage-gcr.c \
	fsimage-p64.c \
	fsimage-probe.c \
	fsimage.c
libdiskimage_a_SOURCES := $(addprefix diskimage/,$(libdiskimage_a_SOURCES))

libfileio_a_SOURCES = \
	cbmfile.c \
	fileio.c \
	p00.c
libfileio_a_SOURCES := $(addprefix fileio/,$(libfileio_a_SOURCES))

#libsounddrv_a_SOURCES = \
	soundaiff.c \
	sounddummy.c \
	sounddump.c \
	soundfs.c \
	soundiff.c \
	soundvoc.c \
	soundwav.c \
	soundmovie.c

SRC += $(libc64_a_SOURCES) $(libc64cartsystem_a_SOURCES) $(libc64cart_a_SOURCES) $(libc64commoncart_a_SOURCES) \
$(libdrive_a_SOURCES) $(libdriveiec_a_SOURCES) $(libdriveiecc64exp_a_SOURCES) $(libdriveiecplus4exp_a_SOURCES) \
$(libdriveiec128dcr_a_SOURCES) $(libdriveiecieee_a_SOURCES) $(libdriveieee_a_SOURCES) $(libdrivetcbm_a_SOURCES) \
$(libp64_a_SOURCES) $(libimagecontents_a_SOURCES) $(libmonitor_a_SOURCES) $(libiecbus_a_SOURCES) $(libcore_a_SOURCES) \
$(libparallel_a_SOURCES) $(libvdrive_a_SOURCES) $(libsid_a_SOURCES) $(librtc_a_SOURCES) $(libtape_a_SOURCES) \
$(libuserport_a_SOURCES) $(libvicii_a_SOURCES) $(libraster_a_SOURCES) $(libvideo_a_SOURCES) $(libserial_a_SOURCES) \
$(libfsdevice_a_SOURCES) $(base_sources) $(libprinterdrv_a_SOURCES) $(librs232drv_a_SOURCES) $(libresid_a_SOURCES) \
$(libdiskimage_a_SOURCES) $(libfileio_a_SOURCES) $(EXTRA_libsid_a_SOURCES) $(libsounddrv_a_SOURCES) \
maincpu.c

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
