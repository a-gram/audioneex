/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef TESTMATCHER_H
#define TESTMATCHER_H

#include <chrono>
#include <thread>


size_t 
get_file_size(const std::string &file)
{
    std::ifstream ifs (file, std::ios::binary | std::ios::ate);
    return ifs.tellg();
}


void IndexData(std::list<std::string> fpfiles)
{
    DATASTORE_T dstore ( "./data" );
	
	// For client/server databases only (e.g. Couchbase)
    dstore.SetServerName( "localhost" );
    dstore.SetServerPort( 8091 );
    dstore.SetUsername( "admin" );
    dstore.SetPassword( "password" );

    REQUIRE_NOTHROW( dstore.Open(KVDataStore::BUILD) );

    // Clear the database prior to performing the tests.
    if(!dstore.IsEmpty()) {
        dstore.Clear();
		while(!dstore.IsEmpty()) {
		    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
    
    auto FID = 1;
    
    for(const auto &file : fpfiles)
    {
		const auto fpsize = get_file_size(file);
		
		REQUIRE( fpsize > 0 );
        //REQUIRE( (fpsize % sizeof(Audioneex::QLocalFingerprint_t)) == 0 );
		
		std::unique_ptr<char[]> fpbuf (new char[fpsize]);
		
        std::ifstream ifp (file, std::ios::binary);
		ifp.read( fpbuf.get(), fpsize );
		
		// Indexer requires a different buffer type
		auto fp = reinterpret_cast<const uint8_t*>(fpbuf.get());

        std::unique_ptr <Audioneex::Indexer> 
        indexer ( Audioneex::Indexer::Create() );
		
        REQUIRE_NOTHROW( indexer->SetDataStore( &dstore ) );
        REQUIRE_NOTHROW( indexer->Start() );
        REQUIRE_NOTHROW( indexer->Index(FID, fp, fpsize) );
        REQUIRE_NOTHROW( indexer->End() );
		
		// Store the fingerprint
		dstore.PutFingerprint(FID, fp, fpsize);
        
        FID++;
    }
}

#endif
