ifeq ($(ENV), linux)
 include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/logger/stdio/build.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/logger/stdio/build.mk
endif
