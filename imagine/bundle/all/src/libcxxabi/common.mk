libcxxVersion := 3.5.0
libcxxabiSrcDir := $(tempDir)/libcxxabi-$(libcxxVersion).src
libcxxabiSrcArchive := libcxxabi-$(libcxxVersion).src.tar.xz

libcxxSrcDir := $(tempDir)/../libcxx/libcxx-$(libcxxVersion).src
libcxxSrcArchive := ../libcxx/libcxx-$(libcxxVersion).src.tar.xz

outputLibFile := $(buildDir)/libcxxabi.a
installIncludeDir := $(installDir)/include

ifeq ($(wildcard $(libcxxabiSrcDir)/src),)
 $(info Extracting libcxxabi...)
 $(shell mkdir -p $(libcxxabiSrcDir))
 $(shell tar -mxJf $(libcxxabiSrcArchive) -C $(libcxxabiSrcDir)/..)
endif

VPATH += $(libcxxabiSrcDir)/src
CPP_SRC := abort_message.cpp cxa_guard.cpp cxa_virtual.cpp \
cxa_aux_runtime.cpp cxa_handlers.cpp exception.cpp \
cxa_default_handlers.cpp cxa_new_delete.cpp private_typeinfo.cpp \
cxa_demangle.cpp cxa_personality.cpp stdexcept.cpp \
cxa_exception.cpp cxa_unexpected.cpp typeinfo.cpp \
cxa_exception_storage.cpp cxa_vector.cpp
CPP_OBJ := $(addprefix $(objDir)/,$(CPP_SRC:.cpp=.o))
OBJ := $(CPP_OBJ)

cxxRTTI := 1
cxxExceptions := 1
CPPFLAGS += -I$(libcxxabiSrcDir)/include -I$(libcxxSrcDir)/include

all : $(outputLibFile) $(libcxxabiSrcDir) $(libcxxSrcDir)

install : $(outputLibFile)
	@echo "Installing libcxxabi to $(installDir)..."
	@mkdir -p $(installIncludeDir) $(installDir)/lib
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libcxxabiSrcDir)/include/*.h $(installIncludeDir)/

.PHONY : all install

$(CPP_SRC) : $(libcxxabiSrcDir) $(libcxxSrcDir)

$(outputLibFile) : $(OBJ)
	@echo "Archiving libcxxabi..."
	@mkdir -p `dirname $@`
	ar cr $@ $(OBJ)

$(libcxxSrcDir) : | $(libcxxSrcArchive)
	@echo "Extracting libcxx..."
	@mkdir -p $(libcxxSrcDir)
	tar -mxJf $| -C $(libcxxSrcDir)/..
