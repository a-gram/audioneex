/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include <list>

#include "dao_common.h"
#include "test_common.h"
#include "test_recognizer.h"

///
/// Prerequisites:
///
/// 1) If linking against the shared version of Audioneex, the library must be built
///    with the TESTING compiler definition in order for the private classes being tested
///    to expose their interfaces.
/// 2) Copy the 'data' folder from the 'tests' directory in the root of the source tree,
///    to the test programs directory.
///
/// Usage: test_recognizer
///


std::list<Audioneex::eMatchType> match_types = { 
   Audioneex::MSCALE_MATCH, 
   Audioneex::XSCALE_MATCH
};
                                                 
std::list<Audioneex::eIdentificationType> id_types = { 
   Audioneex::FUZZY_IDENTIFICATION, 
   Audioneex::BINARY_IDENTIFICATION
};
                                                       
std::list<Audioneex::eIdentificationMode> id_modes = { 
   Audioneex::STRICT_IDENTIFICATION, 
   Audioneex::EASY_IDENTIFICATION
};



TEST_CASE("Recognizer creation") {

    std::unique_ptr <Audioneex::Recognizer> 
    recognizer ( Audioneex::Recognizer::Create() );
    
    REQUIRE_IN(match_types, recognizer->GetMatchType());
    REQUIRE_IN(id_types, recognizer->GetIdentificationType());
    REQUIRE_IN(id_modes, recognizer->GetIdentificationMode());
    REQUIRE_IN_RANGE({0, 1}, recognizer->GetMMS());
    REQUIRE_IN_RANGE({0.5, 1}, recognizer->GetBinaryIdThreshold());
    REQUIRE_ALMOST_EQUAL(0, recognizer->GetBinaryIdMinTime());
    REQUIRE(nullptr == recognizer->GetResults());
    REQUIRE_ALMOST_EQUAL(0, recognizer->GetIdentificationTime());
    REQUIRE(nullptr == recognizer->GetDataStore());
    
}

TEST_CASE("Recognizer accessors") {
    
    std::unique_ptr <Audioneex::Recognizer> 
    recognizer ( Audioneex::Recognizer::Create() );

    recognizer->SetMatchType(Audioneex::MSCALE_MATCH);
    REQUIRE(recognizer->GetMatchType() == Audioneex::MSCALE_MATCH);

    recognizer->SetIdentificationType(Audioneex::FUZZY_IDENTIFICATION);
    REQUIRE(recognizer->GetIdentificationType() == Audioneex::FUZZY_IDENTIFICATION);
    
    recognizer->SetIdentificationMode(Audioneex::EASY_IDENTIFICATION);
    REQUIRE(recognizer->GetIdentificationMode() == Audioneex::EASY_IDENTIFICATION);
    
    recognizer->SetMMS(0.5);
    REQUIRE_ALMOST_EQUAL(0.5, recognizer->GetMMS());
    REQUIRE_THROWS_AS(recognizer->SetMMS(2), 
                      Audioneex::InvalidParameterException);
    REQUIRE_THROWS_AS(recognizer->SetMMS(-1), 
                      Audioneex::InvalidParameterException);

    recognizer->SetBinaryIdThreshold(0.7);
    REQUIRE_ALMOST_EQUAL(0.7, recognizer->GetBinaryIdThreshold());
    REQUIRE_THROWS_AS(recognizer->SetBinaryIdThreshold(2), 
                      Audioneex::InvalidParameterException);
    REQUIRE_THROWS_AS(recognizer->SetBinaryIdThreshold(-1), 
                      Audioneex::InvalidParameterException);

    recognizer->SetBinaryIdMinTime(10);
    REQUIRE_ALMOST_EQUAL(10, recognizer->GetBinaryIdMinTime());
    REQUIRE_THROWS_AS(recognizer->SetBinaryIdMinTime(-1), 
                      Audioneex::InvalidParameterException);
    REQUIRE_THROWS_AS(recognizer->SetBinaryIdMinTime(30), 
                      Audioneex::InvalidParameterException);
                      
    DATASTORE_T dstore ("./data");
    
    recognizer->SetDataStore(&dstore);
    REQUIRE(recognizer->GetDataStore() == &dstore);
    
}


TEST_CASE("Recognizer identification") {
    
	IndexData({
        "./data/rec1.fp", 
        "./data/rec2.fp"
    });
    
    std::unique_ptr <Audioneex::Recognizer> 
    recognizer ( Audioneex::Recognizer::Create() );

    DATASTORE_T dstore ("./data");
    dstore.Open(KVDataStore::FETCH);
    
    AudioSourceFile asource;
    
}

