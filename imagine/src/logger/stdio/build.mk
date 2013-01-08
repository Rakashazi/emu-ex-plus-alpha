ifndef inc_logger_stdio
inc_logger_stdio := 1

ifneq ($(ENV), android) # embedded build
 CPPFLAGS += -DUSE_LOGGER
else
 configDefs += USE_LOGGER
endif

SRC += logger/stdio/logger.cc

ifeq ($(ENV), android)
 LDLIBS += -llog
endif

endif
