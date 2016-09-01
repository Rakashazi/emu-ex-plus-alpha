ifeq ($(ENV), android)
 include $(imagineSrcDir)/font/android.mk
else ifeq ($(ENV), ios)
 include $(imagineSrcDir)/font/uikit.mk
else
 include $(imagineSrcDir)/font/freetype.mk
endif
