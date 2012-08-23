installDir := $(IMAGINE_PATH)/bundle/webos/armv6
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/webos-armv6-gcc.mk

include common.mk

