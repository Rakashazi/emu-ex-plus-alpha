ifndef inc_gui_guitable1d
inc_gui_guitable1d := 1

include $(IMAGINE_PATH)/src/gfx/system.mk
include $(IMAGINE_PATH)/src/input/system.mk
include $(IMAGINE_PATH)/src/gui/build.mk
include $(imagineSrcDir)/gui/ScrollView1D/build.mk

SRC += gui/GuiTable1D/GuiTable1D.cc gui/GuiTable1D/ScrollableGuiTable1D.cc

endif

