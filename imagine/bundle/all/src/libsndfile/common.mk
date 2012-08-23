ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

pkgName := libsndfile
libsndfileVer := 1.0.25
libsndfileSrcDir := libsndfile-$(libsndfileVer)

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

$(outputLibFile) : $(makeFile)
	@echo "Building libsndfile..."
	$(MAKE) -C $(<D)/src libsndfile.la

$(makeFile) : $(libsndfileSrcDir)/configure
	@echo "Configuring libsndfile..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDLIBS)" $$dir/$(libsndfileSrcDir)/configure --disable-sqlite --disable-alsa --disable-external-libs --disable-octave --disable-shared --host=$(CHOST) $(buildArg)

