ifndef CHOST
CHOST := $(shell cc -dumpmachine)
else
buildArg := --build=$(shell cc -dumpmachine)
endif

buildArg += xorg_cv_malloc0_returns_null=yes

libX11Ver := 1.8.7
libX11SrcDir := libX11-$(libX11Ver)
libX11SrcArchive := libX11-$(libX11Ver).tar.xz

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

$(libX11SrcDir)/configure : | $(libX11SrcArchive)
	@echo "Extracting libX11..."
	tar -mxJf $|
	cd $(libX11SrcDir) && autoreconf -isf

$(outputLibFile) : $(makeFile)
	@echo "Building libX11..."
	$(MAKE) -j4 -C $(<D)

$(makeFile) : $(libX11SrcDir)/configure
	@echo "Configuring libX11..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libX11SrcDir)/configure --prefix=$(installDir) --disable-shared --disable-ipv6 --disable-loadable-i18n --disable-lint-library --disable-xf86bigfont --disable-specs --disable-tcp-transport --disable-composecache --disable-loadable-xcursor --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

