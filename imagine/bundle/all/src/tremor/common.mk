ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

libvorbisSrcDir := Tremor

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libvorbisidec.a
installIncludeDir := $(installDir)/include/tremor

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing tremor to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libvorbisSrcDir)/*ivorbis*.h $(installIncludeDir)/
	cp $(buildDir)/vorbisidec.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(outputLibFile) : $(makeFile)
	@echo "Building tremor..."
	$(MAKE) -C $(<D)
	
$(libvorbisSrcDir)/configure : $(libvorbisSrcDir)/configure.in
	@echo "Generating configure for tremor..."
	cd $(libvorbisSrcDir) && autoreconf -isf

$(makeFile) : $(libvorbisSrcDir)/configure
	@echo "Configuring tremor..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include $$dir/Tremor/configure --disable-oggtest --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(system_externalSysroot)/lib/pkgconfig PKG_CONFIG=pkg-config $(buildArg)

