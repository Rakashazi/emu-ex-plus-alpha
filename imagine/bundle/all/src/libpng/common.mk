ifndef pnglibconfFile
pnglibconfFile := pnglibconf.rw-min.h
endif

ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
endif

libpngVer := 1.5.8
libpngSrcDir := libpng-$(libpngVer)

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libpng15.a
installIncludeDir := $(installDir)/include/libpng15

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libpng to $(installDir)..."
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libpngSrcDir)/png.h $(installIncludeDir)/
	cp $(libpngSrcDir)/pngconf.h $(installIncludeDir)/
	cp $(buildDir)/pnglibconf.h $(installIncludeDir)/
	cp $(buildDir)/libpng.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(outputLibFile) : $(makeFile)
	@echo "Building libpng..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libpngSrcDir)/configure
	@echo "Configuring libpng..."
	@mkdir -p $(@D)
	cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDLIBS)" ../../libpng-$(libpngVer)/configure --disable-shared --host=$(CHOST)
	touch $(buildDir)/pnglibconf.dfn
	touch $(buildDir)/pnglibconf.out
	cp $(pnglibconfFile) $(buildDir)/pnglibconf.h
	touch $(buildDir)/pnglibconf.h
