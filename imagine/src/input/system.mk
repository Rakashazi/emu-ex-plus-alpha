ifeq ($(ENV), linux)
 include $(imagineSrcDir)/input/evdev/build.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/input/apple/AppleGameDevice.mk
endif
