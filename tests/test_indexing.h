/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef INDEXINGTEST_H
#define INDEXINGTEST_H

#include <cstdint>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>

#include "Fingerprinter.h"
#include "DataStore.h"
#include "Indexer.h"
#include "Utils.h"
#include "QFGenerator.h"
#include "AudioSource.h"
#include "audioneex.h"


// Just a dummy for testing purposes
class DummyAudioProvider : public Audioneex::AudioProvider
{
    public: int 
    OnAudioData(uint32_t FID, float *buffer, size_t nsamples) 
    { 
        return 0; 
    }
};


// This class will test indexing correctness
class IndexingTest : public Audioneex::AudioProvider
{
    KVDataStore*          m_DataStore;
    Audioneex::Indexer*   m_Indexer;

    QFGenerator           m_QFGenerator;
    uint32_t              m_FID;
    uint32_t              m_NQFs;

    AudioSourceFile       m_AudioSource;
    AudioBuffer<int16_t>  m_InputBuffer;
    AudioBuffer<float>    m_AudioBuffer;

    // AudioProvider implementation
    int 
    OnAudioData(uint32_t FID, float *buffer, size_t nsamples)
    {
        REQUIRE(FID == m_FID);

        m_InputBuffer.Resize( nsamples );
        m_AudioSource.GetAudioData( m_InputBuffer );

        if(m_InputBuffer.Size() == 0)
           return 0;

        m_InputBuffer.Normalize( m_AudioBuffer );

        std::copy(m_AudioBuffer.Data(),
                  m_AudioBuffer.Data() + m_AudioBuffer.Size(),
                  buffer);

        return m_AudioBuffer.Size();
    }

    // Audio indexing test
    void 
    DoIndexing()
    {
        m_AudioSource.Open("./data/rec1.mp3");
        m_Indexer->Index(++m_FID);
        m_AudioSource.Open("./data/rec2.mp3");
        REQUIRE_THROWS( m_Indexer->Index(m_FID) ); // Invalid FID
        m_AudioSource.Open("./data/rec2.mp3");
        m_Indexer->Index(++m_FID);
    }

    // Fingerprint indexing test
    void 
    DoIndexing2()
    {
        int prog, mem=0;

        for(size_t FID=3; FID<=m_NQFs; FID++)
        {
            prog = 100.f*FID/m_NQFs;

            DEBUG_MSG_N("\rTesting indexer ... "<<prog<<"%     ")

            auto& fp = m_QFGenerator.Generate();

            if(fp.empty())
               throw std::runtime_error
               ("Invalid fingerprint (null)");

            const uint8_t* fp_ptr = reinterpret_cast<uint8_t*>(fp.data());
            size_t   fp_bytes = fp.size() * sizeof(Audioneex::QLocalFingerprint_t);

            m_Indexer->Index(FID, fp_ptr, fp_bytes);

            // Cache usage must be monotonically increasing and always below
            // the set limit (the cache is always reset after flushing)
            REQUIRE( m_Indexer->GetCacheUsed() > mem );
            REQUIRE( (m_Indexer->GetCacheUsed()/1048576 < m_Indexer->GetCacheLimit()) );

            m_DataStore->PutFingerprint(FID,fp_ptr,fp_bytes);
            fp_ptr = m_DataStore->GetFingerprint(FID, fp_bytes);

            REQUIRE( (fp_ptr != nullptr && fp_bytes>0) );

            const Audioneex::QLocalFingerprint_t* fp2 =
                    reinterpret_cast<const Audioneex::QLocalFingerprint_t*>(fp_ptr);

            REQUIRE( (fp_bytes / sizeof(Audioneex::QLocalFingerprint_t) == fp.size()) );

            for(size_t n=0; n<fp.size(); n++){
                REQUIRE( fp2[n].T == fp[n].T );
                REQUIRE( fp2[n].F == fp[n].F );
                REQUIRE( fp2[n].W == fp[n].W );
                REQUIRE( fp2[n].E == fp[n].E );
            }

            //DEBUG_MSG("Indexing FID: "<<FID)
        }
    }

public:

    typedef std::unique_ptr <IndexingTest> Ptr;

    IndexingTest() 
    :
        m_DataStore   (nullptr),
        m_Indexer     (nullptr),
        m_FID         (0),
        m_NQFs        (1000),
        m_InputBuffer (Audioneex::Pms::Fs*10, Audioneex::Pms::Fs, Audioneex::Pms::Ca),
        m_AudioBuffer (Audioneex::Pms::Fs*10, Audioneex::Pms::Fs, Audioneex::Pms::Ca)
    {
        m_AudioSource.SetSampleRate( Audioneex::Pms::Fs );
        m_AudioSource.SetChannelCount( Audioneex::Pms::Ca );
        m_AudioSource.SetSampleResolution( 16 );
    }

    void 
    Run()
    {
        REQUIRE(m_DataStore);
        REQUIRE(m_Indexer);

        // Testing indexer

        m_Indexer->Start();
		
        // Try indexing some audio
        DoIndexing();

        // Try indexing some fingerprints
        DoIndexing2();

        m_Indexer->End();

        DEBUG_MSG(" - OK")
		
        // NOTE: write operations may be asynchronous (e.g. in Couchbase),
        // so we need to wait for all the fingerprints being indexed or
        // else the following tests will fail.
        while(m_DataStore->GetFingerprintsCount() != m_NQFs) 
        {
              std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        // Validate index

        size_t fp_count = m_DataStore->GetFingerprintsCount();

        REQUIRE(fp_count > 0);  // There must be fingerprints in the db

        std::unique_ptr<Audioneex::DataStoreImpl::PListIterator> it;

        uint32_t max_term;
        max_term = Audioneex::IndexerImpl::GetMaxTermValue(m_Indexer->GetMatchType());

        int prog;

        for(uint32_t t=0; t<=max_term; t++) {

            prog = 100.f*t/max_term;

            DEBUG_MSG_N("\rVerifing index ... "<<prog<<"%    ")

            // Get an iterator over the postings in the current term's list
            it.reset(Audioneex::DataStoreImpl::GetPListIterator(m_DataStore, t));

            uint32_t FIDo = 0;

            // Iterate through the postings
            for(; !it->get().empty(); it->next()) {

                Audioneex::DataStoreImpl::Posting_t &p = it->get();

                // FIDs must be <= the number of fingerprints in the store
                REQUIRE( p.FID <= fp_count);
                // FIDs must be strict increasing
                REQUIRE( p.FID > FIDo);

                // Iterate through the occurences
                for(size_t occ=0; occ<p.tf; occ++){
                    // LIDs must be strict increasing
                    // Ts must be monotically increasing
                    if(occ>0){
                       REQUIRE( p.LID[occ] > p.LID[occ-1] );
                       REQUIRE( p.T[occ] >= p.T[occ-1] );
                    }
                    // Quantization errors must be <= descriptor size
                    REQUIRE( p.E[occ] <= Audioneex::Pms::IDI );
                }
                FIDo = it->get().FID;
            }
        }

        DEBUG_MSG("\n - OK")

        // Verify fingerprints

        for(uint32_t FID=1; FID<=fp_count; FID++) {

            prog = 100.f*FID/fp_count;

            DEBUG_MSG_N("\rVerifing fingerprints ... "<<prog<<"%    ")

            size_t fp_size;
            const uint8_t* fp_ptr = m_DataStore->GetFingerprint(FID, fp_size);

            REQUIRE( fp_ptr!=nullptr );
            REQUIRE( fp_size>0 );
            REQUIRE( (fp_size % sizeof(Audioneex::QLocalFingerprint_t) == 0) );

            const Audioneex::QLocalFingerprint_t* fp;
            fp = reinterpret_cast<const Audioneex::QLocalFingerprint_t*>(fp_ptr);
            size_t nfp = fp_size / sizeof(Audioneex::QLocalFingerprint_t);

            for(size_t LID=0; LID<nfp; LID++){
                // Times must be monotically increasing
                if(LID>0)
                   REQUIRE( fp[LID].T >= fp[LID-1].T );
                // POI frequency must be within the considered range
                REQUIRE( (fp[LID].F >= Audioneex::Pms::Kmin && fp[LID].F <= Audioneex::Pms::Kmax) );
                // Auditory words IDs must be within Kmed (0-based)
                REQUIRE( fp[LID].W < Audioneex::Pms::Kmed );
                // Quantization errors must be <= descriptor size
                REQUIRE( fp[LID].E <= Audioneex::Pms::IDI );
            }
        }

        DEBUG_MSG("\n - OK")
    }

    void 
    SetFingerprintsNum(uint32_t num) 
    { 
        m_NQFs = num; 
    }
    
    void 
    SetDatastore(KVDataStore* dstore) 
    { 
        m_DataStore = dstore; 
    }
    
    void 
    SetIndexer(Audioneex::Indexer* indexer) 
    { 
        m_Indexer = indexer; 
    }
};


#endif
