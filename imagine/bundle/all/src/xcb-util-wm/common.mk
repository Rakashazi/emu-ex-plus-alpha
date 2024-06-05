ifndef CHOST
CHOST := $(shell cc -dumpmachine)
else
buildArg := --build=$(shell cc -dumpmachine)
endif

xcbUtilWmVer := 0.4.2
xcbUtilWmSrcDir := $(tempDir)/xcb-util-wm-$(xcbUtilWmVer)
xcbUtilWmSrcArchive := xcb-util-wm-$(xcbUtilWmVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFiles := $(buildDir)/icccm/.libs/libxcb-icccm.a
installIncludeDir := $(installDir)/include/xcb

all : $(outputLibFiles)

install : $(outputLibFiles)
	@echo "Installing xcb-util-wm to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFiles) $(installDir)/lib/
	cp $(xcbUtilWmSrcDir)/icccm/*.h $(installIncludeDir)/
	cp $(buildDir)/icccm/xcb-icccm.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(xcbUtilWmSrcDir)/configure : | $(xcbUtilWmSrcArchive)
	@echo "Extracting xcb-util-wm..."
	mkdir -p $(tempDir)
	tar -mxJf $| -C $(tempDir)
	cd $(xcbUtilWmSrcDir) && autoreconf -isf

$(outputLibFiles) : $(makeFile)
	@echo "Building xcb-util-wm..."
	$(MAKE) -C $(<D)

$(makeFile) : $(xcbUtilWmSrcDir)/configure
	@echo "Configuring xcb-util-wm..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $(xcbUtilWmSrcDir)/configure --prefix=$(installDir) --disable-build-docs --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

