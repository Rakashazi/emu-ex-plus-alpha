installDir := $(IMAGINE_PATH)/bundle/ps3/usr
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/ps3-gcc.mk

include common.mk

