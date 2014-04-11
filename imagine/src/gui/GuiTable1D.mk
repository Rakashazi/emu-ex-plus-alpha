ifndef inc_gui_guitable1d
inc_gui_guitable1d := 1

include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gui/View.mk
include $(imagineSrcDir)/gui/ScrollView1D.mk

SRC += gui/GuiTable1D.cc gui/ScrollableGuiTable1D.cc

endif

