ifndef inc_resource_face
inc_resource_face := 1

include $(IMAGINE_PATH)/src/resource2/build.mk

configDefs += CONFIG_RESOURCE_FACE

SRC += resource2/face/ResourceFace.cc

endif
