CXX_SRC := $(filter %.cxx %.cc %.cpp,$(SRC))
C_SRC := $(filter %.c,$(SRC))
OBJC_SRC := $(filter %.m,$(SRC))
OBJCXX_SRC := $(filter %.mm,$(SRC))
ASM_SRC := $(filter %.s,$(SRC))

CXX_OBJ := $(addprefix $(objDir)/,$(patsubst %.cxx, %.o, $(patsubst %.cpp, %.o, $(CXX_SRC:.cc=.o))))
C_OBJ := $(addprefix $(objDir)/,$(C_SRC:.c=.o))
OBJC_OBJ := $(addprefix $(objDir)/,$(OBJC_SRC:.m=.o))
OBJCXX_OBJ := $(addprefix $(objDir)/,$(OBJCXX_SRC:.mm=.o))
ASM_OBJ := $(addprefix $(objDir)/,$(ASM_SRC:.s=.o))
OBJ += $(CXX_OBJ) $(C_OBJ) $(OBJC_OBJ) $(OBJCXX_OBJ) $(ASM_OBJ)
DEP := $(OBJ:.o=.d)

-include $(DEP)

LDFLAGS += $(LDLIBS)

.PHONY: clang-tidy-cxx clang-tidy-c

clangTidyChecks := performance-*

clang-tidy-cxx: $(CXX_SRC)
	@$(CLANG_TIDY) -checks=$(clangTidyChecks) $^ -- $(CPPFLAGS) $(CXXFLAGS)

clang-tidy-c: $(C_SRC)
	@$(CLANG_TIDY) -checks=$(clangTidyChecks) $^ -- $(CPPFLAGS) $(CFLAGS)

.PHONY: cxx cppflags cflags cxxflags objcflags asmflags ldflags pkg-deps

cxx:
	@echo C++ Compiler: $(CXX)

cppflags:
	@echo CPPFLAGS: $(CPPFLAGS)

cflags:
	@echo CFLAGS: $(CFLAGS)

cxxflags:
	@echo CXXFLAGS: $(CXXFLAGS)

objcflags:
	@echo OBJCFLAGS: $(OBJCFLAGS)

asmflags:
	@echo ASMFLAGS: $(ASMFLAGS)

ldflags:
	@echo LDFLAGS: $(LDFLAGS)

pkg-deps:
	@echo pkg-config packages: $(pkgConfigDeps) $(pkgConfigStaticDeps)
