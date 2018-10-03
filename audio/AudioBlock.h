/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef AUDIOBLOCK_H
#define AUDIOBLOCK_H

#include <cstdint>
#include <cmath>
#include <cassert>
#include <memory>
#include <limits>


/// A no-frills class for manipulating audio buffers.

template <class T>
class AudioBlock
{
 public:

    /// Construct an empty audio block.
    AudioBlock();

    /// Construct an audio block with the given parameters.
    /// @param nsamples Maximum size (capacity) of the audio block.
    /// @param sampleRate Sampling frequency of the audio.
    /// @param nchans Number of audio channels.
    /// @param initSize Set the initial size of the block. A negative value means
    /// the block has an initial size equal to the value of the nsamples parameter.
    /// The block's audio data is initialized to zero (samples).
    AudioBlock(size_t nsamples, float sampleRate, size_t nchans, int initSize=-1);

    /// Copy c-tor.
    AudioBlock(const AudioBlock<T> &block);

    /// D-tor.
    ~AudioBlock() = default;

    /// Assignment operator.
    AudioBlock<T>& operator=(AudioBlock<T> block);

    /// Sample access.
    T& operator[](size_t i);

    /// Create an audio block with the given parameters.
    /// @param nsamples Maximum size (capacity) of the audio block.
    /// @param sampleRate Sampling frequency of the audio.
    /// @param nchans Number of audio channels.
    /// @param initSize Initial size of the block (default is nsamples)
    void Create(size_t nsamples, float sampleRate, size_t nchans, int initSize=-1);

    /// Return the maximum size (capacity) of the audio block in samples (all channels)
    size_t  Capacity() const;
    /// Size of the available data in the audio block (in samples, all channels)
    size_t  Size() const;
    /// Size of the available data in the audio block (in bytes)
    size_t  SizeInBytes() const;
    /// Sampling frequency of the audio block
    float   SampleRate() const;
    /// Number of audio channels
    size_t  Channels() const;
    /// Audio block's sample resolution
    size_t  BytesPerSample() const;
    /// Duration of the available data in the audio block (in seconds)
    float   Duration() const;
    /// Duration of the audio block (in seconds)
    float   MaxDuration();
    /// Check whether the audio block is null (no data)
    bool    IsNull() const;
    /// Get normalization factor
    float   NormFactor() const;

    int32_t ID() const;
    void    SetID(int32_t id);
    int64_t Timestamp() const;
    void    SetTimestamp(int64_t tstamp);
    void    SetChannels(int nchans);

    /// Resize the block. This method does not perform any reallocation, it merely
    /// sets the amount of data to be considered in the block (available data).
    /// The method also changes the duration of the block.
    /// @param newsize The new block size in samples. If this value exceeds the
    /// capacity then it is truncated.
    void    Resize(size_t newsize);

    /// Returns the pointer to the raw audio data.
    T*       Data();
    const T* Data() const;

    /// Set the block's audio data. The data in the given buffer is copied in the internal
    /// buffer. Note that an audio block doesn't resize its buffer when written to, so
    /// writing more data than its capacity will result in the data being truncated.
    /// @return the number of written samples.
    size_t  SetData(const T *data, size_t nsamples);

    /// This method returns a normalized version of this audio block in the range [-1,1].
    /// @param nblock An audio block of float samples in which the normalized version
    /// of this audio block is computed. It must have the same size of this block, and
    /// the same sample rate and number of channels.
    void Normalize(AudioBlock<float> &nblock);

    /// Mix this audio block to the given one
    void MixTo(AudioBlock<T> &block);

    /// Apply the given gain to this audio block
    void ApplyGain(float gain);

    /// This method appends the (available) data of the given block to this block's available
    /// data. It does not reallocate this block's buffer if there is not enough space
    /// to append all the data, in which case a truncation occurs.
    AudioBlock<T>& Append(AudioBlock<T> &block);

    /// Append raw audio data to this block. See above for details.
    AudioBlock<T>& Append(const T *data, size_t nsamples);

    /// Get a sub-block
    void GetSubBlock(size_t start, size_t size, AudioBlock<T> &block);
	
    /// Get the RMS power of the block
    float GetPower() const;


 private:

    std::unique_ptr<T[]> mData;
	
    size_t   mCapacity    {0};    ///< Maximum size (fixed) of the audio block.
    float    mDuration    {0};    ///< Time lenght of the available data (seconds)
    size_t   mSize        {0};    ///< Amount of data available in the block for processing.
    float    mSampleRate  {0};    ///< The audio's sampling frequency
    size_t   mChannels    {0};    ///< The number of audio channels
    int32_t  mID          {0};    ///< Audio block identifier (whatever it means)
    uint64_t mTimestamp   {0};    ///< Timestamp [ms] (i.e. the time at which the block was read in the stream)
    float    mNormFactor  {1.f};  ///< Normalization factor used to normalize the audio in the range [-1,1].


    /// Compute the factor to be used for normalizing this block in the range [-1,1].
    /// The value depends on the current block's sample format and is computed when the
    /// block is created.
    void ComputeNormalizationFactor();
    void DoAppend(const T* data, size_t nsamples);
    void Swap(AudioBlock<T> &b1, AudioBlock<T> &b2);

};


// -----------------------------------------------------------------------------
//                              Implementation
// -----------------------------------------------------------------------------


template <class T>
AudioBlock<T>::AudioBlock()
{
    ComputeNormalizationFactor();
}


template <class T>
AudioBlock<T>::AudioBlock(size_t nsamples,
                          float sampleRate, 
                          size_t nchans, 
                          int initSize)
{
    Create(nsamples, sampleRate, nchans, initSize);
    ComputeNormalizationFactor();
}


template <class T>
AudioBlock<T>::AudioBlock(const AudioBlock<T> &block) :
    mCapacity   (block.mCapacity),
    mDuration   (block.mDuration),
    mSize       (block.mSize),
    mSampleRate (block.mSampleRate),
    mChannels   (block.mChannels),
    mData       (block.mCapacity ? new T[block.mCapacity] : nullptr),
    mID         (0),
    mTimestamp  (block.mTimestamp),
    mNormFactor (block.mNormFactor)
{
    std::copy(block.Data(),
              block.Data() + mCapacity, 
              this->Data());
}


template <class T>
inline void AudioBlock<T>::Create(size_t nsamples,
                                  float sampleRate, 
                                  size_t nchans, 
                                  int initSize)
{
    assert(nsamples > 0);
    assert(sampleRate > 0);
    assert(nchans > 0);
    assert(!mData);

    if(mData) return;

    // The number of samples should always be an integral multiple of the
    // number of channels.
    size_t dn = nsamples % nchans;
    nsamples = dn ? nsamples - dn + nchans : nsamples;

    mCapacity = nsamples;
    mSampleRate = sampleRate;
    mChannels = nchans;

    mSize = (initSize < 0) ? nsamples : initSize;

    if(mSize > mCapacity)
       mSize = mCapacity;

    mDuration = mSize / (nchans * sampleRate);
    mData.reset( new T[mCapacity] );
    std::fill(this->Data(), this->Data() + mCapacity, (T)0);
}


template <class T>
inline void AudioBlock<T>::Swap(AudioBlock<T> &b1, AudioBlock<T> &b2)
{
    std::swap(b1.mCapacity, b2.mCapacity);
    std::swap(b1.mDuration, b2.mDuration);
    std::swap(b1.mSize, b2.mSize);
    std::swap(b1.mSampleRate, b2.mSampleRate);
    std::swap(b1.mChannels, b2.mChannels);
    std::swap(b1.mData, b2.mData);
    std::swap(b1.mTimestamp, b2.mTimestamp);
    std::swap(b1.mNormFactor, b2.mNormFactor);
}

template <class T>
inline AudioBlock<T>& AudioBlock<T>::operator=(AudioBlock<T> block)
{
    Swap(*this, block);
    return (*this);
}


template <class T>
inline T& AudioBlock<T>::operator[](size_t i)
{ return mData[i]; }


template <class T>
inline size_t  AudioBlock<T>::Capacity() const
{ return mCapacity; }

template <class T>
inline size_t  AudioBlock<T>::Size() const
{ return mSize; }

template <class T>
inline size_t  AudioBlock<T>::SizeInBytes() const
{ return mSize * sizeof(T); }

template <class T>
inline float   AudioBlock<T>::Duration() const
{ return mDuration; }

template <class T>
inline float   AudioBlock<T>::MaxDuration()
{ return IsNull() ? 0 : mCapacity/(mChannels*mSampleRate); }

template <class T>
inline float   AudioBlock<T>::SampleRate() const
{ return mSampleRate; }

template <class T>
inline size_t  AudioBlock<T>::Channels() const
{ return mChannels; }

template <class T>
inline size_t  AudioBlock<T>::BytesPerSample() const
{ return sizeof(T); }

template <class T>
inline bool    AudioBlock<T>::IsNull() const
{ return (this->Data() == nullptr); }

template <class T>
inline int32_t AudioBlock<T>::ID() const
{ return mID; }

template <class T>
inline int64_t AudioBlock<T>::Timestamp() const
{ return mTimestamp; }

template <class T>
inline float   AudioBlock<T>::NormFactor() const
{ return mNormFactor; }

template <class T>
inline void    AudioBlock<T>::SetID(int32_t id)
{ mID = id; }

template <class T>
inline void    AudioBlock<T>::SetTimestamp(int64_t tstamp)
{ mTimestamp = tstamp; }

template <class T>
inline void    AudioBlock<T>::SetChannels(int nchans)
{ mChannels = nchans; }


template <class T>
inline void    AudioBlock<T>::Resize(size_t newsize)
{
    assert(!IsNull());
    assert(newsize >= 0);

    if(newsize > mCapacity)
       newsize = mCapacity;

    mSize = newsize;
    mDuration = mSize / (mChannels * mSampleRate);
}


template <class T>
inline T*  AudioBlock<T>::Data()
{ return mData.get(); }


template <class T>
inline const T*  AudioBlock<T>::Data() const
{ return mData.get(); }


template <class T>
inline size_t  AudioBlock<T>::SetData(const T* data, size_t nsamples)
{
    assert(!IsNull());

    if(nsamples != mSize)
       Resize(nsamples);

    std::copy(data, data + mSize, this->Data());

    return mSize;
}


template <class T>
inline void AudioBlock<T>::Normalize(AudioBlock<float> &nblock)
{
    assert(!IsNull() && !nblock.IsNull());

    // Resize the output block if sizes mismatch
    if(nblock.Size() != this->mSize)
       nblock.Resize( this->mSize );

    for(size_t i=0; i<mSize; i++)
        nblock[i] = mData[i] / mNormFactor;
}


template <class T>
inline void AudioBlock<T>::ComputeNormalizationFactor()
{
    mNormFactor = pow(2.0f, static_cast<int>(sizeof(T)*8-1));
}


template <>
inline void AudioBlock<float>::ComputeNormalizationFactor(){}


template<typename T>
inline void AudioBlock<T>::MixTo(AudioBlock<T> &block)
{
    assert(!IsNull() && !block.IsNull());
    assert(this->mSampleRate == block.mSampleRate);
    assert(this->mChannels == block.mChannels);

    size_t mixedSamples = std::min(mSize, block.mSize);

    for(size_t i=0; i<mixedSamples; i++)
        mData[i] = static_cast<T>( (static_cast<float>(mData[i]) +
                   static_cast<float>(block.mData[i])) / 2.f);
}


template<typename T>
inline void AudioBlock<T>::ApplyGain(float gain)
{
    assert(gain>=0);
	
    T val;
    T vmax = std::numeric_limits<T>::max();
    T vmin = std::numeric_limits<T>::min();

    for(size_t i=0; i<mSize; i++)
    {
        val = mData[i] * gain;
        val = val>vmax?vmax:val;
        val = val<vmin?vmin:val;
        mData[i] = val;
    }
}


template<>
inline void AudioBlock<float>::ApplyGain(float gain)
{
    assert(gain>=0);

    float val;
    for(size_t i=0; i<mSize; i++)
    {
        val = mData[i] * gain;
        val = val>1.0f?1.0f:val;
        val = val<-1.0f?-1.0f:val;
        mData[i] = val;
    }
}


template <class T>
inline AudioBlock<T>& AudioBlock<T>::Append(AudioBlock<T> &block)
{
    assert(!IsNull() && !block.IsNull());
    assert(block.mChannels == this->mChannels);
    assert(block.mSampleRate == this->mSampleRate);

    // nothing to append? just return
    if(block.IsNull() || block.Size() == 0)
       return *this;

    DoAppend(block.Data(), block.Size());
    return *this;
}


template <class T>
inline AudioBlock<T>& AudioBlock<T>::Append(const T* data, size_t nsamples)
{
    // nothing to append? just return
    if(data == nullptr || nsamples == 0)
       return *this;

    DoAppend(data, nsamples);
    return *this;
}


template <class T>
inline void AudioBlock<T>::DoAppend(const T *data, size_t nsamples)
{
    // compute remaining space in the buffer
    size_t available = mCapacity - mSize;

    if(available > 0)
    {
        // compute copyable data
       size_t copyable = (available >= nsamples) ? nsamples : available;

       // appended data should always be an integer multiple of the # of channels.
       assert(copyable % mChannels == 0);

       std::copy(data, data + copyable, this->Data() + mSize);

       // resize block
       Resize(mSize + copyable);
    }
}


template <class T>
inline void AudioBlock<T>::GetSubBlock(size_t start,
                                       size_t size, 
                                       AudioBlock<T> &block)
{
//    assert(!IsNull() && !block.IsNull());
//    assert(0<=start && start<mSize);
//    assert(size>=0);

    if( (IsNull() || block.IsNull()) ||
        !(0<=start && start<mSize) ||
        !(size>=0))
    {
        block.Resize(0);
        return;
    }

    size_t sbsize = std::min<size_t>(size, mSize - start);

    std::copy(this->Data() + start,
              this->Data() + start + sbsize,
              block.Data());

    block.Resize(sbsize);
}

template <class T>
inline float AudioBlock<T>::GetPower() const
{
    float Etot = 0;

    if(mSize == 0) return 0;

    for(size_t i=0; i<mSize; i++)
        Etot += mData[i] * mData[i];

    return Etot / mSize;
}

#endif // AUDIOBLOCK_H
