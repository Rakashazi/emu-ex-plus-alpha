ifndef inc_base
inc_base := 1

configDefs += CONFIG_BASE_PS3

ps3CellPPUPath := /home/robert/cell/target/ppu
ps3CellPPULibPath := $(ps3CellPPUPath)/lib
#CPPFLAGS += --sysroot=$(ps3CellPPUPath)
CPPFLAGS += -D'PP_REPEAT4(x)={x, x, x, x}'
#CPPFLAGS += -D'PP_REPEAT4(x)={x, x, x, x}' -fpermissive -I/usr/local/cell/target/common/include -I$(ps3CellPPUPath)/include
#LDLIBS += --sysroot=$(ps3CellPPUPath)
LDLIBS += -lusbd_stub -lfs_stub -lio_stub -lsysutil_stub -ldbgfont -lresc_stub -lgcm_cmd -lgcm_sys_stub -lsysmodule_stub -lm

SRC += base/ps3/main.cc

endif
