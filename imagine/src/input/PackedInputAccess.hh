#pragma once

#include <imagine/input/inputDefs.hh>

namespace IG::Input
{

struct PackedInputAccess
{
	uint32_t byteOffset;
	uint32_t mask;
	Key key;

	int updateState(const uint8_t *prev, const uint8_t *curr) const
	{
		bool oldState = prev[byteOffset] & mask,
			newState = curr[byteOffset] & mask;
		if(oldState != newState)
		{
			return newState;
		}
		return -1; // no state change
	}
};

}
