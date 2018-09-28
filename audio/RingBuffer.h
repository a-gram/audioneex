/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <cmath>
#include <climits>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <memory>

#include "AudioBlock.h"


/// A naive ring buffer used by one producer (the audio provider)
/// and one consumer (the audio processor) with simple access control.

template<class T>
class RingBuffer
{
 public:

    RingBuffer() = default;

    // Copy c-tor
    RingBuffer(const RingBuffer<T>& rhs);

    // Create a ring buffer with 'size' empty (null) blocks
    RingBuffer(size_t size);

    /// Create a ring buffer with 'size' blocks with the given block's params
    RingBuffer(size_t size, AudioBlock<T> &block);

    ~RingBuffer() = default;

    /// Set the ring with 'size' empty (null) blocks
    void  Set(size_t size);

    /// Set the ring with 'size' blocks with the given block's parameters
    void  Set(size_t size, AudioBlock<T> &block);

    /// Return the size of the ring. This is the max number of items the ring
    /// can hold (capacity).
    size_t Size();

    /// Return true if the ring is full, false otherwise.
    bool  IsFull();

    /// Return true if the ring is empty, false otherwise.
    bool  IsEmpty();

    /// Check whether the ring contains blocks.
    bool  IsNull();

    /// Return the available items in the ring.
    uint64_t Available();

    /// Get the head block .
    /// @return A pointer to the free (consumed) block that will be used by the
    /// next push operation to the consumer, or null if the buffer is full.
    AudioBlock<T>* GetHead();

    /// Push an element to the consumer. This method returns a pointer to the
    /// pushed element in case the buffer is not full or a null pointer in case
    /// the buffer is full (overflow).
    AudioBlock<T>* Push();

    /// Get an element from the ring. This method returns a pointer to the
    /// available element in case the ring is not empty or a null pointer in
    /// case the ring is empty (starvation).
    AudioBlock<T>* Pull();

    /// Discard all elements currently in the ring
    void Reset();

    /// Assignment operator
    RingBuffer<T>& operator=(RingBuffer<T> rhs);

    /// The swapping function
    void swap(RingBuffer<T>& b1, RingBuffer<T>& b2);

 private:

    std::unique_ptr<AudioBlock<T>[]> mBuffer;
	
    size_t         mSize         {0};
    uint64_t       mConsumed     {0};
    uint64_t       mProduced     {0};
    bool           mConsumeDone  {false};

};


// ----------------------------------------------------------------------------


template <class T>
inline RingBuffer<T>::RingBuffer(const RingBuffer<T>& rhs) :
    mSize        (rhs.mSize),
    mConsumed    (rhs.mConsumed),
    mProduced    (rhs.mProduced),
    mConsumeDone (rhs.mConsumeDone),
    mBuffer      (mSize ? new AudioBlock<T>[mSize] : nullptr)
{
    std::copy(rhs.mBuffer.get(),
              rhs.mBuffer.get() + mSize, 
              mBuffer.get());
}

template <class T>
inline RingBuffer<T>::RingBuffer(size_t size)
{
    Set(size);
}

template <class T>
inline RingBuffer<T>::RingBuffer(size_t size, AudioBlock<T> &block)
{
    Set(size, block);
}

template <class T>
void RingBuffer<T>::swap(RingBuffer<T>& b1, RingBuffer<T>& b2)
{
    std::swap(b1.mSize, b2.mSize);
    std::swap(b1.mBuffer, b2.mBuffer);
    std::swap(b1.mConsumed, b2.mConsumed);
    std::swap(b1.mProduced, b2.mProduced);
    std::swap(b1.mConsumeDone, b2.mConsumeDone);
}

template <class T>
RingBuffer<T>& RingBuffer<T>::operator=(RingBuffer<T> rhs)
{
    swap(*this, rhs);
}

template <class T>
inline void RingBuffer<T>::Set(size_t size)
{
    assert(size > 0);
    assert(mBuffer.get() == nullptr);

    mSize = size;
    mBuffer.reset(new AudioBlock<T>[size]);
}

template <class T>
inline void RingBuffer<T>::Set(size_t size, AudioBlock<T> &block)
{
    assert(size > 0);
    assert(mBuffer.get() == nullptr);

    mSize = size;
    mBuffer.reset(new AudioBlock<T>[size]);

    for(size_t i=0; i<size; i++)
        mBuffer[i] = block;
}

template <class T>
inline size_t RingBuffer<T>::Size()
{
    return mSize;
}

template <class T>
inline bool RingBuffer<T>::IsFull()
{
    return (mProduced - mConsumed) == mSize;
}

template <class T>
inline bool RingBuffer<T>::IsEmpty()
{
    return (mProduced - mConsumed) == 0;
}

template <class T>
inline uint64_t RingBuffer<T>::Available()
{
    return (mProduced - mConsumed);
}

template <class T>
inline bool RingBuffer<T>::IsNull()
{
    return mBuffer.get() == nullptr;
}

template <class T>
inline AudioBlock<T>* RingBuffer<T>::GetHead()
{
    return (IsFull() ? nullptr : &mBuffer[mProduced % mSize]);
}

template <class T>
inline AudioBlock<T>* RingBuffer<T>::Push()
{
    assert(mBuffer.get() != nullptr);
    if(IsFull()) return nullptr;
    AudioBlock<T>* ret = &mBuffer[mProduced % mSize];
    mProduced++;
    return ret;
}

template <class T>
inline AudioBlock<T>* RingBuffer<T>::Pull()
{
    assert(mBuffer.get() != nullptr);

    AudioBlock<T> *b = nullptr;

    // Consuming of the pulled elements is done after this
    // method returns, so we increase the counter the next
    // time we call this method to consume another element.
    if(mConsumeDone)
       mConsumed++;

    if(!IsEmpty())
    {
        b = &mBuffer[mConsumed % mSize];

        // Mark as consuming. The consumed counter will be incremented
        // above next time a new element is read (and the previous one
        // is supposed to have been consumed)
        mConsumeDone = true;
    }
    else
    {
        // Disable consume counting when buffer is empty
        mConsumeDone = false;
    }

    return b;
}

template <class T>
inline void RingBuffer<T>::Reset()
{
   mConsumed = 0;
   mProduced = 0;
   mConsumeDone = false;
}


#endif // RINGBUFFER_H
