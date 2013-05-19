include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
android_ouyaBuild := 1
android_imagineLibPathARMv7 ?= $(IMAGINE_PATH)/lib/android-ouya-16-armv7-release
android_imagineIncludePathARMv7 ?= $(IMAGINE_PATH)/build/android-ouya-16-armv7-release/gen
include $(buildSysPath)/shortcut/meta-builds/android-release.mk