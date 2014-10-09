#include "ImagineSound.hh"
#include <emuframework/EmuSystem.hh>

void ImagineSound::open()
{
	myRegWriteQueue.clear();
	myTIASound.reset();

	// Now initialize the TIASound object which will actually generate sound
	myTIASound.outputFrequency(EmuSystem::pcmFormat.rate);
	myTIASound.channels(soundChannels, 0);

	myLastRegisterSetCycle = 0;
}

void ImagineSound::processAudio(Int16* stream, uint length)
{
	const uint channels = soundChannels;
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
				myTIASound.process(stream + ((uInt32)position * channels),
						length - (uInt32)position);
				info.delta -= duration;
				break;
			}
		}
	}
}

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
