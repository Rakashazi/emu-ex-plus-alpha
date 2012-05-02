ifeq ($(ENV), linux)
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), android)
 include $(imagineSrcDir)/bluetooth/bluez.mk
else ifeq ($(ENV), iOS)
 include $(imagineSrcDir)/bluetooth/btstack.mk
endif
