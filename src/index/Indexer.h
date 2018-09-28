/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef INDEXER_H
#define INDEXER_H

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <cstdint>
#include <boost/unordered_map.hpp>

#include "Codebook.h"
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

/// IndexCache implements a temporary memory buffer for caching the
/// postings lists prior to flushing to the index on disk.

class IndexCache
{
public:

    //typedef std::map<int, std::vector<uint32_t> > buffer_type;
    typedef boost::unordered::unordered_map<int, std::vector<uint32_t> > buffer_type;

    IndexCache() = default;
   ~IndexCache() = default;

    /// Update the cache by appending the given posting's payload to the
    /// last posting in the list for the specified term. If the posting
    /// does not exist, append a new one.
    void Update(int term, int FID, int LID, int T, int E);

    /// Specify the condition that must be met in order for the cache contents
    /// to be flushed on disk.
    bool CanFlush() const;

    /// Get the internal buffer (a map in the current implementation)
    buffer_type& GetBuffer() { return m_Buffer; }

    /// Reset the cache
    void Reset();

    /// Check whether the cache is empty
    bool IsEmpty() const { return m_Buffer.empty(); }

    /// Clear the specified postings list
    void ClearList(int term) { m_Buffer[term].clear(); }

    /// Set the memory limit (in MB) beyond which the cache is flushed
    void   SetMemoryLimit(size_t limit) { m_MemoryLimit = limit; }
    size_t GetMemoryLimit() const { return m_MemoryLimit; }

    /// Get the current cache size in bytes.
    size_t GetMemoryUsed() const { return m_MemoryUsed; }

    /// This is only called for monitoring the rate of duplicate
    /// occurences.
    size_t GetDuplicateOcc() const { return m_DuplicateOcc; }
    void   SetDuplicateOcc(size_t val) { m_DuplicateOcc = val; }

//void Dump(){
//    //DEBUG_MSG("# of entries: " << m_BufferSize)
//    DEBUG_MSG("# of entries: " << m_Buffer.size())
//    DEBUG_MSG("# of postings: " << m_TotalPostings)
//    DEBUG_MSG("Memory used (MB): " << m_MemoryUsed/1048576)
//}

private:

    buffer_type  m_Buffer;              ///< The cache holding the postings lists.
    size_t       m_MemoryLimit   {128}; ///< Max memory (in MB) used by the cache before flushing.
    size_t       m_MemoryUsed    {0};   ///< Memory currently used by the postings lists (in bytes).
    size_t       m_TotalPostings {0};   ///< The total number of postings currently indexed.

    /// Duplicate occurences in a posting list may happen as the result of
    /// quantization errors during the indexing process. It is not harmful
    /// if it happens sporadically. However we track it and watch it.
    size_t       m_DuplicateOcc  {0};

};

// ----------------------------------------------------------------------------

/// The Indexer implementation

class AUDIONEEX_API_TEST IndexerImpl : public Audioneex::Indexer
{
public:
    // Number of frequency bands
    static const uint32_t Nbands = 3;

    // This is the max distance of paired LFs from pivot.
    // It may be in number of LFs or time units. This value determines the size
    // of the final index. The bigger it is, the more LFs are paired to the pivot.
    static const size_t Dmax = 10;

    // Max time distance of paired LFs from pivot. May be used in conjunction
    // with Dmax
    static const size_t Tmax = 73;

    // Bandwidth in frequency units
    static float qB;

    // Max value of Vp frequency component in quantized units
    static int Vpf_max;

    // Max value of Vp time component in quantized units
    static int Vpt_max;

    // Number of bits needed to hash the various components of a term
    static int WORD_BITS;
    static int BAND_BITS;
    static int VPT_BITS;
    static int VPF_BITS;

    static int W1_SHIFT;
    static int B_SHIFT;
    static int W2_SHIFT;
    static int VPT_SHIFT;


    IndexerImpl();

    /// Start the indexing session.
    void Start();

    /// Do indexing of audio recordings. The client shall call this method
    /// for every audio recording to be fingerprinted by passing its unique
    /// fingerprint identifier (FID). Fingerprint Identifiers must be non
    /// negative integral numbers monotonically increasing. Upon calling this
    /// method, the engine shall call the client's audio provider's in order
    /// to get the recording's audio data. This is accomplished by repeatedly
    /// calling AudioProvider::OnAudioData() until there is no more audio for
    /// the recording with identifier FID. The fingerprint for the current
    /// audio recording is emitted by calling DataStore::OnIndexerFingerprint()
    /// and is indexed according to the currently set match type.
    void Index(uint32_t FID);

    ///
    void Index(uint32_t FID, const uint8_t* fpdata, size_t fpsize);

    /// Flush the serialized in-memory index to the InvertedIndex.
    void Flush();

    /// End the indexing session
    void End(bool flush = true);

    /// Set the memory limit (in MB) after which the cached index is flushed
    void SetCacheLimit(size_t limit) { m_Cache.SetMemoryLimit(limit); }

    size_t GetCacheLimit() const { return m_Cache.GetMemoryLimit(); }

    /// Return the amount of memory currently used by the cache in bytes.
    size_t GetCacheUsed() const { return m_Cache.GetMemoryUsed(); }

    /// Set the client's data store implementation
    void SetDataStore(Audioneex::DataStore* dstore) { m_DataStore = dstore; }

    Audioneex::DataStore* GetDataStore() const { return m_DataStore; }

    /// Set the client's audio provider implementation
    void SetAudioProvider(Audioneex::AudioProvider* aprovider) { m_AudioProvider = aprovider; }

    Audioneex::AudioProvider* GetAudioProvider() const { return m_AudioProvider; }

    /// Get the maximum possible value that a term can take.
    static uint32_t GetMaxTermValue();

private:

    Audioneex::DataStore*       m_DataStore      {nullptr};
    Audioneex::AudioProvider*   m_AudioProvider  {nullptr};
    bool                        m_SessionOpen    {false};
    uint32_t                    m_CurrFID        {0};
    IndexCache                  m_Cache;
    std::unique_ptr <Codebook>  m_AudioCodes;

    void DoFlush();
    void IndexSTerms(uint32_t FID, const QLocalFingerprint_t* lfs, size_t Nlfs);
    void IndexBTerms(uint32_t FID, const QLocalFingerprint_t *lfs, size_t Nlfs);

};

}// end namespace Audioneex

#endif // INDEXER_H
