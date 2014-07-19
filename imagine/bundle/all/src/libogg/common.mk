ifndef CHOST
 CHOST := $(shell $(CC) -dumpmachine)
endif

liboggVer := 1.3.2
liboggSrcDir := $(tempDir)/libogg-$(liboggVer)
liboggSrcArchive := libogg-$(liboggVer).tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/src/.libs/libogg.a
installIncludeDir := $(installDir)/include/ogg

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libogg to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(liboggSrcDir)/include/ogg/*.h $(installIncludeDir)/
	cp $(buildDir)/include/ogg/*.h $(installIncludeDir)/
	cp $(buildDir)/ogg.pc $(installDir)/lib/pkgconfig/

.PHONY : all install

$(liboggSrcDir)/configure : | $(liboggSrcArchive)
	@echo "Extracting libogg..."
	@mkdir -p $(liboggSrcDir)
	tar -mxJf $| -C $(liboggSrcDir)/..
	cp ../gnuconfig/config.* $(liboggSrcDir)

$(outputLibFile) : $(makeFile)
	@echo "Building libogg..."
	$(MAKE) -C $(<D)/src libogg.la

$(makeFile) : $(liboggSrcDir)/configure
	@echo "Configuring libogg..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS) $(LDLIBS)" \
	$(liboggSrcDir)/configure --prefix='$${pcfiledir}/../..' --disable-shared --host=$(CHOST) $(buildArg)

