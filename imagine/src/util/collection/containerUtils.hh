#pragma once

#include <algorithm>

template <class T>
struct ReverseRange
{
private:
	T &o;

public:
	constexpr ReverseRange(T &o): o(o) {}
	auto begin() const -> decltype(this->o.rbegin()) { return o.rbegin(); }
	auto end() const -> decltype(this->o.rend()) { return o.rend(); }
	auto cbegin() const -> decltype(this->o.crbegin()) { return o.crbegin(); }
	auto cend() const -> decltype(this->o.crend()) { return o.crend(); }
};

template <class T>
static ReverseRange<T> makeReverseRange(T &o)
{
	return ReverseRange<T>(o);
}

template <class Container>
class ForEachIteratorWrapper
{
public:
	using Iterator = typename Container::iterator;
	Container &c;
	Iterator it;
	bool skipNextInc = false;

	constexpr ForEachIteratorWrapper(Container &c, Iterator it): c(c), it(it) {}
	void erase()
	{
		it = c.erase(it);
		skipNextInc = true;
	}
	decltype(*it) operator*() const { return *it; }
	decltype(&(*it)) operator->() const { return &(*it); }
	void operator++()
	{
		if(skipNextInc)
			skipNextInc = false;
		else
			++it;
	}
	bool operator==(Iterator const& rhs) const { return it == rhs; }
	bool operator!=(Iterator const& rhs) const { return it != rhs; }
	operator Iterator() const { return it; }
};

#define forEachInContainer(container, it) for(ForEachIteratorWrapper<decltype(container)> it {container, (container).begin()}; it != (container).end(); ++it)

template <class Container, class T>
static bool contains(Container c, const T& val)
{
	for(auto &e : c)
	{
		if(e == val)
			return true;
	}
	return false;
	//return std::find(c.begin(), c.end(), val) != c.end();
}
