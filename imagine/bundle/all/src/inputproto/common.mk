ifndef CHOST
CHOST := $(shell $(CC) -dumpmachine)
else
buildArg := --build=$(shell $(CC) -dumpmachine)
endif

inputprotoVer := 2.3
inputprotoSrcDir := inputproto-$(inputprotoVer)
inputprotoSrcArchive := inputproto-$(inputprotoVer).tar.bz2

makeFile := $(buildDir)/Makefile
outputPCFile := $(buildDir)/inputproto.pc
installIncludeDir := $(installDir)/include/X11/extensions

all : $(outputPCFile)

install : $(outputPCFile)
	@echo "Installing inputproto to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputPCFile) $(installDir)/lib/pkgconfig/
	cp $(inputprotoSrcDir)/*.h $(installIncludeDir)/

.PHONY : all install

$(inputprotoSrcDir)/configure : $(inputprotoSrcArchive)
	@echo "Extracting inputproto..."
	tar -mxjf $^

$(outputPCFile) : $(makeFile)
	@echo "Building inputproto..."
	$(MAKE) -C $(<D)

$(makeFile) : $(inputprotoSrcDir)/configure
	@echo "Configuring inputproto..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDLIBS)" $$dir/$(inputprotoSrcDir)/configure --host=$(CHOST) PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) PKG_CONFIG=pkg-config $(buildArg)

