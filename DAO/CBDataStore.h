/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#ifndef CBDATASTORE_H
#define CBDATASTORE_H

#include <libcouchbase/couchbase.h>
#include "KVDataStore.h"

//-----------------------------------------------------------------------------

struct CBKey
{
    int k1 {0};
    int k2 {0};
};

struct CBGetResp
{
    lcb_error_t status        {};
    std::vector<uint8_t>* buf {nullptr};  // single-get buffer's handle
    size_t   data_size        {0};        // size of data to read from value
    size_t   data_offset      {0};        // offset from which to start reading
    size_t   read_size        {0};        // size of data actually read
    size_t   value_size       {0};        // the size of the value
};

struct CBSetResp
{
    std::string        sender;
    lcb_error_t        status {};
    std::vector<CBKey> failedKeys;
};

struct CBHttpResp
{
    lcb_error_t status;
    std::string response;
};


//-----------------------------------------------------------------------------

class CBDataStore;

/// This class provides common database operations for accessing collections in
/// the Couchbase datastore (where they're called 'buckets'), such as opening,
/// closing, deleting, getting the number of records, etc. It does not provide 
/// facilities to read and write data, which are implemented in derived classes 
/// depending on the type of collection.

class CBCollection : public KVCollection
{
protected:

    lcb_t  m_DBHandle;

    operator lcb_t() const {
        return m_DBHandle;
    }

    void 
    THROW_ON_FAIL(const lcb_error_t &res, 
                  const char *msg) const;
	
public:

    CBCollection(CBDataStore*);
    ~CBCollection();

    /// Open the collection
    void 
    Open(int mode = OPEN_READ) override;

    /// Close the collection
    void 
    Close() override;

    /// Drop the collection (all content cleared)
    void 
    Drop() override;

    /// Get the number of records in the collection
    std::uint64_t 
    GetRecordsCount() const override;

};


// ----------------------------------------------------------------------------

/// This class implements functionalities to manipulate a fingerprinting index,
/// a specialized collection of data blocks with a specific layout used in the 
/// recognition process.

class CBIndex : public CBCollection
{
    std::vector<CBKey>  m_KeyCache;
    BlockCache          m_BlocksCache;
    CBSetResp           m_StoreResp;

    /// Conversion method to turn a raw block byte stream into a block structure.
    PListBlock 
    RawBlockToBlock(uint8_t *block, 
                    size_t block_size, 
                    bool isFirst=false);

public:

    CBIndex(CBDataStore*);
    ~CBIndex() = default;

    /// Get the header for the specified index list
    Audioneex::PListHeader 
    GetPListHeader(int list_id);

    /// Get the header for the specified block in the specified list
    Audioneex::PListBlockHeader 
    GetPListBlockHeader(int list_id, int block_id);

    /// Read the specified index list block data into 'buffer'. The 'headers'
    /// flag specifies whether to include the block headers in the read data.
    /// Return the number of read bytes.
    size_t 
    ReadBlock(int list_id, 
              int block_id, 
              std::vector<uint8_t> &buffer, 
               bool headers=true);

    /// Write the contents of the given block in the specified index list.
    /// A new block is created if the specified block does not exist.
    void 
    WriteBlock(int list_id, 
               int block_id, 
               std::vector<uint8_t> &buffer, 
               size_t data_size);

    /// Append a chunk to the specified block. If the block does not exist,
    /// a new one is created.
    void 
    AppendChunk(int list_id,
                Audioneex::PListHeader &lhdr,
                Audioneex::PListBlockHeader &hdr,
                uint8_t* chunk, size_t chunk_size,
                bool new_block=false);

    /// Update the specified list header
    void 
    UpdateListHeader(int list_id, Audioneex::PListHeader &lhdr);

    /// Merge this index with the given one.
    void 
    Merge(CBIndex &lidx);
    
    /// Flush any remaining data in the block cache
    void 
    FlushBlockCache();

    /// Reset all indexing caches
    void 
    ResetCaches();
};


// ----------------------------------------------------------------------------

/// This class implements functionalities to manipulate a fingerprints collection.

class CBFingerprints : public CBCollection
{
public:

    CBFingerprints(CBDataStore*);
    ~CBFingerprints() = default;

    /// Read the size of the specified fingerprint (in bytes)
    size_t 
    ReadFingerprintSize(uint32_t FID);

    /// Read the specified fingerprint's data into the given buffer. If 'size'
    /// is non zero, then 'size' bytes are read starting at offset bo (in bytes)
    size_t 
    ReadFingerprint(uint32_t FID, 
                    std::vector<uint8_t> &buffer, 
                    size_t size, 
                    uint32_t bo);

    /// Write the given fingerprint into the database
    void 
    WriteFingerprint(uint32_t FID, 
                     const uint8_t *data, 
                     size_t size);
};


// ----------------------------------------------------------------------------

/// This class implements functionalities to manipulate a metadata collection.

class CBMetadata : public CBCollection
{
public:

    CBMetadata(CBDataStore*);
    ~CBMetadata() = default;

    /// Read metadata for fingerprint FID
    std::string 
    Read(uint32_t FID);

    /// Write metadata for fingerprint FID
    void
    Write(uint32_t FID, const std::string& meta);
};


// ----------------------------------------------------------------------------

/// This class implements functionalities to manipulate a collection of custom info.

class CBInfo : public CBCollection
{
public:

    CBInfo(CBDataStore*);
    ~CBInfo() = default;

    /// Read custom info
    DBInfo_t
    Read();

    /// Store custom info
    void 
    Write(const DBInfo_t &info);
};


// ----------------------------------------------------------------------------

/// This is an implementation of the KVDataStore interface that uses Couchbase
/// as the data storage backend. It deals with all the low level data manipulation
/// to store and retrieve objects to/from the database by using the CB C API.
/// You can see it as a communication channel to all the stored data used by the
/// recognition engine. We also use a "delta index" for build-merge strategies.


class CBDataStore : public KVDataStore
{
    CBIndex               m_MainIndex;
    CBIndex               m_DeltaIndex;
    CBFingerprints        m_QFingerprints;
    CBMetadata            m_Metadata;
    CBInfo                m_Info;
    std::vector<uint8_t>  m_ReadBuffer;

public:

    explicit CBDataStore(const std::string &url = std::string());
    ~CBDataStore() = default;

    /// Open the datastore in the specified mode using the specified collections.
    void 
    Open(eOperation op = GET,
         bool use_fing_db=false,
         bool use_meta_db=false,
         bool use_info_db=false) override;

    /// Close the datastore
    void 
    Close() override;
	
    /// Check whether the datastore contains no data
    bool 
    Empty() override;

    /// Clear the datastore (delete all contents)
    void 
    Clear() override;

    /// Store a fingerprint
    void 
    PutFingerprint(uint32_t FID, const uint8_t* data, size_t size)  override {
        m_QFingerprints.WriteFingerprint(FID, data, size);
    }

    /// Get a fingerprint
    const uint8_t* 
    GetFingerprint(uint32_t FID, 
                   size_t &read, 
                   size_t nbytes = 0, 
                   uint32_t bo = 0) override;

    /// Store metadata for the specified fingerprint
    void 
    PutMetadata(uint32_t FID, const std::string& meta) override {
        m_Metadata.Write(FID, meta);
    }

    /// Get metadata for the specified fingerprint
    std::string 
    GetMetadata(uint32_t FID) override {
        return m_Metadata.Read(FID);
    }

    /// Store custom info
    void 
    PutInfo(const DBInfo_t& info) override { 
        m_Info.Write(info); 
    }

    /// Get custom info
    DBInfo_t 
    GetInfo() override {
        return m_Info.Read(); 
    }


    // API Interface

    const uint8_t*
    GetPListBlock(int list_id, 
                  int block, 
                  size_t& data_size, 
                  bool headers=true) override;

    size_t
    GetFingerprintSize(uint32_t FID) override;

    size_t 
    GetFingerprintsCount() override;

    void 
    OnIndexerStart() override;

    void
    OnIndexerEnd() override;

    void
    OnIndexerFlushStart() override;

    void 
    OnIndexerFlushEnd() override;
	
    Audioneex::PListHeader
    OnIndexerListHeader(int list_id) override;

    Audioneex::PListBlockHeader 
    OnIndexerBlockHeader(int list_id, int block) override;

    void
    OnIndexerChunk(int list_id,
                   Audioneex::PListHeader &lhdr,
                   Audioneex::PListBlockHeader &hdr,
                   uint8_t* data, size_t data_size) override;

    void
    OnIndexerNewBlock (int list_id,
                       Audioneex::PListHeader &lhdr,
                       Audioneex::PListBlockHeader &hdr,
                       uint8_t* data, size_t data_size) override;

    void
    OnIndexerFingerprint(uint32_t FID,
                         uint8_t* data,
                         size_t size) override;

private:

    int  m_Run  {0};
};

#endif

