include $(IMAGINE_PATH)/make/imagineStaticLibBase.mk

SRC += \
AutosaveManager.cc \
ConfigFile.cc \
EmuApp.cc \
EmuAudio.cc \
EmuInput.cc \
EmuOptions.cc \
EmuSystem.cc \
EmuSystemTask.cc \
EmuTiming.cc \
EmuVideo.cc \
EmuVideoLayer.cc \
InputDeviceConfig.cc \
InputDeviceData.cc \
KeyConfig.cc \
OutputTimingManager.cc \
pathUtils.cc \
RecentContent.cc \
RewindManager.cc \
ToggleInput.cc \
TurboInput.cc \
VideoImageEffect.cc \
VideoImageOverlay.cc \
gui/AudioOptionView.cc \
gui/AutosaveSlotView.cc \
gui/BundledGamesView.cc \
gui/ButtonConfigView.cc \
gui/CPUAffinityView.cc \
gui/CreditsView.cc \
gui/EmuInputView.cc \
gui/EmuView.cc \
gui/EmuViewController.cc \
gui/FilePathOptionView.cc \
gui/FilePicker.cc \
gui/FrameTimingView.cc \
gui/GUIOptionView.cc \
gui/InputManagerView.cc \
gui/InputOverridesView.cc \
gui/LoadProgressView.cc \
gui/MainMenuView.cc \
gui/PlaceVControlsView.cc \
gui/PlaceVideoView.cc \
gui/RecentContentView.cc \
gui/StateSlotView.cc \
gui/SystemActionsView.cc \
gui/SystemOptionView.cc \
gui/TouchConfigView.cc \
gui/VideoOptionView.cc \
vcontrols/VController.cc \
vcontrols/VControllerButton.cc \
vcontrols/VControllerButtonGroup.cc \
vcontrols/VControllerDPad.cc \
vcontrols/VControllerKeyboard.cc

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

CFLAGS_WARN += -Werror

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
