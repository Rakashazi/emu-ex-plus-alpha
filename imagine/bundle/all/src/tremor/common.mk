ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

include $(buildSysPath)/imagineSDKPath.mk

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

$(buildDir)/Tremor/configure.in : | $(libvorbisSrcDir)
	@echo "Copying tremor source to: $(buildDir)"
	@mkdir -p $(buildDir)
	cp -r $| $(buildDir)
	patch -d $(buildDir)/Tremor -p1 < tremor-autoconf-1.13-fix.patch
	patch -d $(buildDir)/Tremor -p1 < tremor-remove-old-ogg-test.patch # causes issues on MacOS X

$(outputLibFile) : $(makeFile)
	@echo "Building tremor..."
	$(MAKE) -C $(<D)
	
$(buildDir)/Tremor/configure : $(buildDir)/Tremor/configure.in
	@echo "Generating configure for tremor..."
	cd $(buildDir)/Tremor && autoreconf -isf

$(makeFile) : $(buildDir)/Tremor/configure
	@echo "Configuring tremor..."
	@mkdir -p $(@D)
	cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	PKG_CONFIG_SYSTEM_INCLUDE_PATH=$(system_externalSysroot)/include ./Tremor/configure \
	--prefix='$${pcfiledir}/../..' --disable-shared --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) \
	PKG_CONFIG=pkg-config $(buildArg)

