ifndef CHOST
CHOST := $(shell cc -dumpmachine)
else
buildArg := --build=$(shell cc -dumpmachine)
endif

libxkbcommonVer := 1.7.0
libxkbcommonSrcDir := $(tempDir)/libxkbcommon-$(libxkbcommonVer)
libxkbcommonSrcArchive := libxkbcommon-$(libxkbcommonVer).tar.xz

libxkbcommonLib := $(buildDir)/libxkbcommon.a
libxkbcommonX11Lib := $(buildDir)/libxkbcommon-x11.a
outputLibFiles := $(libxkbcommonLib) $(libxkbcommonX11Lib)
installIncludeDir := $(installDir)/include/xkbcommon

# Extract libc++ before setting VPATH
ifeq ($(wildcard $(libxkbcommonSrcDir)/src),)
 $(info Extracting libxkbcommon...)
 $(shell mkdir -p $(tempDir))
 $(shell tar -mxJf $(libxkbcommonSrcArchive) -C $(tempDir))
 $(shell cp config.h $(libxkbcommonSrcDir)/src)
 $(shell cp parser.h $(libxkbcommonSrcDir)/src/xkbcomp)
endif

CFLAGS_WARN += -Wno-error=return-type

VPATH += $(libxkbcommonSrcDir)/src

XKBCOMMON_SRC := compose/parser.c \
 compose/paths.c \
 compose/state.c \
 compose/table.c \
 xkbcomp/action.c \
 xkbcomp/ast-build.c \
 xkbcomp/compat.c \
 xkbcomp/expr.c \
 xkbcomp/include.c \
 xkbcomp/keycodes.c \
 xkbcomp/keymap.c \
 xkbcomp/keymap-dump.c \
 xkbcomp/keywords.c \
 xkbcomp/rules.c \
 xkbcomp/scanner.c \
 xkbcomp/symbols.c \
 xkbcomp/types.c \
 xkbcomp/vmod.c \
 xkbcomp/xkbcomp.c \
 atom.c \
 context.c \
 context-priv.c \
 keysym.c \
 keysym-utf.c \
 keymap.c \
 keymap-priv.c \
 state.c \
 text.c \
 utf8.c \
 utils.c
XKBCOMMON_OBJ := $(addprefix $(objDir)/,$(XKBCOMMON_SRC:.c=.o))

XKBCOMMONX11_SRC := x11/keymap.c \
 x11/state.c \
 x11/util.c \
 context-priv.c \
 keymap-priv.c \
 atom.c
XKBCOMMONX11_OBJ := $(addprefix $(objDir)/,$(XKBCOMMONX11_SRC:.c=.o))

CPPFLAGS += -I$(libxkbcommonSrcDir)/include -I$(libxkbcommonSrcDir)/src

all : $(outputLibFiles)

install : $(outputLibFiles)
	@echo "Installing libxkbcommon to $(installDir)..."
	@mkdir -p $(installIncludeDir) $(installDir)/lib
	cp $(outputLibFiles) $(installDir)/lib/
	cp $(libxkbcommonSrcDir)/include/xkbcommon/* $(installIncludeDir)/
	cp xkbcommon.pc xkbcommon-x11.pc $(installDir)/lib/pkgconfig

.PHONY : all install

$(libxkbcommonLib) : $(XKBCOMMON_OBJ)
	@echo "Archiving libxkbcommon..."
	@mkdir -p `dirname $@`
	$(AR) cr $@ $(XKBCOMMON_OBJ)

$(libxkbcommonX11Lib) : $(XKBCOMMONX11_OBJ)
	@echo "Archiving libxkbcommon-x11..."
	@mkdir -p `dirname $@`
	$(AR) cr $@ $(XKBCOMMONX11_OBJ)
