#pragma once

#include <stella/emucore/TIASnd.hxx>
#include <stella/emucore/Sound.hxx>
#include <imagine/util/ansiTypes.h>

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

	void open();

	TIASound &tiaSound()
	{
		return myTIASound;
	}

	void close() { /*logMsg("close");*/ }

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

	void processAudio(Int16* stream, uint length);
};

static const uint soundChannels = 1;
