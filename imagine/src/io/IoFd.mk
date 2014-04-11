ifndef inc_io_fd
inc_io_fd := 1

include $(IMAGINE_PATH)/src/io/Io.mk

configDefs += CONFIG_IO_FD

SRC += io/IoFd.cc

endif
