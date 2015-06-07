#pragma once

#include <stella/emucore/TIASnd.hxx>
#include <stella/emucore/Sound.hxx>
#include "OSystem.hxx"

// Based on the Stella SDL sound class

class SoundGeneric : public Sound
{
	TIASound myTIASound;

	// Indicates the cycle when a sound register was last set
	Int32 myLastRegisterSetCycle;

	// Struct to hold information regarding a TIA sound register write
	struct RegWrite
	{
		uInt16 addr;
		uInt8 value;
		double delta;
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
		double duration();

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

	double tiaSoundRate = 0;
	double framesPerVideoFrame = 0;
	double frames = 0;

public:
	SoundGeneric(OSystem& osystem) : Sound(osystem) {}

	void setEnabled(bool enable) {}

	void adjustCycleCounter(Int32 amount);

	void setChannels(uInt32 channels) {}

	void setFrameRate(float framerate) {}

	void setFrameTime(OSystem &osystem, unsigned int soundRate, double frameTime);

	void open();

	TIASound &tiaSound()
	{
		return myTIASound;
	}

	void close() {}

	void mute(bool state) {}

	void reset();

	void set(uInt16 addr, uInt8 value, Int32 cycle);

	void setVolume(Int32 percent) {}

	void adjustVolume(Int8 direction) {}

	bool save(Serializer&) const;

	bool load(Serializer&);

	std::string name() const { return "TIASound"; }

	unsigned int processAudio(Int16* stream, unsigned int maxLength);
};

static const unsigned int soundChannels = 1;
