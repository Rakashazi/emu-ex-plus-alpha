ifndef inc_resource_face
inc_resource_face := 1

include $(IMAGINE_PATH)/src/resource/build.mk

configDefs += CONFIG_RESOURCE_FACE

SRC += resource/face/ResourceFace.cc

endif
