ifndef inc_io_archive
inc_io_archive := 1

include $(IMAGINE_PATH)/src/io/IO.mk

include $(IMAGINE_PATH)/make/package/libarchive.mk

configDefs += CONFIG_IO_ARCHIVE

SRC += io/ArchiveIO.cc

endif
