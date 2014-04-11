ifeq ($(ENV), ios)
 include $(imagineSrcDir)/data-type/image/quartz2d.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/data-type/image/android.mk
else
 include $(imagineSrcDir)/data-type/image/libpng.mk
endif