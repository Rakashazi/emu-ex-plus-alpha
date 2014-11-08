ifndef inc_gui_tableview
inc_gui_tableview := 1

include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/gui/ScrollView.mk
include $(imagineSrcDir)/gui/MenuItem.mk

SRC += gui/TableView.cc

endif

