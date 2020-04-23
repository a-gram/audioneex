/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

///
/// otaid
///
/// This example shows how to perform identification on over-the-air (OTA) 
/// audio (aka stream monitoring). It uses the StreamMonitoringTask class 
/// along with other helper classes. The general command is as follows:
///
/// Usage: otaid [-u <db_url>] [-m <match_type>] [-i <id_type>] 
///                 [-d <id_mode>] [-b <b_thresh>] [-l] <audio_line>
///
/// where <db_url>, <match_type>, <id_type>, <id_mode> and <b_thresh> are 
/// the same as in example3, the option -l, if used alone, will list the 
/// available audio capture devices installed on the machine (the output may 
/// differ depending on the platform) and <audio_line> is a string identifying 
/// the audio line that can be retrieved from the output of the -l command.
///


#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "ex_common.h"
#include "otaid.h"
#include "CmdLineParser.h"

using namespace Audioneex;


void 
PrintUsage()
{
    std::cout << "\nUSAGE: otaid [-u <db_url>] [-m <match_type>] "
                 "[-i <id_type>] [-d <id_mode>]"
                 " [-b <b_thresh>] [-l] <audio_line>\n\n";
}


int 
main(int argc, char** argv)
{
    CmdLineParser cmdLine;
    CmdLineOptions_t opts;

    try
    {
        cmdLine.Parse(argv, argc, opts);

        // Just list the audio devices
        if(opts.list_dev)
        {
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

        dstore->Open( KVDataStore::FETCH, true, true );

        std::shared_ptr<Recognizer>
        recognizer ( Recognizer::Create() );
        recognizer->SetDataStore( dstore.get() );
        recognizer->SetMatchType( opts.mtype );
        recognizer->SetMMS( opts.mms );
        recognizer->SetIdentificationType( opts.id_type );
        recognizer->SetIdentificationMode( opts.id_mode );
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

