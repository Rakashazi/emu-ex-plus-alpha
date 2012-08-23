installDir := $(IMAGINE_PATH)/bundle/android/armv6
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/android-armv6-gcc.mk

include common.mk

