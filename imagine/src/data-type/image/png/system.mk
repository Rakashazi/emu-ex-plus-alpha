ifeq ($(ENV), ios)
	include $(imagineSrcDir)/data-type/image/png/quartz2d.mk
else
	include $(imagineSrcDir)/data-type/image/png/libpng.mk
endif