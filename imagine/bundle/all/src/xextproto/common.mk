ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

xextprotoVer := 7.2.1
xextprotoSrcDir := xextproto-$(xextprotoVer)
xextprotoSrcArchive := xextproto-$(xextprotoVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputPCFile := $(buildDir)/xextproto.pc
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputPCFile)

install : $(outputPCFile)
	@echo "Installing xextproto to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputPCFile) $(installDir)/lib/pkgconfig/
	cp $(xextprotoSrcDir)/*.h $(installIncludeDir)/

.PHONY : all install

$(xextprotoSrcDir)/configure : $(xextprotoSrcArchive)
	@echo "Extracting xextproto..."
	tar -mxjf $^

$(outputPCFile) : $(makeFile)
	@echo "Building xextproto..."
	$(MAKE) -C $(<D)

$(makeFile) : $(xextprotoSrcDir)/configure
	@echo "Configuring xextproto..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(xextprotoSrcDir)/configure --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

