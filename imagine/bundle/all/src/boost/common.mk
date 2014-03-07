boostVer := 1_55_0
boostSrcDir := boost
boostSrcArchive := boost_$(boostVer)-preprocessor.tar.xz

installIncludeDir := $(installDir)/include/boost

all : $(boostSrcDir)

install : $(outputPCFile)
	@echo "Installing boost to: $(installDir)"
	@mkdir -p $(installIncludeDir)
	cp -r $(boostSrcDir)/* $(installIncludeDir)/

.PHONY : all install

$(boostSrcDir) : | $(boostSrcArchive)
	@echo "Extracting boost..."
	tar -mxJf $|


