ifndef inc_base_x11
inc_base_x11 := 1

include $(imagineSrcDir)/input/build.mk
include $(IMAGINE_PATH)/make/package/x11.mk
include $(IMAGINE_PATH)/make/package/xinput.mk
include $(IMAGINE_PATH)/make/package/xfixes.mk
include $(IMAGINE_PATH)/make/package/xrandr.mk

ifeq ($(SUBENV), pandora)
 pkgConfigDeps += xext xcb xdmcp xau
endif

configDefs += CONFIG_BASE_X11

SRC += base/x11/x11.cc \
 base/x11/XWindow.cc \
 base/x11/XScreen.cc \
 base/x11/xdnd.cc \
 base/x11/input.cc \
 base/x11/FrameTimer.cc

x11GLWinSystem ?= glx

ifeq ($(x11GLWinSystem), glx)
 SRC += base/x11/XGLContextGLX.cc
else
 include $(IMAGINE_PATH)/make/package/egl.mk
 SRC += base/x11/XGLContextEGL.cc base/common/EGLContextBase.cc
 configDefs += CONFIG_BASE_X11_EGL
endif

endif
