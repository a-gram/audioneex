/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef TESTMATCHER_H
#define TESTMATCHER_H

#include "audioneex.h"
#include "TCDataStore.h"
#include "Fingerprint.h"
#include "AudioSource.h"


inline void GetAudio(AudioSourceFile& source,
                     AudioBlock<int16_t>& ibuf,
                     AudioBlock<float>& obuf)
{
    REQUIRE_NOTHROW( source.GetAudioBlock(ibuf) );
    REQUIRE( ibuf.Size() > 0 );
    REQUIRE_NOTHROW( ibuf.Normalize( obuf ) );
}


#endif
