ifndef inc_base
inc_base := 1

include $(IMAGINE_PATH)/make/package/dbus.mk
include $(IMAGINE_PATH)/make/package/x11.mk
include $(IMAGINE_PATH)/make/package/xinput.mk
ifeq ($(SUBENV), pandora)
 include $(IMAGINE_PATH)/make/package/xfixes.mk
endif

LDLIBS += -lpthread

configDefs += CONFIG_BASE_X11 CONFIG_INPUT

SRC += base/x11/main.cc base/common/TimerFd.cc util/string/glibc.c

endif
