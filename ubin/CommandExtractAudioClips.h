/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMANDEXTRACTAUDIOCLIPS_H
#define COMMANDEXTRACTAUDIOCLIPS_H

#include <fstream>
#include <iomanip>
#include <boost\filesystem.hpp>

#include "common.h"
#include "Command.h"
#include "AudioSource.h"
#include "AudioFileWriter.h"
#include "Utils.h"

namespace bfs = boost::filesystem;


/// Execute  --extract-audioclips -i <in_audio> [options] -o <out_clip_dir>

class CommandExtractAudioClips : public Command
{
	AudioSourceFile     m_AudioSource;
	AudioBlock<int16_t> m_iblock;
    std::string         m_AudioInPath;
    std::string         m_AudioOutPath;
	int                 m_ClipsNum;
	int                 m_ClipStart;
	int                 m_ClipLength;

	Audioneex::Utils::rng::natural<int> m_rnd;

	static const int RANDOM_OFFSET = -1;

public:

    CommandExtractAudioClips() :
		m_ClipsNum   (1),
		m_ClipLength (5),
		m_ClipStart  (RANDOM_OFFSET)
	{
        m_Usage = "\n\nSyntax: --extract-audioclips -i <in_audio> [options] -o <out_clip_dir>\n\n";
		m_Usage += "     -i  = input audio file or directory of audio files\n";
		m_Usage += "     -o  = output directory for the extracted clips\n\n";
		m_Usage += "     Options:\n\n";
		m_Usage += "     -n  = number of clips to extract from the input file(s) (default=1)\n";
		m_Usage += "     -ts = time offset from which to extract the clips (default=random)\n";
		m_Usage += "     -tl = length of the clips in seconds (default=5)\n\n";

        m_SupportedArgs.insert("-i");
		m_SupportedArgs.insert("-n");
        m_SupportedArgs.insert("-ts");
        m_SupportedArgs.insert("-tl");
        m_SupportedArgs.insert("-o");
    }

    void Execute()
    {
        if(!ValidArgs())
           PrintUsageAndThrow("Invalid arguments.");

        if(!GetArgValue("-i", m_AudioInPath))
           PrintUsageAndThrow("Argument -i not specified.");

        if(!GetArgValue("-o", m_AudioOutPath))
           PrintUsageAndThrow("Argument -o not specified.");

		GetArgValue("-n", m_ClipsNum);
		GetArgValue("-ts", m_ClipStart);
		GetArgValue("-tl", m_ClipLength);

        m_AudioOutPath += m_AudioOutPath.empty() ? "" :
                         (m_AudioOutPath.back()=='/' || m_AudioOutPath.back()=='\\' ? "" : "/");

        m_AudioSource.SetSampleRate( 44100 );
        m_AudioSource.SetChannelCount( 2 );
        m_AudioSource.SetSampleResolution( 16 );

		m_iblock.Create(44100 * m_ClipLength * 2, 44100, 2);

		Extract( m_AudioInPath );

    }

    void Extract(const std::string &audiofile)
    {
        if(bfs::is_regular_file(audiofile))
        {
           std::string filename = bfs::path(audiofile).stem().string();
           std::string fileext  = bfs::path(audiofile).extension().string();

           try{

			   int duration = AudioSourceFile::GetID3TagsFromFile(audiofile).GetDuration() ;

			   if(duration <= 0)
				  throw std::runtime_error("Audio file "+audiofile+" is invalid");

			   for(int c=1; c<=m_ClipsNum; c++)
			   {
				   std::string clipFile = m_AudioOutPath + filename + "_" + std::to_string(c) + ".wav";

			       int offset = m_ClipStart == RANDOM_OFFSET ?
				                m_rnd.get_in(0, duration - m_ClipLength) :
						    	m_ClipStart;

			       if(offset < 0) offset = 0;

			       m_AudioSource.SetPosition(offset);
			       m_AudioSource.SetDataLength(m_ClipLength);
			       m_AudioSource.Open(audiofile);
                   m_AudioSource.GetAudioBlock(m_iblock);

			       DEBUG_MSG("Extracted clip "<< c <<" from "<< filename << fileext << " @ "
                             << offset << "/" << duration << ", " << m_iblock.Duration() << " sec")

                   // Write clip to file
                   if(m_iblock.Size()>0)
                   {
                      AudioFileWriter::AudioFormat fmt;
                      fmt.Format = AudioFileWriter::WAV;
                      fmt.SampleRate = m_iblock.SampleRate();
                      fmt.SampleResolution = m_iblock.BytesPerSample()*8;
                      fmt.ChannelsCount = m_iblock.Channels();

                      AudioFileWriter af(fmt);
                      af.Open(clipFile);
                      af.Write(m_iblock);
                   }
			   }
           }
           catch(const std::exception &ex){
               ERROR_MSG(ex.what())
           }

           m_iblock.Resize(44100 * m_ClipLength * 2);

		   m_AudioSource.Close();
        }
        else if(bfs::is_directory(audiofile))
        {
           bfs::directory_iterator it(audiofile);
           for(; it!=bfs::directory_iterator(); ++it)
               Extract(it->path().string());
        }
    }

};

#endif
