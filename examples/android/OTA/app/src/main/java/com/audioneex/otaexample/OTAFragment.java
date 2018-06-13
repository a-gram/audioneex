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

package com.audioneex.otaexample;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.audioneex.recognition.AudioIdentificationListener;
import com.audioneex.recognition.RecognitionService;


public class OTAFragment extends Fragment implements AudioIdentificationListener {

    private RecognitionService mRecognitionService = null;

    private ImageView mBtnListen = null;
    private TextView  mTxtView = null;

    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("tokyocabinet");        
        System.loadLibrary("audioneex");
        System.loadLibrary("audioneex-jni");
    }

    public OTAFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState)
    {
        View view = inflater.inflate(R.layout.audioneex_ota_fragment, container, false);
        Setup(view);
        return view;
    }
    
    public void Setup (View view)
    {
        try
        {
            File IdDBDirBase = getActivity().getApplicationContext().getFilesDir();

            /*
             *   This is the directory where the reference database files will
             *   be stored. You can use the command line examples to create the
             *   audio database and then put the produced files data.* in the
             *   'assets' directory. The app will extract these files in IdDBDir.
             */
            File IdDBDir = new File(IdDBDirBase , "id_data" );

            ExtractIdDatastore( IdDBDir );

            mRecognitionService = new RecognitionService( IdDBDir.getPath() );
            mRecognitionService.Signal(this);
            mRecognitionService.SetAutodiscovery(true);
            mRecognitionService.Start();

        }catch(Exception e){
            Log.e("","EXCEPTION []: "+e.getMessage());
        }

        mBtnListen = (ImageView) view.findViewById(R.id.btnListen);
        mTxtView = (TextView) view.findViewById(R.id.txtView1);

        mBtnListen.setOnTouchListener(new View.OnTouchListener() {
			
		@Override
		public boolean onTouch(View v, MotionEvent event)
		{
		     if(event.getAction() != MotionEvent.ACTION_DOWN)
		        return true;
                     if(!mRecognitionService.isSessionRunning()){
                        mRecognitionService.StartSession();
                        mTxtView.setText("Listening...");
                     }else{
                        mRecognitionService.StopSession();
                        mTxtView.setText("Tap to listen");
                     }
		     return true;
		}
	});

    }

    private void ExtractIdDatastore(File dbdir)
    {
        InputStream is = null;
        FileOutputStream os = null;

        byte[] buffer = new byte[4096];
        int bytes_read;

        try {

            if (!dbdir.exists() && !dbdir.mkdir())
                throw new IOException("Couldn't create id datastore directory");

            File[] dbfiles = new File[2];
            dbfiles[0] = new File(dbdir, "data.idx");
            dbfiles[1] = new File(dbdir, "data.met");

            // Extract datastore files from assets
            for(File db: dbfiles) {
                String dbfile = db.getName();
                is = getActivity().getAssets().open(dbfile);
                os = new FileOutputStream(db);
                Log.i("TestApp", "writing db file: " + dbfile);
                while ((bytes_read = is.read(buffer)) != -1)
                    os.write(buffer, 0, bytes_read);
            }
        }
        catch (IOException e) {
            Log.e("TestApp", "EXCEPTION: [ExtractIdDatastore()]: "+e.toString());
        }
        finally {
            if(is != null) try { is.close(); }
            catch (IOException e) { Log.e("TestApp", "EXCEPTION: "+e.toString()); }
            if(os != null) try { os.close(); }
            catch (IOException e) { Log.e("TestApp", "EXCEPTION: "+e.toString()); }
        }
    }

    final Handler OnAudioIdentificationResults = new Handler()
    {
        @Override
        public void handleMessage(Message msg) {

            String jresults = (String) msg.obj;
            String viewmsg = "Tap to listen";

            if(jresults!=null)
            {
                try {
                    JSONObject results = new JSONObject(jresults);
                    if(results.get("status").equals("OK")){
                        JSONArray matches = results.getJSONArray("Matches");
                        if(matches.length() == 1){
                            JSONObject bestmatch = matches.getJSONObject(0);
                            viewmsg = bestmatch.getString("Metadata");
                        }else if(matches.length() > 1){
                            viewmsg = "Multiple matches found";
                        }else{
                       	    if(mRecognitionService.GetAutodiscovery()==false){
                               viewmsg = "No match found";
                       	    } else {
              	               viewmsg = "Listening ...";
                       	    }
                        }
                    }else if(results.get("status").equals("ERROR")){
                        viewmsg = "An error occurred during the identification";
                    }else{
                        //Unknown status
                    }
                }catch (JSONException e) {
                    e.printStackTrace();
                    viewmsg = "EXCEPTION: " + e.getMessage();
                }
            }else{
                Log.e("", "[OnAudioIdentificationResults]: Null results in message");
            }
            mTxtView.setText(viewmsg);
        }
    };

    Handler OnAudioIdentificationError = new Handler() {
        @Override
        public void handleMessage(Message msg) {
        	mTxtView.setText( (String) msg.obj );
        }
    };

    @Override
    public void SignalIdentificationResults(String results) {
        OnAudioIdentificationResults.sendMessage
        (Message.obtain(OnAudioIdentificationResults, 0, results));
    }

    @Override
    public void SignalIdentificationError(String error) {
        OnAudioIdentificationError.sendMessage
        (Message.obtain(OnAudioIdentificationError, 0, error));
    }

}
