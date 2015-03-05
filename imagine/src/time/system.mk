ifeq ($(ENV), linux)
 include $(imagineSrcDir)/time/Timespec.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/time/Timespec.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/time/MachTime.mk
else ifeq ($(ENV), macosx)
 include $(imagineSrcDir)/time/MachTime.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/time/Win32Time.mk
else ifeq ($(ENV), webos)
 include $(imagineSrcDir)/time/Timespec.mk
endif
