ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

libarchiveVer := 3.7.4
libarchiveSrcDir := $(tempDir)/libarchive-$(libarchiveVer)
libarchiveSrcArchive := libarchive-$(libarchiveVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libarchive.a
installIncludeDir := $(installDir)/include

pkgCFlags := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config liblzma --cflags)
pkgLibs := $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) pkg-config liblzma --libs)

ifeq ($(ENV), android)
 CPPFLAGS += -Dset_statfs_transfer_size\(a,b\)=
endif

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libarchive to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libarchiveSrcDir)/libarchive/archive.h $(libarchiveSrcDir)/libarchive/archive_entry.h $(installIncludeDir)/
ifeq ($(ENV),android)
	echo > $(installIncludeDir)/android_lf.h
endif
	cp $(buildDir)/build/pkgconfig/libarchive.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libarchiveSrcDir)/configure : | $(libarchiveSrcArchive)
	@echo "Extracting libarchive..."
	@mkdir -p $(libarchiveSrcDir)
	tar -mxJf $| -C $(libarchiveSrcDir)/..
	patch -d $(libarchiveSrcDir) -p1 < libarchive-3.2.0-entry-crc32.patch # adds ability to read file CRCs per entry
	patch -d $(libarchiveSrcDir) -p1 < libarchive-3.2.1-force-utf8-charset.patch # don't rely on nl_langinfo due to possibly unset locale and Android issues
	cp $(libarchiveSrcDir)/contrib/android/include/android_lf.h $(libarchiveSrcDir)/libarchive/
	cp ../gnuconfig/config.* $(libarchiveSrcDir)/build/autoconf/
	autoreconf -vfi $(libarchiveSrcDir)

$(outputLibFile) : $(makeFile)
	@echo "Building libarchive..."
	$(MAKE) -C $(<D) DEV_CFLAGS=

$(makeFile) : $(libarchiveSrcDir)/configure
	@echo "Configuring libarchive..."
	@mkdir -p $(@D)
	dir=`pwd` && \
	cd $(@D) && \
	$(toolchainEnvParams) \
	CFLAGS="$(CPPFLAGS) $(CFLAGS) $(pkgCFlags)" \
	LDFLAGS="$(LDFLAGS) $(LDLIBS) $(pkgLibs)" \
	$(libarchiveSrcDir)/configure \
	--prefix='$${pcfiledir}/../..' \
	--disable-shared \
	--disable-rpath \
	--disable-bsdtar \
	--disable-bsdcat \
	--disable-bsdcpio \
	--disable-bsdunzip \
	--disable-xattr \
	--disable-acl \
	--without-bz2lib \
	--without-iconv \
	--without-lz4 \
	--without-zstd \
	--without-lzo2 \
	--without-nettle \
	--without-openssl \
	--without-xml2 \
	--without-expat \
	--host=$(CHOST) \
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
	PKG_CONFIG="pkg-config" \
	$(buildArg)
