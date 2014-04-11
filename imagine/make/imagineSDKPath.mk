# check for the default imagine SDK directory 
imagineSDKPkgConfigPath := $(IMAGINE_SDK_PLATFORM_PATH)/lib/pkgconfig
ifneq (,$(wildcard $(imagineSDKPkgConfigPath)/))
 PKG_CONFIG_PATH := $(imagineSDKPkgConfigPath):$(PKG_CONFIG_PATH)
endif
#$(info $(imagineSDKPkgConfigPath))
#$(info $(PKG_CONFIG_PATH))
ifdef pkgConfigAddImagineSDKIncludePath
 imagineSDKIncludePath := $(IMAGINE_SDK_PLATFORM_PATH)/include
 ifneq (,$(wildcard $(imagineSDKPkgConfigPath)/))
  CPPFLAGS += -I$(imagineSDKIncludePath)
 endif
endif
