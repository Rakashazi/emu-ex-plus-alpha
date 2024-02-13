ifndef inc_base_x11
inc_base_x11 := 1

include $(imagineSrcDir)/input/build.mk
include $(IMAGINE_PATH)/make/package/x11.mk
include $(IMAGINE_PATH)/make/package/xinput.mk
include $(IMAGINE_PATH)/make/package/xfixes.mk

ifeq ($(SUBENV), pandora)
 pkgConfigDeps += xext xcb xdmcp xau
else
 include $(IMAGINE_PATH)/make/package/xrandr.mk
endif

SRC += base/x11/Application.cc \
 base/x11/ApplicationContext.cc \
 base/x11/XWindow.cc \
 base/x11/XScreen.cc \
 base/x11/xdnd.cc \
 base/x11/input.cc \
 base/x11/FrameTimer.cc

include $(IMAGINE_PATH)/make/package/egl.mk
SRC += base/x11/XGLContextEGL.cc \
 base/common/EGLContextBase.cc

endif
