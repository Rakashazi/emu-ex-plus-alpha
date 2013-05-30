ifndef pnglibconfFile
pnglibconfFile := pnglibconf.rw-min.h
endif

ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libpngVer := 1.6.2
libpngSrcDir := libpng-$(libpngVer)
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

$(libpngSrcDir)/configure : $(libpngSrcArchive)
	@echo "Extracting libpng..."
	tar -mxJf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libpng..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libpngSrcDir)/configure
	@echo "Configuring libpng..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && $$dir/$(libpngSrcDir)/configure --prefix=$(installDir) --disable-shared --host=$(CHOST) $(buildArg) "CC=$(CC)" "CFLAGS=$(CPPFLAGS) $(CFLAGS)" "LD=$(LD)" "LDFLAGS=$(LDLIBS)"
	touch $(buildDir)/pnglibconf.dfn
	touch $(buildDir)/pnglibconf.out
	dir=`pwd` && cp $$dir/$(pnglibconfFile) $(buildDir)/pnglibconf.h
	touch $(buildDir)/pnglibconf.h
