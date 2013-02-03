ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/dbus.mk

LDLIBS += -lXi -lX11 -lrt -lpthread

configDefs += CONFIG_BASE_X11 CONFIG_INPUT

SRC += base/x11/main.cc base/common/TimerFd.cc

endif
