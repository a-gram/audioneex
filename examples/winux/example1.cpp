/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

///
/// example1
///
/// This example shows how to perform the fingerprinting and indexing of a set of
/// audio files. Different processing jobs performed by the identification engine
/// are implemented into "task" classes. To execute the fingerprinting and indexing
/// of audio recordings the AudioIndexingTask class can be used.
///
/// To run the program, use the following command
///
/// example1 [-u <db_url>] <audio_files_dir>
///
/// where the optional argument <db_url> specifies the location of the database (for
/// TCDataStore it is the directory hosting the database files, while for CBDataStore
/// it is the server address or host name), the mandatory argument <audio_files_dir> 
/// indicates the directory containing the audio files to be indexed.
///

#include <iostream>
#include <vector>
#include <algorithm>

#include "ex_common.h"
#include "example1.h"
#include "CmdLineParser.h"

using namespace Audioneex;


void PrintUsage()
{
    std::cout << "\nUSAGE: example1 [-u <db_url>] <audio_files_dir>\n\n";
}


int main(int argc, char** argv)
{
    CmdLineParser cmdLine;
    CmdLineOptions_t opts;

    try
    {
        cmdLine.Parse(argv, argc, opts);

        AudioIndexingTask itask (opts.apath);

        std::shared_ptr<KVDataStore>
		dstore ( new DATASTORE_T (opts.db_url) );
		
		// For client/server databases only (e.g. Couchbase)
        dstore->SetServerName( "localhost" );
        dstore->SetServerPort( 8091 );
        dstore->SetUsername( "admin" );
        dstore->SetPassword( "password" );
		
        dstore->Open( KVDataStore::BUILD, false, true );
		
        std::shared_ptr<Indexer> 
        indexer ( Indexer::Create() );
        indexer->SetDataStore( dstore.get() );
        indexer->SetAudioProvider( &itask );

        itask.SetFID( opts.FID_base );
        itask.SetDataStore( dstore );
        itask.SetIndexer( indexer );
        itask.Run();

        std::cout << "Done" << std::endl;
    }
    catch(const bad_cmd_line_exception &ex)
	{
        std::cerr << "\nERROR: " << ex.what() << std::endl;
        PrintUsage();
        return -1;
    }
    catch(const std::exception &ex)
	{
        std::cerr << "\nERROR: " << ex.what() << std::endl;
        return -1;
    }

    return 0;
}

