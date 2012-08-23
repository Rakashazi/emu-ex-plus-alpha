installDir := $(IMAGINE_PATH)/bundle/android/x86
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/android-x86-gcc.mk

include common.mk

