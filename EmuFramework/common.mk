ifneq ($(filter linux ios android webos,$(ENV)),)
 emuFramework_onScreenControls := 1
endif

emuFrameworkPath := $(lastMakefileDir)

include $(IMAGINE_PATH)/make/package/imagine.mk

CPPFLAGS += -I$(emuFrameworkPath)/include -I$(projectPath)/src
VPATH += $(emuFrameworkPath)/src

SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
Screenshot.cc ButtonConfigView.cc VideoImageOverlay.cc \
StateSlotView.cc MenuView.cc EmuInput.cc TextEntry.cc \
EmuOptions.cc OptionView.cc EmuView.cc MultiChoiceView.cc \
ConfigFile.cc InputManagerView.cc FileUtils.cc EmuApp.cc \
BundledGamesView.cc VideoImageEffect.cc

ifeq ($(emuFramework_cheats), 1)
 SRC += Cheats.cc
endif

ifeq ($(emuFramework_onScreenControls), 1)
 SRC += TouchConfigView.cc VController.cc
endif