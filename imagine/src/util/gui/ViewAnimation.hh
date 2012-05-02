#pragma once

#include <gui/View.hh>
#include <util/Motion.hh>

template <uint TIME>
class FadeViewAnimation : public ViewAnimation
{
public:
	constexpr FadeViewAnimation() { }

	TimedMotion<float> m;

	void initShow()
	{
		m.initLinear(0, 1, TIME);
	}

	void initActive()
	{
		m.init(1);
	}

	void initDismiss()
	{
		m.initLinear(m.now, 0, TIME);
	}

	bool update()
	{
		return m.update();
	}
};
