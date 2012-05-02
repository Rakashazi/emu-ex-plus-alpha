ifndef inc_io_mmap_generic
inc_io_mmap_generic := 1

include $(IMAGINE_PATH)/src/io/mmap/build.mk

configDefs += CONFIG_IO_MMAP_GENERIC

SRC += io/mmap/generic/IoMmapGeneric.cc

endif
