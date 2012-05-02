#pragma once

typedef struct
{
	uint32 s1, s2;
} RandomMWC;

static void randomMWC_init(RandomMWC *inst, uint32 s1, uint32 s2)
{
	if (s1 != 0)
		inst->s1 = s1;
	else
		inst->s1 = 521288629;
	if (s2 != 0)
		inst->s2 = s2;
	else
		inst->s2 = 362436069;
}

static uint32 randomMWC_uint32(RandomMWC *inst)
{
	inst->s2 = 36969 * (inst->s2 & 65535) + (inst->s2 >> 16);
	inst->s1 = 18000 * (inst->s1 & 65535) + (inst->s1 >> 16);
	return (inst->s2 << 16) + inst->s1;
}

static uint32 randomMWC_uint32Range(RandomMWC *inst, uint32 min, uint32 max)
{
	uint32 r = randomMWC_uint32(inst);
	return (randomMWC_uint32(inst) % ((max-min)+1)) + min;
}
