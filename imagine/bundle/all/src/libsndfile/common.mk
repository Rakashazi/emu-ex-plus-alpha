ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libsndfileVer := 1.0.25
libsndfileSrcDir := libsndfile-$(libsndfileVer)
libsndfileSrcArchive := libsndfile-$(libsndfileVer).tar.gz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libsndfile.a
installIncludeDir := $(installDir)/include

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libsndfile to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(buildDir)/src/sndfile.h $(installIncludeDir)/
	cp $(buildDir)/sndfile.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(libsndfileSrcDir)/configure : $(libsndfileSrcArchive)
	@echo "Extracting libsndfile..."
	tar -mxzf $^
	cp ../gnuconfig/config.* $(libsndfileSrcDir)/Cfg/

$(outputLibFile) : $(makeFile)
	@echo "Building libsndfile..."
	$(MAKE) -C $(<D)/src libsndfile.la

$(makeFile) : $(libsndfileSrcDir)/configure
	@echo "Configuring libsndfile..."
	@mkdir -p $(@D)/src/ $(@D)/tests/
	cp $(libsndfileSrcDir)/src/*.def $(libsndfileSrcDir)/src/*.tpl $(@D)/src/
	cp $(libsndfileSrcDir)/tests/*.def $(libsndfileSrcDir)/tests/*.tpl $(@D)/tests/
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDLIBS)" $$dir/$(libsndfileSrcDir)/configure --disable-sqlite --disable-alsa --disable-external-libs --disable-octave --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

