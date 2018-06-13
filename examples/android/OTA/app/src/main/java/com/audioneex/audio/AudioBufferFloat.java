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

