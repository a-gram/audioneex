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

package com.audioneex.recognition;

import java.lang.Thread.UncaughtExceptionHandler;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

import com.audioneex.audio.AudioBuffer;
import com.audioneex.audio.AudioBuffer16Bit;
import com.audioneex.audio.AudioBufferFloat;
import com.audioneex.audio.AudioFactory;
import com.audioneex.audio.AudioSourceService;
import com.audioneex.audio.AudioSourceServiceListener;
import com.audioneex.audio.BufferQueue;


public class RecognitionService implements Runnable, AudioSourceServiceListener {

	private boolean mSessionComplete = true;
	private boolean mServiceRunning = false;
	private boolean mAutodiscovery = false;
	
	private AudioSourceService mAudioSourceService = null;
	private BufferQueue mBufferQueue = null;
	
	private Handler OnAudioBufferReady = null;
	private Handler OnAudioSourceError = null;
	private Handler OnAudioSourceStart = null;
	private Handler OnAudioSourceStop  = null;
	private Handler OnQuit = null;
	
	private AudioIdentificationListener mAudioIdentificationListener = null;
    
	private AudioBufferFloat mAudioClip = null;
	private Recognizer mRecognizer = null;
	private Thread mAudioThread = null;
	
	private String mIdDataStoreDir = null;

	
	public RecognitionService(String IdDatastoreDir) 
		throws RecognitionServiceException
	{
		if(IdDatastoreDir==null)
                   throw new RecognitionServiceException
                   ("Recognition database directory can't be null");
		
		mIdDataStoreDir = IdDatastoreDir;
		
		// Initialize the native engine
		if(!Initialize(mIdDataStoreDir))
                   throw new RecognitionServiceException("Couldn't initialize engine");
		
		// Create the recognizer
		mRecognizer = new Recognizer();
	        mRecognizer.SetBinaryIdThreshold( 0.6f );

		// Create a queue of 2-second long buffers, 11025Hz, mono
		AudioBuffer captureBuffer = new AudioBuffer16Bit( 11025 * 2, 11025, 1, 0);
                mBufferQueue = AudioFactory.CreateRingBuffer(5, captureBuffer);
        
		// Create the audio provider
		mAudioSourceService = new AudioSourceService();
		mAudioSourceService.setBufferQueue(mBufferQueue);
		mAudioSourceService.Signal(this);		
	}
	
	@Override
	public void run()
	{
		mServiceRunning = true;
		
		Log.i("example", "Recognition service running ...");
		
		try
		{
		
		mAudioClip = new AudioBufferFloat(11025*2, 11025, 1);
		
                Looper.prepare();
        
                OnAudioBufferReady = new Handler() {
                	
                     @Override
    		     public void handleMessage(Message msg) {
    	    	
    	    	         // Do not process any pending audio if the identification session
    	    	         // has been completed.
    	    	         if(mSessionComplete) return;
    	    	
    	    	         AudioBuffer16Bit buffer = (AudioBuffer16Bit) mBufferQueue.Pull();
    	    	
    	                 if(buffer != null)
    	                 {
    	                    buffer.Normalize(mAudioClip);
    	        
    	                    mRecognizer.Identify(mAudioClip.Data(), mAudioClip.Size());
    	                    String res = mRecognizer.GetResults();
    	        
    	                    if(res!=null)
    	                    {
    	                       if(mAudioIdentificationListener!=null)
    	        	          mAudioIdentificationListener.SignalIdentificationResults(res);
    	                       mRecognizer.Reset();
    	              
    	                       // Mark the current identification session as completed.
    	                       // This causes any pending audio buffer to be discarded.
    	                       // This flag should be cleared when the audio thread is
    	                       // restarted for a new identification.
    	                       // If continuous identification is needed (for example
    	                       // to implement autodiscovery services) then disable
    	                       // session completion until explicitly stopped.
    	                       if(!mAutodiscovery){
    	                           mSessionComplete = true;
    	                           mAudioSourceService.Stop();
    	                       }
    	                    }

    	                    buffer.Resize(0);
    	                 }
    	             }
                };
        
                OnAudioSourceStart = new Handler() {
    	             @Override
    		     public void handleMessage(Message msg) {
    	    	         mRecognizer.Reset();
    	    	         mBufferQueue.Reset();
    	             }
                };

                OnAudioSourceStop = new Handler() {
    	             @Override
    		     public void handleMessage(Message msg) {
    	    	         mSessionComplete = true;
    	             }
                };        
        
                OnAudioSourceError = new Handler() {
    	             @Override
    		     public void handleMessage(Message msg) {
    	    	         mSessionComplete = true;
    	    	         String error = (String) msg.obj;
    	    	         if(mAudioIdentificationListener!=null)
    	    	            mAudioIdentificationListener.SignalIdentificationError(error);
    	             }
                };
        
                OnQuit = new Handler() {
    	             @Override
    		     public void handleMessage(Message msg) {
    	    	         Looper.myLooper().quit();
    	             }
                };
        
                Looper.loop();
        
		}
                catch(Exception e)
                {
		     Log.e("example", "EXCEPTION [RecognitionService.run()]: "+e.getMessage());
		     mAudioIdentificationListener.SignalIdentificationError(e.getMessage());
		}        
		
                mServiceRunning = false;

		Log.i("example", "Recognition service stopped");
	}
		
	public boolean isRunning()
	{
		return mServiceRunning;
	}
		
	public void Start()
	{
		try
		{
		   if(!isRunning())
		   {
		      Thread identThread = new Thread( this );
		      
		      identThread.setUncaughtExceptionHandler( new UncaughtExceptionHandler()
		      {
        	           public void uncaughtException(Thread t, Throwable e){
        		       Log.e("example", "EXCEPTION [RecognitionService]: "+e.getMessage());
        	           }
                      });
                      
		      identThread.start();
		   }
		}
		catch(Throwable t){
		      Log.e("example", "EXCEPTION [RecognitionService.Start()]: "+t.getMessage());
		}
	}
	
	public void Stop()
	{
		// Stop the audio service
		if(mAudioSourceService.isRunning())
		   mAudioSourceService.Stop();
		// Wait for the audio thread to finish
		try {
		   mAudioThread.join();
		   mAudioThread = null;
		}
		catch (InterruptedException e) {
	    	   if(mAudioIdentificationListener!=null)
 	    	      mAudioIdentificationListener.SignalIdentificationError(e.getMessage());
		}
		// Send QUIT signal
		if(OnQuit!=null)
		   OnQuit.sendEmptyMessage(0);
	}
	
	public void StartSession()
	{
		if(mSessionComplete){
		   mSessionComplete = false;
		   mAudioThread = mAudioSourceService.Start();
		}
	}
	
	public void StopSession() {
		mAudioSourceService.Stop();
	}
	
	public boolean isSessionRunning() {
		return !mSessionComplete;
	}
	
	public void SetAutodiscovery(boolean enabled){
		mAutodiscovery = enabled;
	}
	
	public boolean GetAutodiscovery() {
		return mAutodiscovery;
	}
	
	public void Signal(AudioIdentificationListener listener){
		mAudioIdentificationListener = listener;
	}	
	
	@Override
	public void SignalAudioBufferReady() {
		if(OnAudioBufferReady!=null);
		   OnAudioBufferReady.sendEmptyMessage(0);
	}

	@Override
	public void SignalAudioSourceError(String error) {
		if(OnAudioSourceError!=null){
		   OnAudioSourceError.sendMessage
		   (Message.obtain(OnAudioSourceError, 0, error));
		}
	}

	@Override
	public void SignalAudioSourceStart() {
		if(OnAudioSourceStart!=null)
                OnAudioSourceStart.sendEmptyMessage(0);
	}

	@Override
	public void SignalAudioSourceStop() {
		if(OnAudioSourceStop!=null)
                OnAudioSourceStop.sendEmptyMessage(0);
	}

	private native boolean Initialize(String datastoreDir);
	
}
