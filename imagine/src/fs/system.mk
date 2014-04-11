ifeq ($(ENV), linux)
 include $(imagineSrcDir)/fs/FsPosix.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/fs/FsPosix.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/fs/FsPosix.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/fs/FsPosix.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/fs/FsWin32.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/fs/FsPosix.mk
else ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/fs/ps3/build.mk
endif
