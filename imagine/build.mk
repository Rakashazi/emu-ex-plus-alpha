ifndef inc_main
inc_main := 1

include $(IMAGINE_PATH)/make/imagineStaticLibBase.mk

HIGH_OPTIMIZE_CFLAGS := -O3 $(HIGH_OPTIMIZE_CFLAGS_MISC)

ifneq ($(ENV), ps3)
 configDefs += CONFIG_INPUT_ICADE
endif

include $(imagineSrcDir)/audio/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/fs/system.mk
include $(imagineSrcDir)/io/system.mk
include $(imagineSrcDir)/bluetooth/system.mk
include $(imagineSrcDir)/gui/GuiTable1D/build.mk
include $(imagineSrcDir)/gui/MenuItem/build.mk
include $(imagineSrcDir)/gui/FSPicker/build.mk
include $(imagineSrcDir)/gui/AlertView.mk
include $(imagineSrcDir)/resource2/font/system.mk
include $(imagineSrcDir)/data-type/image/png/system.mk

ifeq ($(ENV), android)
 configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
endif

target := libimagine

targetDir := lib/$(buildName)

ifeq ($(ENV), android)
 ifdef RELEASE
  imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine-android-9-release.pc
 else
  imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine-android-9.pc
 endif
else ifeq ($(ENV), ios)
 imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine-ios.pc
else ifeq ($(ENV), linux)
 ifeq ($(SUBENV), pandora)
  imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine-linux-pandora.pc
 else
  imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine-linux.pc
 endif
endif

include $(IMAGINE_PATH)/make/imagineStaticLibTarget.mk

endif
