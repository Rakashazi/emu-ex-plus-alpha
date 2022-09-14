ifndef inc_io_mmap
inc_io_mmap := 1

include $(IMAGINE_PATH)/src/io/IO.mk
include $(imagineSrcDir)/vmem/system.mk

SRC += io/MapIO.cc

endif
