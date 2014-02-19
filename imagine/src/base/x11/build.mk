ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/x11.mk
include $(IMAGINE_PATH)/make/package/xinput.mk
include $(IMAGINE_PATH)/make/package/xfixes.mk
include $(IMAGINE_PATH)/make/package/xrandr.mk

LDLIBS += -lpthread

configDefs += CONFIG_BASE_X11 CONFIG_INPUT
x11GLWinSystem ?= glx

SRC += base/x11/main.cc base/x11/XWindow.cc base/x11/xdnd.cc base/common/timer/TimerFD.cc \
 base/common/PosixPipe.cc util/string/glibc.c

linuxEventLoop ?= epoll

ifeq ($(linuxEventLoop), glib)
 configDefs += CONFIG_BASE_GLIB
 SRC += base/common/eventloop/GlibEventLoop.cc
 include $(IMAGINE_PATH)/make/package/glib.mk
else
 SRC += base/common/eventloop/EPollEventLoop.cc
endif

ifneq ($(SUBENV), pandora)
 SRC += base/x11/dbus.cc
 ifeq ($(linuxEventLoop), glib)
  include $(IMAGINE_PATH)/make/package/dbus-glib.mk
 else
  include $(IMAGINE_PATH)/make/package/dbus.mk
 endif
endif

ifeq ($(x11GLWinSystem), glx)
 SRC += base/x11/GLXContextHelper.cc
else
 include $(IMAGINE_PATH)/make/package/egl.mk
 SRC += base/x11/EGLContextHelper.cc
 configDefs += CONFIG_BASE_X11_EGL
endif

endif
