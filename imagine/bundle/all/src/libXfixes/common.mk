ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libXfixesVer := 5.0
libXfixesSrcDir := libXfixes-$(libXfixesVer)
libXfixesSrcArchive := libXfixes-$(libXfixesVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libXfixes.a
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libXfixes to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libXfixesSrcDir)/include/X11/extensions/*.h $(installIncludeDir)/
	cp $(buildDir)/xfixes.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libXfixesSrcDir)/configure : $(libXfixesSrcArchive)
	@echo "Extracting libXfixes..."
	tar -mxjf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libXfixes..."
	$(MAKE) -j4 -C $(<D)

$(makeFile) : $(libXfixesSrcDir)/configure
	@echo "Configuring libXfixes..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libXfixesSrcDir)/configure --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

