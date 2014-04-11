ifndef inc_resource_font_uikit
inc_resource_font_uikit := 1

include $(IMAGINE_PATH)/src/resource/font/build.mk

configDefs += CONFIG_RESOURCE_FONT_UIKIT

SRC += resource/font/ResourceFontUIKit.mm

endif
