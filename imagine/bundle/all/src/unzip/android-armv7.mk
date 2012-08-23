installDir := $(IMAGINE_PATH)/bundle/android/armv7
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/android-armv7-gcc.mk

include common.mk

