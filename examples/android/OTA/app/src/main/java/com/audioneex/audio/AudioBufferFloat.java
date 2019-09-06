/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


package com.audioneex.audio;

public class AudioBufferFloat extends AudioBuffer 
{
    private float[]  mData;

    public AudioBufferFloat()
    {
    	Setup();
    }
        
    public AudioBufferFloat(int nsamples, float sampleRate, int nchans)
    {
        super(nsamples, sampleRate, nchans);
	Setup();
    }

    public AudioBufferFloat(int nsamples, float sampleRate, int nchans, int initSize)
    {
	super(nsamples, sampleRate, nchans, initSize);
	Setup();
    }
    
    void Setup()
    {
    	mFormat = AudioFormat.FLOAT;		
    }
    
    @Override
    public int BytesPerSample()
    {
    	return 4;
    }
    
    @Override
    public boolean IsNull()
    { 
    	return (mData == null); 
    }

    @Override
    protected int GetCapacity()
    {
	return mData.length;
    }
    
    public float[] Data()
    {
    	return mData;
    }

    public float Get(int i)
    {
        return mData[i];
    }
        
    @Override
    public int SetData(float[] data)
    {
        if(data.length != mSize)
           Resize(data.length);

        System.arraycopy(data, 0, mData, 0, data.length);
        
        return mSize;    	
    }

    /// Float buffers are already normalized
    @Override
    public void Normalize(AudioBufferFloat nblock)
    {  	
    }
    
    @Override
    public AudioBuffer Append(AudioBuffer block)
    {
        if(block.IsNull() || block.Size() == 0)
           return this;

        Append( ((AudioBufferFloat)block).Data() );
        return this;
    }
    
    @Override
    public AudioBuffer Append(float[] data)
    {
        if(data == null || data.length == 0)
           return this;

        Append(data, data.length);
        return this;    	
    }
    
    @Override
    protected void AllocateBuffer(int size)
    {   	
    	mData = new float[size];
    }
    
    @Override
    public AudioBuffer Append(float[] data, int nitems)
    {
        int available = GetCapacity() - mSize;

        if(available > 0)
        {
           int copyable = (available >= nitems) ? nitems : available;
           System.arraycopy(data, 0, mData, mSize, copyable);
           Resize(mSize + copyable);
        }
        return this;
    }
    
    public AudioBufferFloat Copy(AudioBufferFloat buffer)
    {
    	if(mData.length < buffer.Data().length)
    	   mData = new float[buffer.Data().length];
    	
    	System.arraycopy(buffer.Data(), 0, mData, 0, mData.length);
        CopyState(buffer);
    	return this;
    }    

}

