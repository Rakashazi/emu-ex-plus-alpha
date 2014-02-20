ifndef inc_io_aasset
inc_io_aasset := 1

include $(IMAGINE_PATH)/src/io/build.mk
include $(IMAGINE_PATH)/src/io/mmap/generic/build.mk

configDefs += CONFIG_IO_AASSET

SRC += io/aasset/AAssetIO.cc

endif
