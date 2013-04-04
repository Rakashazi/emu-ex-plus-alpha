ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

zlibVer := 1.2.7
zlibSrcArchive := zlib-$(zlibVer).tar.gz

configureFile := $(buildDir)/configure
pcFile := $(buildDir)/minizip.pc
outputLibFile := $(buildDir)/.libs/libminizip.a
installIncludeDir := $(installDir)/include/minizip

CPPFLAGS += -DNOUNCRYPT -DNOCRYPT -DUSE_FILE32API

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing minizip to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(buildDir)/*zip.h $(buildDir)/ioapi.h $(installIncludeDir)/
	cp $(pcFile) $(installDir)/lib/pkgconfig/

.PHONY : all install

$(buildDir)/configure.ac : $(zlibSrcArchive)
	@echo "Extracting minizip..."
	@mkdir -p $(buildDir)
	tar -mxzf $^ -C $(buildDir) zlib-$(zlibVer)/contrib/minizip --strip-components=3

$(outputLibFile) : $(pcFile)
	@echo "Building minizip..."
	$(MAKE) -C $(<D)

$(configureFile) : $(buildDir)/configure.ac
	@echo "Generating configure for minizip..."
	cd $(buildDir) && autoreconf -isf

$(pcFile) : $(configureFile)
	@echo "Configuring minizip..."
	@mkdir -p $(@D)
	cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include ./configure --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG=pkg-config $(buildArg)

