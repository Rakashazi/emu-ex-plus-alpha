ifndef EMUFRAMEWORK_PATH
 EMUFRAMEWORK_PATH := $(lastMakefileDir)/../EmuFramework
endif
metadata_confDeps = $(EMUFRAMEWORK_PATH)/metadata/conf.mk

# needed in debug build to avoid linker error "atom not found in symbolIndex(___assert_rtn)"
#iosNoDeadStripArmv6 := 1