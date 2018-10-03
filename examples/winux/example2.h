/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef QFPINDEXINGTASK_H
#define QFPINDEXINGTASK_H

#include <cstdint>
#include <string>
#include <memory>

#include "IdTask.h"
#include "audioneex.h"


class QFPIndexingTask : public IdTask
{
    uint32_t  m_FID;
    bool      m_Terminated;

    void DoIndexing()
    {
        size_t nrec = m_DataStore->GetFingerprintsCount();
	
        if(nrec==0)
           throw std::runtime_error
           ("No fingerprints found in the data store");
	
        std::cout << "Indexing " << nrec << " fingerprints" << std::endl;
	
        for(size_t FID=1; FID<=nrec; FID++)
        {
            if(m_Terminated) break;

            size_t fpsize;

            const uint8_t* fp = m_DataStore->GetFingerprint(FID, fpsize);

            if(fp==nullptr || fpsize==0){
               std::cout << "No fingerprint for FID " << FID
                         << ". Skipping..." << std::endl;
               continue;
            }

            std::cout <<"Indexing FID="<<FID<<std::endl;

            m_Indexer->Index(FID, fp, fpsize);
        }
    }

public:

    QFPIndexingTask() :
        m_FID        (0),
        m_Terminated (false)
    { }

    void Run()
    {
        assert(m_Indexer);
        assert(m_DataStore);

        m_Indexer->Start();
        DoIndexing();
        m_Indexer->End();
    }

    void Terminate() { m_Terminated = true; }
    AudioSource* GetAudioSource() { return nullptr; }
};


#endif
