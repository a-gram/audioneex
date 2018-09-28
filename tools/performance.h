/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef PERFORMANCETASK_H
#define PERFORMANCETASK_H

#include <fstream>
#include <unordered_map>
#include <boost/filesystem.hpp>

#include "common.h"
#include "IdTask.h"
#include "TCDataStore.h"
#include "AudioSource.h"
#include "Utils.h"

#include "audioneex.h"

namespace bfs = boost::filesystem;
namespace util = Audioneex::Utils;

const float IN_BLOCK_LEN = 1.2f;
const char* FAILED_CLIPS_DIR = "./failed";
const char* NOISE_FILE = "./noise.wav";


struct Performance_t
{
    float TP {0.f};
	float FP {0.f};
	float TN {0.f};
	float FN {0.f};

    float GetPrecision()   { return TP/(TP+FP); }
    float GetSensitivity() { return TN/(TN+FN); }
    float GetAccuracy()    { return (TP+TN)/(TP+TN+FP+FN); }
};


class PerformanceTask : public IdTask
{
    AudioSourceFile     m_AudioSource;
	AudioBlock<int16_t> m_iblock;
	AudioBlock<float>   m_iaudio;   ///< the input audio snippets
	AudioBlock<float>   m_noise;    ///< the noise sample
	AudioBlock<float>   m_inoise;   ///< the noise added to the in audio

    std::string         m_Audiofile;
	Performance_t       m_performance;
	std::ofstream       m_logfile;

	size_t              m_noise_offset;
	float               m_mean_SNR;
	size_t              m_nclips;
	bool                m_process_iaudio;
	float               m_input_gain;

	util::rng::natural<size_t> m_rng;


    void Identify(const std::string &audioclip)
    {
        if(bfs::is_regular_file(audioclip))
        {
           std::string filename = bfs::path(audioclip).stem().string();

		   // Get a random noise offset from the sample
		   m_noise_offset = m_rng.get_in(0, m_noise.Size() - 11025 * 15);

		   float Ps = 0, Pn = 0, snr = 0;

           const Audioneex::IdMatch* results = NULL;

           m_AudioSource.Open(audioclip);


           do{
               m_AudioSource.GetAudioBlock(m_iblock);
               m_iblock.Normalize( m_iaudio );

			   if(m_process_iaudio && m_iaudio.Size()>0){
				  m_iaudio.ApplyGain( m_input_gain );
				  Process(m_iaudio);
				  Ps += m_iaudio.GetPower();
				  Pn += m_inoise.GetPower();
			   }

               m_Recognizer->Identify(m_iaudio.Data(), m_iaudio.Size());
               results = m_Recognizer->GetResults();
           }
           while(m_iblock.Size() > 0 && results == nullptr);


           if(!results){
              m_Recognizer->Flush();
			  results = m_Recognizer->GetResults();
		   }

		   std::vector<Audioneex::IdMatch> BestMatch;
		   std::unordered_map<std::string, uint32_t> file_id;

		   if(results)
			 for(int i=0; !Audioneex::IsNull(results[i]); i++)
				 BestMatch.push_back( results[i] );

		   if(BestMatch.size() >= 1)
		   {
			   // Get the file id(s) associated with the best match(es) from the metadata
			   for(Audioneex::IdMatch &m : BestMatch){
				   std::string id = static_cast<KVDataStore*>(m_DataStore)->GetMetadata(m.FID);
				   assert(!id.empty());
				   assert(file_id.find(id) == file_id.end());
				   file_id[id] = m.CuePoint;
			   }

			   // Extract the file id from the clip's file name.
			   // Clip names have the following format:  <file_id>[_<clip_num>].wav
			   // where <clip_num> is the clip number extracted from file_id in case
			   // the dataset contains multiple clips extracted from the same file.
			   std::string sep = filename.find("_") != std::string::npos ? "_" : ".";
			   std::string file_id_clip = filename.substr(0, filename.find(sep));

			   // Check if the audio snippet being processed is in the set of identified
			   // recordings (there can be ties)
			   if(file_id.find(file_id_clip) != file_id.end()){
				  m_performance.TP++;
			   }
			   else{
				  m_performance.FP++;

				  m_logfile << "FP: " << audioclip << " ->";
				  for(auto &e : file_id) 
					  m_logfile << " " << e.first << " (@" << util::FormatTime(e.second) << ")";
				  m_logfile << std::endl;
				  SaveFailedClip(audioclip, "FP");
			   }
		   }
		   else{
			   m_performance.FN++;

			   m_logfile << "FN: " << audioclip << std::endl;
			   SaveFailedClip(audioclip, "FN");
		   }

		   m_nclips++;

		   // Compute the SNR for the current clip and the mean SNR
		   // across all dataset
		   if(Pn>0){
			  snr = 10*log10(Ps/Pn);
		      m_mean_SNR += snr;
		   }

           DEBUG_MSG("- A = " << std::fixed << std::setprecision(2)
			         << (m_performance.GetAccuracy()*100)
					 << std::setprecision(0)
			         << "% (TP="<<m_performance.TP
			         << ", TN="<<m_performance.TN
			         << ", FP="<<m_performance.FP
					 << ", FN="<<m_performance.FN<<")"
					 << std::setprecision(2)
					 << " - Tid: "<< (m_Recognizer->GetIdentificationTime())
					 <<"s, SNR="<<int(snr)<<"dB/"<<int(m_mean_SNR/m_nclips)<<"dB")

		   m_iblock.Resize(size_t(11025 * IN_BLOCK_LEN));
		   m_iaudio.Resize(size_t(11025 * IN_BLOCK_LEN));

           m_Recognizer->Reset();
           m_AudioSource.Close();

        }
        else if(bfs::is_directory(audioclip))
        {
           bfs::directory_iterator it(audioclip);
           for(; it!=bfs::directory_iterator(); ++it)
               Identify(it->path().string());
        }
    }

	// ------------------------------------------------------------------------

	void Process(AudioBlock<float> &iaudio)
	{
		m_noise.GetSubBlock(m_noise_offset, iaudio.Size(), m_inoise);
		iaudio.MixTo( m_inoise );
		m_noise_offset += m_inoise.Size();
	}

	// ------------------------------------------------------------------------

	void SaveFailedClip(const std::string &failed, const std::string &err)
	{
		bfs::path to_file = bfs::path(FAILED_CLIPS_DIR) /
                            bfs::path(err) /
                            bfs::path(failed).filename();

		bfs::copy(bfs::path(failed), to_file);
	}


public:

    explicit PerformanceTask(const std::string &audiofile) :
	    m_iblock    (size_t(11025 * IN_BLOCK_LEN), 11025, 1),
		m_iaudio    (size_t(11025 * IN_BLOCK_LEN), 11025, 1),
		m_inoise    (size_t(11025 * IN_BLOCK_LEN), 11025, 1),
		m_noise_offset (0),
		m_process_iaudio (false),
		m_nclips    (0),
		m_mean_SNR  (0),
		m_input_gain(1),
        m_Audiofile (audiofile),
		m_logfile   ("perf.log")
    {
        m_AudioSource.SetSampleRate( 11025 );
        m_AudioSource.SetChannelCount( 1 );
        m_AudioSource.SetSampleResolution( 16 );
    }

	// ------------------------------------------------------------------------

    void Run()
    {
        assert(m_Recognizer);

        if(!bfs::exists(m_Audiofile))
            throw std::runtime_error("File not found " + m_Audiofile);

		// Create the failed clips dirs
		if(!bfs::exists(bfs::path(FAILED_CLIPS_DIR))){
			bfs::create_directory(bfs::path(FAILED_CLIPS_DIR));
			bfs::create_directory(bfs::path(FAILED_CLIPS_DIR) / bfs::path("FP"));
			bfs::create_directory(bfs::path(FAILED_CLIPS_DIR) / bfs::path("FN"));
		}

		// Load noise sample

		if(m_process_iaudio)
		{
			DEBUG_MSG("Loading noise sample ...")

			AudioSourceFile noise;
			AudioBlock<int16_t> iblock;

			int duration = AudioSourceFile::GetID3TagsFromFile(NOISE_FILE).GetDuration() ;

			if(duration < 30)
				throw std::runtime_error("Please provide a noise sample >= 30 sec");

			iblock.Create(size_t(11025*duration), 11025, 1);
			m_noise.Create(size_t(11025*duration), 11025, 1);

			noise.SetSampleRate( 11025 );
			noise.SetChannelCount( 1 );
			noise.SetSampleResolution( 16 );

			noise.Open(NOISE_FILE);
			noise.GetAudioBlock(iblock);
			iblock.Normalize(m_noise);

			DEBUG_MSG("Loaded noise sample ("<<duration<<"s).")
		}

        Identify( m_Audiofile );
    }

	// ------------------------------------------------------------------------

	Performance_t& GetPerformance() { return m_performance; }
	void SetProcessAudio (bool value) { m_process_iaudio=value; }
	void SetInputGain(float gain) { m_input_gain=gain; }

    void Terminate() {}
    AudioSource* GetAudioSource() { return &m_AudioSource; }

};


#endif

