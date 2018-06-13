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

import java.lang.Thread.UncaughtExceptionHandler;

import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

public class AudioSourceService implements Runnable {

	private int mInputDevice = MediaRecorder.AudioSource.MIC;
	private int mSampleRate = 11025;
	private int mChannels = android.media.AudioFormat.CHANNEL_IN_MONO;
	private int mSampleFormat = android.media.AudioFormat.ENCODING_PCM_16BIT;
	private boolean mServiceRunning = false;
	private BufferQueue mBufferQueue = null;
	private AudioSourceServiceListener mAudioSourceServiceListener = null;
	
	
	public void SetSampleRate(int srate) { mSampleRate = srate; }
	public void SetChannels(int channels) { mChannels = channels; }
	public void SetSampleFormat(int format) { mSampleFormat = format; }

	public AudioSourceService(){
	}
	
	public AudioSourceService(int InputDevice, int SampleRate, int Channels, int SampleFormat)
	{
		mInputDevice = InputDevice;
		mSampleRate = SampleRate;
		mChannels = Channels;
		mSampleFormat = SampleFormat;
	}
	
	@Override
	public void run()
	{
		mServiceRunning = true;
		
		android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_URGENT_AUDIO);
		
		try
		{
			Log.i("example", "Audio service running ...");
			
			if(mAudioSourceServiceListener == null)
			   throw new Exception("No audio source listener registered");
			
			if(mBufferQueue==null)
			   throw new Exception("No buffer queue set");
			
			mAudioSourceServiceListener.SignalAudioSourceStart();
			
			// Minimum internal buffer size in bytes
			int minBuffSize = AudioRecord.getMinBufferSize(mSampleRate, mChannels, mSampleFormat);
			// Give it some more room
			int buffSize = minBuffSize * 4;
			
			// Note that this example code does not deal with audio resampling,
			// so if the required sampling rate of 11025 Hz is not supported
			// by the recording device we'll just let it fail.
			AudioRecord audioSource = new AudioRecord(mInputDevice,
					                          mSampleRate, 
					                          mChannels, 
					                          mSampleFormat, 
					                          buffSize);
			// Number of samples to be read
			int nsamples = buffSize / 2;
			
			short[] tempBuf = new short[nsamples];
			
			audioSource.startRecording();

			while(mServiceRunning)
			{
				int readSamples = audioSource.read(tempBuf, 0, nsamples);

				AudioBuffer buffer = mBufferQueue.GetHead();

				if(buffer!=null){
				   buffer.Append(tempBuf, readSamples);
				   if(buffer.Duration() >= 1.3){
					  if(mBufferQueue.Push() != null)
						 mAudioSourceServiceListener.SignalAudioBufferReady();
				   }
				}else{
				   Log.w("example", "AudioSourceService: Buffer overflow. Dropping audio ...");
				}
			}
			
			audioSource.stop();
			audioSource.release();
		
			Log.i("example", "Audio service stopped");
			
			mServiceRunning = false;
			mAudioSourceServiceListener.SignalAudioSourceStop();			
		}
		catch(Throwable t){
		   mServiceRunning = false;
		   SignalError(t.getMessage());
		   Log.e("example", "AudioSourceService: Recording Failed: "+t.getMessage());
		}
	}

	public boolean isRunning()
	{
		return mServiceRunning;
	}

	public Thread Start()
	{
		Thread audioThread = null;
		try
		{
		   if(!isRunning())
		   {
		      audioThread = new Thread( this );
		      audioThread.setUncaughtExceptionHandler( new UncaughtExceptionHandler() {
        	              public void uncaughtException(Thread t, Throwable e){
        		            Log.e("example", "EXCEPTION [AudioSourceService]: "+e.getMessage());
        		            SignalError(e.getMessage());
        	              }
                      });		
		      audioThread.start();
		      return audioThread;
		   }
		}
		catch(Throwable t)
		{
		   Log.e("example", "EXCEPTION [AudioSourceService.Start()]: "+t.getMessage());
		   SignalError(t.getMessage());
		}
		return audioThread;
	}
	
	private void SignalError(String msg)
	{
                if(mAudioSourceServiceListener!=null)
                   mAudioSourceServiceListener.SignalAudioSourceError(msg);		
	}
	
	public void Stop()
	{
		mServiceRunning = false;
	}
	
	public void setBufferQueue(BufferQueue bqueue)
	{
		mBufferQueue = bqueue;
	}	

	public void Signal(AudioSourceServiceListener listener)
	{
		mAudioSourceServiceListener = listener;
	}
	
}

