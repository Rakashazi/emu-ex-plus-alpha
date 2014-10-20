ifndef inc_io_aasset
inc_io_aasset := 1

include $(IMAGINE_PATH)/src/io/IO.mk
include $(IMAGINE_PATH)/src/io/MapIO.mk

configDefs += CONFIG_IO_AASSET

SRC += io/AAssetIO.cc

endif
