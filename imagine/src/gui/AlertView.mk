ifndef inc_gui_alertview
inc_gui_alertview := 1

include $(IMAGINE_PATH)/src/gui/GuiTable1D/build.mk
include $(IMAGINE_PATH)/src/gui/MenuItem/build.mk

SRC += gui/AlertView.cc

endif
