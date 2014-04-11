boostVer := 1_55_0
boostSrcDir := $(tempDir)/boost
boostSrcArchive := boost_$(boostVer)-preprocessor.tar.xz

installIncludeDir := $(installDir)/include/boost

boostSystemPath = /usr/include/boost

ifneq ($(wildcard $(boostSystemPath)),)
 boostUseSystem = 1
endif

ifeq ($(boostUseSystem),1)

all :

install :
	@echo "Linking system boost to: $(installDir)"
	ln -srf $(boostSystemPath) $(installDir)/include

else

all : $(boostSrcDir)

install : $(boostSrcDir)
	@echo "Installing boost to: $(installDir)"
	@mkdir -p $(installIncludeDir)
	cp -r $(boostSrcDir)/* $(installIncludeDir)/

$(boostSrcDir) : | $(boostSrcArchive)
	@echo "Extracting boost..."
	@mkdir -p $(boostSrcDir)
	tar -mxJf $| -C $(boostSrcDir)/..

endif

.PHONY : all install

