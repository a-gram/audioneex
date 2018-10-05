/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

///
/// example4
///
/// This example shows how to perform identification on live audio streams
/// (stream monitoring). It uses the StreamMonitoringTask class along with
/// other helper classes, which you can use as templates or modify to suit
/// your needs. The general command is as follows:
///
/// Usage: example4 [-u <db_url>] [-b <b_thresh>] [-l] <audio_line>
///
/// where <db_url> is the location of the database, <b_thresh> the binary
/// identification threshold, the option -l if used alone will list the 
/// available audio input devices installed on the machine (the output may 
/// differ depending on the platform) and <audio_line> is a string identifying 
/// the audio line that can be retrieved from the output of the -l command.
///
/// NOTE: You will need to experiment with the binary id threshold (-b) to find
///       the optimal value that gives the best recognition results based on
///       the application context. Usually this value can be found in the range
///       [0.6, 0.7].

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "ex_common.h"
#include "example4.h"
#include "CmdLineParser.h"

using namespace Audioneex;


void PrintUsage()
{
    std::cout << "\nUSAGE: example4 [-u <db_url>] [-b <b_thresh>] [-l] <audio_line>\n\n";
}


int main(int argc, char** argv)
{
    CmdLineParser cmdLine;
    CmdLineOptions_t opts;

    try
    {
        cmdLine.Parse(argv, argc, opts);

        // Just list the audio devices
        if(opts.list_dev){
           AudioSource::ListCaptureDevices();
           return 0;
        }

        StreamMonitoringTask itask (opts.apath);
        StreamMonitoringResultsParser idparser;

        std::shared_ptr<KVDataStore> 
        dstore ( new DATASTORE_T (opts.db_url) );
		
		// For client/server databases only (e.g. Couchbase)
        dstore->SetServerName( "localhost" );
        dstore->SetServerPort( 8091 );
        dstore->SetUsername( "admin" );
        dstore->SetPassword( "password" );

        dstore->Open( KVDataStore::GET, false, true );

        std::shared_ptr<Recognizer>
        recognizer ( Recognizer::Create() );
        recognizer->SetDataStore( dstore.get() );
        recognizer->SetBinaryIdThreshold( opts.b_thresh );

        idparser.SetDatastore( dstore );

        itask.SetDataStore( dstore );
        itask.SetRecognizer( recognizer );
        itask.Connect( &idparser );

        boost::thread task_thread( boost::bind(&StreamMonitoringTask::Run, &itask));

        int key;
        std::cout << "\nPress any key + ENTER to quit\n\n";
        std::cout << "Listening ...\r";
        std::cin >> key;

        // Terminate the task and wait for all threads to finish
        itask.Terminate();

        task_thread.join();

        std::cout << "Done" << std::endl;
    }
    catch(const bad_cmd_line_exception &ex)
	{
        std::cerr << "ERROR: " << ex.what() << std::endl;
        PrintUsage();
        return -1;
    }
    catch(const std::exception &ex)
	{
        std::cerr << "ERROR: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}

