ifndef inc_resource_image
inc_resource_image := 1

include $(IMAGINE_PATH)/src/resource2/build.mk

configDefs += CONFIG_RESOURCE_IMAGE

SRC += resource2/image/ResourceImage.cc

endif
