ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libXextVer := 1.3.1
libXextSrcDir := libXext-$(libXextVer)
libXextSrcArchive := libXext-$(libXextVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libXext.a
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libXext to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libXextSrcDir)/include/X11/extensions/*.h $(installIncludeDir)/
	cp $(buildDir)/xext.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libXextSrcDir)/configure : $(libXextSrcArchive)
	@echo "Extracting libXext..."
	tar -mxjf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libXext..."
	$(MAKE) -j4 -C $(<D)

$(makeFile) : $(libXextSrcDir)/configure
	@echo "Configuring libXext..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libXextSrcDir)/configure --disable-shared --disable-specs --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

