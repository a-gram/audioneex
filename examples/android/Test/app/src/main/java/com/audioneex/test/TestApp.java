/*
    Copyright (c) 2014 Audioneex.com. All rights reserved.
	
	This source code is part of the Audioneex software package and is
	subject to the terms and conditions stated in the accompanying license.
	Please refer to the Audioneex license document provided with the package
	for more information.
	
	Author: Alberto Gramaglia
	
*/

package com.audioneex.test;

import java.io.*;
import java.lang.Thread.UncaughtExceptionHandler;

import android.app.Activity;
import android.os.Bundle;
import android.content.Context;
import android.widget.Button;
import android.widget.TextView;
import android.view.View;
import android.view.View.OnClickListener;
import android.util.Log;
import android.os.Handler;
import android.os.Message;


public class TestApp extends Activity {

	static final int NUM_RECS = 3;

    static {
    	System.loadLibrary("c++_shared");
    	System.loadLibrary("audioneex");
    	System.loadLibrary("tokyocabinet");
        System.loadLibrary("audioneex-test");
    }

    static TextView m_txtMessage = null;
    static Button m_btnTest = null;
    static String m_baseDir = null;
        
    @Override protected void onCreate(Bundle savedInstanceState) {
    	
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_layout);
        
        Context ctx = getApplicationContext();
        
        m_baseDir = ctx.getFilesDir().getPath();
        
        InputStream is = null;
        FileOutputStream os = null;
        
		byte[] buffer = new byte[4096];
		int bytes_read;
        
        try {
        	
			File audioDir = new File(m_baseDir + "/audio");
			File dataDir = new File(m_baseDir + "/data");

			if (!audioDir.exists() && !audioDir.mkdir())
				throw new IOException("Couldn't create audio dir");
			if (!dataDir.exists() && !dataDir.mkdir())
				throw new IOException("Couldn't create data dir");
			
			// Clean up old data files, if any
            CleanUp();

            // Extract audio from assets
			for (int i = 1; i <= NUM_RECS; ++i) {
				String wavFile = audioDir.getAbsolutePath()+"/rec"+i+".wav";
				is = getAssets().open("rec"+i+".wav");
				os = new FileOutputStream(wavFile);
				if (is != null && os != null) {
					Log.i("TestApp", "writing wav file: " + wavFile);
					while ((bytes_read = is.read(buffer)) != -1)
						os.write(buffer, 0, bytes_read);
				}
			}
		}
        catch (IOException e) {
           Log.e("TestApp", "EXCEPTION: "+e.toString());
		}
        finally {
           if(is != null) try { is.close(); } 
              catch (IOException e) { Log.e("TestApp", "EXCEPTION: "+e.toString()); }
           if(os != null) try { os.close(); }
              catch (IOException e) { Log.e("TestApp", "EXCEPTION: "+e.toString()); }
        }
        
        m_txtMessage = (TextView) findViewById(R.id.text);
        m_btnTest = (Button) findViewById(R.id.test_button);
        
        m_btnTest.setOnClickListener(new OnClickListener() {

			@Override
			public void onClick(View v) {
				
		        Runnable runnable = new Runnable() {
		            @Override
		            public void run()  {
         		       CleanUp();
		               OnTestStart.sendEmptyMessage(0);
		               if( doTest( m_baseDir ) )
		                  OnTestSuccess.sendEmptyMessage(0);
		               else
		            	  OnTestFail.sendEmptyMessage(0);
		            }
		        };
		        
		        Thread testThread = new Thread(runnable);
		        
		        testThread.setUncaughtExceptionHandler( new UncaughtExceptionHandler() {
		        	public void uncaughtException(Thread t, Throwable e){
		        		OnTestFail.sendEmptyMessage(0);
		        	}
		        });
		        testThread.start();
			}

        });
        
    }

    @Override protected void onPause() {
        super.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
    }
    
    public void CleanUp() {
    	File dataDir = new File(m_baseDir + "/data");
		File[] dataFiles = dataDir.listFiles();
		for(int i=0; i<dataFiles.length; ++i)
			dataFiles[i].delete();
    }
    
    /* Some Handlers for GUI updates */
    
	static Handler OnTestStart = new Handler() {
		  @Override
		  public void handleMessage(Message msg) {
              m_txtMessage.setText("Running test ...");
              m_btnTest.setEnabled(false);
	      }
    };    
    
	static Handler OnTestEnd = new Handler() {
		  @Override
		  public void handleMessage(Message msg) {
              m_txtMessage.setText("Test completed.");
              m_btnTest.setEnabled(true);
	      }
    };    
    
	static Handler OnTestSuccess = new Handler() {
		  @Override
		  public void handleMessage(Message msg) {
              m_txtMessage.setText("Test succeeded!");
              m_btnTest.setEnabled(true);
	      }
    };    
    
	static Handler OnTestFail = new Handler() {
		  @Override
		  public void handleMessage(Message msg) {
              m_txtMessage.setText("Test failed. Please report this issue.");
              m_btnTest.setEnabled(true);
	      }
    };    
    
    /* Native methods */
    
    public static native boolean doTest(String base_dir);
    
}

