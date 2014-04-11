ifndef inc_io_mmap_generic
inc_io_mmap_generic := 1

include $(IMAGINE_PATH)/src/io/IoMmap.mk

configDefs += CONFIG_IO_MMAP_GENERIC

SRC += io/IoMmapGeneric.cc

endif
