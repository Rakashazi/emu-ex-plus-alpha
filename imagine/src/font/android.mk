ifndef inc_resource_font_android
inc_resource_font_android := 1

include $(IMAGINE_PATH)/src/font/build.mk

LDLIBS += -ljnigraphics

SRC += font/AndroidFont.cc

endif
