ifndef EMUFRAMEWORK_PATH
 EMUFRAMEWORK_PATH := $(lastMakefileDir)/../EmuFramework
endif
metadata_confDeps = $(EMUFRAMEWORK_PATH)/metadata/conf.mk

# ld complains about ___floatdisf if using -dead_strip with LTO, cause unknown but could be linker bug
# seems fixed in Xcode 5.1
#ios_noDeadStrip := 1