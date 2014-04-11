ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

freetypeVer := 2.4.11
freetypeSrcDir := $(tempDir)/freetype-$(freetypeVer)
freetypeSrcArchive := freetype-$(freetypeVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libfreetype.a
installIncludeDir := $(installDir)/include

all : $(outputLibFile)

$(freetypeSrcDir)/configure : | $(freetypeSrcArchive)
	@echo "Extracting freetype..."
	@mkdir -p $(freetypeSrcDir)
	tar -mxjf $| -C $(freetypeSrcDir)/..
	cd $(freetypeSrcDir) && patch -p1 < ../freetype.patch

install : $(outputLibFile)
	@echo "Installing freetype to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp -r $(freetypeSrcDir)/include/* $(installIncludeDir)/
	cp $(buildDir)/freetype2.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(outputLibFile) : $(makeFile)
	@echo "Building freetype..."
	$(MAKE) -C $(<D)

$(makeFile) : $(freetypeSrcDir)/configure
	@echo "Configuring freetype..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	$(freetypeSrcDir)/configure --prefix='$${pcfiledir}/../..' --disable-shared --without-old-mac-fonts \
	--without-bzip2 --host=$(CHOST)


