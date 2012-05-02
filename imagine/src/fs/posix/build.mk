ifndef inc_fs_posix
inc_fs_posix := 1

include $(IMAGINE_PATH)/src/fs/build.mk
include $(imagineSrcDir)/gui/GuiTable1D/build.mk

configDefs += CONFIG_FS_POSIX

SRC +=  fs/posix/FsPosix.cc

endif
