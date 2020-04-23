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

/// Database opening modes
enum
{
    OPEN_READ,
    OPEN_WRITE,
    OPEN_READ_WRITE
};

///Audioneex namespace
namespace Audioneex
{

/// This abstract class implements basic functionality for the KVDataStore
/// interface. It is the starting point for any concrete implementation of
/// custom data access objects (database drivers), which should derive from it.

class KVDataStoreImpl : public Audioneex::KVDataStore
{

protected:

    std::string 
    m_DBURL;
    
    std::string
    m_DBName;
    
    std::string 
    m_ServerName;
    
    int
    m_ServerPort {0};
    
    std::string
    m_Username;
    
    std::string
    m_Password;
    
    bool
    m_IsOpen     {false};
    
    eOperation
    m_Op         {FETCH};

public:

	KVDataStoreImpl() = default;
    
    virtual 
    ~KVDataStoreImpl() = default;


    virtual bool 
    IsOpen() const 
    { 
        return m_IsOpen; 
    }
	
    virtual eOperation 
    GetOpMode() const 
    { 
        return m_Op; 
    }

    virtual void 
    SetOpMode(eOperation mode) 
    {
        m_Op = mode;
    }

    virtual void 
    SetDatabaseURL(const std::string &url) 
    {
        m_DBURL = url;
    }
    
    virtual void 
    SetDatabaseName(const std::string &name) 
    {
        m_DBName = name;
    }
		
    virtual void 
    SetServerName(const std::string &name) 
    { 
        m_ServerName = name;
    }
	
    virtual void 
    SetServerPort(int port) 
    { 
        m_ServerPort = port;
    }
	
    virtual void 
    SetUsername(const std::string &username) 
    { 
        m_Username = username;
    }
	
    virtual void 
    SetPassword(const std::string &password) 
    { 
        m_Password = password;
    }
	
    virtual std::string 
    GetDatabaseURL() const 
    {
        return m_DBURL;
    }

    virtual std::string 
    GetDatabaseName() const 
    {
        return m_DBName;
    }

    virtual std::string 
    GetServerName() const 
    { 
        return m_ServerName;
    }
	
    virtual int 
    GetServerPort() const 
    { 
        return m_ServerPort;
    }
	
    virtual std::string 
    GetUsername() const 
    { 
        return m_Username;
    }
	
    virtual std::string 
    GetPassword() const 
    {
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

    KVDataStore*
    m_Datastore  {nullptr};
    
    std::string
    m_Name;
    
    bool
    m_IsOpen     {false};
    
    std::vector<uint8_t> 
    m_Buffer;
	
    /// Function object used to make low level access keys
    /// used to fetch data from the underlying store.
    template<typename Tk, typename Tc = char*>
    struct key_builder
    {
        Tk key[2];
	
        void
        operator()(Tk k1, Tk k2)
        {
            key[0] = k1;
            key[1] = k2;
        }
        
        const Tc 
        get() const
        {
            return (const Tc)(key);
        }
        
        size_t 
        size() const
        {
            return sizeof(key);
        }
    };
	
public:

    KVCollection(KVDataStore *datastore) 
    :
        m_Datastore (datastore),
        m_Buffer    (32768)
    { }

    virtual ~KVCollection() = default;

    /// Open the collection
    virtual void 
    Open(int mode = OPEN_READ) = 0;

    /// Close the collection
    virtual void 
    Close() = 0;

    /// Drop the collection (clear contents)
    virtual void 
    Drop() = 0;

    /// Get the number of records in the collection
    virtual std::uint64_t 
    GetRecordsCount() const = 0;

    /// Query open status
    virtual bool 
    IsOpen() const 
    {
        return m_IsOpen;
    }

    /// Set the collection name.
    virtual void 
    SetName(const std::string &name) 
    {
        m_Name = name;
    }

    /// Get the collection name.
    virtual std::string 
    GetName() const 
    { 
        return m_Name;
    }
};


// ----------------------------------------------------------------------------


/// Convenience structure to manipulate index list blocks
struct PListBlock
{
    Audioneex::PListHeader*
    ListHeader {nullptr};
    
    Audioneex::PListBlockHeader*
    Header     {nullptr};
    
    uint8_t*
    Body       {nullptr};
    
    size_t
    BodySize   {0};
};

struct BlockCache
{
    /// List to which the blocks belong
    int
    list_id   {0};
    
    /// General-purpose accumulator
    size_t
    accum     {0};
    
    /// Blocks buffer
    block_map 
    buffer;
};

/// Convenience function to check for emptiness
inline bool IsNull(const PListBlock& hdr)
{
    return hdr.ListHeader == nullptr &&
           hdr.Header == nullptr &&
           hdr.Body == nullptr;
}

}// end namespace Audioneex::

#endif

