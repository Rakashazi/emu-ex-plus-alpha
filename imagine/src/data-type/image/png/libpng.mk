ifndef inc_data_type_image_libpng
inc_data_type_image_libpng := 1

include $(IMAGINE_PATH)/make/package/libpng.mk

configDefs += CONFIG_DATA_TYPE_IMAGE_LIBPNG

SRC += data-type/image/png/LibPNG.cc

endif
