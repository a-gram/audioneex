/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define CATCH_CONFIG_MAIN  // Tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "test_matching.h"
#include "Matcher.h"

///
/// Prerequisites:
///
/// 1) libejdb.dll must be present in the search path.
/// 2) If linking against the shared version of Audioneex, the library must be built
///    with the TESTING compiler definition in order for the private classes being tested
///    to expose their interfaces.
/// 3) Copy the 'data' folder from the 'tests' directory in the root of the source tree,
///    to the test programs directory.
///
/// Usage: test_matching
///


TEST_CASE("Matcher accessors") {

    TCDataStore dstore ("./data/db");
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

    TCDataStore dstore ( "./data/db" );

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
    lfs.clear();  // NOTE: The LFs have been acquired and deleted by the Matcher

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

    lfs.clear();  // NOTE: The LFs have been acquired and deleted by the Matcher

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

