ifeq ($(ENV), linux)
 include $(imagineSrcDir)/base/x11/build.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/base/android/build.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/base/iphone/build.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/base/osx/build.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/base/win32/build.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/base/sdl/build.mk
else ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/base/ps3/build.mk
endif

SRC += base/common/Window.cc

