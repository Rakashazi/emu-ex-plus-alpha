ifndef inc_data_type_font_freetype
inc_data_type_font_freetype := 1

include $(imagineSrcDir)/io/system.mk

include $(IMAGINE_PATH)/make/package/freetype.mk

configDefs += CONFIG_DATA_TYPE_FONT_FREETYPE

SRC += data-type/font/FreetypeFontData.cc

endif