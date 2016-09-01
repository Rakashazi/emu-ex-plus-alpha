ifndef inc_resource_font_uikit
inc_resource_font_uikit := 1

include $(IMAGINE_PATH)/src/font/build.mk

configDefs += CONFIG_RESOURCE_FONT_UIKIT

SRC += font/UIKitFont.mm

endif
