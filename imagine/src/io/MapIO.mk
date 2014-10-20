ifndef inc_io_mmap
inc_io_mmap := 1

include $(IMAGINE_PATH)/src/io/IO.mk
include $(IMAGINE_PATH)/src/util/system/pagesize.mk

SRC += io/MapIO.cc io/BufferMapIO.cc

endif
