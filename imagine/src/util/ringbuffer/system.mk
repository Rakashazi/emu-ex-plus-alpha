ifndef inc_ringbuffer
inc_ringbuffer := 1

ifeq ($(ENV_KERNEL), linux)
 SRC += util/ringbuffer/linux.cc
 SRC += util/ringbuffer/RingBuffer.cc
else ifeq ($(ENV_KERNEL), mach)
 SRC += util/ringbuffer/mach.cc
 SRC += util/ringbuffer/RingBuffer.cc
endif

endif
