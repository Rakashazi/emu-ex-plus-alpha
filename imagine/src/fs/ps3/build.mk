ifndef inc_fs_ps3
inc_fs_ps3 := 1

include $(IMAGINE_PATH)/src/fs/build.mk

configDefs += CONFIG_FS_PS3

SRC +=  fs/ps3/FsPs3.cc

endif
