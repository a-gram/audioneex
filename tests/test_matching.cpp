/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define CATCH_CONFIG_MAIN  // Tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "dao_common.h"
#include "test_matching.h"

///
/// Prerequisites:
///
/// 1) If linking against the shared version of Audioneex, the library must be built
///    with the TESTING compiler definition in order for the private classes being tested
///    to expose their interfaces.
/// 2) Copy the 'data' folder from the 'tests' directory in the root of the source tree,
///    to the test programs directory.
///
/// Usage: test_matching
///


TEST_CASE("Matcher accessors") {

    DATASTORE_T dstore ("./data");
    Audioneex::Matcher matcher;

    matcher.SetDataStore( &dstore );
    REQUIRE( matcher.GetDataStore() == &dstore );
    REQUIRE( matcher.GetMatchTime() == 0 );
    REQUIRE( matcher.GetStepsCount() == 0 );
    REQUIRE( matcher.GetResults().Top_K.empty() );
    REQUIRE( matcher.GetResults().Qc.empty() );
    REQUIRE( matcher.GetResults().GetTop(1).empty() );
    REQUIRE( matcher.GetResults().GetTopScore(1) == 0 );
}


TEST_CASE("Matcher processing") {

    int Srate = Audioneex::Pms::Fs;
    int Nchan = Audioneex::Pms::Ca;

    AudioBlock<int16_t>  iblock(Srate*2, Srate, Nchan);
    AudioBlock<float>    audio(Srate*2, Srate, Nchan);
    AudioSourceFile      asource;
	
    DATASTORE_T dstore ("./data");
	
    // For client/server databases only (e.g. Couchbase)
    dstore.SetServerName( "localhost" );
    dstore.SetServerPort( 8091 );
    dstore.SetUsername( "admin" );
    dstore.SetPassword( "password" );
	
    REQUIRE_NOTHROW( dstore.Open(KVDataStore::BUILD) );

    // We need an empty database to perform the tests.
    if(!dstore.Empty()) {
        dstore.Clear();
        // Wait until the clearing is finished as it may be an asynchronous
        // operation, such as in Couchbase, in which case the execution
        // would continue and the tests will fail.
        while(!dstore.Empty()) {
              std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }

    IndexFiles(&dstore, "./data/rec1.fp", 1);
	
    REQUIRE_NOTHROW( dstore.Open() );

    Audioneex::Matcher matcher;

    asource.SetSampleRate( Srate );
    asource.SetChannelCount( Nchan );
    asource.SetSampleResolution( 16 );

    REQUIRE_NOTHROW( asource.Open("./data/rec1.mp3") );

    iblock.Resize(Srate*0.2);
    audio.Resize(Srate*0.2);

    // Try processing a too short audio snippet

    Audioneex::Fingerprint fingerprint;
    GetAudio(asource, iblock, audio);
    fingerprint.Process( audio );
    Audioneex::lf_vector lfs = fingerprint.Get();
    REQUIRE( lfs.empty() == true );

    REQUIRE_THROWS_AS( matcher.Process( lfs ),
                       Audioneex::InvalidParameterException ); // Null data store set

    REQUIRE_NOTHROW( matcher.SetDataStore( &dstore ) );
    REQUIRE( matcher.Process( lfs ) == 0 );  // Not enough fingerprints
    REQUIRE( matcher.GetMatchTime() == 0 );  // Nothing processed
    REQUIRE( matcher.GetStepsCount() == 0 ); // As above
    REQUIRE( matcher.GetResults().Top_K.empty() );  // As above
    REQUIRE( matcher.Flush() == 0 );  // There must be no lfs in the buffer

    matcher.Reset();
    lfs.clear();

    iblock.Resize(Srate*1.5);
    audio.Resize(Srate*1.5);

    // Try processing a long enough audio snippet

    GetAudio(asource, iblock, audio);
    fingerprint.Process( audio );
    lfs = fingerprint.Get();
    REQUIRE( lfs.empty() == false );

    REQUIRE( matcher.Process( lfs ) > 0 );
    REQUIRE( matcher.GetMatchTime() > 0 );
    REQUIRE( matcher.GetStepsCount() > 0 );
    REQUIRE( matcher.GetResults().Top_K.empty() == false );
    REQUIRE( matcher.GetResults().GetTopScore(1) > 0 );

    lfs.clear();

    // Try processing an invalid lfs sequence

    Audioneex::Fingerprint fingerprint2;
    GetAudio(asource, iblock, audio);
    fingerprint2.Process( audio );
    lfs = fingerprint2.Get();
    REQUIRE( lfs.empty() == false);

    // This is an invalid sequence cause we're using a new fingerprint
    // without resetting the matcher
    REQUIRE_THROWS_AS( matcher.Process( lfs ),
                       Audioneex::InvalidMatchSequenceException );


}

