ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libXiVer := 1.7.1
libXiSrcDir := libXi-$(libXiVer)
libXiSrcArchive := libXi-$(libXiVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libXi.a
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libXi to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libXiSrcDir)/include/X11/extensions/*.h $(installIncludeDir)/
	cp $(buildDir)/xi.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libXiSrcDir)/configure : $(libXiSrcArchive)
	@echo "Extracting libXi..."
	tar -mxjf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libXi..."
	$(MAKE) -j4 -C $(<D) SUBDIRS=src

$(makeFile) : $(libXiSrcDir)/configure
	@echo "Configuring libXi..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libXiSrcDir)/configure --disable-shared --disable-specs --disable-docs --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

