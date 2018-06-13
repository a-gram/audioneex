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
    This naive ring buffer is used by one producer (the audio source)
    and one consumer (the audio processor) that work on continuous
    audio streams, so there is no need for thread synchronization
    facilities.
*/


public class BufferQueue
{
    private AudioBuffer[] mBuffer      = null;
    private long          mConsumed    = 0;
    private long          mProduced    = 0;
    private boolean       mConsumeDone = false;

    BufferQueue(){
    	
    }

    BufferQueue(int size, AudioFormat format){
        mBuffer = AudioFactory.CreateAudioBuffers(format, size);      
    }

    BufferQueue(int size, AudioBuffer buf){
        mBuffer = AudioFactory.CreateAudioBuffers(buf.Format(), size,
                                                  buf.Capacity(),
                                                  buf.SampleRate(),
                                                  buf.Channels(),
                                                  buf.Size());       
    }

    public int Size() { return mBuffer.length; }
    public boolean  IsFull() { return Available() == mBuffer.length; }
    public boolean  IsEmpty() { return Available() == 0; }
    public boolean  IsNull() { return mBuffer == null; }
    public long Available() { return (mProduced - mConsumed); }
    
    public AudioBuffer GetHead() { 
    	return (IsFull() ? null : mBuffer[(int) (mProduced % mBuffer.length)]);
    }
    
    public AudioBuffer Push(){
        if(IsFull()) return null;
        AudioBuffer ret = mBuffer[(int) (mProduced % mBuffer.length)];
        mProduced++;
        return ret;    	
    }

    public AudioBuffer Pull()
    {
        AudioBuffer b = null;

        if(mConsumeDone)
           mConsumed++;

        if(!IsEmpty()) {
            b = mBuffer[(int) (mConsumed % mBuffer.length)];
            mConsumeDone = true;
        } else {
            mConsumeDone = false;
        }

        return b;    	
    }

    public void Reset(){
    	mConsumed = 0;
    	mProduced = 0;
    	mConsumeDone = false;
    	if(mBuffer!=null)
    	   for(int i=0; i<mBuffer.length; i++)
    		   mBuffer[i].Resize(0);
    }

}

