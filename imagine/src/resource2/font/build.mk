ifndef inc_resource_font
inc_resource_font := 1

include $(IMAGINE_PATH)/src/resource2/build.mk
include $(IMAGINE_PATH)/src/resource2/face/build.mk

configDefs += CONFIG_RESOURCE_FONT

SRC += resource2/font/ResourceFont.cc

endif
