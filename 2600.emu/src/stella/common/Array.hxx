//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2012 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: Array.hxx 2448 2012-04-20 20:47:53Z stephena $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef ARRAY_HXX
#define ARRAY_HXX

#include <cassert>

#include "bspf.hxx"

namespace Common {

template <class T>
class Array
{
  protected:
    int _capacity = 0;
    int _size = 0;
    T *_data = nullptr;

  public:
    typedef T *iterator;
    typedef const T *const_iterator;

  public:
    constexpr Array<T>() {}
    Array<T>(const Array<T>& array)
    {
      _size = array._size;
      _capacity = _size + 128;
      _data = new T[_capacity];
      for(int i = 0; i < _size; i++)
        _data[i] = array._data[i];
    }

    ~Array<T>()
    {
      if (_data)
        delete [] _data;
    }

    void reserve(int capacity)
    {
      if(capacity <= _capacity)
        return;
      ensureCapacity(capacity - 128);
    }

    void push_back(const T& element)
    {
      ensureCapacity(_size + 1);
      _data[_size++] = element;
    }

    void push_back(const Array<T>& array)
    {
      ensureCapacity(_size + array._size);
      for(int i = 0; i < array._size; i++)
        _data[_size++] = array._data[i];
    }

    void insert_at(int idx, const T& element)
    {
      assert(idx >= 0 && idx <= _size);
      ensureCapacity(_size + 1);
      // The following loop is not efficient if you can just memcpy things around.
      // e.g. if you have a list of ints. But for real objects (String...), memcpy
      // usually isn't correct (specifically, for any class which has a non-default
      // copy behaviour. E.g. the String class uses a refCounter which has to be
      // updated whenever a String is copied.
      for(int i = _size; i > idx; i--)
        _data[i] = _data[i-1];

      _data[idx] = element;
      _size++;
    }

    T remove_at(int idx)
    {
      assert(idx >= 0 && idx < _size);
      T tmp = _data[idx];
      for(int i = idx; i < _size - 1; i++)
        _data[i] = _data[i+1];
      _size--;
      return tmp;
    }

    T& operator [](int idx)
    {
      assert(idx >= 0 && idx < _size);
      return _data[idx];
    }

    const T& operator [](int idx) const
    {
      assert(idx >= 0 && idx < _size);
      return _data[idx];
    }

    Array<T>& operator =(const Array<T>& array)
    {
      if (_data)
        delete [] _data;
      _size = array._size;
      _capacity = _size + 128;
      _data = new T[_capacity];
      for(int i = 0; i < _size; i++)
        _data[i] = array._data[i];

      return *this;
    }

    unsigned int size() const { return _size; }
    unsigned int capacity() const { return _capacity; }

    void clear()
    {
      if(_data)
      {
        delete [] _data;
        _data = 0;
      }
      _size = 0;
      _capacity = 0;
    }
	
    bool isEmpty() const
    {
      return (_size == 0);
    }

    iterator begin()
    {
      return _data;
    }

    iterator end()
    {
      return _data + _size;
    }

    const_iterator begin() const
    {
      return _data;
    }

    const_iterator end() const
    {
      return _data + _size;
    }

  protected:
    void ensureCapacity(int new_len)
    {
      if (new_len <= _capacity)
        return;

      T *old_data = _data;
      _capacity = new_len + 128;
      _data = new T[_capacity];

      if (old_data)
      {
        // Copy old data
        for (int i = 0; i < _size; i++)
          _data[i] = old_data[i];
        delete [] old_data;
      }
    }
};

}  // Namespace Common

typedef Common::Array<int>   IntArray;
typedef Common::Array<bool>  BoolArray;
typedef Common::Array<uInt8> ByteArray;

#endif
