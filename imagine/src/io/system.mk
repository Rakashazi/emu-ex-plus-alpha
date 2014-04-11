ifeq ($(ENV), linux)
 include $(imagineSrcDir)/io/IoFd.mk
 include $(imagineSrcDir)/io/IoMmapFd.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/io/IoFd.mk
 include $(imagineSrcDir)/io/IoMmapFd.mk
 include $(imagineSrcDir)/io/AAssetIO.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/io/IoFd.mk
 include $(imagineSrcDir)/io/IoMmapFd.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/io/IoFd.mk
 include $(imagineSrcDir)/io/IoMmapFd.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/io/IoWin32.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/io/IoFd.mk
 include $(imagineSrcDir)/io/IoMmapFd.mk
else ifeq ($(ENV), ps3)
 include $(imagineSrcDir)/io/IoFd.mk
endif
