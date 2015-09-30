ifndef inc_fs_archive
inc_fs_archive := 1

include $(IMAGINE_PATH)/src/io/ArchiveIO.mk

configDefs += CONFIG_FS_ARCHIVE

SRC += fs/ArchiveFS.cc

endif
