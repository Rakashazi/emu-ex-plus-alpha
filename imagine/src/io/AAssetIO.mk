ifndef inc_io_aasset
inc_io_aasset := 1

include $(IMAGINE_PATH)/src/io/Io.mk
include $(IMAGINE_PATH)/src/io/IoMmapGeneric.mk

configDefs += CONFIG_IO_AASSET

SRC += io/AAssetIO.cc

endif
