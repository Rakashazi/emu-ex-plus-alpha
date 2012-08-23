installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv6
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/ios-armv6-gcc.mk

include common.mk

