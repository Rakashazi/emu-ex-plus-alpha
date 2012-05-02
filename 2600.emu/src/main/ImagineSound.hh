#pragma once

#include <stella/emucore/TIASnd.hxx>

// Based on the Stella SDL sound class

class ImagineSound : public Sound
{
	TIASound myTIASound;

	// Indicates the cycle when a sound register was last set
	Int32 myLastRegisterSetCycle;

	// Struct to hold information regarding a TIA sound register write
	struct RegWrite
	{
		uInt16 addr;
		uInt8 value;
		SysDDec delta;
	};

	class RegWriteQueue
	{
	public:
		/**
		Create a new queue instance with the specified initial
		capacity.  If the queue ever reaches its capacity then it will
		automatically increase its size.
		*/
		RegWriteQueue(uInt32 capacity = 512);

		/**
		Destroy this queue instance.
		*/
		virtual ~RegWriteQueue();

		public:
		/**
		Clear any items stored in the queue.
		*/
		void clear();

		/**
		Dequeue the first object in the queue.
		*/
		void dequeue();

		/**
		Return the duration of all the items in the queue.
		*/
		SysDDec duration();

		/**
		Enqueue the specified object.
		*/
		void enqueue(const RegWrite& info);

		/**
		Return the item at the front on the queue.

		@return The item at the front of the queue.
		*/
		RegWrite& front();

		/**
		Answers the number of items currently in the queue.

		@return The number of items in the queue.
		*/
		uInt32 size() const;

	private:
		// Increase the size of the queue
		void grow();

	private:
		uInt32 myCapacity;
		RegWrite* myBuffer;
		uInt32 mySize;
		uInt32 myHead;
		uInt32 myTail;
	};

	RegWriteQueue myRegWriteQueue;

  public:
	ImagineSound(OSystem* osystem) : Sound(osystem) { }

    void setEnabled(bool enable) { }

    void adjustCycleCounter(Int32 amount)
    {
    	//logMsg("adjustCycleCounter, %d", amount);
    	myLastRegisterSetCycle += amount;
    }

    void setChannels(uInt32 channels) { /*logMsg("setChannels, %d", channels);*/ }

    void setFrameRate(float framerate)
    {
    	//logMsg("setFrameRate, %f", framerate);
    }

    void open()
	{
    	myRegWriteQueue.clear();
    	myTIASound.reset();

		// Now initialize the TIASound object which will actually generate sound
		myTIASound.outputFrequency(EmuSystem::pcmFormat.rate);
		//myTIASound.tiaFrequency(tiafreq);
		//myTIASound.channels(myHardwareSpec.channels);
		myTIASound.clipVolume(0);

		myLastRegisterSetCycle = 0;
	}

    TIASound &tiaSound()
    {
    	return myTIASound;
    }

    void close() { /*logMsg("close");*/ }

    bool isSuccessfullyInitialized() const { return 1; }

    void mute(bool state) { }

    void reset()
    {
		myLastRegisterSetCycle = 0;
		myTIASound.reset();
		myRegWriteQueue.clear();
    }

    void set(uInt16 addr, uInt8 value, Int32 cycle)
    {
		// First, calculate how many seconds would have past since the last
		// register write on a real 2600
    	SysDDec delta = (((SysDDec)(cycle - myLastRegisterSetCycle)) / (1193191.66666667));

		// Now, adjust the time based on the frame rate the user has selected. For
		// the sound to "scale" correctly, we have to know the games real frame
		// rate (e.g., 50 or 60) and the currently emulated frame rate. We use these
		// values to "scale" the time before the register change occurs.
		RegWrite info;
		info.addr = addr;
		info.value = value;
		info.delta = delta;
		myRegWriteQueue.enqueue(info);

		// Update last cycle counter to the current cycle
		myLastRegisterSetCycle = cycle;
    }

    void setVolume(Int32 percent) { }

    void adjustVolume(Int8 direction) { }

    bool save(Serializer&) const { return 1; }

    bool load(Serializer&) { return 1; }

    std::string name() const { return string(""); }

    void processAudio(TIASound::Sample* stream, uint length)
    {
    	const uint channels = 1;
    	// If there are excessive items on the queue then we'll remove some
    	//logMsg("sound duration %f", myRegWriteQueue.duration());
    	SysDDec streamLengthInSecs = (SysDDec)length/(SysDDec)EmuSystem::pcmFormat.rate;
    	SysDDec excessStreamSecs = myRegWriteQueue.duration() - streamLengthInSecs;
    	if(excessStreamSecs > 0.0)
    	{
    		SysDDec removed = 0.0;
    		while(removed < excessStreamSecs)
    		{
    			RegWrite& info = myRegWriteQueue.front();
    			removed += info.delta;
    			myTIASound.set(info.addr, info.value);
    			myRegWriteQueue.dequeue();
    		}
    	}

    	SysDDec position = 0.0;
    	SysDDec remaining = length;

    	while(remaining > 0.0)
    	{
			if(myRegWriteQueue.size() == 0)
			{
				// There are no more pending TIA sound register updates so we'll
				// use the current settings to finish filling the sound fragment
				//    myTIASound.process(stream + (uInt32)position, length - (uInt32)position);
				myTIASound.process(stream + ((uInt32)position * channels),
				length - (uInt32)position);

				// Since we had to fill the fragment we'll reset the cycle counter
				// to zero.  NOTE: This isn't 100% correct, however, it'll do for
				// now.  We should really remember the overrun and remove it from
				// the delta of the next write.
				myLastRegisterSetCycle = 0;
				break;
			}
			else
			{
				// There are pending TIA sound register updates so we need to
				// update the sound buffer to the point of the next register update
				RegWrite& info = myRegWriteQueue.front();

				// How long will the remaining samples in the fragment take to play
				SysDDec duration = remaining / (SysDDec)EmuSystem::pcmFormat.rate;

				// Does the register update occur before the end of the fragment?
				if(info.delta <= duration)
				{
					// If the register update time hasn't already passed then
					// process samples upto the point where it should occur
					if(info.delta > 0.0)
					{
						// Process the fragment upto the next TIA register write.  We
						// round the count passed to process up if needed.
						SysDDec samples = (EmuSystem::pcmFormat.rate * info.delta);
						//        myTIASound.process(stream + (uInt32)position, (uInt32)samples +
						//            (uInt32)(position + samples) -
						//            ((uInt32)position + (uInt32)samples));
						myTIASound.process(stream + ((uInt32)position * channels),
						(uInt32)samples + (uInt32)(position + samples) -
						((uInt32)position + (uInt32)samples));

						position += samples;
						remaining -= samples;
					}
					myTIASound.set(info.addr, info.value);
					myRegWriteQueue.dequeue();
				}
				else
				{
					// The next register update occurs in the next fragment so finish
					// this fragment with the current TIA settings and reduce the register
					// update delay by the corresponding amount of time
					//      myTIASound.process(stream + (uInt32)position, length - (uInt32)position);
					myTIASound.process(stream + ((uInt32)position * channels),
					length - (uInt32)position);
					info.delta -= duration;
					break;
				}
			}
    	}
    }
};


ImagineSound::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myCapacity(capacity),
    myBuffer(0),
    mySize(0),
    myHead(0),
    myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImagineSound::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImagineSound::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImagineSound::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SysDDec ImagineSound::RegWriteQueue::duration()
{
	SysDDec duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ImagineSound::RegWriteQueue::enqueue(const RegWrite& info)
{
  // If an attempt is made to enqueue more than the queue can hold then
  // we'll enlarge the queue's capacity.
  if(mySize == myCapacity)
  {
	  logMsg("queue over capacity");
	  return;
    //grow();
  }

  myBuffer[myTail] = info;
  myTail = (myTail + 1) % myCapacity;
  ++mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ImagineSound::RegWrite& ImagineSound::RegWriteQueue::front()
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 ImagineSound::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*void ImagineSound::RegWriteQueue::grow()
{
  RegWrite* buffer = new RegWrite[myCapacity * 2];
  for(uInt32 i = 0; i < mySize; ++i)
  {
    buffer[i] = myBuffer[(myHead + i) % myCapacity];
  }
  myHead = 0;
  myTail = mySize;
  myCapacity = myCapacity * 2;
  delete[] myBuffer;
  myBuffer = buffer;
}*/
