include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
android_minSDK := 9
include $(buildSysPath)/shortcut/meta-builds/android.mk