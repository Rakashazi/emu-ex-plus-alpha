installDir := $(IMAGINE_PATH)/bundle/webos/armv7
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/webos-armv7-gcc.mk

include common.mk

