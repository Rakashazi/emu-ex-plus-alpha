ifndef inc_resource_font_freetype
inc_resource_font_freetype := 1

include $(IMAGINE_PATH)/src/resource2/font/build.mk
include $(IMAGINE_PATH)/src/data-type/font/freetype.mk

configDefs += CONFIG_RESOURCE_FONT_FREETYPE

SRC += resource2/font/ResourceFontFreetype.cc

ifeq ($(ENV), linux)
 include $(IMAGINE_PATH)/make/package/fontconfig.mk
endif

endif
