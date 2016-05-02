ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

CFLAGS_WARN += -Werror-implicit-function-declaration

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
main/EmuControls.cc \
main/EmuMenuViews.cc \
main/VicePlugin.cc \
main/sysfile.cc \
main/video.cc \
main/sound.cc \
main/log.cc

CPPFLAGS += \
-I$(projectPath)/src \
-I$(projectPath)/src/config \
-I$(projectPath)/src/vice \
-I$(projectPath)/src/vice/c64 \
-I$(projectPath)/src/vice/c64/cart \
-I$(projectPath)/src/vice/c128 \
-I$(projectPath)/src/vice/cbm2 \
-I$(projectPath)/src/vice/pet \
-I$(projectPath)/src/vice/plus4 \
-I$(projectPath)/src/vice/drive \
-I$(projectPath)/src/vice/lib/p64 \
-I$(projectPath)/src/vice/sid \
-I$(projectPath)/src/vice/tape \
-I$(projectPath)/src/vice/userport \
-I$(projectPath)/src/vice/video \
-I$(projectPath)/src/vice/drive/iec/c64exp \
-I$(projectPath)/src/vice/core \
-I$(projectPath)/src/vice/rtc \
-I$(projectPath)/src/vice/vdrive \
-I$(projectPath)/src/vice/imagecontents \
-I$(projectPath)/src/vice/monitor \
-I$(projectPath)/src/vice/platform \
-I$(projectPath)/src/vice/raster \
-I$(projectPath)/src/vice/c64dtv \
-I$(projectPath)/src/vice/vicii \
-I$(projectPath)/src/vice/viciisc \
-I$(projectPath)/src/vice/vdc \
-I$(projectPath)/src/vice/vic20 \
-I$(projectPath)/src/vice/vic20/cart \
-I$(projectPath)/src/vice/crtc \
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
ps2mouse.c \
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
rgcd.c \
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

libcrtc_a_SOURCES = \
crtc-cmdline-options.c \
crtc-color.c \
crtc-draw.c \
crtc-mem.c \
crtc-resources.c \
crtc-snapshot.c \
crtc.c
libcrtc_a_SOURCES := $(addprefix crtc/,$(libcrtc_a_SOURCES))

libiecbus_a_SOURCES = \
iecbus.c
libiecbus_a_SOURCES := $(addprefix iecbus/,$(libiecbus_a_SOURCES))

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
c64cpu.c \
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

libviciisc_a_SOURCES = \
vicii-chip-model.c \
vicii-cmdline-options.c \
vicii-color.c \
vicii-cycle.c \
vicii-draw.c \
vicii-draw-cycle.c \
vicii-fetch.c \
vicii-irq.c \
vicii-lightpen.c \
vicii-mem.c \
vicii-phi1.c \
vicii-resources.c \
vicii-snapshot.c \
vicii-timing.c \
vicii.c
libviciisc_a_SOURCES := $(addprefix viciisc/,$(libviciisc_a_SOURCES))

libscpu64_a_SOURCES = \
scpu64-cmdline-options.c \
scpu64-resources.c \
scpu64-snapshot.c \
scpu64.c \
scpu64cpu.c \
scpu64gluelogic.c \
scpu64mem.c \
scpu64memsnapshot.c \
scpu64meminit.c \
scpu64model.c \
scpu64rom.c \
scpu64stubs.c
libscpu64_a_SOURCES := $(addprefix scpu64/,$(libscpu64_a_SOURCES))

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

libc128_a_SOURCES = \
c128-cmdline-options.c \
c128-resources.c \
c128-snapshot.c \
c128.c \
c128cia1.c \
c128cpu.c \
c128drive.c \
c128embedded.c \
c128fastiec.c \
c128mem.c \
c128meminit.c \
c128memlimit.c \
c128memrom.c \
c128memsnapshot.c \
c128mmu.c \
c128model.c \
c128rom.c \
c128romset.c \
c128video.c \
daa.c \
functionrom.c \
z80.c \
z80mem.c
libc128_a_SOURCES := $(addprefix c128/,$(libc128_a_SOURCES))

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

libvdc_a_SOURCES = \
vdc-cmdline-options.c \
vdc-color.c \
vdc-draw.c \
vdc-mem.c \
vdc-resources.c \
vdc-snapshot.c \
vdc.c
libvdc_a_SOURCES := $(addprefix vdc/,$(libvdc_a_SOURCES))

libvic20_a_SOURCES = \
vic-cmdline-options.c \
vic-color.c \
vic-cycle.c \
vic-draw.c \
vic-mem.c \
vic-resources.c \
vic-snapshot.c \
vic-timing.c \
vic.c \
vic20-cmdline-options.c \
vic20-resources.c \
vic20-snapshot.c \
vic20.c \
vic20bus.c \
vic20cpu.c \
vic20datasette.c \
vic20drive.c \
vic20embedded.c \
vic20iec.c \
vic20ieeevia1.c \
vic20ieeevia2.c \
vic20io.c \
vic20mem.c \
vic20memrom.c \
vic20memsnapshot.c \
vic20model.c \
vic20printer.c \
vic20rom.c \
vic20romset.c \
vic20rsuser.c \
vic20sound.c \
vic20via1.c \
vic20via2.c \
vic20video.c
libvic20_a_SOURCES := $(addprefix vic20/,$(libvic20_a_SOURCES))

libvic20cart_a_SOURCES = \
finalexpansion.c \
megacart.c \
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

libpet_a_SOURCES = \
6809.c \
petcpu.c \
pet-cmdline-options.c \
pet-resources.c \
pet-sidcart.c \
pet-snapshot.c \
pet.c \
petacia1.c \
petbus.c \
petcolour.c \
petdatasette.c \
petdrive.c \
petdww.c \
petembedded.c \
pethre.c \
petiec.c \
petmem.c \
petmemsnapshot.c \
petmodel.c \
petpia1.c \
petpia2.c \
petprinter.c \
petreu.c \
petrom.c \
petromset.c \
petsound.c \
petvia.c \
petvideo.c
libpet_a_SOURCES := $(addprefix pet/,$(libpet_a_SOURCES))

libplus4_a_SOURCES = \
digiblaster.c \
plus4-cmdline-options.c \
plus4-resources.c \
plus4-sidcart.c \
plus4-snapshot.c \
plus4.c \
plus4acia.c \
plus4bus.c \
plus4cart.c \
plus4cpu.c \
plus4datasette.c \
plus4drive.c \
plus4embedded.c \
plus4iec.c \
plus4mem.c \
plus4memcsory256k.c \
plus4memhacks.c \
plus4memhannes256k.c \
plus4memlimit.c \
plus4memrom.c \
plus4memsnapshot.c \
plus4model.c \
plus4parallel.c \
plus4pio1.c \
plus4pio2.c \
plus4printer.c \
plus4rom.c \
plus4romset.c \
plus4speech.c \
plus4tcbm.c \
plus4video.c \
ted-badline.c \
ted-cmdline-options.c \
ted-color.c \
ted-draw.c \
ted-fetch.c \
ted-irq.c \
ted-mem.c \
ted-resources.c \
ted-snapshot.c \
ted-sound.c \
ted-timer.c \
ted-timing.c \
ted.c
libplus4_a_SOURCES := $(addprefix plus4/,$(libplus4_a_SOURCES))

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
cbm2mem.c \
cbm2memsnapshot.c \
cbm2model.c \
cbm2printer.c \
cbm2rom.c \
cbm2romset.c \
cbm2sound.c \
cbm2tpi1.c \
cbm2tpi2.c \
cbm2video.c
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
cbm5x0mem.c \
cbm2memsnapshot.c \
cbm2model.c \
cbm2printer.c \
cbm5x0rom.c \
cbm2romset.c \
cbm2sound.c \
cbm2tpi1.c \
cbm2tpi2.c \
cbm5x0video.c
libcbm5x0_a_SOURCES := $(addprefix cbm2/,$(libcbm5x0_a_SOURCES))

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

libdrivetcbm_a_SOURCES = \
glue1551.c \
mem1551.c \
tcbm-cmdline-options.c \
tcbm-resources.c \
tcbm.c \
tcbmrom.c \
tpid.c
libdrivetcbm_a_SOURCES := $(addprefix drive/tcbm/,$(libdrivetcbm_a_SOURCES))

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
asm65816.c \
asmz80.c \
mon_assemble6502.c \
mon_assembleR65C02.c \
mon_assemble65816.c \
mon_assemble6809.c \
mon_assemblez80.c \
mon_breakpoint.c \
mon_command.c \
mon_disassemble.c \
mon_drive.c \
mon_file.c \
mon_memory.c \
mon_register.c \
mon_register6502.c \
mon_register6502dtv.c \
mon_register6809.c \
mon_registerR65C02.c \
mon_register65816.c \
mon_registerz80.c \
mon_ui.c \
mon_util.c \
monitor.c \
monitor_network.c \
mon_parse.c \
mon_lex.c
libmonitor_a_SOURCES := $(addprefix monitor/,$(libmonitor_a_SOURCES))

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
EXTRA_libsid_a_SOURCES := $(addprefix sid/,$(EXTRA_libsid_a_SOURCES))

libsid_a_SOURCES += $(EXTRA_libsid_a_SOURCES)

libresid_a_SOURCES = \
sid.cc \
voice.cc \
wave.cc \
envelope.cc \
filter.cc \
dac.cc \
extfilt.cc \
pot.cc
libresid_a_SOURCES := $(addprefix resid/,$(libresid_a_SOURCES))

libresiddtv_a_SOURCES = \
sid.cc \
voice.cc \
wave.cc \
envelope.cc \
filter.cc \
extfilt.cc
libresiddtv_a_SOURCES := $(addprefix resid-dtv/,$(libresiddtv_a_SOURCES))

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
render1x2.c \
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
drv-1520.c \
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
maincpu.c

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
$(libc64dtvstubs_a_SOURCES)

c128_src = \
$(plugin_src) \
$(libc128_a_SOURCES) \
$(libc64cartsystem_a_SOURCES) \
$(libc64cart_a_SOURCES) \
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
$(libresid_a_SOURCES)

pet_src = \
$(plugin_src) \
$(libpet_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
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
$(libresid_a_SOURCES) \
main/iecbusStubs.c

plus4_src = \
$(plugin_src) \
$(libplus4_a_SOURCES) \
$(libdriveiec_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveiecplus4exp_a_SOURCES) \
$(libdrivetcbm_a_SOURCES) \
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
$(libresid_a_SOURCES)

cbm2_src = \
$(plugin_src) \
$(libcbm2_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
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
$(libresid_a_SOURCES) \
main/iecbusStubs.c

cbm5x0_src = \
$(plugin_src) \
$(libcbm5x0_a_SOURCES) \
$(libdriveiecieee_a_SOURCES) \
$(libdriveieee_a_SOURCES) \
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
$(libresid_a_SOURCES) \
main/iecbusStubs.c

pluginLDFlags = $(CFLAGS_TARGET) $(LDFLAGS_SYSTEM) $(LDLIBS_SYSTEM) -lz

ifeq ($(ENV),android)
 # must link to the app's main shared object so Android resolves runtime symbols correctly
 pluginLDFlags += -llog $(targetDir)/$(targetFile)
endif

c64_obj := $(addprefix $(objDir)/,$(c64_src:.c=.o))
c64_obj := $(c64_obj:.cc=.o)
c64_module := $(targetDir)/libc64$(loadableModuleExt)

$(c64_module) : $(c64_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(c64_obj) $(pluginLDFlags)

c64sc_obj := $(addprefix $(objDir)/,$(c64sc_src:.c=.o))
c64sc_obj := $(c64sc_obj:.cc=.o)
c64sc_module := $(targetDir)/libc64sc$(loadableModuleExt)

$(c64sc_module) : $(c64sc_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(c64sc_obj) $(pluginLDFlags)

scpu64_obj := $(addprefix $(objDir)/,$(scpu64_src:.c=.o))
scpu64_obj := $(scpu64_obj:.cc=.o)
scpu64_module := $(targetDir)/libscpu64$(loadableModuleExt)

$(scpu64_module) : $(scpu64_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(scpu64_obj) $(pluginLDFlags)

c64dtv_obj := $(addprefix $(objDir)/,$(c64dtv_src:.c=.o))
c64dtv_obj := $(c64dtv_obj:.cc=.o)
c64dtv_module := $(targetDir)/libc64dtv$(loadableModuleExt)

$(c64dtv_module) : $(c64dtv_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(c64dtv_obj) $(pluginLDFlags)

c128_obj := $(addprefix $(objDir)/,$(c128_src:.c=.o))
c128_obj := $(c128_obj:.cc=.o)
c128_module := $(targetDir)/libc128$(loadableModuleExt)

$(c128_module) : $(c128_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(c128_obj) $(pluginLDFlags)

vic_obj := $(addprefix $(objDir)/,$(vic_src:.c=.o))
vic_obj := $(vic_obj:.cc=.o)
vic_module := $(targetDir)/libvic$(loadableModuleExt)

$(vic_module) : $(vic_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(vic_obj) $(pluginLDFlags)

pet_obj := $(addprefix $(objDir)/,$(pet_src:.c=.o))
pet_obj := $(pet_obj:.cc=.o)
pet_module := $(targetDir)/libpet$(loadableModuleExt)

$(pet_module) : $(pet_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(pet_obj) $(pluginLDFlags)

plus4_obj := $(addprefix $(objDir)/,$(plus4_src:.c=.o))
plus4_obj := $(plus4_obj:.cc=.o)
plus4_module := $(targetDir)/libplus4$(loadableModuleExt)

$(plus4_module) : $(plus4_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(plus4_obj) $(pluginLDFlags)

cbm2_obj := $(addprefix $(objDir)/,$(cbm2_src:.c=.o))
cbm2_obj := $(cbm2_obj:.cc=.o)
cbm2_module := $(targetDir)/libcbm2$(loadableModuleExt)

$(cbm2_module) : $(cbm2_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(cbm2_obj) $(pluginLDFlags) -s

cbm5x0_obj := $(addprefix $(objDir)/,$(cbm5x0_src:.c=.o))
cbm5x0_obj := $(cbm5x0_obj:.cc=.o)
cbm5x0_module := $(targetDir)/libcbm5x0$(loadableModuleExt)

$(cbm5x0_module) : $(cbm5x0_obj) $(linkerLibsDep)
	@echo "Linking $@"
	@mkdir -p `dirname $@`
	$(PRINT_CMD) $(LD) -o $@ $(linkLoadableModuleAction) $(cbm5x0_obj) $(pluginLDFlags)

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
