/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/


#ifndef DATASTORETYPES_H
#define DATASTORETYPES_H

#include <cstdint>
#include <vector>
#include <memory>
#include <boost/unordered_map.hpp>

#include "audioneex.h"

typedef boost::unordered::unordered_map< int, std::vector<uint8_t> > block_map;
struct DBInfo_t;

/// Database opening modes
enum{
    OPEN_READ,
    OPEN_WRITE,
    OPEN_READ_WRITE
};


/// This abstract class implements the DataStore interface and extends it by
/// adding common database operations and some other application-specific
/// functionality. We use key-value datastores, so we name it accordingly.

class KVDataStore : public Audioneex::DataStore
{
public:

    enum eOperation{
        GET,
        BUILD,
        BUILD_MERGE
    };

protected:

    std::string  m_DBURL;
    std::string  m_ServerName;
    int          m_ServerPort  {0};
    std::string  m_Username;
    std::string  m_Password;
    bool         m_IsOpen      {false};
    eOperation   m_Op          {GET};

public:

	KVDataStore() = default;
    virtual ~KVDataStore() = default;

    /// Open the datastore. This will open all database/collections
    /// used by the recognition engine.
    virtual void Open(eOperation op = GET,
                      bool use_fing_db=false,
                      bool use_meta_db=false,
                      bool use_info_db=false) = 0;

    /// Close the datastore. This will close all database/collections
    /// used by the identification engine
    virtual void Close() = 0;
	
    /// Chech whether the datastore is empty
    virtual bool Empty() = 0;

    /// Clear the datastore (delete all data)
    virtual void Clear() = 0;

    /// Query for open status
    virtual bool IsOpen() const {
        return m_IsOpen;
    }
	
    /// Get the number of fingerprints in the data store
    virtual size_t GetFingerprintsCount() = 0;

    /// Save a fingerprint in the datastore
    virtual void PutFingerprint(uint32_t FID, 
                                const uint8_t* data, 
                                size_t size) = 0;

    /// Write metadata associated to a fingerprint
    virtual void PutMetadata(uint32_t FID, const std::string& meta) = 0;

    /// Get metadata associated to a fingerprint
    virtual std::string GetMetadata(uint32_t FID) = 0;

    /// Save datastore info
    virtual void PutInfo(const DBInfo_t& info) = 0;

    /// Get datastore info
    virtual DBInfo_t GetInfo() = 0;

    /// Get operation mode
    virtual eOperation GetOpMode() const { 
        return m_Op; 
    }

    /// Set operation mode
    virtual void SetOpMode(eOperation mode) {
        m_Op = mode;
    }

	/// Set the URL where all database will be located.
    virtual void SetDatabaseURL(const std::string &url) {
        m_DBURL = url;
    }
		
	/// Set the host name of the server where the datastore is located
    virtual void SetServerName(const std::string &name) { 
        m_ServerName = name;
    }
	
	/// Set the port number of the server where the store is located
    virtual void SetServerPort(int port) { 
        m_ServerPort = port;
    }
	
	/// Set the username to access the data store
    virtual void SetUsername(const std::string &username) { 
        m_Username = username;
    }
	
	/// Set the password to access the data store
    virtual void SetPassword(const std::string &password) { 
        m_Password = password;
    }
	
    /// Get the URL where all database are located
    virtual std::string GetDatabaseURL() const {
        return m_DBURL;
    }

	/// Get the host name of the server where the datastore is located
    virtual std::string GetServerName() const { 
        return m_ServerName;
    }
	
	/// Get the port number of the server where the store is located
    virtual int GetServerPort() const { 
        return m_ServerPort;
    }
	
	/// Get the username to access the data store
    virtual std::string GetUsername()   const { 
        return m_Username;
    }
	
    /// Get the password to access the data store
    virtual std::string GetPassword()   const {
        return m_Password;
    }
};


// ----------------------------------------------------------------------------

/// This abstract class represents a collection of items within the datastore.
/// A "collection" is a logically grouped set of records, such as a table, file,
/// list, etc. Concrete classes must provide the functionality to access specific 
/// implementation of collections.

class KVCollection
{
protected:

    KVDataStore*          m_Datastore  {nullptr};
    std::string           m_Name;
    bool                  m_IsOpen     {false};
    std::vector<uint8_t>  m_Buffer;
	
    /// Function object used to make low level access keys
    /// used to fetch data from the underlying store.
    template<typename Tk, typename Tc = char*>
    struct key_builder
    {
        Tk key[2];
	
        void operator()(Tk k1, Tk k2) {
            key[0] = k1;
            key[1] = k2;
        }
        const Tc get() const {
            return (const Tc)(key);
        }
        size_t size() const {
            return sizeof(key);
        }
    };
	
public:

    KVCollection(KVDataStore *datastore) :
        m_Datastore (datastore),
        m_Buffer    (32768)
    { }

    virtual ~KVCollection() = default;

    /// Open the collection
    virtual void Open(int mode = OPEN_READ) = 0;

    /// Close the collection
    virtual void Close() = 0;

    /// Drop the collection (clear contents)
    virtual void Drop() = 0;

    /// Get the number of records in the collection
    virtual std::uint64_t GetRecordsCount() const = 0;

    /// Query open status
    virtual bool IsOpen() const {
        return m_IsOpen;
    }

    /// Set the collection name.
    virtual void SetName(const std::string &name) {
        m_Name = name;
    }

    /// Get the collection name.
    virtual std::string GetName() const { 
        return m_Name;
    }
};


// ----------------------------------------------------------------------------


/// Data store info record
struct DBInfo_t{
    int MatchType  {0};
};

/// Convenience structure to manipulate index list blocks
struct PListBlock
{
    Audioneex::PListHeader*      ListHeader {nullptr};
    Audioneex::PListBlockHeader* Header     {nullptr};
    uint8_t*                     Body       {nullptr};
    size_t                       BodySize   {0};
};

struct BlockCache
{
    int list_id      {0};  // List to which the blocks belong
    size_t accum     {0};  // General-purpose accumulator
    block_map buffer;      // Blocks buffer
};

/// Convenience function to check for emptiness
inline bool IsNull(const PListBlock& hdr)
{
    return hdr.ListHeader==nullptr &&
           hdr.Header==nullptr &&
           hdr.Body==nullptr;
}


#endif

