ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

viceSrcPath := $(projectPath)/src/vice

CFLAGS_LANG += -Werror=implicit-function-declaration
CFLAGS_WARN += -Wno-implicit-fallthrough -Wno-sign-compare

ifeq ($(ENV),android)
 LDLIBS += -ldl
else ifeq ($(ENV),linux)
 CFLAGS_CODEGEN += -fPIC
 LDLIBS += -ldl
 LDFLAGS += -Wl,-E
else ifeq ($(ENV),ios)
 CFLAGS_CODEGEN := $(filter-out -mdynamic-no-pic,$(CFLAGS_CODEGEN))
endif

SRC += main/Main.cc \
main/options.cc \
main/input.cc \
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/VicePlugin.cc \
main/sysfile.cc \
main/video.cc \
main/sound.cc \
main/log.cc \
main/zfile.cc

CPPFLAGS += \
-I$(projectPath)/src \
-I$(projectPath)/src/config \
-I$(projectPath)/src/vice \
-I$(viceSrcPath)/c64 \
-I$(viceSrcPath)/c64/cart \
-I$(viceSrcPath)/c128 \
-I$(viceSrcPath)/cbm2 \
-I$(viceSrcPath)/pet \
-I$(viceSrcPath)/plus4 \
-I$(viceSrcPath)/drive \
-I$(viceSrcPath)/lib/p64 \
-I$(viceSrcPath)/sid \
-I$(viceSrcPath)/tape \
-I$(viceSrcPath)/userport \
-I$(viceSrcPath)/video \
-I$(viceSrcPath)/drive/iec/c64exp \
-I$(viceSrcPath)/core \
-I$(viceSrcPath)/rtc \
-I$(viceSrcPath)/vdrive \
-I$(viceSrcPath)/imagecontents \
-I$(viceSrcPath)/monitor \
-I$(viceSrcPath)/platform \
-I$(viceSrcPath)/raster \
-I$(viceSrcPath)/c64dtv \
-I$(viceSrcPath)/vicii \
-I$(viceSrcPath)/viciisc \
-I$(viceSrcPath)/vdc \
-I$(viceSrcPath)/vic20 \
-I$(viceSrcPath)/vic20/cart \
-I$(viceSrcPath)/crtc \
-I$(viceSrcPath)/tapeport \
-I$(viceSrcPath)/joyport \
-I$(viceSrcPath)/samplerdrv \
-I$(viceSrcPath)/drive/iec \
-I$(viceSrcPath)/drive/ieee \
-I$(viceSrcPath)/drive/tcbm \
-I$(viceSrcPath)/diag \
-I$(viceSrcPath)/rs232drv \
-DSTDC_HEADERS=1 \
-DHAVE_SYS_TYPES_H=1 \
-DHAVE_SYS_STAT_H=1 \
-DHAVE_STDLIB_H=1 \
-DHAVE_STRING_H=1 \
-DHAVE_MEMORY_H=1 \
-DHAVE_STRINGS_H=1 \
-DHAVE_INTTYPES_H=1 \
-DHAVE_STDINT_H=1 \
-DHAVE_UNISTD_H=1

VPATH += $(viceSrcPath)
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
kbdbuf.c \
keyboard.c \
lib.c \
libm_math.c \
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
traps.c \
util.c \
vsync.c \
zipcode.c

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
clockport.c \
comal80.c \
cpmcart.c \
daa.c \
debugcart.c \
delaep256.c \
delaep64.c \
delaep7x8.c \
diashowmaker.c \
dinamic.c \
dqbb.c \
easycalc.c \
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
gmod2.c \
gs.c \
ide64.c \
isepic.c \
kcs.c \
kingsoft.c \
mach5.c \
magicdesk.c \
magicformel.c \
magicvoice.c \
maxbasic.c \
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
rgcd.c \
ross.c \
rrnetmk3.c \
shortbus.c \
shortbus_digimax.c \
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

# don't include cpmcart.c for C128 due to conflicting Z80 regs definition
libc128cart_a_SOURCES := $(libc64cart_a_SOURCES:c64/cart/cpmcart.c=) main/cpmcartStubs.c

libc64commoncart_a_SOURCES = \
c64acia1.c \
digimax.c \
ds12c887rtc.c \
georam.c \
sfx_soundexpander.c \
sfx_soundsampler.c
libc64commoncart_a_SOURCES := $(addprefix c64/cart/,$(libc64commoncart_a_SOURCES))

libcrtc_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/crtc/*)))

libiecbus_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/iecbus/*)))

libserial_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/serial/*)))

libc64_a_SOURCES = \
c64-cmdline-options.c \
c64-memory-hacks.c \
c64-resources.c \
c64-snapshot.c \
c64.c \
c64_256k.c \
c64bus.c \
c64cia1.c \
c64cia2.c \
c64cpu.c \
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
libc64_a_SOURCES := $(addprefix c64/,$(libc64_a_SOURCES))

libc64scpu64_a_SOURCES = \
c64bus.c \
c64cia1.c \
c64cia2.c \
c64datasette.c \
c64drive.c \
c64embedded.c \
c64export.c \
c64fastiec.c \
c64iec.c \
c64io.c \
c64keyboard.c \
c64memsnapshot.c \
c64parallel.c \
c64printer.c \
c64romset.c \
c64rsuser.c \
c64sound.c \
c64video.c \
patchrom.c \
reloc65.c
libc64scpu64_a_SOURCES := $(addprefix c64/,$(libc64scpu64_a_SOURCES))

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

libviciitv_a_SOURCES = \
vicii-badline.c \
vicii-clock-stretch.c \
vicii-cmdline-options.c \
viciidtv-color.c \
viciidtv-draw.c \
vicii-fetch.c \
vicii-irq.c \
vicii-mem.c \
vicii-phi1.c \
vicii-resources.c \
viciidtv-snapshot.c \
vicii-sprites.c \
vicii-timing.c \
vicii.c
libviciitv_a_SOURCES := $(addprefix vicii/,$(libviciitv_a_SOURCES))

libc64sc_a_SOURCES = \
c64-cmdline-options.c \
c64-memory-hacks.c \
c64-resources.c \
c64-snapshot.c \
c64.c \
c64_256k.c \
c64bus.c \
c64cia1.c \
c64cia2.c \
c64cpusc.c \
c64datasette.c \
c64drive.c \
c64export.c \
c64fastiec.c \
c64gluelogic.c \
c64iec.c \
c64io.c \
c64keyboard.c \
c64memsc.c \
c64meminit.c \
c64memlimit.c \
c64memrom.c \
c64memsnapshot.c \
c64scmodel.c \
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
libc64sc_a_SOURCES := $(addprefix c64/,$(libc64sc_a_SOURCES))

libviciisc_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/viciisc/*)))

libscpu64_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/scpu64/*)))

libc64c64dtv_a_SOURCES = \
c64bus.c \
c64drive.c \
c64fastiec.c \
c64keyboard.c \
c64parallel.c \
c64rom.c \
c64romset.c \
c64rsuser.c \
c64video.c \
patchrom.c
libc64c64dtv_a_SOURCES := $(addprefix c64/,$(libc64c64dtv_a_SOURCES))

libc64dtv_a_SOURCES = \
c64dtvmemsnapshot.c \
c64dtvmem.c \
c64dtvmemrom.c \
c64dtvblitter.c \
c64dtvcpu.c \
c64dtvdma.c \
c64dtvflash.c \
c64dtv-cmdline-options.c \
c64dtv-resources.c \
c64dtv-snapshot.c \
c64dtv.c \
c64dtvcia1.c \
c64dtvcia2.c \
c64dtvembedded.c \
c64dtviec.c \
c64dtvmeminit.c \
c64dtvmodel.c \
c64dtvpla.c \
c64dtvprinter.c \
c64dtvsound.c \
debugcart.c \
flash-trap.c \
hummeradc.c
libc64dtv_a_SOURCES := $(addprefix c64dtv/,$(libc64dtv_a_SOURCES))

libc64dtvstubs_a_SOURCES = \
c64dtvcart.c
libc64dtvstubs_a_SOURCES := $(addprefix c64dtv/,$(libc64dtvstubs_a_SOURCES))

libviciidtv_a_SOURCES = \
vicii-badline.c \
vicii-cmdline-options.c \
viciidtv-color.c \
viciidtv-draw.c \
vicii-fetch.c \
vicii-irq.c \
vicii-mem.c \
vicii-phi1.c \
vicii-resources.c \
viciidtv-snapshot.c \
vicii-sprites.c \
vicii-timing.c \
vicii.c
libviciidtv_a_SOURCES := $(addprefix vicii/,$(libviciidtv_a_SOURCES))

libc128_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/c128/*)))

libc64c128_a_SOURCES = \
c64bus.c \
c64cia2.c \
c64datasette.c \
c64export.c \
c64gluelogic.c \
c64iec.c \
c64io.c \
c64keyboard.c \
c64meminit.c \
c64memrom.c \
c64printer.c \
c64pla.c \
c64parallel.c \
c64rsuser.c \
c64sound.c \
patchrom.c
libc64c128_a_SOURCES := $(addprefix c64/,$(libc64c128_a_SOURCES))

libvdc_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/vdc/*)))

libvic20_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/vic20/*)))

libvic20cart_a_SOURCES = \
behrbonz.c \
debugcart.c \
finalexpansion.c \
ioramcart.c \
megacart.c \
ultimem.c \
vic-fp.c \
vic20-generic.c \
vic20-ieee488.c \
vic20-midi.c \
vic20-sidcart.c \
vic20cart.c \
vic20cartmem.c
libvic20cart_a_SOURCES := $(addprefix vic20/cart/,$(libvic20cart_a_SOURCES))

libmascuerade_a_SOURCES = \
mascuerade-stubs.c
libmascuerade_a_SOURCES := $(addprefix vic20/cart/,$(libmascuerade_a_SOURCES))

libpet_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/pet/*)))

libplus4_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/plus4/*)))

libcbm2_a_SOURCES = \
cbm2-cmdline-options.c \
cbm2-resources.c \
cbm2-snapshot.c \
cbm2.c \
cbm2acia1.c \
cbm2bus.c \
cbm2cart.c \
cbm2cia1.c \
cbm2cpu.c \
cbm2datasette.c \
cbm2drive.c \
cbm2embedded.c \
cbm2iec.c \
cbm2io.c \
cbm2mem.c \
cbm2memsnapshot.c \
cbm2model.c \
cbm2printer.c \
cbm2rom.c \
cbm2romset.c \
cbm2sound.c \
cbm2tpi1.c \
cbm2tpi2.c \
cbm2video.c \
debugcart.c
libcbm2_a_SOURCES := $(addprefix cbm2/,$(libcbm2_a_SOURCES))

libcbm5x0_a_SOURCES = \
cbm2-cmdline-options.c \
cbm5x0-resources.c \
cbm5x0-snapshot.c \
cbm5x0.c \
cbm5x0embedded.c \
cbm2acia1.c \
cbm2bus.c \
cbm2cart.c \
cbm2cia1.c \
cbm2cpu.c \
cbm2datasette.c \
cbm2drive.c \
cbm2iec.c \
cbm2io.c \
cbm5x0mem.c \
cbm2memsnapshot.c \
cbm2model.c \
cbm2printer.c \
cbm5x0rom.c \
cbm2romset.c \
cbm2sound.c \
cbm2tpi1.c \
cbm2tpi2.c \
cbm5x0video.c \
debugcart.c
libcbm5x0_a_SOURCES := $(addprefix cbm2/,$(libcbm5x0_a_SOURCES))

libdrive_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/*)))

libdriveiec_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/iec/*)))

libdriveiecc64exp_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/iec/c64exp/*)))

libdriveiecplus4exp_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/iec/plus4exp/*)))

libdriveiec128dcr_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/iec128dcr/*)))

libdriveiecieee_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/iecieee/*)))

libdriveieee_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/ieee/*)))

libdrivetcbm_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/drive/tcbm/*)))

libp64_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/lib/p64/*)))

libimagecontents_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/imagecontents/*)))

libmonitor_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/monitor/*)))

libparallel_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/parallel/*)))

libvdrive_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/vdrive/*)))

libsid_a_SOURCES = \
fastsid.c \
sid-cmdline-options.c \
sid-resources.c \
sid-snapshot.c \
sid.c
libsid_a_SOURCES := $(addprefix sid/,$(libsid_a_SOURCES))

libresid_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.cc, $(wildcard $(viceSrcPath)/resid/*))) sid/resid.cc

libresiddtv_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.cc, $(wildcard $(viceSrcPath)/resid-dtv/*))) sid/resid-dtv.cc

librtc_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/rtc/*)))

libtape_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/tape/*)))

libcore_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/core/*)))

libuserport_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/userport/*)))

libraster_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/raster/*)))

libvideo_a_SOURCES = \
render1x1.c \
render1x2.c \
render1x1crt.c \
render1x2crt.c \
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
libvideo_a_SOURCES := $(addprefix video/,$(libvideo_a_SOURCES))

libfsdevice_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/fsdevice/*)))

libprinterdrv_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/printerdrv/*)))

librs232drv_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/rs232drv/*)))

libdiag_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/diag/*)))

libdiskimage_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/diskimage/*)))

libfileio_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/fileio/*)))

libjoyport_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/joyport/*)))

libtapeport_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/tapeport/*)))

libsamplerdrv_a_SOURCES := $(subst $(viceSrcPath)/,,$(filter %.c, $(wildcard $(viceSrcPath)/samplerdrv/*)))

pluginNoTape_src = \
main/pluginCommon.c \
$(libsounddrv_a_SOURCES) \
$(libprinterdrv_a_SOURCES) \
$(librs232drv_a_SOURCES) \
$(libdiskimage_a_SOURCES) \
$(libfsdevice_a_SOURCES) \
$(libfileio_a_SOURCES) \
$(libserial_a_SOURCES) \
$(libcore_a_SOURCES) \
$(libjoyport_a_SOURCES) \
$(libsamplerdrv_a_SOURCES) \
$(libdrivetcbm_a_SOURCES) \
$(libdiag_a_SOURCES) \
$(base_sources)

plugin_src = \
$(pluginNoTape_src) \
$(libtape_a_SOURCES)

c64_src = \
$(plugin_src) \
$(libc64_a_SOURCES) \
$(libc64cartsystem_a_SOURCES) \
$(libc64cart_a_SOURCES) \
$(libc64commoncart_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecc64exp_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libvicii_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libresid_a_SOURCES) \
$(libtapeport_a_SOURCES)

c64sc_src = \
$(plugin_src) \
$(libc64sc_a_SOURCES) \
$(libc64cartsystem_a_SOURCES) \
$(libc64cart_a_SOURCES) \
$(libc64commoncart_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecc64exp_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libviciisc_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES)

scpu64_src = \
$(pluginNoTape_src) \
$(libscpu64_a_SOURCES) \
$(libc64cartsystem_a_SOURCES) \
$(libc64cart_a_SOURCES) \
$(libc64commoncart_a_SOURCES) \
$(libc64scpu64_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecc64exp_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libviciisc_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libresid_a_SOURCES)

c64dtv_src = \
$(plugin_src) \
$(libc64dtv_a_SOURCES) \
$(libc64c64dtv_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecc64exp_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libviciitv_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libresiddtv_a_SOURCES) \
$(libc64dtvstubs_a_SOURCES) \
ps2mouse.c

c128_src = \
$(plugin_src) \
$(libc128_a_SOURCES) \
$(libc64cartsystem_a_SOURCES) \
$(libc128cart_a_SOURCES) \
$(libc64commoncart_a_SOURCES) \
$(libc64c128_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiec128dcr_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecc64exp_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libvicii_a_SOURCES) \
$(libvdc_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES)

vic_src = \
$(plugin_src) \
$(libvic20_a_SOURCES) \
$(libvic20cart_a_SOURCES) \
$(libc64commoncart_a_SOURCES) \
$(libmascuerade_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES)

pet_src = \
$(plugin_src) \
$(libpet_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libcrtc_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES) \
main/iecbusStubs.c

plus4_src = \
$(plugin_src) \
$(libplus4_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdriveiecplus4exp_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libiecbus_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES)

cbm2_src = \
$(plugin_src) \
$(libcbm2_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libcrtc_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES) \
main/iecbusStubs.c

cbm5x0_src = \
$(plugin_src) \
$(libcbm5x0_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdrive_a_SOURCES) \
$(libp64_a_SOURCES) \
$(libparallel_a_SOURCES) \
$(libvdrive_a_SOURCES) \
$(libmonitor_a_SOURCES) \
$(libvicii_a_SOURCES) \
$(libraster_a_SOURCES) \
$(libuserport_a_SOURCES) \
$(librtc_a_SOURCES) \
$(libvideo_a_SOURCES) \
$(libsid_a_SOURCES) \
$(libimagecontents_a_SOURCES) \
$(libtapeport_a_SOURCES) \
$(libresid_a_SOURCES) \
main/iecbusStubs.c

pluginLDFlags = $(CFLAGS_TARGET) $(LDFLAGS_SYSTEM) $(LDLIBS_SYSTEM) -lz

ifeq ($(ENV),android)
 # must link to the app's main shared object so Android resolves runtime symbols correctly
 pluginLDFlags += -llog $(targetDir)/$(targetFile)
endif

# needed to prevent miscompile when building with -Ofast
%/resid/dac.o : CFLAGS_OPTIMIZE += -fno-finite-math-only

c64_obj := $(addprefix $(objDir)/,$(c64_src:.c=.o))
c64_obj := $(c64_obj:.cc=.o)
c64_module := $(targetDir)/libc64$(loadableModuleExt)

$(c64_module) : $(c64_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(c64_obj) $(pluginLDFlags)

c64sc_obj := $(addprefix $(objDir)/,$(c64sc_src:.c=.o))
c64sc_obj := $(c64sc_obj:.cc=.o)
c64sc_module := $(targetDir)/libc64sc$(loadableModuleExt)

$(c64sc_module) : $(c64sc_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(c64sc_obj) $(pluginLDFlags)

scpu64_obj := $(addprefix $(objDir)/,$(scpu64_src:.c=.o))
scpu64_obj := $(scpu64_obj:.cc=.o)
scpu64_module := $(targetDir)/libscpu64$(loadableModuleExt)

$(scpu64_module) : $(scpu64_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(scpu64_obj) $(pluginLDFlags)

c64dtv_obj := $(addprefix $(objDir)/,$(c64dtv_src:.c=.o))
c64dtv_obj := $(c64dtv_obj:.cc=.o)
c64dtv_module := $(targetDir)/libc64dtv$(loadableModuleExt)

$(c64dtv_module) : $(c64dtv_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(c64dtv_obj) $(pluginLDFlags)

c128_obj := $(addprefix $(objDir)/,$(c128_src:.c=.o))
c128_obj := $(c128_obj:.cc=.o)
c128_module := $(targetDir)/libc128$(loadableModuleExt)

$(c128_module) : $(c128_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(c128_obj) $(pluginLDFlags)

vic_obj := $(addprefix $(objDir)/,$(vic_src:.c=.o))
vic_obj := $(vic_obj:.cc=.o)
vic_module := $(targetDir)/libvic$(loadableModuleExt)

$(vic_module) : $(vic_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(vic_obj) $(pluginLDFlags)

pet_obj := $(addprefix $(objDir)/,$(pet_src:.c=.o))
pet_obj := $(pet_obj:.cc=.o)
pet_module := $(targetDir)/libpet$(loadableModuleExt)

$(pet_module) : $(pet_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(pet_obj) $(pluginLDFlags)

plus4_obj := $(addprefix $(objDir)/,$(plus4_src:.c=.o))
plus4_obj := $(plus4_obj:.cc=.o)
plus4_module := $(targetDir)/libplus4$(loadableModuleExt)

$(plus4_module) : $(plus4_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(plus4_obj) $(pluginLDFlags)

cbm2_obj := $(addprefix $(objDir)/,$(cbm2_src:.c=.o))
cbm2_obj := $(cbm2_obj:.cc=.o)
cbm2_module := $(targetDir)/libcbm2$(loadableModuleExt)

$(cbm2_module) : $(cbm2_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(cbm2_obj) $(pluginLDFlags) -s

cbm5x0_obj := $(addprefix $(objDir)/,$(cbm5x0_src:.c=.o))
cbm5x0_obj := $(cbm5x0_obj:.cc=.o)
cbm5x0_module := $(targetDir)/libcbm5x0$(loadableModuleExt)

$(cbm5x0_module) : $(cbm5x0_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(CC) -o $@ $(linkLoadableModuleAction) $(cbm5x0_obj) $(pluginLDFlags)

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

$(c64_module) : | $(targetDir)/$(targetFile)
$(c64sc_module) : | $(targetDir)/$(targetFile)
$(scpu64_module)  : | $(targetDir)/$(targetFile)
$(c64dtv_module) : | $(targetDir)/$(targetFile)
$(c128_module) : | $(targetDir)/$(targetFile)
$(vic_module) : | $(targetDir)/$(targetFile)
$(pet_module) : | $(targetDir)/$(targetFile)
$(plus4_module) : | $(targetDir)/$(targetFile)
$(cbm2_module) : | $(targetDir)/$(targetFile)
$(cbm5x0_module) : | $(targetDir)/$(targetFile)

all: $(c64_module)
all: $(c64sc_module)
all: $(scpu64_module)
all: $(c64dtv_module)
all: $(c128_module)
all: $(vic_module)
all: $(pet_module)
all: $(plus4_module)
all: $(cbm2_module)
all: $(cbm5x0_module)

endif
