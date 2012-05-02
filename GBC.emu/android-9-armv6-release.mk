O_RELEASE := 1
O_CONCAT := 1
android_ndkApiLevel := 9
HIGH_OPTIMIZE_CFLAGS = -Os $(NORMAL_OPTIMIZE_CFLAGS_MISC) -funsafe-loop-optimizations -Wunsafe-loop-optimizations
include $(IMAGINE_PATH)/make/android-armv6-gcc.mk
include build.mk
