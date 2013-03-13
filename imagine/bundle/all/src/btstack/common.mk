btstackSrcDir := btstack-svn

outputLibFile := $(buildDir)/libBTstack.a
installIncludeDir := $(installDirUniversal)/include/btstack

VPATH += $(btstackSrcDir)/src
C_SRC := btstack.c hci_cmds.c linked_list.c run_loop.c sdp_util.c socket_connection.c utils.c
OBJC_SRC := run_loop_cocoa.m
C_OBJ := $(addprefix $(objDir)/,$(C_SRC:.c=.o))
OBJC_OBJ := $(addprefix $(objDir)/,$(OBJC_SRC:.m=.o))
OBJ := $(C_OBJ) $(OBJC_OBJ)

CPPFLAGS += -I$(btstackSrcDir)/include -I.

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing btstack to $(installDir)..."
	@mkdir -p $(installIncludeDir) $(installDir)/lib
	cp $(outputLibFile) $(installDir)/lib/
	cp $(btstackSrcDir)/include/btstack/* $(installIncludeDir)/
	cp config.h $(installIncludeDir)/

.PHONY : all install

$(outputLibFile) : $(OBJ)
	@echo "Archiving btstack..."
	@mkdir -p `dirname $@`
	ar cr $@ $(OBJ)

