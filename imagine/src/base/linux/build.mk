ifndef inc_base
inc_base := 1

include $(imagineSrcDir)/base/Base.mk
include $(imagineSrcDir)/input/build.mk
include $(imagineSrcDir)/util/fdUtils.mk

LDLIBS += -ldl

SRC += base/linux/Application.cc \
 base/linux/ApplicationContext.cc \
 base/linux/FBDevFrameTimer.cc \
 base/common/SimpleFrameTimer.cc \
 base/common/timer/TimerFD.cc \
 base/common/PosixPipe.cc \
 base/common/eventloop/FDCustomEvent.cc

linuxWinSystem ?= x11

ifeq ($(linuxWinSystem), x11)
 include $(imagineSrcDir)/base/x11/build.mk
endif

SRC += base/common/eventloop/GlibEventLoop.cc
include $(IMAGINE_PATH)/make/package/glib.mk

ifeq ($(SUBENV), pandora)
 SRC += base/linux/compat.c
else # Linux Desktop
 SRC += base/linux/dbus.cc base/linux/DRMFrameTimer.cc
 include $(IMAGINE_PATH)/make/package/libdrm.mk
 include $(IMAGINE_PATH)/make/package/gio.mk
endif

endif
