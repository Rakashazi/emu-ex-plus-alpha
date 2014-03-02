ifndef EMUFRAMEWORK_PATH
 EMUFRAMEWORK_PATH := $(lastMakefileDir)/../EmuFramework
endif
metadata_confDeps = $(EMUFRAMEWORK_PATH)/metadata/conf.mk

# needed to avoid assertion failure in linker ld64-234.1
iosNoDeadStripArmv6 := 1