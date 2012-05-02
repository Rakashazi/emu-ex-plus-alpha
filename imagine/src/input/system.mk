ifeq ($(ENV), android)
	include $(imagineSrcDir)/input/android/build.mk
else ifeq ($(ENV), ps3)
	include $(imagineSrcDir)/input/ps3/build.mk
else
	include $(imagineSrcDir)/base/system.mk
endif
