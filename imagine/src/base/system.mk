ifeq ($(ENV), linux)
 include $(imagineSrcDir)/base/linux/build.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/base/android/build.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/base/iphone/build.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/base/osx/build.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/base/win32/build.mk
endif
