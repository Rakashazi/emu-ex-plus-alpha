ifndef inc_logger_ps3
inc_logger_ps3 := 1

CPPFLAGS += -DUSE_LOGGER

LDLIBS += -ldbgfont

SRC += logger/ps3/logger.cc

endif
