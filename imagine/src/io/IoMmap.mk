ifndef inc_io_mmap
inc_io_mmap := 1

include $(IMAGINE_PATH)/src/io/Io.mk

configDefs += CONFIG_IO_MMAP

SRC += io/IoMmap.cc

endif
