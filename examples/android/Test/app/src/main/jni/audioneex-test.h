/*
    Copyright (c) 2014 Audioneex.com. All rights reserved.
	
	This source code is part of the Audioneex software package and is
	subject to the terms and conditions stated in the accompanying license.
	Please refer to the Audioneex license document provided with the package
	for more information.
	
	Author: Alberto Gramaglia
	
*/

#ifndef AUDIOINDEXINGTASK_H
#define AUDIOINDEXINGTASK_H

#include "TCDataStore.h"
#include "AudioSourceWav.h"
#include "audioneex.h"

#define TEST_REC_NUM 3


template<class T>
inline std::string n_to_string(const T &val) {
    std::stringstream out;
    out << val;
    return out.str ();
}


class IndexingTask : public Audioneex::AudioProvider
{
    Audioneex::Indexer* m_Indexer;
    std::string         m_AudioDir;
    uint32_t            m_FID;
    AudioSourceWavFile  m_AudioSource;
    AudioBlock<int16_t> m_InputBlock;
    AudioBlock<float>   m_AudioBlock;


    int OnAudioData(uint32_t FID, float *buffer, size_t nsamples)
    {
        assert(FID == m_FID);

        // Read 'nsamples' from the audio source into the indexer buffer.
        // 'nsamples' always amount to less then 10 seconds, so you can
        // safely allocate 10 second input buffers.

        // NOTE: Audio must be 16 bit normalized in [-1,1], mono, 11025 Hz.

        // Set the input buffer to the requested amount of samples.
        m_InputBlock.Resize( nsamples );

        m_AudioSource.Read( m_InputBlock );

        // If we reached the end of the stream, signal it to the indexer
        // by returning 0.
        if(m_InputBlock.Size() == 0)
           return 0;

        // The audio must be normalized prior to fingerprinting
        m_InputBlock.Normalize( m_AudioBlock );

        // Copy the audio into indexer buffer
        std::copy(m_AudioBlock.Data(),
                  m_AudioBlock.Data() + m_AudioBlock.Size(),
                  buffer);

        return m_AudioBlock.Size();
	}
	

    void DoIndexing(const std::string &audiofile)
    {
		std::string filename = audiofile;//filename.substr(filename.find_last_of("/")+1);

		m_AudioSource.Open(audiofile);

		LOG_D("[FID:%d] - File: %s", (m_FID+1), filename.c_str())

		// Assign a strict increasing FID to the recordings.
		m_FID++;

		// You may want to catch indexing errors here for recoverable failures
		// to avoid aborting the whole indexing process.
		// Invalid fingerprint exceptions are thrown if a fingerprint cannot be
		// extracted for some reason, such as invalid audio or incorrect FIDs.
		// These errors may be recovered by just skipping the failed fingerprint.
		// Anything else is a serious error and indexing should be aborted.
		try{
			// Start indexing of current audio file.
			// This call is synchronous. It will call OnAudioData() repeatedly
			// and will return once all audio for the current recording has been
			// consumed (this must be signalled in OnAudioData() by returning 0).
			m_Indexer->Index(m_FID);

			// Here, after the recording has been successfully indexed,
			// you may want to store the FID and associated metadata in
			// a database ...

		}
		catch(const Audioneex::InvalidFingerprintException &ex){
            LOG_E("FAILED: %s", ex.what())
            m_FID--; // reuse FID
		}

		// Reset all resources for next audio file
		m_AudioSource.Close();

	}

public:

    explicit IndexingTask(const std::string &audio_dir) :
        m_Indexer     (nullptr),
        m_AudioDir    (audio_dir),
        m_InputBlock  (11025*10, 11025, 1), //< 10s buffer is enough
        m_AudioBlock  (11025*10, 11025, 1), //< the normalized audio
        m_FID         (0)
    {}

    void Run()
    {
        assert(m_Indexer);

        m_Indexer->Start();

        for(int i=1; i<=TEST_REC_NUM; i++){
        	LOG_D("Indexing rec %d ...", i)
            DoIndexing( m_AudioDir+"/rec"+n_to_string(i)+".wav" );
        }

        m_Indexer->End();
	}

    void SetIndexer(Audioneex::Indexer* indexer) { m_Indexer = indexer; }

};

// ---------------------------------------------------------------------

class IdentificationTask
{
	Audioneex::Recognizer* m_Recognizer;
    AudioSourceWavFile     m_AudioSource;

    uint32_t DoIdentify(const std::string &audioclip)
    {
	   LOG_D("Identifying %s ...", audioclip.c_str())

	   const Audioneex::IdMatch* results = nullptr;

	   AudioBlock<int16_t> iblock;
	   AudioBlock<float>   iaudio;

	   m_AudioSource.Open( audioclip );

	   // process 1-2 second audio blocks

	   // create the input buffer
	   iblock.Create(size_t(11025 * 1.2), 11025, 1);
	   // create the normalized input audio block
	   iaudio.Create(size_t(11025 * 1.2), 11025, 1);

	   // Read audio blocks from the source and perform identification
	   // until results are produced or all audio is exhausted
	   do{
		   m_AudioSource.Read(iblock);
		   iblock.Normalize( iaudio );
		   m_Recognizer->Identify(iaudio.Data(), iaudio.Size());
		   results = m_Recognizer->GetResults();
	   }
	   while(iblock.Size() > 0 && results == nullptr);

	   // The id engine will always produce results if enough audio
	   // is provided for it to make a decision. If the audio data
	   // is exhausted before the engine returns a results you may
	   // flush the internal buffers and check again.
	   if(!results) {
           m_Recognizer->Flush();
           results = m_Recognizer->GetResults();
       }

	   uint32_t res = 0;

       // Parse the results, if any
	   if(results){
          std::vector<Audioneex::IdMatch> BestMatch;

          // Get the best match(es), if any (there may be ties)
          for(int i=0; !Audioneex::IsNull(results[i]); i++)
              BestMatch.push_back( results[i] );

          if(BestMatch.size() == 1){
        	 LOG_D("Got a match: %d", BestMatch[0].FID)
			 res = BestMatch[0].FID;
          }
       }
	   else
	     LOG_D("Not enough audio.")

	   // IMPORTANT:
	   // Always reset the Recognizer instance if reused for new identifications.
	   m_Recognizer->Reset();
	   
	   return res;
    }

public:

    IdentificationTask() :
        m_Recognizer  (nullptr)
    { }

    uint32_t Identify(const std::string &audioclip)
    {
        assert(m_Recognizer);
        return DoIdentify( audioclip );
    }

    void SetRecognizer(Audioneex::Recognizer* recognizer) { m_Recognizer = recognizer; }
    AudioSourceWavFile& GetAudioSource() { return m_AudioSource; }

};


#endif
