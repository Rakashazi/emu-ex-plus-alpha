ifndef inc_resource_image_png
inc_resource_image_png := 1

include $(IMAGINE_PATH)/src/data-type/image/libpng/build.mk
include $(IMAGINE_PATH)/src/resource2/image/build.mk

configDefs += CONFIG_RESOURCE_IMAGE_PNG

SRC += resource2/image/png/ResourceImagePng.cc

endif
