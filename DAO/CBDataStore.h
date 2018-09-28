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


/// Struct representing a key in the datastore
struct CBKey
{
    int k1;
    int k2;
};

struct CBGetResp
{
    lcb_error_t status;
    std::vector<uint8_t>* buf;  // single-get buffer's handle
    size_t   data_size;         // size of data to read from value
    size_t   data_offset;       // offset from which to start reading
    size_t   read_size;         // size of data actually read
    size_t   value_size;        // the size of the value
};

struct CBSetResp
{
    std::string sender;
    lcb_error_t status;
    std::vector<CBKey> failedKeys;
};

struct CBHttpResp
{
    lcb_error_t status;
    std::string response;
};


//-----------------------------------------------------------------------------

class CBDataStore;

/// Defines a key-value database/collection in the data store.
/// This is represented by a file in the datastore directory.

class CBCollection
{
protected:

    CBDataStore*   m_Datastore;
    lcb_t          m_DBHandle;
    std::string    m_DBName;
    bool           m_IsOpen;

    /// Internal buffer for read/write operations
    std::vector<uint8_t>   m_Buffer;

    operator lcb_t() const { return m_DBHandle; }
    
public:

    CBCollection(CBDataStore* datastore);
    virtual ~CBCollection();

    /// Set the database file name
    void SetName(const std::string &name) { m_DBName = name; }

    /// Get the database file name
    std::string GetName() const { return m_DBName; }

    /// Open the databse
    void Open(int mode = OPEN_READ);

    /// Close the database
    void Close();

    /// Drop the database (all content cleared)
    void Drop();

    /// Query open status
    bool IsOpen() const { return m_IsOpen; }

    /// Get the number of records in the database
    std::uint64_t GetRecordsCount() const;

    /// Merge this collection to the given one
    virtual void Merge(CBCollection*) {}

};

// ----------------------------------------------------------------------------

/// The fingerprints index

class CBIndex : public CBCollection
{
    // Caches used during indexing 
    std::vector<CBKey>  m_KeyCache;
    BlockCache          m_BlocksCache;
    CBSetResp           m_StoreResp;
	
public:

    CBIndex(CBDataStore* dstore);
    ~CBIndex(){}

    /// Get the header for the specified index list
    Audioneex::PListHeader GetPListHeader(int list_id);

    /// Get the header for the spcified block in the specified list
    Audioneex::PListBlockHeader GetPListBlockHeader(int list_id, int block_id);

    /// Read the specified index list block data into 'buffer'. The 'headers'
    /// flag specifies whether to include the block headers in the read data.
    /// Return the number of read bytes.
    size_t ReadBlock(int list_id, 
                     int block_id, 
					 std::vector<uint8_t> &buffer, 
					 bool headers=true);

    /// Write the contents of the given block in the specified index list.
    /// A new block is created if the specified block does not exist.
    void WriteBlock(int list_id, 
                    int block_id, 
					std::vector<uint8_t> &buffer, 
					size_t data_size);

    /// Append a chunk to the specified block. If the block does not exist,
    /// a new one is created.
    void AppendChunk(int list_id,
                     Audioneex::PListHeader &lhdr,
                     Audioneex::PListBlockHeader &hdr,
                     uint8_t* chunk, size_t chunk_size,
                     bool new_block=false);

    /// Update the specified list header
    void UpdateListHeader(int list_id, Audioneex::PListHeader &lhdr);

    /// Merge this index with the given index.
    void Merge(CBCollection *plidx);

    /// Turn a raw block byte stream into a block structure.
    PListBlock RawBlockToBlock(uint8_t *block, 
                               size_t block_size, 
							   bool isFirst=false);
    
    /// Flush any remaining data in the block cache
    void FlushBlockCache();

    /// Reset all indexing caches
    void ResetCaches();
};

// ----------------------------------------------------------------------------

/// The fingerprints database

class CBFingerprints : public CBCollection
{
public:

    CBFingerprints(CBDataStore* dstore);
    ~CBFingerprints() = default;

    /// Read the size of the specified fingerprint (in bytes)
    size_t ReadFingerprintSize(uint32_t FID);

    /// Read the specified fingerprint's data into the given buffer. If 'size'
    /// is non zero, then 'size' bytes are read starting at offset bo (in bytes)
    size_t ReadFingerprint(uint32_t FID, 
                           std::vector<uint8_t> &buffer, 
						   size_t size, 
						   uint32_t bo);

    /// Write the given fingerprint into the database
    void   WriteFingerprint(uint32_t FID, 
                            const uint8_t *data, 
							size_t size);
};

// ----------------------------------------------------------------------------

/// Metadata database

class CBMetadata : public CBCollection
{
public:

    CBMetadata(CBDataStore* dstore);
    ~CBMetadata() = default;

    /// Read metadata for fingerprint FID
    std::string Read(uint32_t FID);

    /// Write metadata for fingerprint FID
    void   Write(uint32_t FID, const std::string& meta);
};

// ----------------------------------------------------------------------------

/// Datastore info database

class CBInfo : public CBCollection
{
public:

    CBInfo(CBDataStore* dstore);
    ~CBInfo() = default;

    DBInfo_t Read();
    void Write(const DBInfo_t &info);
};

// ----------------------------------------------------------------------------

/// Implements a data store connection. In our context a connection is
/// a communication channel (and related resources) to all the databases
/// used by the audio identification library, namely the index database and
/// the fingerprints database. Here we also use an additional "delta index"
/// database for the build-merge strategy.

class CBDataStore : public KVDataStore
{
    std::string           m_DBURL;          ///< URL to all database
    CBIndex               m_MainIndex;      ///< The index database
    CBIndex               m_DeltaIndex;     ///< The delta index database
    CBFingerprints        m_QFingerprints;  ///< The fingerprints database
    CBMetadata            m_Metadata;       ///< The metadata database
    CBInfo                m_Info;           ///< Datastore info

    std::string           m_ServerName;
    int                   m_ServerPort;
    std::string           m_Username;
    std::string           m_Password;

    bool                  m_IsOpen;
    
    /// Internal buffer used to cache all data accessed by the identification
    /// instance using this connection.
    std::vector<uint8_t>  m_ReadBuffer;

public:

    explicit CBDataStore(const std::string &url = std::string());
    ~CBDataStore(){}

    void Open(eOperation op = GET,
              bool use_fing_db=true,
              bool use_meta_db=false,
              bool use_info_db=false);

    void Close();

    void SetDatabaseURL(const std::string &url) { m_DBURL = url; }

    std::string GetDatabaseURL()  { return m_DBURL; }

    void SetServerName(const std::string &name)   { m_ServerName = name; }
    void SetServerPort(int port)                  { m_ServerPort = port; }
    void SetUsername(const std::string &username) { m_Username = username; }
    void SetPassword(const std::string &password) { m_Password = password; }
    std::string GetServerName() const             { return m_ServerName; }
    int         GetServerPort() const             { return m_ServerPort; }
    std::string GetUsername()   const             { return m_Username; }
    std::string GetPassword()   const             { return m_Password; }
    
    bool Empty();

    void Clear();

    bool IsOpen() { return m_IsOpen; }

    eOperation GetOpMode() { return m_Op; }

    void SetOpMode(eOperation mode) { m_Op = mode; }

    void PutFingerprint(uint32_t FID, const uint8_t* data, size_t size){
        m_QFingerprints.WriteFingerprint(FID, data, size);
    }

    void PutMetadata(uint32_t FID, const std::string& meta){
        m_Metadata.Write(FID, meta);
    }

    std::string GetMetadata(uint32_t FID) { return m_Metadata.Read(FID); }

    DBInfo_t GetInfo() { return m_Info.Read(); }

    void PutInfo(const DBInfo_t& info) { m_Info.Write(info); }

    // API Interface

    const uint8_t* GetPListBlock(int list_id, 
                                 int block, 
								 size_t& data_size, 
								 bool headers=true);
								 
    size_t GetFingerprintSize(uint32_t FID);
	
    const uint8_t* GetFingerprint(uint32_t FID, 
                                  size_t &read, 
								  size_t nbytes = 0, 
								  uint32_t bo = 0);
								  
    size_t GetFingerprintsCount();
    void OnIndexerStart();
    void OnIndexerEnd();
    void OnIndexerFlushStart();
    void OnIndexerFlushEnd();
    Audioneex::PListHeader OnIndexerListHeader(int list_id);
    Audioneex::PListBlockHeader OnIndexerBlockHeader(int list_id, int block);

    void OnIndexerChunk(int list_id,
                        Audioneex::PListHeader &lhdr,
                        Audioneex::PListBlockHeader &hdr,
                        uint8_t* data, size_t data_size);

    void OnIndexerNewBlock (int list_id,
                           Audioneex::PListHeader &lhdr,
                           Audioneex::PListBlockHeader &hdr,
                           uint8_t* data, size_t data_size);

    void OnIndexerFingerprint(uint32_t FID, uint8_t* data, size_t size);

private:

    eOperation m_Op;
    int        m_Run;
};


#endif
