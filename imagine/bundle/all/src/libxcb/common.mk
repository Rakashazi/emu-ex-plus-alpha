ifndef CHOST
CHOST := $(shell cc -dumpmachine)
else
buildArg := --build=$(shell cc -dumpmachine)
endif

libxcbVer := 1.17.0
libxcbSrcDir := $(tempDir)/libxcb-$(libxcbVer)
libxcbSrcArchive := libxcb-$(libxcbVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFiles := $(buildDir)/src/.libs/libxcb.a $(buildDir)/src/.libs/libxcb-xinput.a $(buildDir)/src/.libs/libxcb-randr.a \
 $(buildDir)/src/.libs/libxcb-xkb.a $(buildDir)/src/.libs/libxcb-xfixes.a $(buildDir)/src/.libs/libxcb-render.a $(buildDir)/src/.libs/libxcb-shape.a
installIncludeDir := $(installDir)/include/xcb

all : $(outputLibFiles)

install : $(outputLibFiles)
	@echo "Installing libxcb to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFiles) $(installDir)/lib/
	cp $(libxcbSrcDir)/src/*.h $(buildDir)/src/*.h $(installIncludeDir)/
	cp $(buildDir)/xcb.pc $(buildDir)/xcb-xinput.pc $(buildDir)/xcb-randr.pc $(buildDir)/xcb-xkb.pc $(buildDir)/xcb-xfixes.pc $(buildDir)/xcb-render.pc $(buildDir)/xcb-shape.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libxcbSrcDir)/configure : | $(libxcbSrcArchive)
	@echo "Extracting libxcb..."
	mkdir -p $(tempDir)
	tar -mxJf $| -C $(tempDir)
	cd $(libxcbSrcDir) && autoreconf -isf

$(outputLibFiles) : $(makeFile)
	@echo "Building libxcb..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libxcbSrcDir)/configure
	@echo "Configuring libxcb..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $(libxcbSrcDir)/configure --prefix=$(installDir) --disable-build-docs --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

