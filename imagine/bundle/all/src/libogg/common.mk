ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

pkgName := libogg
liboggVer := 1.3.0
liboggSrcDir := libogg-$(liboggVer)

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

$(outputLibFile) : $(makeFile)
	@echo "Building libogg..."
	$(MAKE) -C $(<D)/src libogg.la

$(makeFile) : $(liboggSrcDir)/configure
	@echo "Configuring libogg..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDLIBS)" $$dir/$(liboggSrcDir)/configure --disable-shared --host=$(CHOST) $(buildArg)

