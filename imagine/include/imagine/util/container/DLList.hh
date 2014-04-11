#pragma once

#include <imagine/util/algorithm.h>
#include <imagine/util/memory.h>
#include <imagine/util/operators.hh>
#include <assert.h>
#include <utility>
#include <imagine/logger/logger.h>
//#include <imagine/util/preprocessor/repeat.h>
#include "containerUtils.hh"
#include <iterator>

#define DLListNodeInit(z, n, arr) \
{ n == 0 ? nullptr : &arr[n-1], \
n == sizeofArrayConst(arr)-1 ? nullptr : &arr[n+1] },

// TODO: needs more testing
//#define DLListNodeArrayInit(n, arr) BOOST_PP_REPEAT(n, DLListNodeInit, arr)

#define DLListNodeArray(name, size) \
name[size] INITFIRST { DLListNodeArrayInit(size, name) }

#ifndef NDEBUG
#define DEBUG_LIST
#endif

template<class NODE>
class DLFreeList
{
public:
	constexpr DLFreeList() { }

	template <size_t S>
	DLFreeList(NODE (&n)[S]): free(n), size(S)
	{
		initNodes(n);
		/*#ifdef DEBUG_LIST
		checkInit();
		#endif*/
	}

	void checkInit()
	{
		iterateTimes(size, i)
		{
			logMsg("idx %d prev: %p next: %p\n", i, free[i].prev, free[i].next);
			assert(free[i].prev == (i == 0 ? nullptr : &free[i-1]));
			assert(free[i].next == (i == (uint)size-1 ? nullptr : &free[i+1]));
		}
	}

	template <size_t S>
	void init(NODE (&n)[S])
	{
		//logMsg("init free-list with %d nodes", S);
		free = n;
		size = S;
		initNodes(n);
	}

	template <size_t S>
	static void initNodes(NODE (&n)[S])
	{
		iterateTimes(S, i)
		{
			n[i].prev = i == 0 ? nullptr : &n[i-1];
			n[i].next = i == (uint)S-1 ? nullptr : &n[i+1];
		}
	}

	NODE *get()
	{
		NODE *newN = free;
		if(!newN)
		{
			logMsg("no more nodes in free list");
			return 0;
		}

		if(newN->next)
		{
			newN->next->prev = nullptr; // unlink next free-list node
		}
		free = newN->next; // set free-list to next node, which may be NULL

		size--;
		assert(size >= 0);
		if(size)
		{
			assert(free);
		}
		return newN;
	}

	void add(NODE *n)
	{
		if(free)
			free->prev = n; // link free-list prev to this node

		// add this node to head of free-list
		n->next = free;
		n->prev = nullptr;
		free = n;
		size++;
	}

	bool hasFree() const
	{
		return free;
	}

private:
	NODE *free = nullptr;
public:
	int size = 0;
};

template<class T>
class DLList
{
public:
	class Node
	{
	public:
		constexpr Node() { }
		constexpr Node(Node *prev, Node *next): prev(prev), next(next) { }
		T d;
		Node *prev;
		Node *next;

		bool operator ==(Node const& rhs) const
		{
			return d == rhs;
		}
	};

	Node *list = nullptr, *listEnd = nullptr;
private:
	int size_ = 0;
	DLFreeList<Node> free;

public:
	constexpr DLList() {}
	template <size_t S>
	constexpr DLList(Node (&n)[S]): free(n) {}

	template <size_t S>
	void init(Node (&n)[S])
	{
		free.init(n);
		list = listEnd = nullptr;
		size_ = 0;
		//logMsg("init list of size %d", S);
	}

	int add()
	{
		Node *newN = free.get();
		if(!newN)
		{
			return 0;
		}

		// move new node to head of list
		newN->prev = nullptr;
		newN->next = list;

		if(list)
		{
			list->prev = newN; // link former head of list to new node
		}
		else
		{
			listEnd = newN; // list was empty, set end node
		}

		list = newN; // set head of list to new node

		size_++;
		return 1;
	}

	int add(const T &d)
	{
		if(!add())
		{
			return 0;
		}
		front() = d; // copy the data
		//logMsg("added %p to list, size %d", &d, size_);
		return 1;
	}

	int addToEnd()
	{
		Node *newN = free.get();
		if(!newN)
		{
			return 0;
		}

		// move new node to end of list
		newN->prev = listEnd;
		newN->next = nullptr;

		if(listEnd)
		{
			listEnd->next = newN; // link former end of list to new node
		}
		else
		{
			list = newN; // list was empty, set first node
		}

		listEnd = newN; // set end of list to new node

		size_++;
		return 1;
	}

	int addToEnd(const T &d)
	{
		if(!addToEnd())
		{
			return 0;
		}
		back() = d; // copy the data
		return 1;
	}

	void removeNode(Node *n)
	{
		if(n == list) // if n is the head of the list, move the head to the next node
			list = n->next;
		if(n == listEnd) // if n is the end of the list, move the head to the prev node
			listEnd = n->prev;

		if(n->prev)
			n->prev->next = n->next; // link prev node to next of this one
		if(n->next)
			n->next->prev = n->prev; // link next node to prev of this one

		free.add(n);
		size_--;
		//logMsg("removed node %p, size %d", n, size);
		assert(size_ >= 0);
	}

	bool remove(const T &d)
	{
		for(Node *n = list; n; n = n->next)
		{
			if(n->d == d)
			{
				removeNode(n);
				//logMsg("removed %p from list", &d);
				return true;
			}
		}
		return false;
	}

	T *index(uint idx)
	{
		assert(idx < (uint)size_);
		auto it = begin();
		iterateTimes(idx, i)
		{
			++it;
		}
		return &(*it);
	}

	bool isFull() const
	{
		return !free.hasFree();
	}

	int freeSpace() const
	{
		return free.size;
	}

	// Iterators (STL API)
	template <bool REVERSE = false>
	class ConstIterator : public std::iterator<std::bidirectional_iterator_tag, T>,
		public NotEquals<ConstIterator<REVERSE>>
	{
	public:
		Node *n;

		constexpr ConstIterator(Node *n): n(n) {}
		const T& operator*() const { return n->d; }
		const T* operator->() const { return &n->d; }
		void operator++() { n = REVERSE ? n->prev : n->next; }
		void operator--() { n = REVERSE ? n->next : n->prev; }
		bool operator==(ConstIterator const& rhs) const { return n == rhs.n; }
	};

	template <bool REVERSE = false>
	class Iterator : public ConstIterator<REVERSE>
	{
	public:
		using ConstIterator<REVERSE>::ConstIterator;
		using ConstIterator<REVERSE>::n;
		T& operator*() const { return n->d; }
		T* operator->() const { return &n->d; }
	};

	using iterator = Iterator<>;
	using const_iterator = ConstIterator<>;
	using reverse_iterator = Iterator<true>;
	using const_reverse_iterator = ConstIterator<true>;
	iterator begin() { return iterator(list); }
	iterator end() { return iterator(nullptr); }
	const_iterator cbegin() const { return const_iterator(list); }
	const_iterator cend() const { return const_iterator(nullptr); }
	reverse_iterator rbegin() { return reverse_iterator(listEnd); }
	reverse_iterator rend() { return reverse_iterator(nullptr); }
	const_reverse_iterator crbegin() const { return const_reverse_iterator(listEnd); }
	const_reverse_iterator crend() const { return const_reverse_iterator(nullptr); }

	// Capacity (STL API)
	uint size() const { return size_; }
	bool empty() const { return !size(); };
	uint max_size() const { return size() + free.size; }

	// Element Access (STL API)
	T &front() { assert(list); return list->d;	}
	T &back() { assert(listEnd); return listEnd->d; }

	// Modifiers (STL API)
	void push_front(const T &d)
	{
		add(d);
	}

	void push_back(const T &d)
	{
		addToEnd(d);
	}

	template <class... ARGS>
	void emplace_front(ARGS&&... args)
	{
		if(!add())
		{
			bug_exit("out of space in list");
		}
		new(&front()) T(std::forward<ARGS>(args)...);
	}

	template <class... ARGS>
	void emplace_back(ARGS&&... args)
	{
		if(!addToEnd())
		{
			bug_exit("out of space in list");
		}
		new(&back()) T(std::forward<ARGS>(args)...);
	}

	void pop_front()
	{
		assert(list);
		removeNode(list);
	}

	void pop_back()
	{
		assert(listEnd);
		removeNode(listEnd);
	}

	iterator erase(const_iterator position)
	{
		auto nextIt = iterator(position.n->next);
		removeNode(position.n);
		return nextIt;
	}

	void clear()
	{
		logMsg("removing all list items (%d)", size_);
		uint counts = size_;
		iterateTimes(counts, i)
		{
			removeNode(list);
		}
		assert(size_ == 0);
	}
};

template<class T, size_t S>
class StaticDLList : public DLList<T>
{
	typename DLList<T>::Node node[S];
public:

	constexpr StaticDLList(): DLList<T>(node) { }
};

#ifdef DEBUG_LIST
#undef DEBUG_LIST
#endif
