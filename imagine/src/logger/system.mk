ifeq ($(ENV), linux)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), android)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), ios)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), macosx)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), webos)
	include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), ps3)
	include $(imagineSrcDir)/logger/ps3/build.mk
endif
