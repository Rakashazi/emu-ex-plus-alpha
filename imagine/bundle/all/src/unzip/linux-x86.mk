installDir := $(IMAGINE_PATH)/bundle/linux-x86/usr
objDir := $(installDir)/lib
NO_SRC_DEPS := 1

include $(IMAGINE_PATH)/make/linux-x86-gcc.mk
CPPFLAGS += '-DOF(args)=args'
include common.mk

