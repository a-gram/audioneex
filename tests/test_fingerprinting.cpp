/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "Fingerprint.h"
#include "AudioSource.h"
#include "Tester.h"

#ifdef PLOTTING_ENABLED
 #include "gnuplot.h"
#endif

///
/// Prerequisites:
///
/// 1) Copy the 'data' folder from the 'tests' directory in the source tree to
///    the test programs directory.
/// 2) If linking against the shared version of Audioneex, the library must be built
///    with the TESTING compiler definition in order for the private classes being tested
///    to expose their interfaces.
/// 3) (Optional) Add a TESTING and PLOTTING_ENABLED compiler definition to generate
///    a plot of the fingerprint (requires gnuplot to be in the search PATH).
///
/// Usage:  test_fingerprinting
///

TEST_CASE("Fingerprint accessors") {

    Audioneex::Fingerprint fingerprint;

    fingerprint.SetBufferSize(11025);
    REQUIRE( fingerprint.GetBufferSize() == 11025 + Audioneex::Pms::OrigWindowSize );
}


TEST_CASE("Fingerprint processing") {

    int Srate = Audioneex::Pms::Fs;
    int Nchan = Audioneex::Pms::Ca;

#ifdef PLOTTING_ENABLED
    Audioneex::Tester TESTER;
#endif

    AudioBlock<S16bit>  iblock(Srate*2, Srate, Nchan);
    AudioBlock<Sfloat>  audio(Srate*2, Srate, Nchan);

    AudioSourceFile     asource;

    asource.SetSampleRate( Srate );
    asource.SetChannelCount( Nchan );
    asource.SetSampleResolution( 16 );

    iblock.Resize(Srate*0.2);
    audio.Resize(Srate*0.2);

    REQUIRE_NOTHROW( asource.Open("./data/rec1.mp3") );
    REQUIRE_NOTHROW( asource.GetAudioBlock(iblock) );
    REQUIRE_NOTHROW( iblock.Normalize( audio ) );

    Audioneex::Fingerprint_t fp;
    Audioneex::Fingerprint fingerprint;

    // Try fingerprinting some invalid audio
    REQUIRE_NOTHROW ( fingerprint.Process( audio ) );
    REQUIRE( fingerprint.Get().empty() );  // Audio shorter than 0.5s

    iblock.Resize(Srate*2);
    audio.Resize(Srate*2);

    // Try fingerprinting some audio
    do{
        REQUIRE_NOTHROW( asource.GetAudioBlock(iblock) );
        iblock.Normalize( audio );
        REQUIRE_NOTHROW( fingerprint.Process(audio) );
        const Audioneex::lf_vector &lfs = fingerprint.Get();

        for(size_t i=0; i<lfs.size(); i++)
            fp.LFs.push_back(lfs[i]);
#ifdef PLOTTING_ENABLED
        REQUIRE_NOTHROW( TESTER.AddToPlotSpectrum(fingerprint) );
#endif
    }
    while(iblock.Size() > 0);

    // Validate the fingerprint.
    for(size_t i=0; i<fp.LFs.size(); i++){
        if(i>0){
           REQUIRE( (fp.LFs[i].ID == fp.LFs[i-1].ID+1) );
           REQUIRE( fp.LFs[i].T >= fp.LFs[i-1].T );
        }
        REQUIRE( (fp.LFs[i].F >= Audioneex::Pms::Kmin && fp.LFs[i].F <= Audioneex::Pms::Kmax) );
        REQUIRE( fp.LFs[i].D.size() == Audioneex::Pms::IDI_b );
    }

    fingerprint.Reset();
    REQUIRE( fingerprint.Get().empty() );

#ifdef PLOTTING_ENABLED
    REQUIRE_NOTHROW( Gnuplot::setTempFilesDir("./data") );
    REQUIRE_NOTHROW( Gnuplot::Init() );
    REQUIRE_NOTHROW( TESTER.PlotSpectrum("./data/rec1.mp3", 10000) );
    REQUIRE_NOTHROW( Gnuplot::Terminate() );
#endif
}

