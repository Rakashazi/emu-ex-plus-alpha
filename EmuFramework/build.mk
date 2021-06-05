include $(IMAGINE_PATH)/make/imagineStaticLibBase.mk

ifneq ($(filter linux ios android,$(ENV)),)
 emuFramework_onScreenControls := 1
endif

SRC += AudioOptionView.cc \
BundledGamesView.cc \
ButtonConfigView.cc \
Cheats.cc \
ConfigFile.cc \
CreditsView.cc \
EmuApp.cc \
EmuAudio.cc \
EmuInput.cc \
EmuInputView.cc \
EmuLoadProgressView.cc \
EmuMainMenuView.cc \
EmuOptions.cc \
EmuSystemActionsView.cc \
EmuSystem.cc \
EmuSystemTask.cc \
EmuTiming.cc \
EmuVideo.cc \
EmuVideoLayer.cc \
EmuView.cc \
EmuViewController.cc \
FilePicker.cc \
FileUtils.cc \
GUIOptionView.cc \
InputManagerView.cc \
Recent.cc \
RecentGameView.cc \
StateSlotView.cc \
SystemOptionView.cc \
VideoImageEffect.cc \
VideoImageOverlay.cc \
VideoOptionView.cc

ifeq ($(emuFramework_onScreenControls), 1)
 SRC += TouchConfigView.cc \
 VController.cc
endif

libName := emuframework$(libNameExt)
ifndef RELEASE
 libName := $(libName)-debug
endif

target := lib$(libName)

targetDir := lib/$(buildName)

prefix ?= $(IMAGINE_SDK_PLATFORM_PATH)

include $(projectPath)/metadata/conf.mk

imaginePkgconfigTemplate := $(projectPath)/pkgconfig/emuframework.pc
pkgName := EmuFramework
pkgDescription := Emulator App Framework
pkgVersion := $(metadata_version)
LDLIBS := -l$(libName) $(LDLIBS)

CFLAGS_WARN += -Werror=implicit-fallthrough

include $(IMAGINE_PATH)/make/package/imagine.mk
include $(IMAGINE_PATH)/make/package/stdc++.mk

include $(IMAGINE_PATH)/make/imagineStaticLibTarget.mk

install : main
	@echo "Installing lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)cp lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)cp lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)cp -r $(projectPath)/include/emuframework $(prefix)/include/

install-links : main
	@echo "Installing symlink lib & headers to $(prefix)"
	$(PRINT_CMD)mkdir -p $(prefix)/lib/pkgconfig $(prefix)/include/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/lib$(libName).a $(prefix)/lib/
	$(PRINT_CMD)$(LN) -srf lib/$(buildName)/$(libName).pc $(prefix)/lib/pkgconfig/
	$(PRINT_CMD)$(LN) -srf $(projectPath)/include/emuframework $(prefix)/include/
