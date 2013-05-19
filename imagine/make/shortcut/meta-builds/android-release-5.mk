include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
android_minSDK := 5
include $(buildSysPath)/shortcut/meta-builds/android-release.mk