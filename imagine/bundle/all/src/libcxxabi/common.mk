libcxxabiSrcArchive := libcxxabi-182079.tar.xz
libcxxabiSrcDir := libcxxabi

outputLibFile := $(buildDir)/libcxxabi.a
installIncludeDir := $(installDir)/include

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
CPPFLAGS += -I$(libcxxabiSrcDir)/include -I/usr/lib/c++/v1

all : $(outputLibFile)

install : $(outputLibFile)
	@echo "Installing libcxxabi to $(installDir)..."
	@mkdir -p $(installIncludeDir) $(installDir)/lib
	cp $(outputLibFile) $(installDir)/lib/
	cp $(libcxxabiSrcDir)/include/* $(installIncludeDir)/

.PHONY : all install

$(outputLibFile) : $(OBJ)
	@echo "Archiving libcxxabi..."
	@mkdir -p `dirname $@`
	ar cr $@ $(OBJ)

$(CPP_SRC) : $(libcxxabiSrcDir)

$(libcxxabiSrcDir) : $(libcxxabiSrcArchive)
	@echo "Extracting libcxxabi..."
	tar -mxJf $^
