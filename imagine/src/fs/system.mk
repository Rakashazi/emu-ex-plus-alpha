ifeq ($(ENV), linux)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/fs/Win32FS.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/fs/PosixFS.mk
endif
