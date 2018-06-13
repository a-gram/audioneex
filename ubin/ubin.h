/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef UBIN_H
#define UBIN_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "CommandMakeAudioCodes.h"
#include "CommandPlotAudioSpectrum.h"
#include "CommandExtractAudioClips.h"
#include "CommandGenerateFingerprintDB.h"
#include "CommandFingerprintGenAnalysis.h"

typedef std::map<std::string, std::unique_ptr<Command> > command_map;


/// This class parses the command line and sets the appropriate command
/// instance, depending on the command specified in the comand line.
/// Clients will call the Parse() method followed by the GetCommand()
/// method and execute the command by calling Execute() on the returned
/// Command instance.
class CmdLineParser
{
    command_map  m_Commands;
    std::vector<std::string>  m_CommandLine;


	void PrintAvailableCommands()
	{
		std::cout << "Available commands:\n\n";

		for(auto &kv : m_Commands){
			if(kv.first != "null")  //< skip the null command
			   std::cout << "\t" << kv.first << std::endl;
		}

		std::cout << std::endl;
	}


 public:

    CmdLineParser()
	{
		m_Commands["null"].reset( new NullCommand );

		// Add commands here ...
		m_Commands["--make-audiocodes"].reset( new CommandMakeAudioCodes );
        m_Commands["--make-audiocodes"].reset( new CommandMakeAudioCodes );
        m_Commands["--plot-spectrum"].reset( new CommandPlotAudioSpectrum );
		m_Commands["--extract-audioclips"].reset( new CommandExtractAudioClips );
		m_Commands["--generate-fingerprint-db"].reset( new CommandGenerateFingerprintDB );
		m_Commands["--fingerprint-gen-analysis"].reset( new CommandFingerprintGenAnalysis );
    }

    /// Get the command instance
    Command* GetCommand()
    {
        if(m_CommandLine.empty())
           throw std::runtime_error("Command line is empty. Did you call Parse()?");

        std::string cmd = m_CommandLine[0];

		// If -l provided, print available commands and return
		if(cmd == "-l"){
		   PrintAvailableCommands();
		   return m_Commands["null"].get();
		}

        command_map::iterator it = m_Commands.find(cmd);

        if(it == m_Commands.end())
           throw bad_cmd_line_exception("Command "+cmd+" not available.");

		Command *pcmd = (*it).second.get();

        // Strip off the command name from the arguments list and set up the arg list
        std::vector<std::string> args;
        args.insert(args.end(), m_CommandLine.begin() + 1, m_CommandLine.end());

        pcmd->SetArgs(args);

        return pcmd;
    }

    /// Parse the command line and set up the command instance
    void Parse(char** argv, int argc)
    {
        if(argc < 2)
           throw bad_cmd_line_exception("Invalid command line");

         m_CommandLine.clear();
         m_CommandLine.insert(m_CommandLine.end(), argv + 1, argv + argc);
    }

};

#endif // UBIN_H
