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

public class AudioFactory
{
   private AudioFactory(){   
   }
   
   /* Audio buffers */
   
   static AudioBuffer[] AllocateBuffers(AudioFormat format, int nbuf)
   {
	   AudioBuffer[] buffers;
	   switch(format){
	      case S_16_BIT: 
	    	  buffers = new AudioBuffer16Bit[nbuf];
	    	  for(int i=0; i<nbuf; i++)
	    		  buffers[i] = new AudioBuffer16Bit();
	    	  break;
	      case FLOAT:
	    	  buffers = new AudioBufferFloat[nbuf];
	    	  for(int i=0; i<nbuf; i++)
	    		  buffers[i] = new AudioBufferFloat();
	    	  break;	    	  
	      default: buffers = null;
	   }
	   return buffers;
   }
   
   static AudioBuffer[] CreateAudioBuffers(AudioFormat format, int nbuf)
   {
	   return AllocateBuffers(format, nbuf);
   }
   
   static AudioBuffer[] CreateAudioBuffers(AudioFormat format,
                                           int nbuf,
                                           int nsamples,
                                           float sampleRate,
                                           int nchans)
   {
       return CreateAudioBuffers(format, nbuf, nsamples, sampleRate, nchans, -1);
   }
   
   static AudioBuffer[] CreateAudioBuffers(AudioFormat format,
                                           int nbuf,
                                           int nsamples,
                                           float sampleRate,
                                           int nchans,
                                           int initSize)
   {
	   AudioBuffer[] buffers = AllocateBuffers(format, nbuf);
	   if(buffers!=null){
		   for(int i=0; i<nbuf; i++)
			   buffers[i].Create(nsamples, sampleRate, nchans, initSize);
	   }
	   return buffers;
   }
   
   /* Ring buffers */
   
   public static BufferQueue CreateRingBuffer(int nbuf, AudioFormat format)
   {
	   return new BufferQueue(nbuf, format);
   }
   
   public static BufferQueue CreateRingBuffer(int nbuf, AudioBuffer buf)
   {
	   return new BufferQueue(nbuf, buf);
   }
   
}
