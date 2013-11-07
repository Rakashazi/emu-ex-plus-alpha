ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/dbus.mk
include $(IMAGINE_PATH)/make/package/x11.mk
include $(IMAGINE_PATH)/make/package/xinput.mk
include $(IMAGINE_PATH)/make/package/xfixes.mk
include $(IMAGINE_PATH)/make/package/xrandr.mk

LDLIBS += -lpthread

configDefs += CONFIG_BASE_X11 CONFIG_INPUT
x11GLWinSystem ?= glx

ifdef config_gfx_openGLES
 x11GLWinSystem := egl
endif

SRC += base/x11/main.cc base/x11/XWindow.cc base/x11/xdnd.cc base/common/TimerFd.cc util/string/glibc.c

ifeq ($(x11GLWinSystem), glx)
 SRC += base/x11/GLXContextHelper.cc
else
 SRC += base/x11/EGLContextHelper.cc
 configDefs += CONFIG_BASE_X11_EGL
endif

endif
