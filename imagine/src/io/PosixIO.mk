ifndef inc_io_fd
inc_io_fd := 1

include $(IMAGINE_PATH)/src/io/IO.mk
include $(IMAGINE_PATH)/src/io/MapIO.mk

SRC += io/PosixIO.cc io/PosixFileIO.cc

endif
