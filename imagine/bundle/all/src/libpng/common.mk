ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

libpngVer := 1.6.43
libpngSrcDir := $(tempDir)/libpng-$(libpngVer)
libpngSrcArchive := libpng-$(libpngVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libpng16.a
installIncludeDir := $(installDir)/include/libpng16

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libpng to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libpngSrcDir)/png.h $(installIncludeDir)/
	cp $(libpngSrcDir)/pngconf.h $(installIncludeDir)/
	cp $(buildDir)/pnglibconf.h $(installIncludeDir)/
	cp $(buildDir)/libpng.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libpngSrcDir)/configure : | $(libpngSrcArchive)
	@echo "Extracting libpng..."
	@mkdir -p $(libpngSrcDir)
	tar -mxJf $| -C $(libpngSrcDir)/..

$(outputLibFile) : $(makeFile)
	@echo "Building libpng..."
	$(MAKE) -C $(<D) libpng16.la

$(makeFile) : $(libpngSrcDir)/configure
	@echo "Configuring libpng..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && $(libpngSrcDir)/configure --prefix='$${pcfiledir}/../..' --disable-shared \
	--host=$(CHOST) $(buildArg) "CC=$(CC)" "CPPFLAGS=$(CPPFLAGS)" "CFLAGS=$(CFLAGS)" \
	"LDFLAGS=$(LDFLAGS) $(LDLIBS)" PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config
