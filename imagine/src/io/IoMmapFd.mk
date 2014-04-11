ifndef inc_io_mmap_fd
inc_io_mmap_fd := 1

include $(IMAGINE_PATH)/src/util/system/pagesize.mk
include $(IMAGINE_PATH)/src/io/IoMmap.mk

configDefs += CONFIG_IO_MMAP_FD

SRC += io/IoMmapFd.cc

endif

