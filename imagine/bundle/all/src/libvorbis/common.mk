ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

libvorbisVer := 1.3.7
libvorbisSrcDir := $(tempDir)/libvorbis-$(libvorbisVer)
libvorbisSrcArchive := libvorbis-$(libvorbisVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/lib/.libs/libvorbis.a $(buildDir)/lib/.libs/libvorbisfile.a
installIncludeDir := $(installDir)/include/vorbis

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libvorbis to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libvorbisSrcDir)/include/vorbis/*.h $(installIncludeDir)/
	cp $(buildDir)/vorbis.pc $(buildDir)/vorbisfile.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libvorbisSrcDir)/configure : | $(libvorbisSrcArchive)
	@echo "Extracting libvorbis..."
	@mkdir -p $(libvorbisSrcDir)
	tar -mxJf $| -C $(libvorbisSrcDir)/..
	cp ../gnuconfig/config.* $(libvorbisSrcDir)/
	patch -d $(libvorbisSrcDir) -p1 < remove-mno-ieee-fp-for-clang.patch
	autoreconf -vfi $(libvorbisSrcDir)

$(outputLibFile) : $(makeFile)
	@echo "Building libvorbis..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libvorbisSrcDir)/configure
	@echo "Configuring libvorbis..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && $(toolchainEnvParams) CFLAGS="$(CPPFLAGS) $(CFLAGS)" \
	LDFLAGS="$(LDFLAGS) $(LDLIBS)" $(libvorbisSrcDir)/configure \
	--prefix='$${pcfiledir}/../..' --disable-docs --disable-examples --disable-oggtest \
	--disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)
