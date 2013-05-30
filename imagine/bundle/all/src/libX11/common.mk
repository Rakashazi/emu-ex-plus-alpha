ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libX11Ver := 1.5.0
libX11SrcDir := libX11-$(libX11Ver)
libX11SrcArchive := libX11-$(libX11Ver).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libX11.a
installIncludeDir := $(installDir)/include/X11

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libX11 to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libX11SrcDir)/include/X11/*.h $(buildDir)/include/X11/*.h $(installIncludeDir)/
	cp $(buildDir)/x11.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libX11SrcDir)/configure : $(libX11SrcArchive)
	@echo "Extracting libX11..."
	tar -mxjf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libX11..."
	$(MAKE) -j4 -C $(<D)

$(makeFile) : $(libX11SrcDir)/configure
	@echo "Configuring libX11..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libX11SrcDir)/configure --disable-xthreads --disable-shared --disable-ipv6  --disable-loadable-i18n --disable-lint-library --disable-xf86bigfont --disable-specs --disable-tcp-transport --disable-secure-rpc --disable-composecache --disable-loadable-xcursor --disable-xcms --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

