#pragma once

/*  This file is part of EmuFramework.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with EmuFramework.  If not, see <http://www.gnu.org/licenses/> */

#include <imagine/gui/TextEntry.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/util/concepts.hh>
#include <cstdio>

namespace EmuEx
{

using namespace IG;

enum class ScanValueMode
{
	Normal, AllowBlank
};

template <std::same_as<const char*> T>
inline std::pair<T, int> scanValue(const char* str, ScanValueMode mode)
{
	return {str, mode == ScanValueMode::AllowBlank || strlen(str) ? 1 : 0};
}

template <std::integral T>
inline std::pair<T, int> scanValue(const char* str, ScanValueMode)
{
	int val;
	int items = sscanf(str, "%d", &val);
	return {val, items};
}

template <std::floating_point T>
inline std::pair<T, int> scanValue(const char* str, ScanValueMode)
{
	T val, denom;
	int items = sscanf(str, std::is_same_v<T, double> ? "%lf /%lf" : "%f /%f", &val, &denom);
	if(items > 1 && denom != 0)
	{
		val /= denom;
	}
	return {val, items};
}

template <class T>
requires std::same_as<T, std::pair<float, float>> || std::same_as<T, std::pair<double, double>>
inline std::pair<T, int> scanValue(const char* str, ScanValueMode)
{
	// special case for getting a fraction
	using PairValue = typename T::first_type;
	PairValue val, denom{};
	int items = sscanf(str, std::is_same_v<PairValue, double> ? "%lf /%lf" : "%f /%f", &val, &denom);
	if(denom == 0)
	{
		denom = 1.;
	}
	return {{val, denom}, items};
}

template <class T>
requires std::same_as<T, std::pair<int, int>>
inline std::pair<T, int> scanValue(const char *str, ScanValueMode)
{
	using PairValue = typename T::first_type;
	PairValue val, val2{};
	int items = sscanf(str, "%d %d", &val, &val2);
	return {{val, val2}, items};
}

void pushAndShowModalView(std::unique_ptr<View> v, const Input::Event& e);
Gfx::TextureSpan collectTextCloseAsset(ApplicationContext);
void postErrorMessage(ApplicationContext, std::string_view);

inline void pushAndShowModalView(std::unique_ptr<View> v)
{
	auto e = v->appContext().defaultInputEvent();
	pushAndShowModalView(std::move(v), e);
}

inline void pushAndShowNewCollectTextInputView(ViewAttachParams attach, const Input::Event& e, const char* msgText,
	const char *initialContent, CollectTextInputView::OnTextDelegate onText)
{
	pushAndShowModalView(std::make_unique<CollectTextInputView>(attach, msgText, initialContent,
		collectTextCloseAsset(attach.appContext()), onText), e);
}

template<class T, ScanValueMode mode = ScanValueMode::Normal>
inline void pushAndShowNewCollectValueInputView(ViewAttachParams attach, const Input::Event& e,
	CStringView msgText, CStringView initialContent, IG::Callable<bool, CollectTextInputView&, T> auto&& collectedValueFunc)
{
	pushAndShowNewCollectTextInputView(attach, e, msgText, initialContent,
		[collectedValueFunc](CollectTextInputView& view, const char* str)
		{
			if(!str)
			{
				view.dismiss();
				return false;
			}
			auto [val, items] = scanValue<T>(str, mode);
			if(items <= 0)
			{
				postErrorMessage(view.appContext(), "Enter a value");
				return true;
			}
			else if(!collectedValueFunc(view, val))
			{
				return true;
			}
			else
			{
				view.dismiss();
				return false;
			}
		});
}

template<class T, auto low, auto high>
inline void pushAndShowNewCollectValueRangeInputView(ViewAttachParams attach, const Input::Event& e,
	CStringView msgText, CStringView initialContent, IG::Callable<bool, CollectTextInputView&, T> auto&& collectedValueFunc)
{
	pushAndShowNewCollectValueInputView<T>(attach, e, msgText, initialContent,
		[collectedValueFunc](CollectTextInputView& view, auto val)
		{
			if(val >= low && val <= high)
			{
				return collectedValueFunc(view, val);
			}
			else
			{
				postErrorMessage(view.appContext(), "Value not in range");
				return false;
			}
		});
}

template<class T, auto low, auto high, auto low2, auto high2>
inline void pushAndShowNewCollectValuePairRangeInputView(ViewAttachParams attach, const Input::Event& e,
	CStringView msgText, CStringView initialContent, Callable<bool, CollectTextInputView&, std::pair<T, T>> auto&& collectedValueFunc)
{
	pushAndShowNewCollectValueInputView<std::pair<T, T>>(attach, e, msgText, initialContent,
		[collectedValueFunc](CollectTextInputView& view, auto val)
		{
			if(val.first >= low && val.first <= high && val.second >= low2 && val.second <= high2)
			{
				return collectedValueFunc(view, val);
			}
			else
			{
				postErrorMessage(view.appContext(), "Values not in range");
				return false;
			}
		});
}

inline void pushAndShowNewYesNoAlertView(ViewAttachParams, const Input::Event&,
	const char* label, const char* choice1, const char* choice2,
	TextMenuItem::SelectDelegate onYes, TextMenuItem::SelectDelegate onNo);

}
