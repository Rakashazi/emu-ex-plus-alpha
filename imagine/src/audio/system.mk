ifdef config_audioModule

ifneq ($(config_audioModule), none)
 include $(imagineSrcDir)/audio/$(config_audioModule)/build.mk
endif

else

ifeq ($(ENV), linux)
 include $(imagineSrcDir)/audio/alsa/build.mk
else ifeq ($(ENV), android)
 ifeq ($(android_hasSDK9), 1)
  include $(imagineSrcDir)/audio/opensl/build.mk
 else
  include $(imagineSrcDir)/audio/android/build.mk
 endif
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/audio/coreaudio/build.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/audio/coreaudio/build.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/audio/sdl/build.mk
else ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/audio/ps3/build.mk
endif

endif