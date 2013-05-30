libcxxSrcDir := libcxx
libcxxSrcArchive := libcxx-182079.tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/lib/libc++.a
installIncludeDir := $(installDir)/include/c++/v1

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libcxx to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib/pkgconfig
	cp $(outputLibFile) $(installDir)/lib/
	cp -r $(libcxxSrcDir)/include/* $(installIncludeDir)/

.PHONY : all install

$(libcxxSrcDir)/CMakeLists.txt : $(libcxxSrcArchive)
	@echo "Extracting libcxx..."
	tar -mxJf $^

$(outputLibFile) : $(makeFile)
	@echo "Building libcxx..."
	$(MAKE) -C $(<D) -j4 VERBOSE=1

$(makeFile) : $(libcxxSrcDir)/CMakeLists.txt
	@echo "Configuring libcxx..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CXX=clang CFLAGS="$(IOS_FLAGS)" cmake -DLIBCXX_ENABLE_SHARED=OFF -DLIBCXX_ENABLE_RTTI=OFF -DCMAKE_BUILD_TYPE=Release -DLIBCXX_CXX_FEATURE_FLAGS="$(IOS_FLAGS)" -DLIBCXX_LIBCXXABI_INCLUDE_PATHS=$$dir/../libcxxabi/libcxxabi/include -DLIBCXX_CXX_ABI=libcxxabi $(buildArg) $$dir/$(libcxxSrcDir)/

