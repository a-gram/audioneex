/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef COMMANDMAKEAUDIOCODES_H
#define COMMANDMAKEAUDIOCODES_H

#include <fstream>
#include <iomanip>

#include "Command.h"

/// Execute  --make-audiocodes -i <codes_file> -o <cpp_file>

class CommandMakeAudioCodes : public Command
{
    std::string m_AudioCodesBinFile;
    std::string m_AudioCodesCPPFile;

public:

    CommandMakeAudioCodes()
	{
        m_Usage = "\nSyntax: --make-audiocodes -i <codes_file> -o <cpp_file>\n";;
        m_SupportedArgs.insert("-i");
        m_SupportedArgs.insert("-o");
    }

    void Execute()
    {
        if(!ValidArgs())
           PrintUsageAndThrow("Invalid arguments.");

        if(m_Args.size() != 4)
           PrintUsageAndThrow("Invalid number of arguments.");

        // Look for -i argument
        if(!GetArgValue("-i", m_AudioCodesBinFile))
           PrintUsageAndThrow("Argument -i not specified.");

        // Look for -o argument
        if(!GetArgValue("-o", m_AudioCodesCPPFile))
           PrintUsageAndThrow("Argument -o not specified.");

        // Load the audio codes data from the specified file

        std::ifstream ifile(m_AudioCodesBinFile.c_str(),
                            std::ios::in|std::ios::binary);

        if(!ifile)
           throw std::runtime_error("Couldn't open audio codes file " +
                                    m_AudioCodesBinFile);

        std::vector<uint8_t> data((std::istreambuf_iterator<char>(ifile)),
                                  (std::istreambuf_iterator<char>()));

        if(ifile.fail())
            throw std::runtime_error("Error while reading audio codes file "+
                                     m_AudioCodesBinFile);

        // We should check that the read audio codes are valid
        // ...

        // Build the .cpp file

        std::ofstream ofile(m_AudioCodesCPPFile.c_str());

        if(!ofile)
           throw std::runtime_error("Couldn't open output .cpp file " +
                                    m_AudioCodesCPPFile);

        ofile << "#include <cstdint>" << std::endl;
        ofile << std::endl << "namespace { const uint8_t AUDIO_CODES[] = {" << std::endl;

        for(size_t i=0; i<data.size(); ++i)
            ofile << "0x" << std::hex << static_cast<int>(data[i])
                  << (i==data.size()-1 ? "" : ", ")
                  << (i&&(i%16==0) ? "\n" : "");

        ofile << std::endl << "};}" << std::endl;
    }
};

#endif
