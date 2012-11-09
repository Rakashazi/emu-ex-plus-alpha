ifndef inc_resource_font_android
inc_resource_font_android := 1

include $(IMAGINE_PATH)/src/resource2/font/build.mk
include $(IMAGINE_PATH)/src/resource2/image/glyph/build.mk

configDefs += CONFIG_RESOURCE_FONT_ANDROID

LDLIBS += -ljnigraphics

SRC += resource2/font/ResourceFontAndroid.cc

endif
