ifeq ($(ENV), android)
 include $(imagineSrcDir)/resource/font/android.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/resource/font/uikit.mk
else
 include $(imagineSrcDir)/resource/font/freetype.mk
endif
