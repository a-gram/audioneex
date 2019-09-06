/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

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


