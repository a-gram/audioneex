/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

///
/// example2
///
/// This example shows how to perform the indexing of a set of fingerprints from a
/// previously fingerprinted set of audio files.
///
/// To use the program, issue the following command
///
/// example2 [-u <db_url>]
///
/// where <db_url> specifies the location of the database (for TCDataStore it is the 
/// directory hosting the database files, while for CBDataStore it is the server address
/// or host name).
///

#include <iostream>
#include <vector>
#include <algorithm>

#include "ex_common.h"
#include "example2.h"
#include "CmdLineParser.h"

using namespace Audioneex;


void PrintUsage()
{
    std::cout << "\nUSAGE: example2 [-u <db_url>]\n\n";
}


int main(int argc, char** argv)
{
    CmdLineParser cmdLine;
    CmdLineOptions_t opts;

    try
    {
        cmdLine.Parse(argv, argc, opts);

        QFPIndexingTask itask;

        std::shared_ptr<KVDataStore> 
        dstore ( new DATASTORE_T (opts.db_url) );
		
		// For client/server databases only (e.g. Couchbase)
        dstore->SetServerName( "localhost" );
        dstore->SetServerPort( 8091 );
        dstore->SetUsername( "admin" );
        dstore->SetPassword( "password" );

        dstore->Open( KVDataStore::BUILD, true );

        std::shared_ptr<Indexer> 
        indexer ( Indexer::Create() );
        indexer->SetDataStore( dstore.get() );
        indexer->SetCacheLimit( 256 );

        itask.SetDataStore( dstore );
        itask.SetIndexer( indexer );
        itask.Run();

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

