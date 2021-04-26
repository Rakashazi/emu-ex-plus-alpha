#pragma once

#include <genplus-config.h>

static const unsigned PCM_STEP_SHIFT = 11;

void pcm_write(unsigned int a, unsigned int d);
void scd_pcm_setRate(int rate);
void scd_pcm_update(PCMSampleType *buffer, int length, int stereo);
