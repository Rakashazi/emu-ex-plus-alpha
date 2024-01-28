ifndef inc_main
inc_main := 1

ccNoStrictAliasing := 1

include $(IMAGINE_PATH)/make/imagineAppBase.mk

ifeq ($(SUBARCH), armv6)
 # compile app code in ARMv7, link to ARMv6 libs so dynarec works
 android_cpuFlags = -marm -march=armv7-a -mtune=generic -mfloat-abi=softfp -mfpu=vfp
endif

ifneq ($(config_compiler),clang)
 CFLAGS_OPTIMIZE_RELEASE_DEFAULT += -O3 -fno-tree-vectorize
endif

ifeq ($(ENV), linux)
 ifndef RELEASE
  # debug Saturn.emu build doesn't use address sanitizer but libimagine.a needs it
  LDFLAGS += -fsanitize=address
 endif
endif

SRC += main/Main.cc \
main/input.cc \
main/options.cc \
main/EmuMenuViews.cc

CPPFLAGS += -I$(projectPath)/src \
-DHAVE_SYS_TIME_H=1 \
-DHAVE_GETTIMEOFDAY=1 \
-DHAVE_STDINT_H=1 \
-DVERSION=\"0.9.10\" \
-DHAVE_STRCASECMP=1

ifeq ($(ARCH), arm)
 ifneq ($(ENV), ios)
  CPPFLAGS += -DUSE_DYNAREC=1 \
  -DSH2_DYNAREC=1
  SRC += yabause/sh2_dynarec/linkage_arm.s \
  yabause/sh2_dynarec/sh2_dynarec.c
 endif
else ifeq ($(ARCH), x86_64)
 # TODO: x86_64 dynarec has 64/32-bit pointer conversion issues
 #CPPFLAGS += -DCPU_X64=1 -DUSE_DYNAREC=1 -DSH2_DYNAREC=1
 #SRC += yabause/sh2_dynarec/linkage_x64.s yabause/sh2_dynarec/sh2_dynarec.c
else ifeq ($(ARCH), x86)
 CPPFLAGS += -DCPU_X86=1 \
 -DUSE_DYNAREC=1 \
 -DSH2_DYNAREC=1
 SRC += yabause/sh2_dynarec/linkage_x86.s \
 yabause/sh2_dynarec/sh2_dynarec.c
endif

SRC += yabause/bios.c \
yabause/cdbase.c \
yabause/cheat.c \
yabause/coffelf.c \
yabause/cs0.c \
yabause/cs1.c \
yabause/cs2.c \
yabause/debug.c \
yabause/error.c \
yabause/memory.c \
yabause/m68kcore.c \
yabause/m68kd.c \
yabause/movie.c \
yabause/netlink.c \
yabause/peripheral.c \
yabause/profile.c \
yabause/scu.c \
yabause/sh2core.c \
yabause/sh2d.c \
yabause/sh2idle.c \
yabause/sh2int.c \
yabause/sh2trace.c \
yabause/smpc.c \
yabause/snddummy.c \
yabause/titan/titan.c \
yabause/vdp1.c \
yabause/vdp2.c \
yabause/vdp2debug.c \
yabause/vidshared.c \
yabause/vidsoft.c \
yabause/yabause.c \
yabause/scsp.c \
yabause/japmodem.c

#SRC += yabause/c68k/c68kexec.c yabause/c68k/c68k.c yabause/m68kc68k.c
#CPPFLAGS += -DHAVE_C68K=1
SRC += yabause/q68/q68.c \
yabause/q68/q68-core.c \
yabause/m68kq68.c
CPPFLAGS += -DHAVE_Q68=1
# TODO: -DQ68_USE_JIT=1

include $(EMUFRAMEWORK_PATH)/package/emuframework.mk

include $(IMAGINE_PATH)/make/imagineAppTarget.mk

endif
