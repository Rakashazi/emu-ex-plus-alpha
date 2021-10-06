ifndef inc_fs_posix
inc_fs_posix := 1

include $(IMAGINE_PATH)/src/fs/FS.mk

SRC += fs/PosixFS.cc

endif
