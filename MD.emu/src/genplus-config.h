#pragma once

#include <cstdint>
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using INT8 = int8_t;
using INT16 = int16_t;
using INT32 = int32_t;
using UINT8 = uint8_t;
using UINT16 = uint16_t;
using UINT32 = uint32_t;
typedef int16 FMSampleType;
typedef int16 PCMSampleType;
static const uint8 config_dac_bits = 14;
static const int16 config_psg_preamp = 100;
static const int32 config_fm_preamp = 100;
static const uint8 config_overscan = 0;//3;
static const uint8 config_render = 0;
static const uint8 config_hq_fm = 1; // TODO: non-hq mode causes random seg-faults
static const uint8 config_filter = 0;
static const uint8 config_clipSound = 0;
static const uint8 config_psgBoostNoise = 0;
static const int16 config_lp_range = 50;
static const int16 config_low_freq = 880;
static const int16 config_high_freq = 5000;
static const int16 config_lg = 1;
static const int16 config_mg = 1;
static const int16 config_hg = 1;
static const double config_rolloff = 0.990;
extern bool config_ym2413_enabled;
static const int16 config_ym2612_clip = 1;
static const uint8 config_force_dtack = 0;
static const uint8 config_addr_error = 1;

#define MAX_INPUTS 8

struct t_input_config
{
  unsigned char padtype;
};

struct t_config
{
  uint8 region_detect;
  uint8 tmss;
  uint8 lock_on;
  uint8 hot_swap;
  uint8 romtype;
  t_input_config input[MAX_INPUTS];
};
