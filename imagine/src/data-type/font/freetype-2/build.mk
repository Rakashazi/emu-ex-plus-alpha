ifndef inc_data_type_font_freetype_2
inc_data_type_font_freetype_2 := 1

include $(imagineSrcDir)/io/system.mk

include $(IMAGINE_PATH)/make/package/freetype.mk

configDefs += CONFIG_DATA_TYPE_FONT_FREETYPE_2

SRC += data-type/font/freetype-2/reader.cc

endif