#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int en = 0;

typedef struct {
    void*  ptr;
    size_t size;
} MemInfo;

#define MAX_MEMINFO 1024

static MemInfo memInfo[MAX_MEMINFO];
static int     memIndex = 0;

void* dbgMalloc(size_t size) {
    void* ptr = malloc(size);
    if (en) {
        int i;
        for (i = 0; i < MAX_MEMINFO; i++) {
            if (memInfo[i].ptr == NULL) {
                memInfo[i].ptr = ptr;
                memInfo[i].size = size;
                break;
            }
        }
    }
    return ptr;
}

void* dbgCalloc(size_t size, size_t count) {
    void* ptr = dbgMalloc(size * count);

    if (ptr != NULL) {
        memset(ptr, 0, size * count);
    }

    return ptr;
}

void dbgFree(void* ptr) {
    if (en) {
        int i;
        for (i = 0; i < MAX_MEMINFO; i++) {
            if (memInfo[i].ptr == ptr) {
                memInfo[i].ptr  = NULL;
                memInfo[i].size = 0;
                break;
            }
        }
    }
    free(ptr);
}

void dbgEnable() {
    memset(memInfo, 0, sizeof(memInfo));
    en = 1;
}

void dbgDisable() {
    en = 0;
}

void dbgPrint() {
    int i;
    printf("MEMORY DUMP:\n");
    for (i = 0; i < MAX_MEMINFO; i++) {
        if (memInfo[i].ptr != NULL) {
            printf("%d\t%.8x : %d bytes\n", i, memInfo[i].ptr, memInfo[i].size);
        }
    }
}
