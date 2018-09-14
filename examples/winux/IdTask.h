/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef IDTASK_H
#define IDTASK_H

#include <cassert>
#include <memory>

#include "AudioSource.h"
#include "audioneex.h"


/// Abstract class for various tasks performed by the identification
/// engine. Concrete implementations will provide task-specific functionality.

class IdTask
{
protected:

    std::unique_ptr<Audioneex::Indexer>     m_Indexer;
    std::unique_ptr<Audioneex::Recognizer>  m_Recognizer;
    std::unique_ptr<Audioneex::DataStore>   m_DataStore;

public:

    IdTask() {}

    virtual ~IdTask() {}

    virtual void Run() = 0;
    virtual void Terminate() = 0;

    virtual AudioSource* GetAudioSource() = 0;

    void SetIndexer(Audioneex::Indexer* indexer)
	{
        assert(indexer);
        m_Indexer.reset( indexer );
    }

    void SetRecognizer(Audioneex::Recognizer* recognizer)
	{
        assert(recognizer);
        m_Recognizer.reset( recognizer );
    }

    void SetDataStore(Audioneex::DataStore* datastore)
	{
        m_DataStore.reset( datastore );
    }

};


#endif
