ifndef inc_io_zip
inc_io_zip := 1

include $(IMAGINE_PATH)/src/io/IO.mk

include $(IMAGINE_PATH)/make/package/unzip.mk
include $(IMAGINE_PATH)/make/package/zlib.mk

configDefs += CONFIG_IO_ZIP

SRC += io/ZipIO.cc

endif
