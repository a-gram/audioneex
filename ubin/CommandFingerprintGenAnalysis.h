/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMANDFINGERPRINTGENANALYSIS_H
#define COMMANDFINGERPRINTGENANALYSIS_H

#include <boost/filesystem.hpp>

#include "common.h"
#include "Command.h"
#include "Fingerprint.h"
#include "AudioSource.h"

namespace bfs = boost::filesystem;


/// Execute  --fingerprint-gen-analysis [options] -i <audio_files>

class CommandFingerprintGenAnalysis : public Command
{
    AudioSourceFile     m_audio_source;
	AudioBlock<int16_t> m_iblock;
	AudioBlock<float>   m_iaudio;

	std::string         m_AudioFile;
	float               m_iblock_len;
	std::vector<int>    m_lfs_pdf;
	int                 m_nblocks;


	void Analyse(const std::string &audiofile)
	{
        if(bfs::is_regular_file(audiofile))
        {
			Audioneex::Fingerprint fingerprint;
			Audioneex::Fingerprint_t fp;

			m_audio_source.Open(audiofile);

			do{
				m_audio_source.GetAudioBlock(m_iblock);
				m_iblock.Normalize( m_iaudio );
				fingerprint.Process( m_iaudio );
				const Audioneex::lf_vector &lfs = fingerprint.Get();

                for(size_t i=0; i<lfs.size(); i++)
                    fp.LFs.push_back(lfs[i]);

				if(m_iaudio.Size()>0){
				   m_lfs_pdf[lfs.size()]++;
				   m_nblocks++;
				}
            }
            while(m_iblock.Size() > 0);

		   m_iblock.Resize(size_t(11025 * m_iblock_len));
		   m_iaudio.Resize(size_t(11025 * m_iblock_len));

           m_audio_source.Close();

		   ProcessResults();
        }
        else if(bfs::is_directory(audiofile))
        {
           bfs::directory_iterator it(audiofile);
           for(; it!=bfs::directory_iterator(); ++it)
               Analyse(it->path().string());
        }
	}

	void ProcessResults()
	{
		std::stringstream ss;

		int bmax = m_lfs_pdf.size()-1;
        // Only process the used histogram bins
		for(; bmax>=0 && m_lfs_pdf[bmax]==0; bmax--);

		if(bmax>=0){
			std::vector<int>::iterator max = std::max_element(m_lfs_pdf.begin(),
				                                              m_lfs_pdf.begin() + bmax + 1);
			ss << "---" << std::endl;
			for(int i=0; i<=bmax; i++){
				float nfreq = static_cast<float>(m_lfs_pdf[i]) /
					          static_cast<float>(*max);
				float  freq = static_cast<float>(m_lfs_pdf[i]) /
					          static_cast<float>(m_nblocks);
				std::string bar(nfreq*15, ']');
				ss << i << (i<10?" ":"") << ","
				   << std::fixed << std::setprecision(2)
				   << freq << "|" << bar << std::endl;
			}
			ss << "---" << std::endl;
		}
		DEBUG_MSG(ss.str())
	}


public:

    CommandFingerprintGenAnalysis() :
		m_iblock_len (1.2f),
		m_lfs_pdf   (100),
		m_nblocks   (0)
	{
        m_Usage = "\n\nSyntax: --fingerprint-gen-analysis [options] -i <audio_files>\n\n";
		m_Usage += "     -i = input audio file or directory of audio files\n\n";
		m_Usage += "     Options:\n\n";
		m_Usage += "     -b = audio block length in seconds (default=1.2s)\n\n";

		m_SupportedArgs.insert("-i");
		m_SupportedArgs.insert("-b");
    }

    void Execute()
    {
        if(!ValidArgs())
           PrintUsageAndThrow("Invalid arguments.");

        if(!GetArgValue("-i", m_AudioFile))
           PrintUsageAndThrow("Argument -i not specified.");

		GetArgValue("-b", m_iblock_len);

        if(!bfs::exists(m_AudioFile))
            throw std::runtime_error("File not found " + m_AudioFile);

        m_audio_source.SetSampleRate( 11025 );
        m_audio_source.SetChannelCount( 1 );
        m_audio_source.SetSampleResolution( 16 );

		m_iblock.Create(size_t(11025 * m_iblock_len), 11025, 1);
		m_iaudio.Create(size_t(11025 * m_iblock_len), 11025, 1);

		Analyse(m_AudioFile);
		ProcessResults();
    }

};

#endif
