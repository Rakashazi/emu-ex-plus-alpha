#pragma once

#include <util/cLang.h>
#include <util/memory.h>
#include <util/branch.h>
#include <assert.h>
#include <utility>
#include <logger/interface.h>
#include <util/preprocessor/repeat.h>

/*#define forEachInDLList(listAddr, e) \
for(typeof ((listAddr)->list) e ## _node = (listAddr)->list; e ## _node != 0; e ## _node = e ## _node->next) \
for(typeof (e ## _node->d) &e = e ## _node->d, forLoopExecOnceDummy)*/

#define forEachInDLList(listAddr, e) \
for(typeof ((listAddr)->iterator()) e ## _it = (listAddr)->iterator(); e ## _it.curr != 0; e ## _it.advance()) \
for(typeof (e ## _it.curr->d) &e = e ## _it.curr->d, forLoopExecOnceDummy)

#define forEachInDLListReverse(listAddr, e) \
for(typeof ((listAddr)->endIterator()) e ## _it = (listAddr)->endIterator(); e ## _it.curr != 0; e ## _it.reverse()) \
for(typeof (e ## _it.curr->d) &e = e ## _it.curr->d, forLoopExecOnceDummy)

#define forEachDInDLList(listAddr, e) \
for(typeof ((listAddr)->iterator()) e ## _it = (listAddr)->iterator(); e ## _it.curr != 0; e ## _it.advance()) \
for(typeof (*e ## _it.curr->d) &e = *e ## _it.curr->d, forLoopExecOnceDummy)

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

	bool hasFree()
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
	int size = 0;
private:
	DLFreeList<Node> free;

public:
	constexpr DLList() { }
	template <size_t S>
	constexpr DLList(Node (&n)[S]): free(n) { }

	class Iterator
	{
	public:
		DLList &dlList;
		Node *curr, *next, *prev;

		constexpr Iterator(DLList &dlList) : dlList(dlList), curr(dlList.list), next(curr ? curr->next : nullptr), prev(nullptr) { }
		constexpr Iterator(DLList &dlList, Node *curr) :
			dlList(dlList), curr(curr), next(curr ? curr->next : nullptr), prev(curr ? curr->prev : nullptr) { }

		void advance()
		{
			curr = next;
			prev = next ? next->prev : nullptr;
			next = next ? next->next : nullptr;
		}

		void reverse()
		{
			curr = prev;
			prev = prev ? prev->prev : nullptr;
			next = prev ? prev->next : nullptr;
		}

		void removeElem()
		{
			assert(curr);
			dlList.removeNode(curr);
			curr = nullptr;
		}

		T &obj()
		{
			return curr->d;
		}
	};

	Iterator iterator()
	{
		return Iterator(*this);
	}

	Iterator endIterator()
	{
		return Iterator(*this, listEnd);
	}

	template <size_t S>
	void init(Node (&n)[S])
	{
		free.init(n);
		list = listEnd = nullptr;
		size = 0;
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

		size++;
		return 1;
	}

	int add(const T &d)
	{
		if(!add())
		{
			return 0;
		}
		*first() = d; // copy the data
		//logMsg("added %p to list, size %d", &d, size);
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

		size++;
		return 1;
	}

	int addToEnd(const T &d)
	{
		if(!addToEnd())
		{
			return 0;
		}
		*last() = d; // copy the data
		return 1;
	}

	void push_back(const T &d)
	{
		addToEnd(d);
	}

	template <class... ARGS>
	void emplace_back(ARGS&&... args)
	{
		if(!addToEnd())
		{
			bug_exit("out of space in list");
		}
		new(last()) T(std::forward<ARGS>(args)...);
	}

	int contains(const T &d)
	{
		for(Node *n = list; n; n = n->next)
		{
			if(n->d == d)
				return 1;
		}
		return 0;
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
		size--;
		//logMsg("removed node %p, size %d", n, size);
		assert(size >= 0);
	}

	int remove(const T &d)
	{
		for(Node *n = list; n; n = n->next)
		{
			if(n->d == d)
			{
				removeNode(n);
				//logMsg("removed %p from list", &d);
				return 1;
			}
		}
		return 0;
	}

	int removeLast()
	{
		if(listEnd)
		{
			removeNode(listEnd);
			return 1;
		}
		return 0;
	}

	int removeFirst()
	{
		if(list)
		{
			removeNode(list);
			return 1;
		}
		return 0;
	}

	void removeAll()
	{
		logMsg("removing all list items (%d)", size);
		uint counts = size;
		iterateTimes(counts, i)
		{
			removeNode(list);
		}
		assert(size == 0);
	}

	T *first() { return list ? &list->d : nullptr;	}
	T *last() { return listEnd ? &listEnd->d : nullptr;	}

	T &back() { return listEnd->d;	}

	T *index(uint idx)
	{
		assert(idx < (uint)size);
		auto it = iterator();
		iterateTimes(idx, i)
		{
			it.advance();
		}
		return &it.obj();
	}

	bool isFull()
	{
		return !free.hasFree();
	}

	int freeSpace()
	{
		return free.size;
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
