ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
endif

CBUILD := $(shell cc -dumpmachine)

pkgName := freetype
freetypeVer := 2.4.10
freetypeSrcDir := freetype-$(freetypeVer)

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/.libs/libfreetype.a
installIncludeDir := $(installDir)/include

all : $(outputLibFile)

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
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDLIBS)" $$dir/$(freetypeSrcDir)/configure --disable-shared --without-old-mac-fonts --without-bzip2 --host=$(CHOST) --build=$(CBUILD)

