ifndef inc_resource_font
inc_resource_font := 1

include $(IMAGINE_PATH)/src/resource/build.mk
include $(IMAGINE_PATH)/src/resource/face/build.mk

configDefs += CONFIG_RESOURCE_FONT

SRC += resource/font/ResourceFont.cc

endif
