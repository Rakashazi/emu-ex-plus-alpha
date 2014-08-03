ifeq ($(ENV), linux)
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/bluetooth/android.mk
else ifeq ($(ENV), ios)
 ifneq ($(ARCH),x86)
  include $(imagineSrcDir)/bluetooth/btstack.mk
 endif
endif
