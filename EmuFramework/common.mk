ifneq ($(ENV), android)
 embedImagine := 1
endif

emuFrameworkPath := $(currPath)

ifdef embedImagine

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
include $(imagineSrcDir)/resource2/image/png/build.mk

ifeq ($(ENV), android)
 configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
endif

else

# TODO: rework non-embedded build
configIncNext := <config.h>
imagineLibPath := $(IMAGINE_PATH)/lib/$(buildName)
imagineStaticLib := $(IMAGINE_PATH)/lib/$(buildName)/libimagine.a
CPPFLAGS += -I$(IMAGINE_PATH)/build/$(buildName)/gen
CPPFLAGS += $(shell PKG_CONFIG_PATH=$(imagineLibPath) PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(IMAGINE_PATH)/src pkg-config imagine --cflags --static --define-variable=prefix=$(system_externalSysroot))
LDLIBS += -L$(imagineLibPath) $(shell PKG_CONFIG_PATH=$(imagineLibPath) PKG_CONFIG_SYSTEM_LIBRARY_PATH=$(system_externalSysroot)/lib pkg-config imagine --libs --static --define-variable=prefix=$(system_externalSysroot))

endif

CPPFLAGS += -I$(emuFrameworkPath)/include
VPATH += $(emuFrameworkPath)/src

SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
Screenshot.cc ButtonConfigView.cc VideoImageOverlay.cc \
StateSlotView.cc MenuView.cc EmuInput.cc TextEntry.cc \
TouchConfigView.cc EmuOptions.cc OptionView.cc EmuView.cc \
ConfigFile.cc InputManagerView.cc

ifdef emuFramework_cheats
 SRC += Cheats.cc
endif

ifneq ($(ENV), ps3)
SRC += VController.cc
endif