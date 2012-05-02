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

.PHONY: cppcheck

cppcheck: $(CXX_SRC) $(C_SRC)
	cppcheck $^ $(CPPFLAGS) -D__GNUC__ -DCHAR_BIT=8
#--check-config
