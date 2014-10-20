ifeq ($(ENV), linux)
 include $(imagineSrcDir)/io/PosixIO.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/io/PosixIO.mk
 include $(imagineSrcDir)/io/AAssetIO.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/io/PosixIO.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/io/PosixIO.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/io/Win32IO.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/io/PosixIO.mk
else ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/io/PosixIO.mk
endif
