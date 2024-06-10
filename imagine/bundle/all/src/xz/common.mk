ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

xzVer := 5.6.2
xzSrcDir := $(tempDir)/xz-$(xzVer)
xzSrcArchive := xz-$(xzVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/liblzma/.libs/liblzma.a
installIncludeDir := $(installDir)/include

cpuIsBigEndian := no

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing xz to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(xzSrcDir)/src/liblzma/api/lzma.h $(installIncludeDir)/
	cp -r $(xzSrcDir)/src/liblzma/api/lzma $(installIncludeDir)/
	cp $(buildDir)/src/liblzma/liblzma.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(xzSrcDir)/configure : | $(xzSrcArchive)
	@echo "Extracting xz..."
	@mkdir -p $(xzSrcDir)
	tar -mxJf $| -C $(xzSrcDir)/..
	cp ../gnuconfig/config.* $(xzSrcDir)/build-aux/
	autoreconf -vfi $(xzSrcDir)

$(outputLibFile) : $(makeFile)
	@echo "Building xz..."
	ac_cv_c_bigendian=$(cpuIsBigEndian) \
	gl_cv_cc_visibility=no \
	$(MAKE) -C $(<D)

$(makeFile) : $(xzSrcDir)/configure
	@echo "Configuring xz..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && $(toolchainEnvParams) \
	CFLAGS="$(CPPFLAGS) $(CFLAGS)" \
	LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	ac_cv_c_bigendian=$(cpuIsBigEndian) \
	gl_cv_cc_visibility=no \
	$(xzSrcDir)/configure \
	--prefix='$$$${pcfiledir}/../..' \
	--enable-encoders=lzma1,lzma2 \
	--enable-decoders=lzma1,lzma2 \
	--disable-xz \
	--disable-xzdec \
	--disable-lzmadec \
	--disable-lzmainfo \
	--disable-rpath \
	--disable-shared \
	--host=$(CHOST) \
	PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
	PKG_CONFIG=pkg-config \
	$(buildArg)
