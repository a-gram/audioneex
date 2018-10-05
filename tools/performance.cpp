/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

///
/// performance
///
/// This example shows how to perform the identification of audio recordings.
/// To run the program, use the following command
///
/// performance [-u <db_url>] [-m <match_type>] [-i <id_type>] [-d <id_mode>] [-b <b_thresh>] 
///             [-t <b_min_time>] [-s <offset>] <audio_path>
///
/// where  <db_url> specifies the location of the database, <match_type> is one of
/// [MSCALE | XSCALE], <id_type> one of [BINARY | FUZZY], <id_mode> one of [STRICT | EASY],
/// <b_thresh> the binary identification threshold, <offset> the starting point within the
/// audio at which to perform the identification and <audio_path> the path to the directory
/// containing the audio file(s) to be identified.
///

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

#include "ex_common.h"
#include "performance.h"
#include "CmdLineParser.h"

using namespace Audioneex;


void PrintUsage(){
    DEBUG_MSG("\nUSAGE: performance [-u <db_url>] [-m <match_type>] [-i <id_type>] [-d <id_mode>]"
              " [-b <b_thresh>] [-t <b_min_time>] [-s <offset>] <audio_path>\n")
}


int main(int argc, char** argv)
{
    CmdLineParser cmdLine;
    CmdLineOptions_t opts;

    try
    {
        cmdLine.Parse(argv, argc, opts);

        PerformanceTask ptask (opts.apath);

        // Get a connection instance to the datastore
        std::unique_ptr<KVDataStore> dstore ( new DATASTORE_T (opts.db_url) );
        dstore->Open( KVDataStore::GET, true, true );

        // Create and set up the recognizer
        std::unique_ptr<Recognizer> recognizer ( Recognizer::Create() );
        recognizer->SetDataStore( dstore.get() );
        recognizer->SetMatchType( opts.mtype );
        recognizer->SetIdentificationType( opts.id_type );
        recognizer->SetIdentificationMode( opts.id_mode );
        recognizer->SetBinaryIdThreshold( opts.b_thresh );
		recognizer->SetBinaryIdMinTime( opts.b_min_time );

        ptask.SetDataStore( dstore.get() );
        ptask.SetRecognizer( recognizer.release() );
		ptask.SetProcessAudio( opts.process );
		ptask.SetInputGain( opts.gain );

        ptask.Run();

        DEBUG_MSG("Done")
    }
    catch(const bad_cmd_line_exception &ex){
        ERROR_MSG(ex.what())
        PrintUsage();
        return -1;
    }
    catch(const std::exception &ex){
        ERROR_MSG(ex.what())
        return -1;
    }

    return 0;
}

