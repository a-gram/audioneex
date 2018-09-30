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


/// This abstract class implements the DataStore interface and extends it by
/// adding basic database operations and some other application-specific
/// functionality. In the examples we use key-value datastores, so we call
/// this class accordingly.

class KVDataStore : public Audioneex::DataStore
{
	
public:

    enum eOperation{
        GET,
        BUILD,
        BUILD_MERGE
    };

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

protected:

    std::string  m_DBURL;
    std::string  m_ServerName;
	int          m_ServerPort;
	std::string  m_Username;
	std::string  m_Password;
	bool         m_IsOpen;
	eOperation   m_Op;

};


// ----------------------------------------------------------------------------


/// Data store info record
struct DBInfo_t{
    int MatchType;
};

/// Database open modes
enum{
    OPEN_READ,
    OPEN_WRITE,
    OPEN_READ_WRITE
};


/// Convenience structure to manipulate index list blocks
struct PListBlock
{
    Audioneex::PListHeader* ListHeader;
    Audioneex::PListBlockHeader* Header;
    uint8_t* Body;
    size_t BodySize;
};

struct BlockCache
{
    int list_id      {0};  // List to which the blocks belong
    size_t accum     {0};  // General-purpose accumulator
    block_map buffer;      // Blocks buffer
};

/// Convenience function to check for emptiness
inline bool IsNull(const PListBlock& hdr){
    return hdr.ListHeader==nullptr &&
           hdr.Header==nullptr &&
           hdr.Body==nullptr;
}


#endif

