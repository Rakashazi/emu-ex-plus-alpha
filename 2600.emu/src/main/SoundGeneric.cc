/*  This file is part of 2600.emu.

	2600.emu is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	2600.emu is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with 2600.emu.  If not, see <http://www.gnu.org/licenses/> */

#include "SoundGeneric.hh"
#include <stella/emucore/Settings.hxx>
// TODO: Some Stella types collide with MacTypes.h
#define BytePtr BytePtrMac
#define Debugger DebuggerMac
#include <emuframework/EmuSystem.hh>
#include <imagine/logger/logger.h>
#undef BytePtr
#undef Debugger

void SoundGeneric::open()
{
	myRegWriteQueue.clear();
	myTIASound.reset();
	myTIASound.channels(soundChannels, 0);
	myLastRegisterSetCycle = 0;
	frames = 0;
}

void SoundGeneric::setFrameTime(OSystem &osystem, unsigned int soundRate, double frameTime)
{
	tiaSoundRate = std::round(soundRate * (osystem.settings().getFloat("framerate") * frameTime));
	framesPerVideoFrame = soundRate * frameTime;
	myTIASound.outputFrequency(tiaSoundRate);
	logMsg("set sound rate %.2f", tiaSoundRate);
}

unsigned int SoundGeneric::processAudio(Int16* stream, unsigned int maxLength)
{
	const uint channels = soundChannels;
	frames += framesPerVideoFrame;
	double lengthDecimal = 0;
	frames = std::modf(frames, &lengthDecimal);
	uint length = lengthDecimal;
	assert(length <= maxLength);
	//logMsg("sound duration %f, %d frames", myRegWriteQueue.duration(), length);
	// If there are excessive items on the queue then we'll remove some
	double streamLengthInSecs = (double)length/(double)tiaSoundRate;
	double excessStreamSecs = myRegWriteQueue.duration() - streamLengthInSecs;
	if(excessStreamSecs > 0.0)
	{
		//logMsg("excess duration %f", excessStreamSecs);
		double removed = 0.0;
		while(removed < excessStreamSecs)
		{
			RegWrite& info = myRegWriteQueue.front();
			removed += info.delta;
			myTIASound.set(info.addr, info.value);
			myRegWriteQueue.dequeue();
		}
	}

	double position = 0.0;
	double remaining = length;

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
			double duration = remaining / (double)tiaSoundRate;

			// Does the register update occur before the end of the fragment?
			if(info.delta <= duration)
			{
				// If the register update time hasn't already passed then
				// process samples upto the point where it should occur
				if(info.delta > 0.0)
				{
					// Process the fragment upto the next TIA register write.  We
					// round the count passed to process up if needed.
					double samples = (tiaSoundRate * info.delta);
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
	return length;
}

void SoundGeneric::adjustCycleCounter(Int32 amount)
{
	myLastRegisterSetCycle += amount;
}

void SoundGeneric::reset()
{
	myLastRegisterSetCycle = 0;
	myTIASound.reset();
	myRegWriteQueue.clear();
}

void SoundGeneric::set(uInt16 addr, uInt8 value, Int32 cycle)
{
	// First, calculate how many seconds would have past since the last
	// register write on a real 2600
	double delta = (((double)(cycle - myLastRegisterSetCycle)) / (1193191.66666667));

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

bool SoundGeneric::save(Serializer& out) const
{
	try
	{
		out.putString(name());

		uInt8 reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0, reg6 = 0;

		reg1 = myTIASound.get(0x15);
		reg2 = myTIASound.get(0x16);
		reg3 = myTIASound.get(0x17);
		reg4 = myTIASound.get(0x18);
		reg5 = myTIASound.get(0x19);
		reg6 = myTIASound.get(0x1a);

		out.putByte(reg1);
		out.putByte(reg2);
		out.putByte(reg3);
		out.putByte(reg4);
		out.putByte(reg5);
		out.putByte(reg6);

		out.putInt(myLastRegisterSetCycle);
	}
	catch(...)
	{
		myOSystem.logMessage("ERROR: SoundGeneric::save", 0);
		return false;
	}

	return true;
}

bool SoundGeneric::load(Serializer& in)
{
	try
	{
		if(in.getString() != name())
			return false;

		uInt8 reg1 = in.getByte(),
			reg2 = in.getByte(),
			reg3 = in.getByte(),
			reg4 = in.getByte(),
			reg5 = in.getByte(),
			reg6 = in.getByte();

		myLastRegisterSetCycle = (Int32) in.getInt();

		myRegWriteQueue.clear();
		myTIASound.set(0x15, reg1);
		myTIASound.set(0x16, reg2);
		myTIASound.set(0x17, reg3);
		myTIASound.set(0x18, reg4);
		myTIASound.set(0x19, reg5);
		myTIASound.set(0x1a, reg6);
	}
	catch(...)
	{
		myOSystem.logMessage("ERROR: SoundGeneric::load", 0);
		return false;
	}

	return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundGeneric::RegWriteQueue::RegWriteQueue(uInt32 capacity)
  : myCapacity(capacity),
    myBuffer(0),
    mySize(0),
    myHead(0),
    myTail(0)
{
  myBuffer = new RegWrite[myCapacity];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundGeneric::RegWriteQueue::~RegWriteQueue()
{
  delete[] myBuffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundGeneric::RegWriteQueue::clear()
{
  myHead = myTail = mySize = 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundGeneric::RegWriteQueue::dequeue()
{
  if(mySize > 0)
  {
    myHead = (myHead + 1) % myCapacity;
    --mySize;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
double SoundGeneric::RegWriteQueue::duration()
{
	double duration = 0.0;
  for(uInt32 i = 0; i < mySize; ++i)
  {
    duration += myBuffer[(myHead + i) % myCapacity].delta;
  }
  return duration;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundGeneric::RegWriteQueue::enqueue(const RegWrite& info)
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
SoundGeneric::RegWrite& SoundGeneric::RegWriteQueue::front()
{
  assert(mySize != 0);
  return myBuffer[myHead];
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SoundGeneric::RegWriteQueue::size() const
{
  return mySize;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*void SoundGeneric::RegWriteQueue::grow()
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
