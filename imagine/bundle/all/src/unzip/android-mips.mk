installDir := $(IMAGINE_PATH)/bundle/android/mips
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/android-mips-gcc.mk

include common.mk

