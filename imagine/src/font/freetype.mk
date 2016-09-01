ifndef inc_resource_font_freetype
inc_resource_font_freetype := 1

include $(IMAGINE_PATH)/src/font/build.mk
include $(imagineSrcDir)/io/system.mk

configDefs += CONFIG_RESOURCE_FONT_FREETYPE

SRC += font/FreetypeFont.cc

include $(IMAGINE_PATH)/make/package/freetype.mk

ifeq ($(ENV), linux)
 include $(IMAGINE_PATH)/make/package/fontconfig.mk
endif

endif
