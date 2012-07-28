ifeq ($(ENV), linux)
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), android)
 ifeq ($(android_minSDK), 9) # dual back-end support
  include $(imagineSrcDir)/bluetooth/android.mk
 endif
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), iOS)
 include $(imagineSrcDir)/bluetooth/btstack.mk
endif
