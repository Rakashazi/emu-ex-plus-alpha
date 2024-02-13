CPPFLAGS += -I$(genPath) -I$(projectPath)/include
VPATH += $(projectPath)/src

.SUFFIXES: 
.PHONY: all main config
all : main
