/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <iostream>
#include <exception>
#include <cmath>

#include "common.h"
#include "Indexer.h"
#include "AudioCodes.h"
#include "DataStore.h"
#include "Parameters.h"
#include "Utils.h"

#ifdef TESTING
 #include "Tester.h"
#endif

TEST_HERE( namespace { Audioneex::Tester TEST; } )


//=============================================================================
//                                 Indexer
//=============================================================================

Audioneex::Indexer* Audioneex::Indexer::Create() { return new IndexerImpl; }

//=============================================================================
//                               IndexerImpl
//=============================================================================

// Statics

float Audioneex::IndexerImpl::qB        = float(Pms::Kmax - Pms::Kmin + 1) / Nbands;
int   Audioneex::IndexerImpl::Vpf_max   = ceil(qB / Pms::qF);
int   Audioneex::IndexerImpl::Vpt_max   = ceil(Tmax / Pms::qT);
int   Audioneex::IndexerImpl::WORD_BITS = ceil(Utils::log2(Pms::Kmed));
int   Audioneex::IndexerImpl::BAND_BITS = ceil(Utils::log2(Nbands));
int   Audioneex::IndexerImpl::VPT_BITS  = ceil(Utils::log2(Vpt_max));
int   Audioneex::IndexerImpl::VPF_BITS  = ceil(Utils::log2(Vpf_max)) + 1;//<- for negatives;
int   Audioneex::IndexerImpl::W1_SHIFT  = VPF_BITS + VPT_BITS + WORD_BITS + BAND_BITS;
int   Audioneex::IndexerImpl::B_SHIFT   = W1_SHIFT - BAND_BITS;
int   Audioneex::IndexerImpl::W2_SHIFT  = B_SHIFT - WORD_BITS;
int   Audioneex::IndexerImpl::VPT_SHIFT = W2_SHIFT - VPT_BITS;



Audioneex::IndexerImpl::IndexerImpl()
{
    // Check that all components fit into a word
    assert(2*WORD_BITS+BAND_BITS+VPT_BITS+VPF_BITS <= sizeof(int)*8);

    //size_t max_term = Pms::Kmed * Pms::Kmed * Nbands * Vpt_max * Vpf_max;
    //m_Cache.Create(max_term);
}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::Start()
{
    // Check if a session is already in progress
    if(m_SessionOpen)
       throw Audioneex::InvalidIndexerStateException
             ("An indexing session is already open.");

    // Check that we have a valid data store set.
    if(m_DataStore == nullptr)
       throw Audioneex::InvalidParameterException
             ("No data provider set.");

    // Get the audio codes (codebook) from the data store if none
    if(!m_AudioCodes)
    {
       m_AudioCodes = Audioneex::Codebook::deserialize(GetAudioCodes(),
                                                       GetAudioCodesSize());

       if(!m_AudioCodes)
          throw Audioneex::InvalidAudioCodesException
                ("Could't get audio codes");
    }

    m_Cache.Reset();

    m_CurrFID = 0;

    // At this point the session can be considered open
    m_SessionOpen = true;

    // Signal the data store that an indexing session has started.
    m_DataStore->OnIndexerStart();

}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::Index(uint32_t FID)
{
    // Check if a session is open
    if(!m_SessionOpen)
       throw Audioneex::InvalidIndexerStateException
             ("No indexing session open.");

    // Check whether a valid audio provider is set
    if(m_AudioProvider == nullptr)
       throw Audioneex::InvalidParameterException
             ("No audio provider set.");


    size_t tduration = 0;

    std::vector<Audioneex::QLocalFingerprint_t> QLFs;
    QLFs.reserve(4096);

    AudioBlock<float> buffer;
    AudioBlock<float> block;

    // Set a buffer large enough to fingerprint about 60 seconds of audio
    size_t bufferSize = static_cast<int>(Pms::Fs * 66) * Pms::Ca;

    // Get 5 second blocks from the audio provider
    size_t blockSize = static_cast<int>(Pms::Fs * 5) * Pms::Ca;

    // create the input data block and the accumulator buffer
    buffer.Create(bufferSize, Pms::Fs, Pms::Ca, 0);
    block.Create(blockSize+16, Pms::Fs, Pms::Ca);

    Fingerprint fingerprint( buffer.Capacity() + Pms::OrigWindowSize );

    // Fingerprinting and indexing loop.
    // Audio data is received from the registered audio provider and buffered
    // until a reasonable amount is reached. The provider will signal the end
    // of data by returning 0, or an error by returning a negative value.
    do{
        int nsamples = m_AudioProvider->OnAudioData(FID, block.Data(), blockSize);

        // Error getting data. Clean up and throw.
        if(nsamples < 0){
           m_Cache.Reset();
           throw std::runtime_error("Error getting audio data.");
        }

        block.Resize(nsamples);

        // Accumulate
        buffer.Append(block);

        tduration += block.Duration();

        if(tduration >= Pms::MaxRecordingLength)
           throw Audioneex::InvalidFingerprintException
                ("Recordings longer than 30m may affect performances. "
                 "Split them into 30m long parts and reindex them.");

        // Fingerprint 60 second audio chunks
        if(buffer.Duration() >= 60 || (buffer.Size()>0 && nsamples==0))
        {
           // compute fingerprint of current audio block
           fingerprint.Process(buffer);

           // get LF stream for current block
           const lf_vector &lfs = fingerprint.Get();

           // quantize the LFs
           for(const auto &lf : lfs)
           {
               Codebook::QResults quant = m_AudioCodes->quantize(lf);
               QLocalFingerprint_t qlf;
               qlf.T = lf.T;
               qlf.F = lf.F;
               qlf.W = quant.word;
               qlf.E = quant.dist; // Clipped. See NOTE in Codebook::quantize()
               QLFs.push_back(qlf);
               // Just check that we have a correct LF ID sequence
               assert(lf.ID == QLFs.size()-1);
           }

           buffer.Resize(0);
        }
    }
    while(block.Size() > 0);


    // This may happen if there is something wrong with the recording.
    if(QLFs.empty())
       throw Audioneex::InvalidFingerprintException
            ("No fingerprint for recording " + std::to_string(FID));

    // Check here the validity of the FIDs. This will avoid nasty issus afterwards.
    if(FID <= m_CurrFID)
       throw Audioneex::InvalidFingerprintException
            ("Invalid FID. Fingerprint IDs must be positive and strict increasing.");

    m_CurrFID = FID;

    // At this point we have successfully fingerprinted the audio. Proceed to indexing.

    IndexSTerms(m_CurrFID, QLFs.data(), QLFs.size());

    // Emit the quantized fingerprint
    uint8_t* QLFs_ptr = reinterpret_cast<uint8_t*>(QLFs.data());
    size_t QLFs_nbytes = QLFs.size() * sizeof(QLocalFingerprint_t);

    m_DataStore->OnIndexerFingerprint(m_CurrFID, QLFs_ptr, QLFs_nbytes);

//m_Cache.Dump();
    // Check whether the cache needs to be flushed to disk
    if(m_Cache.CanFlush())
       Flush();
}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::Index(uint32_t FID, const uint8_t *fpdata, size_t fpsize)
{
    // Check if a session is open
    if(!m_SessionOpen)
       throw Audioneex::InvalidIndexerStateException
             ("No indexing session open.");

    // Check fingerprint validity

    if(fpdata == nullptr)
       throw Audioneex::InvalidFingerprintException
             ("Invalid fingerprint data (null)");

    if(fpsize == 0)
       throw Audioneex::InvalidFingerprintException
             ("Invalid fingerprint size (0)");

    if(fpsize % sizeof(QLocalFingerprint_t) != 0)
       throw Audioneex::InvalidFingerprintException
             ("Invalid fingerprint size");

    // Check here the validity of the FIDs. This will avoid nasty issus afterwards.
    if(FID <= m_CurrFID)
       throw Audioneex::InvalidFingerprintException
            ("Invalid FID. Fingerprint IDs must be positive and strict increasing.");

    m_CurrFID = FID;

    size_t NLFs = fpsize / sizeof(Audioneex::QLocalFingerprint_t);

    const QLocalFingerprint_t *QLFs = reinterpret_cast<const QLocalFingerprint_t*>(fpdata);

    IndexSTerms(FID, QLFs, NLFs);

//m_Cache.Dump();
    // Check whether the cache needs to be flushed to disk
    if(m_Cache.CanFlush())
       Flush();
}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::IndexSTerms(uint32_t FID, const QLocalFingerprint_t *lfs, size_t Nlfs)
{
    assert(lfs != nullptr);

    // Indexing loop
    for(size_t i=0; i<Nlfs; i++)
    {
        // Create term <word|channel>
        int chan = (lfs[i].F - Pms::Kmin + 1) / Pms::qF;
        int term = (lfs[i].W << 6) | chan;

        m_Cache.Update(term, FID, i, lfs[i].T, lfs[i].E);
    }
}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::DoFlush()
{
    // Produce and emit postings lists chunks in lexicographic order.

    // In order to produce a chunk to be appended to the postings list
    // we need the length of the last block in the postings list and
    // the last FID used in that block.
    // The length of the last block is used to decide whether the
    // produced chunk can still be appended to it or needs to be put in
    // a new block because the block's threshold size has been exceeded.
    // Based on this decision, the delta encoding of the FIDs in the
    // chunk will either continue from the last used in the block or will
    // start from the first FID of the new block.
    // Both these info are found in the block's header.
    // We also need to know the last block number for the currently
    // processed postings list. This value is equal to the number of
    // blocks in the list, so we can retrieve this value (stored in the
    // postings list header).

    Audioneex::BlockEncoder blockEncoder;

    // Allocate a buffer to host the encoded block chunks.
    // NOTE: If this size is not enough to hold the compressed chunks the
    //       encoding will fail!
    //       Here we don't know the size of the serialized chunks, so this
    //       size must be big enough to accommodate all of them, since the
    //       buffer can't be reallocated in the encoding routines.
    std::vector<uint8_t> bchunk( BlockEncoder::GetEncodedSizeEstimate(DataStoreImpl::POSTINGSLIST_BLOCK_THRESHOLD) );

    std::vector<uint32_t*> plchunk;

    uint8_t* bchunk_ptr = bchunk.data();
    size_t bchunk_size  = bchunk.capacity();

    size_t plchunk_size       = 0;
    size_t plchunk_size_bytes = 0;
    size_t plchunk_nposts     = 0;

    IndexCache::buffer_type &buffer = m_Cache.GetBuffer();


    for(IndexCache::buffer_type::value_type &elem : buffer)
    {
        int term = elem.first;
        std::vector<uint32_t> &plist = elem.second;

        assert(!plist.empty());

        // Get the postings list header. It will be used to read the number of
        // blocks in the list, which is equal to the last block number/id.
        PListHeader lhdr = m_DataStore->OnIndexerListHeader(term);

        PListBlockHeader hdr = {};

        // Get the last block header. If we get a null list header then we
        // assume the postings list does not exist, else we must receive a
        // non-null block's header. If not, we have an inconsistent index.
        if(!IsNull(lhdr))
		{
           hdr = m_DataStore->OnIndexerBlockHeader(term, lhdr.BlockCount);
		   
           if(IsNull(hdr))
               throw Audioneex::InvalidIndexDataException
                   ("Got an empty header for existing block ?");
        }

        // Minimum size of a posting must be <FID,tf,LID,T,E>
        assert(plist.size() >= 5);

        // Split the cached postings lists into chunks at the posting level

        // Get pointers to the last and first posting
        uint32_t tflast   = plist.back();
        uint32_t last_pos = plist.size()-1-3*tflast-2;
        uint32_t* pcurr   = plist.data();
        uint32_t* plast   = plist.data() + last_pos;

        // Iterate thru the postings until we reach the chunk limit
        while(pcurr <= plast)
        {
            plchunk.push_back(pcurr);

            plchunk_nposts++;
            // Accumulate size of postings <FID,tf,{LID},{T},{E}>
            plchunk_size += 2 + *(pcurr+1) * 3;
            plchunk_size_bytes = plchunk_size * sizeof(uint32_t);

            if(plchunk_size_bytes >= DataStoreImpl::POSTINGSLIST_CHUNK_THRESHOLD ||
               pcurr == plast)
            {
               // Here we could insert a warning if there are chunks that are
               // much bigger than the block limit (say 2-3x bigger)
               // ...

               // Clients can mess up the FIDs so we check them here before they
               // get into the system.
               if(*plchunk.back() <= hdr.FIDmax)
                  throw Audioneex::InvalidIndexDataException
                       ("Invalid FID have been assigned. When adding new "
                        "fingerprints make sure that the new FID are strict "
                        "increasing from the maximum FID in the database "
                        "(new FID "+std::to_string(*plchunk.back())+
                        " must be > max FID "+std::to_string(hdr.FIDmax)+").");

               const uint32_t* const* plchunk_ptr = plchunk.data();

               size_t ebytes = 0;

               // Append the chunk to the current block if its size is below the threshold
               // else append it to a new block
               if(!IsNull(hdr) && hdr.BodySize < DataStoreImpl::POSTINGSLIST_BLOCK_THRESHOLD){

                   blockEncoder.Encode(plchunk_ptr, plchunk_nposts,
                                       bchunk_ptr, bchunk_size,
                                       ebytes, hdr.FIDmax);
                   hdr.BodySize += ebytes;
                   hdr.FIDmax = *plchunk.back();
                   m_DataStore->OnIndexerChunk(term, lhdr, hdr, bchunk_ptr, ebytes);
               }
			   else{
                   blockEncoder.Encode(plchunk_ptr, plchunk_nposts,
                                       bchunk_ptr, bchunk_size,
                                       ebytes, 0);
                   hdr.ID++;
                   hdr.BodySize = ebytes;
                   hdr.FIDmax = *plchunk.back();
                   lhdr.BlockCount++;
                   m_DataStore->OnIndexerNewBlock(term, lhdr, hdr, bchunk_ptr, ebytes);
               }

               plchunk.clear();
               plchunk_nposts = 0;
               plchunk_size = 0;
               plchunk_size_bytes = 0;
            }

            // Next posting
            pcurr += 2 + *(pcurr+1) * 3;
        }

        assert(plchunk.empty());
        //plist.clear();

    }//end for(list,buffer)

}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::Flush()
{
    // Check if a session is open
    if(!m_SessionOpen)
       throw Audioneex::InvalidIndexerStateException("No indexing session open.");

    m_DataStore->OnIndexerFlushStart();
    DoFlush();
    m_Cache.Reset();
    m_DataStore->OnIndexerFlushEnd();
}

// ----------------------------------------------------------------------------

void Audioneex::IndexerImpl::End(bool flush)
{
    // Return if no session is open
    if(!m_SessionOpen)
       return;

    // Flush any remaining data in the cache before closing.
    if(!m_Cache.IsEmpty() && flush)
       Flush();

    // Do all the cleanup here ...

    m_SessionOpen = false;

    // Signal the data store that the indexing session has ended
    m_DataStore->OnIndexerEnd();
}

// ----------------------------------------------------------------------------

uint32_t Audioneex::IndexerImpl::GetMaxTermValue()
{
    return Pms::Kmed << 6 | Pms::GetChannelsCount();
}


//=============================================================================
//                               IndexCache
//=============================================================================



void Audioneex::IndexCache::Update(int term, int FID, int LID, int T, int E)
{
    std::vector<uint32_t> &plist = m_Buffer[term];

    // List is empty. Append a new posting.
    if (plist.empty()){
        plist.push_back(FID);
        plist.push_back(1);
        plist.push_back(LID);
        plist.push_back(T),
        plist.push_back(E);
        plist.push_back(1);
        // Update cache usage
        m_TotalPostings++;
        m_MemoryUsed += sizeof(std::vector<uint32_t>) +
                        sizeof(buffer_type::value_type) +
                        plist.size() * sizeof(uint32_t);
    }
    else
    {
        // Get last posting's FID and tf
        uint32_t Ne = plist.back(); //< # of entries in the last posting
        uint32_t FIDo = plist[plist.size()-1-3*Ne-2];
        uint32_t &tfo = plist[plist.size()-1-3*Ne-1];

        assert(Ne == tfo);

        // This should never happen as we check the validity of the FIDS in the
        // Index() methods, so if it does happen we've got a bug.
        assert(FID >= FIDo);


        // Current FID is the last inserted posting. Append new element.
        if(FID == FIDo){
            uint32_t last_LID = plist[plist.size()-1-3];
            uint32_t last_T   = plist[plist.size()-1-2];
            uint32_t last_E   = plist[plist.size()-1-1];

            // Skip and keep track of duplicate postings
            if(LID==last_LID && T==last_T && E==last_E){
               m_DuplicateOcc++;
               return;
            }

            // update tf
            Ne++;
            tfo++;

            assert(Ne == tfo);

            // Remove tail header before appending
            plist.resize(plist.size()-1);

            // Append new posting element
            plist.push_back(LID);
            plist.push_back(T);
            plist.push_back(E);
            plist.push_back(Ne);

            // Update cache usage
            m_MemoryUsed += 3 * sizeof(uint32_t);
        }
        // New posting
        else if(FID > FIDo){

            plist.resize(plist.size()-1);

            plist.push_back(FID);
            plist.push_back(1);
            plist.push_back(LID);
            plist.push_back(T),
            plist.push_back(E);
            plist.push_back(1);

            // Update cache usage
            m_TotalPostings++;
            m_MemoryUsed += 5 * sizeof(uint32_t);
        }
        // (FID < FIDo) should never occurr here as this condition
        // is checked upfront in the Index() method.
    }

}

// ----------------------------------------------------------------------------

bool Audioneex::IndexCache::CanFlush() const
{
    return m_MemoryUsed / 1048576 >= m_MemoryLimit;
}

// ----------------------------------------------------------------------------

void Audioneex::IndexCache::Reset()
{
    // Clear postings lists
    m_Buffer.clear();
    m_MemoryUsed = 0;
    m_TotalPostings = 0;
    m_DuplicateOcc=0;
}

