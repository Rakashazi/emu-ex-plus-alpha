ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

zlibVer := 1.2.7
zlibSrcDir := zlib-$(zlibVer)/contrib/minizip
zlibSrcArchive := zlib-$(zlibVer).tar.gz

configureFile := $(zlibSrcDir)/configure
pcFile := $(buildDir)/minizip.pc
outputLibFile := $(buildDir)/.libs/libminizip.a
installIncludeDir := $(installDir)/include/minizip

CPPFLAGS += -DNOUNCRYPT -DNOCRYPT -DUSE_FILE32API

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing minizip to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(zlibSrcDir)/*zip.h $(zlibSrcDir)/ioapi.h $(installIncludeDir)/
	cp $(pcFile) $(installDir)/lib/pkgconfig/

.PHONY : all install

$(zlibSrcDir)/configure.ac : $(zlibSrcArchive)
	@echo "Extracting minizip..."
	tar -xzf $^

$(outputLibFile) : $(pcFile)
	@echo "Building minizip..."
	$(MAKE) -C $(<D)

$(configureFile) : $(zlibSrcDir)/configure.ac
	@echo "Generating configure for minizip..."
	cd $(zlibSrcDir) && autoreconf -isf

$(pcFile) : $(configureFile)
	@echo "Configuring minizip..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include $$dir/$(zlibSrcDir)/configure --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG=pkg-config $(buildArg)

