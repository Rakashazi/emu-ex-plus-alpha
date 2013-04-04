ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libvorbisVer := 1.3.3
libvorbisSrcDir := libvorbis-$(libvorbisVer)
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

$(libvorbisSrcDir)/configure : $(libvorbisSrcArchive)
	@echo "Extracting libvorbis..."
	tar -mxJf $^
	cp ../gnuconfig/config.* $(libvorbisSrcDir)/

$(outputLibFile) : $(makeFile)
	@echo "Building libvorbis..."
	$(MAKE) -C $(<D)

$(makeFile) : $(libvorbisSrcDir)/configure
	@echo "Configuring libvorbis..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(libvorbisSrcDir)/configure --disable-docs --disable-examples --disable-oggtest --disable-shared --host=$(CHOST) $(buildArg)

