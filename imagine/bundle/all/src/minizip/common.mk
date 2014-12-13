ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

# needs GNU version of tar to support --strip-components
# Note: if on MacOSX, install gnutar from MacPorts
osName := $(shell uname -s)
ifeq ($(osName),Darwin)
 TAR ?= gnutar
else
 TAR ?= tar
endif

zlibVer := 1.2.8
zlibSrcArchive := zlib-$(zlibVer).tar.gz

configureFile := $(buildDir)/configure
pcFile := $(buildDir)/minizip.pc
outputLibFile := $(buildDir)/.libs/libminizip.a
installIncludeDir := $(installDir)/include/minizip

CPPFLAGS += -DNOUNCRYPT -DNOCRYPT -DUSE_FILE32API

ifeq ($(ENV), linux)
 # compatibilty with Gentoo system zlib
 CPPFLAGS += -DOF\(args\)=args
endif

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing minizip to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
ifeq ($(ENV), win32)
	$(RANLIB) $(outputLibFile)
endif
	cp $(outputLibFile) $(installDir)/lib/
	cp $(buildDir)/*zip.h $(buildDir)/ioapi.h $(installIncludeDir)/
	cp $(pcFile) $(installDir)/lib/pkgconfig/

.PHONY : all install

$(buildDir)/configure.ac : | $(zlibSrcArchive)
	@echo "Extracting minizip..."
	@mkdir -p $(buildDir)
	$(TAR) -mxzf $| -C $(buildDir) zlib-$(zlibVer)/contrib/minizip --strip-components=3

$(outputLibFile) : $(pcFile)
	@echo "Building minizip..."
	$(MAKE) -C $(<D)

$(configureFile) : $(buildDir)/configure.ac
	@echo "Generating configure for minizip..."
	cd $(buildDir) && autoreconf -isf

$(pcFile) : $(configureFile)
	@echo "Configuring minizip..."
	@mkdir -p $(@D)
	cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include ./configure --prefix='$${pcfiledir}/../..' --disable-shared \
	--host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

