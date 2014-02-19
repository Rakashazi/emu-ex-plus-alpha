ifndef buildName
 buildName := $(firstMakefileName:.mk=)
endif

ifndef buildPath
 buildPath := build/$(buildName)
endif

ifndef genPath
 genPath := $(buildPath)/gen
endif

ifndef objDir
 objDir := $(buildPath)/obj
endif

ifndef targetDir
 targetDir := target/$(ENV)
endif

# C
$(objDir)/%.o : %.c
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CFLAGS) -o $@

# C++
$(objDir)/%.o : %.cc
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

$(objDir)/%.o : %.cpp
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

$(objDir)/%.o : %.cxx
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) -o $@

# Objective C
$(objDir)/%.o : %.m
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CFLAGS) $(OBJCFLAGS) -o $@

# Objective C++
$(objDir)/%.o : %.mm
	@echo "Compiling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(CC) $(compileAction) $< $(CPPFLAGS) $(CXXFLAGS) $(OBJCFLAGS) -o $@

# Assembly
$(objDir)/%.o : %.s
	@echo "Assembling $<"
	@mkdir -p $(@D)
	$(PRINT_CMD)$(AS) $< $(ASMFLAGS) -o $@
