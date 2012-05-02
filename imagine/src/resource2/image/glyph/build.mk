ifndef inc_resource_image_glyph
inc_resource_image_glyph := 1

include $(IMAGINE_PATH)/src/resource2/image/build.mk

configDefs += CONFIG_RESOURCE_IMAGE_GLYPH

SRC += resource2/image/glyph/ResourceImageGlyph.cc

endif
