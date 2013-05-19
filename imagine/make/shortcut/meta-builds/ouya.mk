include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
android_ouyaBuild := 1
include $(buildSysPath)/shortcut/meta-builds/android.mk