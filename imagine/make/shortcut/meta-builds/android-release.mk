include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../config.mk
android_makefileOpts ?= O_RELEASE=1 O_LTO=1 
android_minSDK ?= 9
android_imagineLibPathARM ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-armv6-release
android_imagineIncludePathARM ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-armv6-release/gen
android_imagineLibPathARMv7 ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-armv7-release
android_imagineIncludePathARMv7 ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-armv7-release/gen
android_imagineLibPathX86 ?= $(IMAGINE_PATH)/lib/android-$(android_minSDK)-x86-release
android_imagineIncludePathX86 ?= $(IMAGINE_PATH)/build/android-$(android_minSDK)-x86-release/gen
include $(buildSysPath)/shortcut/meta-builds/android.mk