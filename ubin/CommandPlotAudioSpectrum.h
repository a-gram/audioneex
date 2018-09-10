/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMANDPLOTAUDIOSPECTRUM_H
#define COMMANDPLOTAUDIOSPECTRUM_H

#define PLOTTING_ENABLED

#include <fstream>
#include <iomanip>

#include "Command.h"
#include "Fingerprint.h"
#include "AudioSource.h"
#include "Tester.h"
#include "gnuplot.h"

/// Execute  --plot-spectrum -i [options] <audio_file>

class CommandPlotAudioSpectrum : public Command
{
	std::string  m_audio_file;
	float        m_start_time;
	float        m_time_length;
	size_t       m_pic_width;


	void PlotSpectrum(const std::string &audiofile)
	{
        Audioneex::Tester TESTER;

        AudioBlock<int16_t>  iblock(11025*m_time_length, 11025, 1);
        AudioBlock<float>    audio(11025*m_time_length, 11025, 1);

        AudioSourceFile     asource;

        asource.SetSampleRate( 11025 );
        asource.SetChannelCount( 1 );
        asource.SetSampleResolution( 16 );
        asource.SetPosition( m_start_time );
        asource.SetDataLength( m_time_length );

        asource.Open( audiofile );

		Audioneex::Fingerprint_t fp;
        Audioneex::Fingerprint fingerprint( 11025*m_time_length + Audioneex::Pms::windowSize );

        do{
            asource.GetAudioBlock( iblock );
            iblock.Normalize( audio );
            fingerprint.Process( audio );
            const Audioneex::lf_vector &lfs = fingerprint.Get();

            for(size_t i=0; i<lfs.size(); i++)
                fp.LFs.push_back(lfs[i]);

            TESTER.AddToPlotSpectrum( fingerprint );
        }
        while(iblock.Size() > 0);

        //Gnuplot::setTempFilesDir("./data");
        Gnuplot::Init();
        TESTER.PlotSpectrum(audiofile, m_pic_width);
        Gnuplot::Terminate();
	}


public:

    CommandPlotAudioSpectrum() :
		m_start_time  (0.f),
		m_time_length (5.f),
		m_pic_width   (1000)
	{
        m_Usage = "\n\nSyntax: --plot-spectrum [options] -i <audio_file>\n\n";
		m_Usage += "     -i = input audio file\n\n";
		m_Usage += "     Options:\n\n";
		m_Usage += "     -ts = Start time within the audio to be plotted (default=0s)\n";
		m_Usage += "     -tl = Length of the audio to be plotted (default=5s)\n";
		m_Usage += "     -pw = Width of the plot picture (default=1000 pixels)\n\n";

        m_SupportedArgs.insert("-i");
        m_SupportedArgs.insert("-ts");
        m_SupportedArgs.insert("-tl");
        m_SupportedArgs.insert("-pw");
    }

    void Execute()
    {

        if(!ValidArgs())
           PrintUsageAndThrow("Invalid arguments.");

        // Look for -i argument
        if(!GetArgValue("-i", m_audio_file))
           PrintUsageAndThrow("Argument -i not specified.");

        // Get optional arguments
        GetArgValue("-ts", m_start_time);
        GetArgValue("-tl", m_time_length);
        GetArgValue("-pw", m_pic_width);

		PlotSpectrum(m_audio_file);
    }

};

#endif
