ifndef inc_vmem
inc_vmem := 1

ifeq ($(ENV_KERNEL), linux)
 SRC += vmem/linux.cc
 SRC += vmem/RingBuffer.cc
else ifeq ($(ENV_KERNEL), mach)
 SRC += vmem/mach.cc
 SRC += vmem/RingBuffer.cc
endif

endif
