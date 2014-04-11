ifndef inc_logger_stdio
inc_logger_stdio := 1

SRC += logger/stdio/logger.cc

ifeq ($(ENV), android)
 LDLIBS += -llog
endif

endif
