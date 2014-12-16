libcxxVersion := 3.5.0
libcxxSrcDir := $(tempDir)/libcxx-$(libcxxVersion).src
libcxxSrcArchive := libcxx-$(libcxxVersion).src.tar.xz

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

$(libcxxSrcDir)/CMakeLists.txt : | $(libcxxSrcArchive)
	@echo "Extracting libcxx..."
	@mkdir -p $(libcxxSrcDir)
	tar -mxJf $| -C $(libcxxSrcDir)/..

$(outputLibFile) : $(makeFile)
	@echo "Building libcxx..."
	$(MAKE) -C $(<D) -j4 VERBOSE=1

$(makeFile) : $(libcxxSrcDir)/CMakeLists.txt
	@echo "Configuring libcxx..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(@D) && CC=clang CXX=clang cmake -DLIBCXX_ENABLE_SHARED=OFF \
	-DLIBCXX_ENABLE_RTTI=ON -DLIBCXX_ENABLE_ASSERTIONS=OFF \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="$(IOS_FLAGS) $(OPTIMIZE_CFLAGS) -flto -fvisibility=hidden -DNDEBUG" \
	-DLIBCXX_LIBCXXABI_INCLUDE_PATHS=$(tempDir)/../libcxxabi/libcxxabi-$(libcxxVersion).src/include -DLIBCXX_CXX_ABI=libcxxabi \
	$(buildArg) $(libcxxSrcDir)/
