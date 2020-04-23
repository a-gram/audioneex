/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#ifndef AUDIOBUFFER_H
#define AUDIOBUFFER_H

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cassert>
#include <memory>
#include <limits>


/// A no-frills class for manipulating audio buffers.

template <class T>
class AudioBuffer
{
 public:

    /// Construct an empty audio buffer.
    AudioBuffer();

    /// Construct an audio buffer with the given parameters.
    /// @param nsamples Maximum size (capacity) of the audio buffer.
    /// @param sampleRate Sampling frequency of the audio.
    /// @param nchans Number of audio channels.
    /// @param initSize Set the initial size of the buffer. A negative value 
    /// means the buffer's initial size is equal to the nsamples parameter.
    /// The buffer's audio data is initialized to zero (samples).
    AudioBuffer(size_t nsamples, 
                float sampleRate, 
                size_t nchans, 
                int initSize=-1);

    /// Copy constructor.
    AudioBuffer(const AudioBuffer<T> &buffer);

    /// D-tor.
    ~AudioBuffer() = default;

    /// Assignment operator.
    AudioBuffer<T>& 
    operator=(AudioBuffer<T> buffer);

    /// Sample access.
    T& 
    operator[](size_t i);

    /// Create an audio buffer with the given parameters.
    /// @param nsamples Maximum size (capacity) of the audio buffer.
    /// @param sampleRate Sampling frequency of the audio.
    /// @param nchans Number of audio channels.
    /// @param initSize Initial size of the buffer (default is nsamples)
    void 
    Create(size_t nsamples, 
           float sampleRate, 
           size_t nchans, 
           int initSize=-1);

    /// Maximum allowed size of the audio buffer in samples (all channels)
    size_t  
    Capacity() const;
    
    /// Size of the available data in the audio buffer in samples (all channels)
    size_t 
    Size() const;
    
    /// Size of the available data in the audio buffer in bytes (all channels)
    size_t 
    SizeInBytes() const;
    
    /// Sampling frequency of the audio buffer
    float 
    SampleRate() const;
    
    /// Number of audio channels
    size_t
    Channels() const;
    
    /// Audio buffer's sample resolution
    size_t
    BytesPerSample() const;
    
    /// Duration of the available data in the audio buffer (in seconds)
    float
    Duration() const;
    
    /// Maximum allowed duration of the audio buffer (in seconds)
    float
    MaxDuration();
    
    /// Check whether the audio buffer is null (i.e. not allocated)
    bool
    IsNull() const;
    
    /// Get normalization factor
    float
    NormFactor() const;

    /// Get the buffer's identifier
    int32_t
    ID() const;
    
    /// Set the buffer's identifier
    void
    SetID(int32_t id);
    
    /// Get the buffer's timestamp
    int64_t
    Timestamp() const;
    
    /// Set the buffer's timestamp
    void
    SetTimestamp(int64_t tstamp);
    
    /// Set the buffer's number of channels
    void
    SetChannels(int nchans);

    /// Resize the buffer. This method does not perform any reallocation but 
    /// merely sets the amount of "available data" in the buffer. The method 
    /// also changes the duration of the buffer.
    /// @param newsize The new buffer size in samples.
    /// NOTE: if this value exceeds the capacity then it is truncated.
    void
    Resize(size_t newsize);

    /// Returns a raw pointer to the audio data.
    T*
    Data();
    
    /// Returns a const raw pointer to the audio data.
    const T*
    Data() const;

    /// Set the buffer's audio data. The data in the given buffer is copied in 
    /// this buffer. NOTE that an audio buffer doesn't do reallocations, so
    /// writing more data than its capacity will result in data truncation.
    /// @return the number of written samples.
    size_t
    SetData(const T *data, size_t nsamples);

    /// Return a normalized version of this buffer in the range [-1,1].
    /// @param nbuffer A buffer of float samples in which the normalized version
    /// of this buffer is computed. It must have the same size of this buffer,
    /// and the same sample rate and number of channels.
    void
    Normalize(AudioBuffer<float> &nbuffer);

    /// Mix this audio buffer with the given one
    void
    MixTo(AudioBuffer<T> &buffer);

    /// Apply the given gain to this audio buffer (no clipping checks performed)
    void
    ApplyGain(float gain);

    /// Append the (available) data of the given buffer to this buffer's. 
    /// It does not reallocate this buffer if there is not enough space
    /// to append all the data, in which case a truncation will occur.
    AudioBuffer<T>&
    Append(AudioBuffer<T> &buffer);
    
    /// Append audio data from an array to this buffer. See above for details.
    AudioBuffer<T>&
    Append(const T *data, size_t nsamples);

    /// Get a slice of this buffer
    void
    GetSlice(size_t start, 
             size_t size, 
             AudioBuffer<T> &buffer);
	
    /// Get the RMS power of this buffer
    float
    GetPower() const;


 private:

    /// The samples buffer
    std::unique_ptr<T[]> 
    mData;
	
    /// Maximum allowed size of the audio buffer (immutable).
    size_t
    mCapacity   {0};
    
    /// Time lenght of the available data (seconds)
    float
    mDuration   {0};
    
    /// Amount of data available in the buffer for processing.
    size_t
    mSize       {0};
    
    /// The audio sampling frequency
    float
    mSampleRate {0};
    
    /// The number of audio channels
    size_t
    mChannels   {0};
    
    /// Audio buffer identifier (whatever it means)
    int32_t
    mID         {0};
    
    /// Timestamp [ms]
    uint64_t
    mTimestamp  {0};
    
    /// Normalization factor used to normalize the audio in the range [-1,1].
    float
    mNormFactor {1.f};


    /// Compute the factor to normalize this buffer in the range [-1,1].
    /// The value depends on the current buffer's sample format and is computed 
    /// when the buffer is created.
    void
    ComputeNormalizationFactor();
    
    /// Helper append method
    void
    DoAppend(const T* data, size_t nsamples);
    
    /// Swap method
    void
    Swap(AudioBuffer<T> &b1, AudioBuffer<T> &b2);

};


// -----------------------------------------------------------------------------
//                              Implementation
// -----------------------------------------------------------------------------


template <class T>
AudioBuffer<T>::AudioBuffer()
{
    ComputeNormalizationFactor();
}


template <class T>
AudioBuffer<T>::AudioBuffer(size_t nsamples,
                            float sampleRate,
                            size_t nchans,
                            int initSize)
{
    Create(nsamples, sampleRate, nchans, initSize);
    ComputeNormalizationFactor();
}


template <class T>
AudioBuffer<T>::AudioBuffer(const AudioBuffer<T> &buffer) 
:
    mCapacity   (buffer.mCapacity),
    mDuration   (buffer.mDuration),
    mSize       (buffer.mSize),
    mSampleRate (buffer.mSampleRate),
    mChannels   (buffer.mChannels),
    mData       (buffer.mCapacity ? new T[buffer.mCapacity] : nullptr),
    mID         (0),
    mTimestamp  (buffer.mTimestamp),
    mNormFactor (buffer.mNormFactor)
{
    std::copy(buffer.Data(),
              buffer.Data() + mCapacity,
              this->Data());
}


template <class T>
inline void AudioBuffer<T>::Create(size_t nsamples, 
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
    std::fill(this->Data(),
              this->Data() + mCapacity, 
              static_cast<T>(0));
}


template <class T> inline
void 
AudioBuffer<T>::Swap(AudioBuffer<T> &b1, AudioBuffer<T> &b2)
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

template <class T> inline 
AudioBuffer<T>&
AudioBuffer<T>::operator=(AudioBuffer<T> buffer)
{
    Swap(*this, buffer);
    return (*this);
}


template <class T> inline 
T& 
AudioBuffer<T>::operator[](size_t i)
{ return mData[i]; }


template <class T> inline 
size_t
AudioBuffer<T>::Capacity() const
{ return mCapacity; }

template <class T> inline 
size_t
AudioBuffer<T>::Size() const
{ return mSize; }

template <class T> inline 
size_t
AudioBuffer<T>::SizeInBytes() const
{ return mSize * sizeof(T); }

template <class T> inline
float
AudioBuffer<T>::Duration() const
{ return mDuration; }

template <class T> inline 
float
AudioBuffer<T>::MaxDuration()
{ return IsNull() ? 0 : mCapacity/(mChannels*mSampleRate); }

template <class T> inline 
float 
AudioBuffer<T>::SampleRate() const
{ return mSampleRate; }

template <class T> inline 
size_t
AudioBuffer<T>::Channels() const
{ return mChannels; }

template <class T> inline 
size_t
AudioBuffer<T>::BytesPerSample() const
{ return sizeof(T); }

template <class T> inline 
bool
AudioBuffer<T>::IsNull() const
{ return (this->Data() == nullptr); }

template <class T> inline 
int32_t 
AudioBuffer<T>::ID() const
{ return mID; }

template <class T> inline 
int64_t 
AudioBuffer<T>::Timestamp() const
{ return mTimestamp; }

template <class T> inline 
float
AudioBuffer<T>::NormFactor() const
{ return mNormFactor; }

template <class T> inline 
void 
AudioBuffer<T>::SetID(int32_t id)
{ mID = id; }

template <class T> inline 
void 
AudioBuffer<T>::SetTimestamp(int64_t tstamp)
{ mTimestamp = tstamp; }

template <class T> inline 
void
AudioBuffer<T>::SetChannels(int nchans)
{ mChannels = nchans; }


template <class T> inline 
void
AudioBuffer<T>::Resize(size_t newsize)
{
    assert(!IsNull());
    assert(newsize >= 0);

    if(newsize > mCapacity)
       newsize = mCapacity;

    mSize = newsize;
    mDuration = mSize / (mChannels * mSampleRate);
}


template <class T> inline 
T*
AudioBuffer<T>::Data()
{ return mData.get(); }

template <class T> inline 
const T*
AudioBuffer<T>::Data() const
{ return mData.get(); }

template <class T> inline 
size_t
AudioBuffer<T>::SetData(const T* data, size_t nsamples)
{
    assert(!IsNull());

    if(nsamples != mSize)
       Resize(nsamples);

    std::copy(data, data + mSize, this->Data());

    return mSize;
}


template <class T> inline 
void 
AudioBuffer<T>::Normalize(AudioBuffer<float> &nbuffer)
{
    assert(!IsNull() && !nbuffer.IsNull());

    // Resize the output buffer if sizes mismatch
    if(nbuffer.Size() != this->mSize)
       nbuffer.Resize( this->mSize );

    for(size_t i=0; i<mSize; i++)
        nbuffer[i] = mData[i] / mNormFactor;
}


template <class T> inline 
void 
AudioBuffer<T>::ComputeNormalizationFactor()
{
    mNormFactor = pow(2.0f, static_cast<int>(sizeof(T)*8-1));
}


template <> inline 
void 
AudioBuffer<float>::ComputeNormalizationFactor(){}


template<class T> inline 
void 
AudioBuffer<T>::MixTo(AudioBuffer<T> &buffer)
{
    assert(!IsNull() && !buffer.IsNull());
    assert(this->mSampleRate == buffer.mSampleRate);
    assert(this->mChannels == buffer.mChannels);

    size_t mixedSamples = std::min(mSize, buffer.mSize);

    for(size_t i=0; i<mixedSamples; i++)
        mData[i] = static_cast<T>( (static_cast<float>(mData[i]) +
                   static_cast<float>(buffer.mData[i])) / 2.f);
}


template<class T> inline 
void 
AudioBuffer<T>::ApplyGain(float gain)
{
    assert(gain>=0);
	
    T val;
    T vmax = std::numeric_limits<T>::max();
    T vmin = std::numeric_limits<T>::min();

    for(size_t i=0; i<mSize; i++)
    {
	    val = mData[i] * gain;
	    val = val > vmax ? vmax : val;
	    val = val < vmin ? vmin : val;
	    mData[i] = val;
    }
}


template<> inline 
void 
AudioBuffer<float>::ApplyGain(float gain)
{
    assert(gain>=0);

    float val;
    for(size_t i=0; i<mSize; i++)
    {
	    val = mData[i] * gain;
	    val = val > 1.0f ? 1.0f : val;
	    val = val < -1.0f ? -1.0f : val;
	    mData[i] = val;
    }
}


template <class T> inline 
AudioBuffer<T>& 
AudioBuffer<T>::Append(AudioBuffer<T> &buffer)
{
    assert(!IsNull() && !buffer.IsNull());
    assert(buffer.mChannels == this->mChannels);
    assert(buffer.mSampleRate == this->mSampleRate);

    // nothing to append? just return
    if(buffer.IsNull() || buffer.Size() == 0)
       return *this;

    DoAppend(buffer.Data(), buffer.Size());
    return *this;
}


template <class T> inline 
AudioBuffer<T>& 
AudioBuffer<T>::Append(const T* data, size_t nsamples)
{
    // nothing to append? just return
    if(data == nullptr || nsamples == 0)
       return *this;

    DoAppend(data, nsamples);
    return *this;
}


template <class T> inline 
void 
AudioBuffer<T>::DoAppend(const T *data, size_t nsamples)
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

       // resize buffer
       Resize(mSize + copyable);
    }
}


template <class T> inline 
void 
AudioBuffer<T>::GetSlice(size_t start,
                         size_t size,
                         AudioBuffer<T> &buffer)
{
//    assert(!IsNull() && !buffer.IsNull());
//    assert(0<=start && start<mSize);
//    assert(size>=0);

    if( (IsNull() || buffer.IsNull()) ||
        !(0<=start && start<mSize) ||
        !(size>=0))
    {
        buffer.Resize(0);
        return;
    }

    size_t sbsize = std::min<size_t>(size, mSize - start);

    std::copy(this->Data() + start,
              this->Data() + start + sbsize,
              buffer.Data());

    buffer.Resize(sbsize);
}

template <class T> inline 
float 
AudioBuffer<T>::GetPower() const
{
    float Etot = 0;

    if(mSize == 0) return 0;

    for(size_t i=0; i<mSize; i++)
        Etot += mData[i] * mData[i];

    return Etot / mSize;
}

#endif // AUDIOBUFFER_H
