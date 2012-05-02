ifndef inc_io_mmap_fd
inc_io_mmap_fd := 1

include $(IMAGINE_PATH)/src/io/mmap/build.mk

configDefs += CONFIG_IO_MMAP_FD

SRC += io/mmap/fd/IoMmapFd.cc

endif

