ifndef inc_gui_basemenuview
inc_gui_basemenuview := 1

include $(IMAGINE_PATH)/src/gui/GuiTable1D/build.mk
include $(IMAGINE_PATH)/src/gui/MenuItem/build.mk

SRC += gui/BaseMenuView.cc

endif
