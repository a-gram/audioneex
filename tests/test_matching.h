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

#include "audioneex.h"
#include "Fingerprint.h"
#include "Matcher.h"
#include "AudioSource.h"


inline void GetAudio(AudioSourceFile& source,
                     AudioBlock<int16_t>& ibuf,
                     AudioBlock<float>& obuf)
{
    REQUIRE_NOTHROW( source.GetAudioBlock(ibuf) );
    REQUIRE( ibuf.Size() > 0 );
    REQUIRE_NOTHROW( ibuf.Normalize( obuf ) );
}


class IndexFiles {
	
	size_t get_file_size(const std::string &file)
	{
		std::ifstream ifs (file, std::ios::binary | std::ios::ate);
		return ifs.tellg();
	}
	
public:

	IndexFiles(KVDataStore *dstore, const std::string &file, uint32_t FID)
	{
		size_t fpsize = get_file_size(file);
		
		REQUIRE( fpsize > 0 );
        REQUIRE( (fpsize % sizeof Audioneex::QLocalFingerprint_t) == 0 );
		
		std::unique_ptr<char[]> fpbuf (new char[fpsize]);
		
        std::ifstream ifp (file, std::ios::binary);
		ifp.read( fpbuf.get(), fpsize );
		
		// Indexer requires a different buffer type
		auto fp = reinterpret_cast<const uint8_t*>(fpbuf.get());

        std::unique_ptr <Audioneex::Indexer> 
        indexer ( Audioneex::Indexer::Create() );
		
        REQUIRE_NOTHROW( indexer->SetDataStore( dstore ) );
        REQUIRE_NOTHROW( indexer->Start() );
        REQUIRE_NOTHROW( indexer->Index(FID, fp, fpsize) );
        REQUIRE_NOTHROW( indexer->End() );
	}
};

#endif
