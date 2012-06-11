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
include $(imagineSrcDir)/resource2/font/freetype/build.mk
include $(imagineSrcDir)/resource2/image/png/build.mk

ifeq ($(ENV), android)
 ifneq ($(ARCH), x86)
  configDefs += SUPPORT_ANDROID_DIRECT_TEXTURE
 endif
endif

else

# TODO: rework non-embedded build
CPPFLAGS += -include $(IMAGINE_PATH)/build/$(buildName)/gen/config.h
LDLIBS += $(IMAGINE_PATH)/lib/$(buildName)/libimagine.a
include $(IMAGINE_PATH)/make/package/libpng.mk
include $(IMAGINE_PATH)/make/package/freetype.mk
include $(IMAGINE_PATH)/make/package/opengl.mk

ifeq ($(ENV), linux)
 include $(IMAGINE_PATH)/make/package/bluez.mk
else ifeq ($(ENV), android)
 include $(IMAGINE_PATH)/make/package/bluez.mk
else ifeq ($(ENV), iOS)
 include $(IMAGINE_PATH)/make/package/btstack.mk
endif

ifeq ($(ENV), linux)
 LDLIBS +=  -lasound
else ifeq ($(ENV), iOS)
 LDLIBS += -framework AudioToolbox -framework CoreAudio
else ifeq ($(ENV), macOSX)
 LDLIBS += -framework AudioToolbox -framework CoreAudio
else ifeq ($(ENV), ps3)
 LDLIBS += -laudio_stub
endif

ifeq ($(ENV), linux)
 LDLIBS += -lXi -lX11 -lrt
else ifeq ($(ENV), android)
 ifeq ($(android_hasSDK9), 1)
  LDLIBS += -lEGL -landroid
 endif
else ifeq ($(ENV), iOS)
 LDLIBS += -framework UIKit -framework QuartzCore -framework Foundation -framework CoreFoundation -framework CoreGraphics -lobjc
else ifeq ($(ENV), macOSX)
 LDLIBS += -framework AppKit
else ifeq ($(ENV), webos)
 include $(IMAGINE_PATH)/make/package/sdl.mk
else ifeq ($(ENV), ps3)
 LDLIBS += -lusbd_stub -lfs_stub -lio_stub -lsysutil_stub -ldbgfont -lresc_stub -lgcm_cmd -lgcm_sys_stub -lsysmodule_stub -lm
endif

endif

CPPFLAGS += -I../EmuFramework/include
VPATH += ../EmuFramework/src
SRC += CreditsView.cc MsgPopup.cc FilePicker.cc EmuSystem.cc Recent.cc \
AlertView.cc Screenshot.cc ButtonConfigView.cc VideoImageOverlay.cc \
StateSlotView.cc MenuView.cc EmuInput.cc TextEntry.cc

ifneq ($(ENV), ps3)
SRC += VController.cc
endif