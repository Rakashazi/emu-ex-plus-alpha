ifneq ($(filter linux android,$(ENV)),)
 include $(imagineSrcDir)/thread/PThread.mk
 include $(imagineSrcDir)/thread/PosixSemaphore.mk
else ifneq ($(filter ios macosx,$(ENV)),)
 include $(imagineSrcDir)/thread/PThread.mk
 include $(imagineSrcDir)/thread/MachSemaphore.mk
else ifeq ($(ENV), win32)
 include $(imagineSrcDir)/thread/Win32Thread.mk
endif
