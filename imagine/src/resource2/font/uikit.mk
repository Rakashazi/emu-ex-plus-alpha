ifndef inc_resource_font_uikit
inc_resource_font_uikit := 1

include $(IMAGINE_PATH)/src/resource2/font/build.mk

configDefs += CONFIG_RESOURCE_FONT_UIKIT

SRC += resource2/font/ResourceFontUIKit.mm

endif
