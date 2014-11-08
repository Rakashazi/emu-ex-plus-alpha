ifndef inc_gui_fspicker
inc_gui_fspicker := 1

include $(imagineSrcDir)/fs/system.mk
include $(imagineSrcDir)/gui/TableView.mk
include $(imagineSrcDir)/gui/NavView.mk

SRC += gui/FSPicker.cc

endif

