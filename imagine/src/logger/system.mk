ifeq ($(ENV), linux)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), android)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), iOS)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), macOSX)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), webos)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), ps3)
	include $(imagineSrcDir)/logger/ps3/build.mk
endif
