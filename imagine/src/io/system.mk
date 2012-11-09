ifeq ($(ENV), linux)
	include $(imagineSrcDir)/io/fd/build.mk
	include $(imagineSrcDir)/io/mmap/fd/build.mk
else ifeq ($(ENV), android)
	include $(imagineSrcDir)/io/fd/build.mk
	include $(imagineSrcDir)/io/mmap/fd/build.mk
	include $(imagineSrcDir)/io/zip/build.mk
else ifeq ($(ENV), ios)
	include $(imagineSrcDir)/io/fd/build.mk
	include $(imagineSrcDir)/io/mmap/fd/build.mk
else ifeq ($(ENV), macosx)
	include $(imagineSrcDir)/io/fd/build.mk
	include $(imagineSrcDir)/io/mmap/fd/build.mk
else ifeq ($(ENV), webos)
	include $(imagineSrcDir)/io/fd/build.mk
	include $(imagineSrcDir)/io/mmap/fd/build.mk
else ifeq ($(ENV), ps3)
	include $(imagineSrcDir)/io/fd/build.mk
endif
