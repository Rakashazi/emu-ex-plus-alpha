#ifndef VLM5030_h
#define VLM5030_h

#define WRITE8_HANDLER(name) 	\
    void     name(int offset, unsigned char data)

typedef void* sound_stream;
typedef int stream_sample_t;
extern void stream_update(sound_stream,int);
void vlm5030_update_callback(stream_sample_t *_buffer, int length);

void *vlm5030_start(int clock);

void vlm5030_LoadState();
void vlm5030_SaveState();


struct VLM5030interface
{
	int memory_region;  /* memory region of speech rom    */
	int memory_size;    /* memory size of speech rom (0=memory region length) */
};

/* set speech rom address */
void VLM5030_set_rom(void *speech_rom, int length);

/* get BSY pin level */
int VLM5030_BSY(void);
/* latch contoll data */
WRITE8_HANDLER( VLM5030_data_w );
/* set RST pin level : reset / set table address A8-A15 */
void VLM5030_RST (int pin );
/* set VCU pin level : ?? unknown */
void VLM5030_VCU(int pin );
/* set ST pin level  : set table address A0-A7 / start speech */
void VLM5030_ST(int pin );

#endif
