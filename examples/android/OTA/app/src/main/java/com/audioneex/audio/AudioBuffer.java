/*

Copyright (c) 2014, Audioneex.com.
Copyright (c) 2014, Alberto Gramaglia.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

3. The name of the author(s) may not be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

*/

package com.audioneex.audio;

/**
    A no-frills class for manipulating audio buffers. It is intended to be
    used in contexts where audio buffers are mostly fixed size and dynamic
    allocations are exceptional events.

    TODO: This is probably more elegantly implemented using java.nio.Buffer
*/

public abstract class AudioBuffer
{
    protected float mDuration;
    protected int   mSize;
    protected float mSampleRate;
    protected int   mChannels;
    protected float mNormFactor = 1.0f;

    protected AudioFormat mFormat = AudioFormat.UNDEFINED;
	
    public AudioBuffer()
    {
    }

    public AudioBuffer(int nsamples, float sampleRate, int nchans, int initSize)
    {
        Create(nsamples, sampleRate, nchans, initSize);
    }

    public AudioBuffer(int nsamples, float sampleRate, int nchans)
    {
        Create(nsamples, sampleRate, nchans, -1);
    }
    
    public void Create(int nsamples, float sampleRate, int nchans, int initSize)
    {
        int dn = nsamples % nchans;
        nsamples = dn>0 ? nsamples - dn + nchans : nsamples;

        mSampleRate = sampleRate;
        mChannels = nchans;

        mSize = (initSize < 0) ? nsamples : initSize;

        if(mSize > nsamples)
           mSize = nsamples;

        mDuration = mSize / (nchans * sampleRate);
        
        AllocateBuffer(nsamples);
    }

    public void Create(int nsamples, float sampleRate, int nchans){
    	Create(nsamples, sampleRate, nchans, -1);
    }
       
    public int   Capacity() { return GetCapacity(); }
    public int   Size() { return mSize; }
    public float SampleRate() { return mSampleRate; }
    public int   Channels() { return mChannels; }
    public float Duration() { return mDuration; }
    public float MaxDuration() { return IsNull() ? 0 : GetCapacity()/(mChannels*mSampleRate); }
    public float NormFactor() { return mNormFactor; }
    
    public AudioFormat Format() { return mFormat; }

    public void  SetChannels(int nchans) { mChannels = nchans; }

    /// Resize the block. This method does not perform any reallocation, it merely
    /// sets the amount of data to be considered in the block (available data).
    /// The method also changes the duration of the block.
    public void Resize(int newsize)
    {
        if(newsize > GetCapacity())
           newsize = GetCapacity();

        mSize = newsize;
        mDuration = mSize / (mChannels * mSampleRate);    	
    }
    
    public AudioBuffer Append(AudioBuffer buffer) { return null; }
    public AudioBuffer Append(short[] data) { return null; }
    public AudioBuffer Append(short[] data, int nitems) { return null; }
    public AudioBuffer Append(float[] data) { return null; }
    public AudioBuffer Append(float[] data, int nitems) { return null; }    
    public int SetData(short[] data) { return 0; }
    public int SetData(float[] data) { return 0; }    
    
    /// Allocate the format-specific buffer in concrete classes.
    abstract protected void AllocateBuffer(int size);
    /// Get the format-specific bytes per sample
    abstract public int  BytesPerSample();
    /// Check whether the audio block is null (no data)
    abstract public boolean IsNull();
    /// Get buffer capacity
    abstract protected int GetCapacity();
    /// Normalize into a float buffer
    abstract public void Normalize(AudioBufferFloat buffer);

    protected void CopyState(AudioBuffer buffer)
    {
    	this.mDuration = buffer.Duration();
    	this.mSize = buffer.Size();
    	this.mSampleRate = buffer.SampleRate();
    	this.mChannels = buffer.Channels();
    	this.mNormFactor = buffer.NormFactor();
    	this.mFormat = buffer.Format();
    }
}


