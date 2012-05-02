ifndef inc_resource_font_freetype
inc_resource_font_freetype := 1

include $(IMAGINE_PATH)/src/resource2/font/build.mk
include $(IMAGINE_PATH)/src/resource2/image/glyph/build.mk
include $(IMAGINE_PATH)/src/data-type/font/freetype-2/build.mk

configDefs += CONFIG_RESOURCE_FONT_FREETYPE

SRC += resource2/font/freetype/ResourceFontFreetype.cc

endif
