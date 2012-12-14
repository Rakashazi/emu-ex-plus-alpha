embedImagine := 1

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
CPPFLAGS += -include $(IMAGINE_PATH)/build/$(buildName)/gen/config.h
LDLIBS += $(IMAGINE_PATH)/lib/$(buildName)/libimagine.a
include $(IMAGINE_PATH)/build/$(buildName)/gen/flags.mk

endif

CPPFLAGS += -I../EmuFramework/include
VPATH += ../EmuFramework/src
SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
Screenshot.cc ButtonConfigView.cc VideoImageOverlay.cc \
StateSlotView.cc MenuView.cc EmuInput.cc TextEntry.cc \
TouchConfigView.cc EmuOptions.cc OptionView.cc EmuView.cc \
ConfigFile.cc

ifneq ($(ENV), ps3)
SRC += VController.cc
endif