ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/input/ps3/build.mk
else ifeq ($(ENV), linux)
 include $(imagineSrcDir)/input/evdev/build.mk
else ifeq ($(ENV), ios)
 ifneq ($(SUBARCH),armv6)
  include $(imagineSrcDir)/input/apple/AppleGameDevice.mk
 endif
endif
