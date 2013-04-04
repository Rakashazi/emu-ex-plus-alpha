ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
endif

CBUILD := $(shell cc -dumpmachine)

freetypeVer := 2.4.11
freetypeSrcDir := freetype-$(freetypeVer)
freetypeSrcArchive := freetype-$(freetypeVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libfreetype.a
installIncludeDir := $(installDir)/include

all : $(outputLibFile)

$(freetypeSrcDir)/configure : $(freetypeSrcArchive)
	@echo "Extracting freetype..."
	tar -mxjf $^
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
	$(MAKE) -C $(<D) CC_BUILD=cc

$(makeFile) : $(freetypeSrcDir)/configure
	@echo "Configuring freetype..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(freetypeSrcDir)/configure --disable-shared --without-old-mac-fonts --without-bzip2 --host=$(CHOST) --build=$(CBUILD)

