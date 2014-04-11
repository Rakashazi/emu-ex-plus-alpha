ifeq ($(ENV), android)
 ifeq ($(android_hasSDK9), 1)
  include $(imagineSrcDir)/resource/font/android.mk
 else
  include $(imagineSrcDir)/resource/font/freetype.mk
 endif
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/resource/font/uikit.mk
else
 include $(imagineSrcDir)/resource/font/freetype.mk
endif
