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
include $(imagineSrcDir)/io/zip/build.mk
include $(imagineSrcDir)/io/mmap/generic/build.mk
include $(imagineSrcDir)/bluetooth/system.mk
include $(imagineSrcDir)/gui/GuiTable1D/build.mk
include $(imagineSrcDir)/gui/MenuItem/build.mk
include $(imagineSrcDir)/gui/FSPicker/build.mk
include $(imagineSrcDir)/gui/AlertView.mk
include $(imagineSrcDir)/gui/ViewStack.mk
include $(imagineSrcDir)/gui/BaseMenuView.mk
include $(imagineSrcDir)/resource2/font/system.mk
include $(imagineSrcDir)/data-type/image/png/system.mk
include $(imagineSrcDir)/util/system/pagesize.mk

ifeq ($(ENV), android)
 configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
endif

target := libimagine

targetDir := lib/$(buildName)

imaginePkgconfigTemplate := $(IMAGINE_PATH)/pkgconfig/imagine.pc
pkgPrefix := $(IMAGINE_PATH)
pkgBuild := $(buildName)
pkgName := imagine
pkgDescription := Game/Multimedia Engine
pkgVersion := 1.5.16-beta2
LDLIBS := -limagine $(LDLIBS)

include $(IMAGINE_PATH)/make/imagineStaticLibTarget.mk

endif
