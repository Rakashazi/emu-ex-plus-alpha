ifndef inc_gui_fspicker
inc_gui_fspicker := 1

include $(IMAGINE_PATH)/src/fs/system.mk
include $(IMAGINE_PATH)/src/gui/build.mk
include $(imagineSrcDir)/gui/MenuItem/build.mk
include $(imagineSrcDir)/gui/NavView.mk

SRC += gui/FSPicker/FSPicker.cc

endif

