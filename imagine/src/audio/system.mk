ifdef config_audioModule

ifneq ($(config_audioModule), none)
 include $(imagineSrcDir)/audio/$(config_audioModule)/build.mk
endif

else

ifeq ($(ENV), linux)
	include $(imagineSrcDir)/audio/alsa/build.mk
else ifeq ($(ENV), android)
	include $(imagineSrcDir)/audio/android/build.mk
else ifeq ($(ENV), iOS)
	include $(imagineSrcDir)/audio/coreaudio/build.mk
else ifeq ($(ENV), macOSX)
	include $(imagineSrcDir)/audio/coreaudio/build.mk
else ifeq ($(ENV), webos)
	include $(imagineSrcDir)/audio/sdl/build.mk
else ifeq ($(ENV), ps3)
	include $(imagineSrcDir)/audio/ps3/build.mk
endif

endif