ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

zlibVer := 1.2.8
zlibSrcArchive := zlib-$(zlibVer).tar.gz

configureFile := $(buildDir)/configure
pcFile := $(buildDir)/zlib.pc
outputLibFile := $(buildDir)/libz.a
installIncludeDir := $(installDir)/include

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing zlib to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp $(buildDir)/zconf.h $(buildDir)/zlib.h $(installIncludeDir)/
	cp $(pcFile) $(installDir)/lib/pkgconfig/

.PHONY : all install

$(configureFile) : $(zlibSrcArchive)
	@echo "Extracting zlib to: $(buildDir)"
	@mkdir -p $(buildDir)
	tar -mxzf $^ -C $(buildDir) --strip-components=1

$(outputLibFile) : $(pcFile)
	@echo "Building zlib..."
	$(MAKE) -C $(<D) libz.a

$(pcFile) : $(configureFile)
	@echo "Configuring zlib..."
	cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" ./configure --static

