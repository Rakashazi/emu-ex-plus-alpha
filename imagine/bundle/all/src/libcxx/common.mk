libcxxVersion := 14.0.6
libcxxSrcDir := $(tempDir)/llvm-project-$(libcxxVersion).src/libcxx
libcxxabiSrcDir := $(tempDir)/llvm-project-$(libcxxVersion).src/libcxxabi
# Archive containing the libcxx & libcxxabi directories along with a minimal set of cmake support files
libcxxSrcArchive := llvm-project-libcxx-$(libcxxVersion).src.tar.xz

makeFile := $(buildDir)/Makefile
outputLibFile := $(buildDir)/lib/libc++.a
outputLibcxxabiFile := $(buildDir)/lib/libc++abi.a
installIncludeDir := $(installDir)/include/c++/v1

VPATH += $(libcxxabiSrcDir)/src
CXXABI_SRC := abort_message.cpp \
 cxa_aux_runtime.cpp \
 cxa_default_handlers.cpp \
 cxa_demangle.cpp \
 cxa_exception.cpp \
 cxa_exception_storage.cpp \
 cxa_guard.cpp \
 cxa_handlers.cpp \
 cxa_personality.cpp \
 cxa_thread_atexit.cpp \
 cxa_vector.cpp \
 cxa_virtual.cpp \
 fallback_malloc.cpp \
 private_typeinfo.cpp \
 stdlib_exception.cpp \
 stdlib_new_delete.cpp \
 stdlib_stdexcept.cpp \
 stdlib_typeinfo.cpp

CXXABI_OBJ := $(addprefix $(objDir)/,$(CXXABI_SRC:.cpp=.o))
OBJ := $(CXXABI_OBJ)

all : $(outputLibFile) $(outputLibcxxabiFile)

install : $(outputLibFile) $(outputLibcxxabiFile)
	@echo "Installing libc++ to: $(installDir)"
	@mkdir -p $(installIncludeDir) $(installDir)/lib
	cp $(outputLibFile) $(outputLibcxxabiFile) $(installDir)/lib/
	cp -r $(buildDir)/include/c++/v1/* $(installIncludeDir)/
	cp -r $(libcxxabiSrcDir)/include/* $(installIncludeDir)/

.PHONY : all install

$(libcxxSrcDir)/CMakeLists.txt : | $(libcxxSrcArchive)
	@echo "Extracting libc++..."
	@mkdir -p $(tempDir)
	tar -mxJf $| -C $(tempDir)

$(outputLibcxxabiFile) : $(OBJ)
	@echo "Archiving libc++abi..."
	@mkdir -p `dirname $@`
	$(AR) cr $@ $(OBJ)

$(outputLibFile) : $(makeFile)
	@echo "Building libc++..."
	$(MAKE) -C $(<D)

# build libc++abi after libc++ and use its build headers
$(CXXABI_OBJ) : $(outputLibFile)
$(CXXABI_OBJ) : CPPFLAGS += -DHAVE___CXA_THREAD_ATEXIT_IMPL -D_LIBCPP_DISABLE_EXTERN_TEMPLATE -D_LIBCPP_BUILDING_LIBRARY -D_LIBCXXABI_BUILDING_LIBRARY -nostdinc++ -I$(libcxxSrcDir)/src -I$(buildDir)/include/c++/v1

$(makeFile) : $(libcxxSrcDir)/CMakeLists.txt
	@echo "Configuring libc++..."
	@mkdir -p $(@D)
	dir=`pwd` && cd $(libcxxSrcDir) && $(toolchainEnvParams) cmake -DLIBCXX_ENABLE_SHARED=OFF \
	-DLIBCXX_CXX_ABI=libcxxabi -DLLVM_INCLUDE_TESTS=OFF -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF \
	-DLIBCXX_ENABLE_DEBUG_MODE_SUPPORT=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$(CPPFLAGS) $(CXXFLAGS)" \
	-DLIBCXX_ABI_UNSTABLE=ON -DLIBCXX_ENABLE_INCOMPLETE_FEATURES=ON -DCMAKE_CXX_COMPILER_TARGET=$(clangTarget) \
	$(buildArg) -B $(@D)
