installDir := $(IMAGINE_PATH)/bundle/darwin-iOS/armv7
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/ios-armv7-gcc.mk

include common.mk

