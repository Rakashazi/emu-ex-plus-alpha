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
// Copyright (c) 1995-2018 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef TIA_DELAY_QUEUE
#define TIA_DELAY_QUEUE

#include "Serializable.hxx"
#include "bspf.hxx"
#include "smartmod.hxx"
#include "DelayQueueMember.hxx"

template<unsigned length, unsigned capacity>
class DelayQueueIteratorImpl;

template<unsigned length, unsigned capacity>
class DelayQueue : public Serializable
{
  public:
    friend DelayQueueIteratorImpl<length, capacity>;

  public:
    DelayQueue();

  public:

    void push(uInt8 address, uInt8 value, uInt8 delay);

    void reset();

    template<class T> void execute(T executor);

    /**
      Serializable methods (see that class for more information).
    */
    bool save(Serializer& out) const override;
    bool load(Serializer& in) override;
    string name() const override;

  private:
    DelayQueueMember<capacity> myMembers[length];
    uInt8 myIndex;
    uInt8 myIndices[0xFF];

  private:
    DelayQueue(const DelayQueue&) = delete;
    DelayQueue(DelayQueue&&) = delete;
    DelayQueue& operator=(const DelayQueue&) = delete;
    DelayQueue& operator=(DelayQueue&&) = delete;
};

// ############################################################################
// Implementation
// ############################################################################

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
DelayQueue<length, capacity>::DelayQueue()
  : myIndex(0)
{
  memset(myIndices, 0xFF, 0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
void DelayQueue<length, capacity>::push(uInt8 address, uInt8 value, uInt8 delay)
{
  if (delay >= length)
    throw runtime_error("delay exceeds queue length");

  uInt8 currentIndex = myIndices[address];

  if (currentIndex < 0xFF)
    myMembers[currentIndex].remove(address);

  uInt8 index = smartmod<length>(myIndex + delay);
  myMembers[index].push(address, value);

  myIndices[address] = index;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
void DelayQueue<length, capacity>::reset()
{
  for (uInt8 i = 0; i < length; i++)
    myMembers[i].clear();

  myIndex = 0;
  memset(myIndices, 0xFF, 0xFF);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
template<class T>
void DelayQueue<length, capacity>::execute(T executor)
{
  DelayQueueMember<capacity>& currentMember = myMembers[myIndex];

  for (uInt8 i = 0; i < currentMember.mySize; i++) {
    executor(currentMember.myEntries[i].address, currentMember.myEntries[i].value);
    myIndices[currentMember.myEntries[i].address] = 0xFF;
  }

  currentMember.clear();

  myIndex = smartmod<length>(myIndex + 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueue<length, capacity>::save(Serializer& out) const
{
  try
  {
    out.putInt(length);

    for (uInt8 i = 0; i < length; i++)
      myMembers[i].save(out);

    out.putByte(myIndex);
    out.putByteArray(myIndices, 0xFF);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::save" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
bool DelayQueue<length, capacity>::load(Serializer& in)
{
  try
  {
    if (in.getInt() != length) throw runtime_error("delay queue length mismatch");

    for (uInt8 i = 0; i < length; i++)
      myMembers[i].load(in);

    myIndex = in.getByte();
    in.getByteArray(myIndices, 0xFF);
  }
  catch(...)
  {
    cerr << "ERROR: TIA_DelayQueue::load" << endl;
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<unsigned length, unsigned capacity>
string DelayQueue<length, capacity>::name() const
{
  return "TIA_DelayQueue";
}

#endif //  TIA_DELAY_QUEUE
