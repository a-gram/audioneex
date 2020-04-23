/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#ifndef AUDIOINEEX_ACI_H
#define AUDIOINEEX_ACI_H

#include <memory>

#include "TCDataStore.h"
#include "audioneex.h"


// Error codes

enum 
{
	UNSPECIFIED_ERROR = -1
};

// A singleton class providing the identification services

class ACIEngine
{
	static ACIEngine 
    mInstance;

	std::unique_ptr<Audioneex::Recognizer> 
    mRecognizer;
    
	std::unique_ptr<KVDataStore>
    mDataStore;
    
	bool
    mInitialized;

protected:

	ACIEngine() :
	   mInitialized(false)
	{}

public:

	static ACIEngine& 
    instance() { return mInstance; }

	void 
    init(std::string dataDir)
	{
        mDataStore.reset( new TCDataStore (dataDir) );
        mDataStore->Open( KVDataStore::FETCH, true, true );
	   
        mRecognizer.reset( Audioneex::Recognizer::Create() );
        mRecognizer->SetDataStore( mDataStore.get() );

        mInitialized = true;
	}

	Audioneex::Recognizer* 
    recognizer()
	{
        if(!mInitialized)
           throw std::runtime_error
           ("ACI engine not initialized");
           
        return mRecognizer.get();
	}

	KVDataStore* 
    datastore()
	{
        if(!mInitialized)
           throw std::runtime_error
           ("ACI engine not initialized");
           
        return mDataStore.get();
	}

};

ACIEngine ACIEngine::mInstance;


#endif
