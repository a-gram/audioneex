/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

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

