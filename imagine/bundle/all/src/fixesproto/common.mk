ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

fixesprotoVer := 5.0
fixesprotoSrcDir := fixesproto-$(fixesprotoVer)
fixesprotoSrcArchive := fixesproto-$(fixesprotoVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputPCFile := $(buildDir)/fixesproto.pc
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputPCFile)

install : $(outputPCFile)
	@echo "Installing fixesproto to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputPCFile) $(installDir)/lib/pkgconfig/
	cp $(fixesprotoSrcDir)/*.h $(installIncludeDir)/

.PHONY : all install

$(fixesprotoSrcDir)/configure : $(fixesprotoSrcArchive)
	@echo "Extracting fixesproto..."
	tar -mxjf $^

$(outputPCFile) : $(makeFile)
	@echo "Building fixesproto..."
	$(MAKE) -C $(<D)

$(makeFile) : $(fixesprotoSrcDir)/configure
	@echo "Configuring fixesproto..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(fixesprotoSrcDir)/configure --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

