ifeq ($(ENV), linux)
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), android)
 #ifeq ($(android_hasSDK9), 1) # dual back-end support
  include $(imagineSrcDir)/bluetooth/android.mk
 #endif
 #include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), ios)
 ifneq ($(ARCH),x86)
  include $(imagineSrcDir)/bluetooth/btstack.mk
 endif
endif
