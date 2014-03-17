ifndef EMUFRAMEWORK_PATH
 EMUFRAMEWORK_PATH := $(lastMakefileDir)/../EmuFramework
endif
metadata_confDeps = $(EMUFRAMEWORK_PATH)/metadata/conf.mk

# TODO: address sanitizer makes emulation very slow and triggers
# fault with call to sh2_recompile_block() -> sh2_set_const()
compiler_noSanitizeAddress := 1