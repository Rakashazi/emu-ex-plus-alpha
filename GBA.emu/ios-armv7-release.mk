config_ios_jb := 1
O_RELEASE := 1
# Clang 3.1 has speed issues espceially if using thumb
# Clang 3.2 (trunk 159164) works a little better but has broken LTO
#O_LTO := 1
-include config.mk
include $(IMAGINE_PATH)/make/iOS-armv7-gcc.mk
include build.mk
HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)
