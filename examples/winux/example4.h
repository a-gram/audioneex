/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef STREAMMONITORINGTASK_H
#define STREAMMONITORINGTASK_H

#include <cstdint>
#include <string>
#include <memory>
#include <boost/asio.hpp>

#include "IdTask.h"
#include "AudioSource.h"
#include "AudioBlock.h"
#include "RingBuffer.h"
#include "audioneex.h"



class StreamMonitoringTask : public IdTask,
                             public AudioSourceDataListener
{

    AudioSourceDevice   m_AudioSource;
    AudioBlock<float>   m_AudioSnippet;
    RingBuffer<int16_t> m_RingBuffer;

    boost::asio::io_service                        m_event_loop;
    std::unique_ptr<boost::asio::io_service::work> m_work;

    std::string                    m_InputLineName;
    IdentificationResultsListener *m_Listener;


    /// This event handler is called by the audio source thread whenever
    /// a new chunk of audio has been read. This call must be quick at
    /// returning. It's only job is to put the audio data in the accumulator
    /// and signal the identification thread if there is enough audio to
    /// perform the identification.
    void OnAudioSourceData(AudioBlock<int16_t> &audio)
    {
        AudioBlock<int16_t> *abuffer = m_RingBuffer.GetHead();

        if(abuffer)
        {
           abuffer->Append(audio);

           if(abuffer->Duration() >= 1.2)
              if(m_RingBuffer.Push())
                 m_event_loop.post( boost::bind(&StreamMonitoringTask::DoIdentification, this) );
        }
        else
           std::cout << "WARNING: Ring buffer overflow. Dropping block..." << std::endl;
    }


    /// This method is called by the identification thread (the thread running
    /// the event loop) whenever a new event is received from the audio thread.
    void DoIdentification()
    {
        AudioBlock<int16_t> *block = m_RingBuffer.Pull();

        if(block == nullptr)
            return;

        block->Normalize( m_AudioSnippet );

        m_Recognizer->Identify(m_AudioSnippet.Data(), m_AudioSnippet.Size());

        const Audioneex::IdMatch* results = m_Recognizer->GetResults();

        if(results)
        {
           // Notify whoever wants to be notified about the results.
           // NOTE: The listeners should make a deep copy of the results
           // if they are used in asynchronous contexts as the pointer
           // will become invalid after resetting the recognizer.
           if(m_Listener) m_Listener->OnResults( results );
           // IMPORTANT:
           // Reset the instance for new identifications
           m_Recognizer->Reset();
        }

        // Reset the consumed buffer
        block->Resize(0);
    }

public:

    explicit StreamMonitoringTask(const std::string& i_audio_line) :
        m_InputLineName (i_audio_line),
        m_Listener      (nullptr)
    {
        m_AudioSource.SetDataListener(this);
        m_AudioSource.SetSampleRate( 11025 );
        m_AudioSource.SetChannelCount( 1 );
        m_AudioSource.SetSampleResolution( 16 );

        // Create 3 second audio buffers
        m_AudioSnippet.Create(11025 * 3, 11025, 1);
        AudioBlock<int16_t> buffer (11025 * 3, 11025, 1, 0);
        m_RingBuffer.Set(10, buffer );
    }

    /// Run the stream monitoring task. The thread calling this
    /// method will run the event loop (blocking) and will execute
    /// the identification events posted by the audio thread.
    void Run()
    {
        assert(m_Recognizer);

        m_work.reset( new boost::asio::io_service::work(m_event_loop) );

        // Open the input line and start the audio capture thread
        m_AudioSource.Open( m_InputLineName );
        m_AudioSource.StartCapture();

        // The calling thread will block here
        m_event_loop.run();
    }

    /// Terminate the stream monitoring task.
    /// Wait for the audio thread to finish and stop the event loop.
    void Terminate()
	{
        m_AudioSource.StopCapture( true );
        m_AudioSource.Close();
        m_event_loop.stop();
    }

    void Connect(IdentificationResultsListener *listener)
	{
        m_Listener = listener;
    }

    AudioSource* GetAudioSource() { return &m_AudioSource; }

};


/// ---------------------------------------------------------------------------

/// This class implements a parser for stream identification results.
/// It is just a convenience class that connects to the monitoring
/// task to get the results and do something with them.

class StreamMonitoringResultsParser : public IdentificationResultsListener
{
    std::shared_ptr<KVDataStore> m_Datastore;

    std::string FormatTime(int sec)
    {
        char buf[32];
        ::sprintf(buf, "%02d:%02d:%02d", sec/3600, (sec%3600)/60, (sec%3600)%60);
        return std::string(buf);
    }

  public:

    void OnResults(const Audioneex::IdMatch *results)
    {
        std::vector<Audioneex::IdMatch> BestMatch;

        // Get the best match(es), if any (there may be ties)
        if(results)
           for(int i=0; !Audioneex::IsNull(results[i]); i++)
               BestMatch.push_back( results[i] );

        // We have a single best match
        if(BestMatch.size() == 1)
        {
           assert(m_Datastore);

           std::cout << std::endl;

           // Get metadata for the best match
           std::string meta = m_Datastore->GetMetadata(BestMatch[0].FID);

           std::cout << std::endl;
           std::cout << "IDENTIFIED  FID: " << BestMatch[0].FID << std::endl;
           std::cout << "Score: " << BestMatch[0].Score << ", ";
           std::cout << "Conf.: " << BestMatch[0].Confidence << std::endl;
           //std::cout << "Id.Time: " << m_Recognizer->GetIdentificationTime() << "s"<<std::endl;
           std::cout << (meta.empty() ? "No metadata" : meta) << std::endl;
           std::cout << "-----------------------------------" << std::endl;
        }
        // There are ties for the best match
        else if(BestMatch.size() > 1){
            std::cout << "There are " << BestMatch.size() << " ties for the best match\n";
        }
        else{
            // No match found
        }

        std::cout << "Listening ...\r";

        std::cout.flush();
    }

    void SetDatastore(std::shared_ptr<KVDataStore> &store) {
        m_Datastore = store;
    }
};

#endif
