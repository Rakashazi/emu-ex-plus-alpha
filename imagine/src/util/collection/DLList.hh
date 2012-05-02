#pragma once

#include <util/cLang.h>
#include <assert.h>
#include <logger/interface.h>

/*#define forEachInDLList(listAddr, e) \
for(typeof ((listAddr)->list) e ## _node = (listAddr)->list; e ## _node != 0; e ## _node = e ## _node->next) \
for(typeof (e ## _node->d) &e = e ## _node->d, forLoopExecOnceDummy)*/

#define forEachInDLList(listAddr, e) \
for(typeof ((listAddr)->iterator()) e ## _it = (listAddr)->iterator(); e ## _it.curr != 0; e ## _it.advance()) \
for(typeof (e ## _it.curr->d) &e = e ## _it.curr->d, forLoopExecOnceDummy)

template<class NODE>
class DLFreeList
{
public:
	DLFreeList() { }
	template <size_t S>
	DLFreeList(NODE (&n)[S]) { init(n); }

	template <size_t S>
	void init(NODE (&n)[S])
	{
		//logMsg("init free-list with %d nodes\n", S);
		free = n;
		size = S;
		iterateTimes(S, i)
		{
			n[i].prev = i == 0 ? NULL : &n[i-1];
			n[i].next = i == S-1 ? NULL : &n[i+1];
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
			newN->next->prev = NULL; // unlink next free-list node
		}
		free = newN->next; // set free-list to next node, which may be NULL

		size--;
		assert(size >= 0);
		return newN;
	}

	void add(NODE *n)
	{
		if(free)
			free->prev = n; // link free-list prev to this node

		// add this node to head of free-list
		n->next = free;
		n->prev = NULL;
		free = n;
		size++;
	}

private:
	NODE *free;
	int size;
};

template<class T>
class DLList
{
public:
	class Node
	{
	public:
		T d;
		Node *prev;
		Node *next;

		bool operator ==(Node const& rhs) const
		{
			return d == rhs;
		}
	};

	Node *list, *listEnd;
	int size;

	constexpr DLList(): list(nullptr), listEnd(nullptr), size(0) { }
	template <size_t S>
	DLList(Node (&n)[S]) { init(n); }

	class Iterator
	{
	public:
		DLList &dlList;
		Iterator(DLList &dlList) : dlList(dlList)
		{
			curr = dlList.list;
			next = curr ? curr->next : 0;;
		}
		Node *curr, *next;

		void advance()
		{
			curr = next;
			next = next ? next->next : 0;
		}

		void removeElem()
		{
			assert(curr);
			dlList.removeNode(curr);
			curr = 0;
		}
	};

	Iterator iterator()
	{
		return Iterator(*this);
	}

	template <size_t S>
	void init(Node (&n)[S])
	{
		free.init(n);
		list = listEnd = 0;
		size = 0;
		//logMsg("init list of size %d", S);
	}

	int add(const T &d)
	{
		Node *newN = free.get();
		if(newN == NULL)
		{
			return 0;
		}

		// move new node to head of list
		newN->prev = NULL;
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

		newN->d = d; // copy the data

		size++;
		logMsg("added %p to list, size %d", &d, size);
		return 1;
	}

	int addToEnd(const T &d)
	{
		Node *newN = free.get();
		if(newN == NULL)
		{
			return 0;
		}

		// move new node to end of list
		newN->prev = listEnd;
		newN->next = 0;

		if(listEnd)
		{
			listEnd->next = newN; // link former end of list to new node
		}
		else
		{
			list = newN; // list was empty, set first node
		}

		listEnd = newN; // set end of list to new node

		newN->d = d; // copy the data

		size++;
		return 1;
	}

	int contains(const T &d)
	{
		for(Node *n = list; n != NULL; n = n->next)
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

		if(n->prev != NULL)
			n->prev->next = n->next; // link prev node to next of this one
		if(n->next != NULL)
			n->next->prev = n->prev; // link next node to prev of this one

		free.add(n);
		size--;
		logMsg("removed node %p, size %d", n, size);
		assert(size >= 0);
	}

	int remove(const T &d)
	{
		for(Node *n = list; n != NULL; n = n->next)
		{
			if(n->d == d)
			{
				removeNode(n);
				logMsg("removed %p from list", &d);
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

private:
	DLFreeList<Node> free;
};

template<class T, uint size>
class StaticDLList : public DLList<T>
{
	typename DLList<T>::Node node[size];
public:
	void init()
	{
		DLList<T>::init(node);
	}
};
