installDir := $(IMAGINE_PATH)/bundle/macosx-x86/usr
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/macOSX-x86-gcc.mk

include common.mk

