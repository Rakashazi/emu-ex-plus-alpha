ifndef inc_fs_posix
inc_fs_posix := 1

include $(IMAGINE_PATH)/src/fs/Fs.mk

configDefs += CONFIG_FS_POSIX

SRC +=  fs/FsPosix.cc

endif
