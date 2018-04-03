/*

Copyright (c) 2014, Audioneex.com.
Copyright (c) 2014, Alberto Gramaglia.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list
   of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
   list of conditions and the following disclaimer in the documentation and/or other
   materials provided with the distribution.

3. The name of the author(s) may not be used to endorse or promote products derived
   from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.

*/

#ifndef AUDIOINEEX_ACI_H
#define AUDIOINEEX_ACI_H

#include <memory>

#include "TCDataStore.h"
#include "audioneex.h"


// Error codes

enum {
	UNSPECIFIED_ERROR = -1
};

// A singleton class implementing the identification engine

class ACIEngine
{
	static ACIEngine mInstance;

	std::unique_ptr<Audioneex::Recognizer> mRecognizer;
	std::unique_ptr<KVDataStore>           mDataStore;
	bool                                   mInitialized;

public:

	ACIEngine() :
	   mInitialized(false)
	{}

	static ACIEngine& instance() { return mInstance; }

	void init(std::string dataDir)
	{
	    mDataStore.reset( new TCDataStore (dataDir) );
	    mDataStore->Open( KVDataStore::GET, false, true );
	   
        mRecognizer.reset( Audioneex::Recognizer::Create() );
        mRecognizer->SetDataStore( mDataStore.get() );

        mInitialized = true;
	}

	Audioneex::Recognizer* recognizer()
	{
        if(!mInitialized)
           throw std::runtime_error("ACI engine not initialized");
        return mRecognizer.get();
	}

	KVDataStore* datastore()
	{
        if(!mInitialized)
           throw std::runtime_error("ACI engine not initialized");
        return mDataStore.get();
	}

};

ACIEngine ACIEngine::mInstance;


#endif
