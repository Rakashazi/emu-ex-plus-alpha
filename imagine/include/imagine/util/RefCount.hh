#pragma once

template <class C>
class RefCount
{
	uint count = 1;

public:
	constexpr RefCount() { }

	void ref()
	{
		count++;
		//logMsg("added ref (%d)", count);
	}

	void freeRef()
	{
		assert(count);
		count--;
		//logMsg("freed ref (%d)", count);
		if(!count)
		{
			static_cast<C*>(this)->free();
		}
	}
};
