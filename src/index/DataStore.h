/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef DATASTORE_H
#define DATASTORE_H

#include <stdint.h>
#include <stdexcept>

#include "common.h"
#include "BlockCodec.h"
#include "Utils.h"
#include "audioneex.h"

// The following classes are not part of the public API but we need
// their interfaces exposed when testing DLLs.
#ifdef TESTING
  #define AUDIONEEX_API_TEST AUDIONEEX_API
#else
  #define AUDIONEEX_API_TEST
#endif


namespace Audioneex
{
namespace DataStoreImpl
{

/// Fuzzy block size limiter. Postings list blocks growing beyond this limit
/// will be terminated (not truncated) and new appended chunks will be put
/// in a new block. This allows us to have blocks of similar sizes.
const int POSTINGSLIST_BLOCK_THRESHOLD = 32768;

/// Postings list chunk size limit. A chunk of about 20-25% the block's limit
/// should be sufficient. For uncompressed chunks 8-10% should be enough.
const int POSTINGSLIST_CHUNK_THRESHOLD = POSTINGSLIST_BLOCK_THRESHOLD * 0.2;


/// Posting cursor used to iterate over the postings lists.
struct AUDIONEEX_API_TEST Posting_t
{
    uint32_t  FID {0};
    uint32_t  tf  {0};
    uint32_t* LID {nullptr};
    uint32_t* T   {nullptr};
    uint32_t* E   {nullptr};

    bool empty() const { 
        return !FID && !tf && !LID && !T && !E;
	}
	
    void reset() { 
        FID=0; tf=0; LID=nullptr; T=nullptr; E=nullptr;
    }
	
    operator bool() const { 
        return !empty(); 
    }
};

/// NOTE: To iterate over the postings we need 2 iterators: one to iterate
///       over the blocks and another to iterate over the postings within
///       a block. However, the block iterator will be transparent to the
///       clients, which will have to deal only with the postings iterator
///       interface.

/// PListIterator provides an interface to iterate over the postings
/// in a postings list. The actual postings iterator is "embedded" within
/// this iterator.
class AUDIONEEX_API_TEST PListIterator
{
    friend AUDIONEEX_API_TEST PListIterator* GetPListIterator(Audioneex::DataStore* store, int term);

    Audioneex::DataStore*    m_DataStore   {nullptr};
    int                      m_Term        {0};
    uint32_t                 m_NextBlock   {1};
    Posting_t                m_Cursor;
    bool                     m_EOL         {false};
    BlockEncoder             m_BlockCodec;
    std::vector<uint32_t>    m_BlockDecoded;

    // -------- Postings iterator ---------

    uint32_t*                m_begin       {nullptr};
    uint32_t*                m_end         {nullptr};

    // Fetch the next block from the index. Returns false if there are
    // no more blocks (EOL), true otherwise.
    bool NextBlock()
    {
        size_t block_size = 0;
        size_t m_BlockDecoded_size = 0; // No vector::resize() pls

        const uint8_t* pblock = m_DataStore->GetPListBlock(m_Term,
                                                           m_NextBlock,
                                                           block_size);

        if(pblock && block_size)
		{
            size_t decsize = BlockEncoder::GetDecodedSizeEstimate(block_size);

            if(m_BlockDecoded.capacity() < decsize)
               m_BlockDecoded.reserve(decsize);

            int res = m_BlockCodec.Decode(pblock, block_size,
                                          m_BlockDecoded.data(),
                                          m_BlockDecoded.capacity(),
                                          m_BlockDecoded_size);

            // If decoding fails the client provided invalid data.
            if(res<0)
               throw Audioneex::InvalidIndexDataException
                    ("Block decoding failed. Invalid data.");

            // If this happens, we've got a bug
            if(m_BlockDecoded_size==0)
               throw std::runtime_error("Block decoding failed.");

            m_begin = m_BlockDecoded.data();
            m_end   = m_begin + m_BlockDecoded_size;
            m_NextBlock++;
            return true;
        }
		else{
            m_Cursor.reset();
            m_EOL   = true;
            return false;
        }
    }

    /// Get the next posting in the current block.
    void NextPosting()
    {
        if(m_begin < m_end){
            m_Cursor.FID = *m_begin++;
            m_Cursor.tf  = *m_begin++;
            m_Cursor.LID = m_begin; m_begin+=m_Cursor.tf;
            m_Cursor.T   = m_begin; m_begin+=m_Cursor.tf;
            m_Cursor.E   = m_begin; m_begin+=m_Cursor.tf;
        }
		else {
            m_Cursor.reset();
		}
    }

public:

    PListIterator()
    {
        // reserve some space for the decoded blocks
        m_BlockDecoded.resize(POSTINGSLIST_BLOCK_THRESHOLD);
        // We could load the 1st block here
        //...
    }

    /// Advance the cursor to the next postings in the list. When the end
    /// of the list is reached, an empty posting will be set.
    void next()
    {
        if(m_EOL) return;
        NextPosting();
        if(m_Cursor.empty() && NextBlock())
           NextPosting();
    }

    /// Get the posting at current cursor position.
    Posting_t& get()
    {
        // If the iterator hasn't been initialized, call next()
        if(!m_begin)
            next();
        return m_Cursor;
    }

};

/// Get a postings itarator for the specified postings list from the specified data store.
AUDIONEEX_API_TEST inline PListIterator* GetPListIterator(Audioneex::DataStore* store, int term){
    assert(store != nullptr);
    PListIterator* it = new PListIterator;
    it->m_Term = term;
    it->m_DataStore = store;
    return it;
}

} // end namespace DatastoreImpl

}// end namespace Audioneex

#endif // DATASTORE_H
