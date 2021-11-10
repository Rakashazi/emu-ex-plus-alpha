ifeq ($(ENV), linux)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/fs/PosixFS.mk
 include $(imagineSrcDir)/fs/AAssetFS.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/fs/PosixFS.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/fs/Win32FS.mk
endif
