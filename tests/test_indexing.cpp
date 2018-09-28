/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define CATCH_CONFIG_MAIN  // Tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "test_indexing.h"

///
/// Prerequisites:
///
/// 1) tcejdbdll.dll must be present in the search path.
/// 2) If linking against the shared version of Audioneex, the library must be built
///    with the TESTING compiler definition in order for the private classes being tested
///    to expose their interfaces.
/// 3) Copy the 'data' folder from the 'tests' directory in the root of the source tree,
///    to the test programs directory.
///
/// Usage test_indexing
///

TEST_CASE("Indexer accessors") {

    std::unique_ptr <Audioneex::Indexer> indexer ( Audioneex::Indexer::Create() );
    IndexingTest itest;
    TCDataStore dstore ("./data");

    REQUIRE( indexer->GetCacheLimit() > 0 );
    indexer->SetCacheLimit( 128 );
    REQUIRE( indexer->GetCacheLimit() == 128 );
    REQUIRE( indexer->GetCacheUsed() == 0 );
    indexer->SetAudioProvider( &itest );
    REQUIRE( indexer->GetAudioProvider() == &itest );
    indexer->SetDataStore( &dstore );
    REQUIRE( indexer->GetDataStore() == &dstore );
    uint32_t maxv = Audioneex::IndexerImpl::GetMaxTermValue();
    // NOTE: The following tests depend on values that may be changed in the future ...
    REQUIRE( (maxv > 5000 && maxv < 7000) );
}


TEST_CASE("Indexer indexing") {

    IndexingTest itest;
    DummyAudioProvider dummyAudioProvider;
    std::unique_ptr <Audioneex::Indexer> indexer ( Audioneex::Indexer::Create() );
    TCDataStore dstore ("./data");

    std::vector<uint8_t> fake (1077);
    std::generate(fake.begin(), fake.end(), std::rand);

    REQUIRE_NOTHROW( dstore.Open( KVDataStore::BUILD, true ) );

    // Try indexing without starting a session
    REQUIRE_THROWS( indexer->Index(1) );
    REQUIRE_THROWS( indexer->Index(1,fake.data(),fake.size()) );

    // Try starting a session without a data provider
    REQUIRE_THROWS( indexer->Start() );

    REQUIRE_NOTHROW( indexer->SetDataStore( &dstore ) );

    // Try opening multiple sessions on a single instance
    REQUIRE_NOTHROW( indexer->Start() );
    REQUIRE_THROWS( indexer->Start() );
    REQUIRE_NOTHROW( indexer->End() );

    // Try indexing invalid data
    REQUIRE_NOTHROW( indexer->Start() );
    REQUIRE_THROWS( indexer->Index(1) );  // No Audio provider set
    REQUIRE_NOTHROW( indexer->SetAudioProvider( &dummyAudioProvider ) );
    REQUIRE_THROWS( indexer->Index(1) );  // No fingerprint extracted
    REQUIRE_THROWS( indexer->Index(1,nullptr,0) );   // Invalid fp (null)
    REQUIRE_THROWS( indexer->Index(1,fake.data(),0) );  // Invalid fp size (0)
    REQUIRE_THROWS( indexer->Index(1,fake.data(),fake.size()) ); // Invalid size
    REQUIRE_THROWS( indexer->Index(0,fake.data(),sizeof(Audioneex::QLocalFingerprint_t)) ); // Invalid FID (0)
    REQUIRE_NOTHROW( indexer->End() );

    // Try indexing

    REQUIRE_NOTHROW( indexer->SetAudioProvider( &itest ) );
    //indexer->SetCacheLimit( 512 );

    REQUIRE( dstore.GetFingerprintsCount() == 0 );

    REQUIRE_NOTHROW( itest.SetDatastore( &dstore ) );
    REQUIRE_NOTHROW( itest.SetIndexer( indexer.get() ) );
    REQUIRE_NOTHROW( itest.SetFingerprintsNum( 1000 ) );
    REQUIRE_NOTHROW( itest.Run() );

}

