installDir := $(IMAGINE_PATH)/bundle/macosx-x86_64/usr
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/macOSX-x86_64-gcc.mk

include common.mk

