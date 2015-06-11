ifdef config_audioModule
 ifneq ($(config_audioModule), none)
  include $(imagineSrcDir)/audio/$(config_audioModule)/build.mk
 endif
else
 ifeq ($(ENV), linux)
  ifneq ($(SUBENV), pandora)
   include $(imagineSrcDir)/audio/pulseaudio/build.mk
  else
   include $(imagineSrcDir)/audio/alsa/build.mk
  endif
  include $(imagineSrcDir)/audio/BasicAudioManager.mk
 else ifeq ($(ENV), android)
  include $(imagineSrcDir)/audio/opensl/build.mk
  include $(imagineSrcDir)/audio/AndroidAudioManager.mk
 else ifeq ($(ENV), ios)
  include $(imagineSrcDir)/audio/coreaudio/build.mk
  include $(imagineSrcDir)/audio/IOSAudioManager.mk
 else ifeq ($(ENV), macosx)
  include $(imagineSrcDir)/audio/coreaudio/build.mk
  include $(imagineSrcDir)/audio/BasicAudioManager.mk
 endif
endif