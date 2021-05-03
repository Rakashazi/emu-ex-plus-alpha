ifeq ($(ENV), linux)
 ifneq ($(SUBENV), pandora)
  include $(imagineSrcDir)/audio/pulseaudio/build.mk
  include $(imagineSrcDir)/audio/alsa/build.mk
 else
  include $(imagineSrcDir)/audio/alsa/build.mk
 endif
 include $(imagineSrcDir)/audio/BasicManager.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/audio/opensl/build.mk
 include $(imagineSrcDir)/audio/android/build.mk
 include $(imagineSrcDir)/audio/AndroidManager.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/audio/coreaudio/build.mk
 include $(imagineSrcDir)/audio/AvfManager.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/audio/coreaudio/build.mk
 include $(imagineSrcDir)/audio/BasicManager.mk
endif

include $(imagineSrcDir)/audio/build.mk
