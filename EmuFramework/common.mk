ifneq ($(filter linux ios android,$(ENV)),)
 staticLibImagine := 1
endif

ifneq ($(filter linux ios android webos,$(ENV)),)
 emuFramework_onScreenControls := 1
endif

emuFrameworkPath := $(currPath)

ifndef staticLibImagine

ifneq ($(ENV), ps3)
configDefs += CONFIG_INPUT_ICADE
endif

include $(imagineSrcDir)/audio/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/fs/system.mk
include $(imagineSrcDir)/io/system.mk
include $(imagineSrcDir)/bluetooth/system.mk
ifeq ($(ENV), android)
include $(imagineSrcDir)/io/zip/build.mk
endif
include $(imagineSrcDir)/gui/GuiTable1D/build.mk
include $(imagineSrcDir)/gui/MenuItem/build.mk
include $(imagineSrcDir)/gui/FSPicker/build.mk
include $(imagineSrcDir)/gui/AlertView.mk
include $(imagineSrcDir)/resource2/font/system.mk
include $(imagineSrcDir)/resource2/data-type/image/libpng/build.mk

ifeq ($(ENV), android)
 configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
endif

else

# TODO: rework non-embedded build
configIncNext := <config.h>
imagineLibPath ?= $(IMAGINE_PATH)/lib/$(buildName)
imagineStaticLib := $(imagineLibPath)/libimagine.a
imagineIncludePath ?= $(IMAGINE_PATH)/build/$(buildName)/gen
CPPFLAGS += -I$(imagineIncludePath)
LDLIBS += -L$(imagineLibPath)
PKG_CONFIG_PATH := $(PKG_CONFIG_PATH):$(imagineLibPath)
PKG_CONFIG_SYSTEM_INCLUDE_PATH := $(PKG_CONFIG_SYSTEM_INCLUDE_PATH):$(IMAGINE_PATH)/src
pkgConfigStaticDeps += imagine

endif

CPPFLAGS += -I$(emuFrameworkPath)/include
VPATH += $(emuFrameworkPath)/src

SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
Screenshot.cc ButtonConfigView.cc VideoImageOverlay.cc \
StateSlotView.cc MenuView.cc EmuInput.cc TextEntry.cc \
EmuOptions.cc OptionView.cc EmuView.cc \
ConfigFile.cc InputManagerView.cc FileUtils.cc

ifeq ($(emuFramework_cheats), 1)
 SRC += Cheats.cc
endif

ifeq ($(emuFramework_onScreenControls), 1)
 SRC += TouchConfigView.cc VController.cc
endif