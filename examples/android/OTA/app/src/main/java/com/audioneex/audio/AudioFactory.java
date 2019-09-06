/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
