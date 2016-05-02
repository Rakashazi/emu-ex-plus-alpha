ifndef inc_gui
inc_gui := 1

include $(imagineSrcDir)/gfx/system.mk
include $(imagineSrcDir)/input/system.mk
include $(imagineSrcDir)/fs/system.mk

SRC += gui/AlertView.cc \
gui/FSPicker.cc \
gui/MenuItem.cc \
gui/TextTableView.cc \
gui/NavView.cc \
gui/ScrollView.cc \
gui/TableView.cc \
gui/TextEntry.cc \
gui/View.cc \
gui/ViewStack.cc

endif

