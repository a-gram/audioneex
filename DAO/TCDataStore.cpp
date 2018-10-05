/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <string>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cstdio>

#include "TCDataStore.h"

using namespace Audioneex;


// ----------------------------------------------------------------------------

TCDataStore::TCDataStore(const std::string &url) :
    m_MainIndex     (this),
    m_QFingerprints (this),
    m_DeltaIndex    (this),
    m_Metadata      (this),
    m_Info          (this),
    m_ReadBuffer    (32768)
{
    m_DBURL = url;
	
    m_MainIndex.SetName("data.idx");
    m_QFingerprints.SetName("data.qfp");
    m_Metadata.SetName("data.met");
    m_Info.SetName("data.inf");
    m_DeltaIndex.SetName("data.tmp");
}

// ----------------------------------------------------------------------------

void TCDataStore::Open(eOperation op,
                       bool use_fing_db, 
                       bool use_meta_db,
                       bool use_info_db)
{
    if(m_IsOpen)
       Close();

    int open_mode = op == GET ? OPEN_READ : OPEN_READ_WRITE;

    // Append the path separator if missing (Windows accepts '/' as well)
    m_DBURL += m_DBURL.empty() ? "" :
              (m_DBURL.back()=='/' || m_DBURL.back()=='\\' ? "" : "/");

    // Set databases relative URL
    m_MainIndex.SetURL(m_DBURL);
    m_DeltaIndex.SetURL(m_DBURL);
    m_QFingerprints.SetURL(m_DBURL);
    m_Metadata.SetURL(m_DBURL);
    m_Info.SetURL(m_DBURL);

    // Open the main index
    m_MainIndex.Open(open_mode);

    // NOTE: The fingerprints database is not required by the engine
    //       and the metadata and info database are optional. The
	//       delta index is only used for build&merge strategies.

    if(use_fing_db)
       m_QFingerprints.Open(open_mode);

    if(use_meta_db)
       m_Metadata.Open(open_mode);

    if(use_info_db)
       m_Info.Open(open_mode);

    m_Op = op;
    m_IsOpen = true;
}

// ----------------------------------------------------------------------------

void TCDataStore::Close()
{
    m_MainIndex.Close();
    m_DeltaIndex.Close();
    m_QFingerprints.Close();
    m_Metadata.Close();
    m_Info.Close();

    m_IsOpen=false;
}

// ----------------------------------------------------------------------------

bool TCDataStore::Empty()
{
    return m_MainIndex.GetRecordsCount() == 0 &&
           m_QFingerprints.GetRecordsCount() == 0 &&
           m_Metadata.GetRecordsCount() == 0;
}

// ----------------------------------------------------------------------------

void TCDataStore::Clear()
{
    m_MainIndex.Drop();
    m_QFingerprints.Drop();
    m_Metadata.Drop();
    m_Info.Drop();
}

// ----------------------------------------------------------------------------

size_t TCDataStore::GetFingerprintsCount()
{
    return m_QFingerprints.GetRecordsCount();
}

// ----------------------------------------------------------------------------

const uint8_t* TCDataStore::GetPListBlock(int list_id,
                                          int block, 
                                          size_t &data_size, bool headers)
{
    // Read block from datastore into read buffer
    // (or get a reference to its memory location if the block is cached)
    data_size = m_MainIndex.ReadBlock(list_id, block, m_ReadBuffer, headers);
    return m_ReadBuffer.data();
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerStart()
{
    if(m_Op == GET)
       throw std::invalid_argument
       ("OnIndexerStart(): Invalid operation (GET)");

     if(m_Op == BUILD_MERGE)
        m_DeltaIndex.Open(OPEN_READ_WRITE);

     m_Run = 0;

}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerEnd()
{ 
    // Here we shall merge the temporary delta index with the live index
    // if we're performing a build-merge operation.

    // NOTE: The merge operations should be performed in one single transaction
    //       across all data stores hosting the live index.

    if(m_Op == BUILD_MERGE)
    {
       std::cout << "Merging..." << std::endl;

       m_DeltaIndex.Merge(m_MainIndex);

       //std::cout << ("Clearing delta index...") << std::endl;
       //m_DeltaIndex.Drop();

       std::cout << "Deleting delta index..." << std::endl;

       m_DeltaIndex.Close();
       if(std::remove( m_DeltaIndex.GetName().c_str() ))
          std::cout<<"Couldn't remove "<<m_DeltaIndex.GetName()<<std::endl;
    }
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerFlushStart()
{
    std::cout<<"Flushing..."<<std::endl;

    m_Run ++;

    m_MainIndex.ClearCache();
    m_DeltaIndex.ClearCache();
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerFlushEnd()
{
    // Flush the block cache to the database
    if(m_Op == BUILD)
       m_MainIndex.FlushBlockCache();
    else
       m_DeltaIndex.FlushBlockCache();
}

// ----------------------------------------------------------------------------

PListHeader TCDataStore::OnIndexerListHeader(int list_id)
{
    // If we're build-merging read the headers to be updated from the
    // main index on first run as the delta index is still empty, read
    // them from the delta index after the first run, and only read from
    // main index if they cannot be found in the delta.
    // NOTE: This rudimentary headers update mechanism can be more efficiently
    //       implemented using in-memory caching.

    if(m_Op == BUILD_MERGE){
       if(m_Run == 1)
          return m_MainIndex.GetPListHeader(list_id);
       else{
          PListHeader hdr = m_DeltaIndex.GetPListHeader(list_id);
          return !IsNull(hdr) ? hdr : m_MainIndex.GetPListHeader(list_id);
       }
    }
    else if(m_Op == BUILD)
       return m_MainIndex.GetPListHeader(list_id);
    else
       throw std::invalid_argument
       ("OnIndexerListHeader(): Invalid operation");
}

// ----------------------------------------------------------------------------

PListBlockHeader TCDataStore::OnIndexerBlockHeader(int list_id, int block)
{
    if(m_Op == BUILD_MERGE){
       if(m_Run == 1)
          return m_MainIndex.GetPListBlockHeader(list_id, block);
       else{
          PListBlockHeader hdr = m_DeltaIndex.GetPListBlockHeader(list_id, block);
          return !IsNull(hdr) ? hdr : m_MainIndex.GetPListBlockHeader(list_id, block);
       }
    }
    else if(m_Op == BUILD)
       return m_MainIndex.GetPListBlockHeader(list_id, block);
    else
       throw std::invalid_argument
       ("OnIndexerBlockHeader(): Invalid operation");
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerChunk(int list_id,
                                 PListHeader &lhdr,
                                 PListBlockHeader &hdr,
                                 uint8_t* data, size_t data_size)
{
    if(m_Op == BUILD_MERGE)
       m_DeltaIndex.AppendChunk(list_id, lhdr, hdr, data, data_size);
    else if(m_Op == BUILD)
       m_MainIndex.AppendChunk(list_id, lhdr, hdr, data, data_size);
    else
       throw std::invalid_argument
       ("OnIndexerChunkAppend(): Invalid operation");
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerNewBlock(int list_id,
                                    PListHeader &lhdr,
                                    PListBlockHeader &hdr,
                                    uint8_t* data, size_t data_size)
{
    if(m_Op == BUILD_MERGE)
       m_DeltaIndex.AppendChunk(list_id, lhdr, hdr, data, data_size, true);
    else if(m_Op == BUILD)
       m_MainIndex.AppendChunk(list_id, lhdr, hdr, data, data_size, true);
    else
       throw std::invalid_argument
       ("OnIndexerChunkNewBlock(): Invalid operation");
}

// ----------------------------------------------------------------------------

void TCDataStore::OnIndexerFingerprint(uint32_t FID, uint8_t *data, size_t size)
{
    if(m_QFingerprints.IsOpen())
       m_QFingerprints.WriteFingerprint(FID, data, size);
}

// ----------------------------------------------------------------------------

size_t TCDataStore::GetFingerprintSize(uint32_t FID)
{
    return m_QFingerprints.ReadFingerprintSize(FID);
}

// ----------------------------------------------------------------------------

const uint8_t* TCDataStore::GetFingerprint(uint32_t FID, 
                                           size_t &read, 
                                           size_t nbytes, 
                                           uint32_t bo)
{
    read = m_QFingerprints.ReadFingerprint(FID, m_ReadBuffer, nbytes, bo);
    return m_ReadBuffer.data();
}



//=============================================================================
//                                TCCollection
//=============================================================================



#define CHECK_OP(db)       std::string estr = "[TokyoCabinet] - ";   \
                           int err = tchdbecode(db);                 \
                           estr.append(tchdberrmsg(err));            \
                           if(err!=TCESUCCESS)                       \
                              throw std::runtime_error(estr);        \


TCCollection::TCCollection(TCDataStore *datastore) :
    KVCollection (datastore),
    m_DBHandle   ()
{
}

// ----------------------------------------------------------------------------

TCCollection::~TCCollection()
{
    if(m_IsOpen) Close();
}

// ----------------------------------------------------------------------------

void TCCollection::Open(int mode)
{
    int db_mode = 0;

    if(mode == OPEN_READ)
       db_mode = HDBOREADER;
    else if(mode == OPEN_WRITE || mode == OPEN_READ_WRITE)
       db_mode = HDBOWRITER|HDBOCREAT;
    else
       throw std::logic_error
       ("Unrecognized database opening mode");

    // Close current database if open
    if(m_IsOpen)
       Close();

    m_DBHandle = tchdbnew();

    tchdbtune(m_DBHandle, 1000000, 4, 10, HDBTLARGE);
    tchdbsetcache(m_DBHandle, 1000000);

    std::string full_url = m_DBURL + m_Name;

    if(tchdbopen(m_DBHandle, full_url.c_str(), db_mode)){
        //mDbMode = mode;
    }else{
        int err = tchdbecode(m_DBHandle);
        std::string estr = tchdberrmsg(err);
        estr += " " + full_url;
        tchdbdel(m_DBHandle);
        m_DBHandle = nullptr;
        throw std::runtime_error(estr);
    }

    m_IsOpen = true;
}

// ----------------------------------------------------------------------------

void TCCollection::Close()
{
    if(m_DBHandle){
       tchdbclose(m_DBHandle);
       tchdbdel(m_DBHandle);
    }

    m_DBHandle  = nullptr;
    m_IsOpen = false;
}

// ----------------------------------------------------------------------------

void TCCollection::Drop()
{
    if(m_DBHandle && !tchdbvanish(m_DBHandle)){
       CHECK_OP(m_DBHandle)
    }
}

// ----------------------------------------------------------------------------

uint64_t TCCollection::GetRecordsCount() const
{
    return m_DBHandle ? tchdbrnum(m_DBHandle) : 0;
}



//=============================================================================
//                                TCIndex
//=============================================================================



TCIndex::TCIndex(TCDataStore *dstore) :
    TCCollection (dstore)
{
}

// ----------------------------------------------------------------------------

PListHeader TCIndex::GetPListHeader(int list_id)
{
    int               bsize;
    void             *block;
    PListHeader       hdr = {};
	key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, 1);
	
    block = tchdbget(m_DBHandle, key.get(), key.size(), &bsize);

    // Block found
    if(block){
       assert(bsize > sizeof(PListHeader) + sizeof(PListBlockHeader));
       hdr = *reinterpret_cast<PListHeader*>(block);
       tcfree(block);
    }

    return hdr;
}

// ----------------------------------------------------------------------------

PListBlockHeader TCIndex::GetPListBlockHeader(int list_id, int block_id)
{
    int               bsize;
    void             *block;
    PListBlockHeader  hdr = {};
	key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, block_id);

    block = tchdbget(m_DBHandle, key.get(), key.size(), &bsize);

    // Block found
    if(block){
        int hoff = 0;
        // Skip list header if first block
        if(block_id==1){
           hoff = sizeof(PListHeader);
           assert(bsize > sizeof(PListHeader) + sizeof(PListBlockHeader));
        }else
           assert(bsize > sizeof(PListBlockHeader));

        uint8_t *pdata = reinterpret_cast<uint8_t*>(block);
        hdr = *reinterpret_cast<PListBlockHeader*>(pdata + hoff);

       tcfree(block);
    }

    return hdr;
}

// ----------------------------------------------------------------------------

size_t TCIndex::ReadBlock(int list_id, 
                          int block_id, 
                          std::vector<uint8_t> &buffer, 
                          bool headers)
{
    int               bsize;
    void             *block;
    size_t            off=0;
	key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, block_id);

    block = tchdbget(m_DBHandle, key.get(), key.size(), &bsize);

    size_t rbytes = 0;

    if(block){
        if(!headers)
           off = block_id==1 ? sizeof(PListHeader) +
                               sizeof(PListBlockHeader)
                             :
                               sizeof(PListBlockHeader);
        rbytes = bsize - off;

        if(rbytes > buffer.size())
           buffer.resize(rbytes);

        uint8_t *pdata = reinterpret_cast<uint8_t*>(block) + off;
        std::copy(pdata, pdata + rbytes, buffer.begin());

       tcfree(block);
    }

    return rbytes;
}

// ----------------------------------------------------------------------------

void TCIndex::WriteBlock(int list_id, 
                         int block_id, 
                         std::vector<uint8_t> &buffer, 
                         size_t data_size)
{
    assert(!buffer.empty());
    assert(data_size <= buffer.size());
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key_builder<int>  key;
	key(list_id, block_id);

    if(!tchdbputasync(m_DBHandle, key.get(), key.size(), buffer.data(), data_size)){
        CHECK_OP(m_DBHandle);
    }
}

// ----------------------------------------------------------------------------

void TCIndex::AppendChunk(int list_id,
                          PListHeader &lhdr,
                          PListBlockHeader &hdr,
                          uint8_t *chunk, size_t chunk_size,
                          bool new_block)
{
    assert(chunk && chunk_size);
    assert(!IsNull(hdr));

    // Very simple caching mechanism. Something more sophisticated at
    // this point would greatly increase performances.

    if(list_id != m_BlocksCache.list_id){

       // Schedule blocks for batch write.
       for(auto &iblock : m_BlocksCache.buffer)
           WriteBlock(m_BlocksCache.list_id,
                      iblock.first,
                      iblock.second,
                      iblock.second.size());

       // Reset cache for current list
       m_BlocksCache.list_id = list_id;
       m_BlocksCache.buffer.clear();
    }

    // Get block from cache (create new one if not found)
    std::vector<uint8_t> &block = m_BlocksCache.buffer[hdr.ID];

    // Read block from database if not in cache and not new
    if(block.empty() && !new_block)
       ReadBlock(list_id, hdr.ID, block);

    // Compute block header offset and headers size
    size_t hoff = hdr.ID==1 ? sizeof(PListHeader) : 0;
    size_t hsize = hoff + sizeof(PListBlockHeader);

    // If block does not exist, create new one.
    if(block.empty())
       block.resize(hsize);

    // Copy/Update list header if first block
    if(hdr.ID==1){
       assert(!IsNull(lhdr));
       (*reinterpret_cast<PListHeader*>(block.data())) = lhdr;
    }

    // Copy/Update block header
    (*reinterpret_cast<PListBlockHeader*>(block.data()+hoff)) = hdr;

    // Append chunk
    block.insert(block.end(), chunk, chunk + chunk_size);

    // If this is a new block we need to update the index list header
    // for the curent list_id, located in the first block (not necessary if
    // we're processing the first block as it's already updated above).
    if(new_block && hdr.ID!=1)
       UpdateListHeader(list_id, lhdr);
}

// ----------------------------------------------------------------------------

void TCIndex::UpdateListHeader(int list_id, PListHeader &lhdr)
{
    // Try the cache first
    std::vector<uint8_t> &block = m_BlocksCache.buffer[1];

    // Read from database if cache miss
    if(block.empty())
       ReadBlock(list_id, 1, block);

    if(block.empty())
       block.resize(sizeof(PListHeader));

    assert(block.size() >= sizeof(PListHeader));

    // Copy list header
    PListHeader& lhdr_old = *reinterpret_cast<PListHeader*>(block.data());
    lhdr_old = lhdr;
}

// ----------------------------------------------------------------------------

void TCIndex::Merge(TCIndex &lidx)
{
    void *key, *val;
    int ksize, vsize;

    tchdbiterinit(m_DBHandle);

    std::vector<uint8_t>& lblock = m_Buffer;


    while((key = tchdbiternext(m_DBHandle, &ksize)))
    {
        val = tchdbget(m_DBHandle, key, ksize, &vsize);

        // Extract the list id and block number from the key
        int *pkey    = static_cast<int*>(key);
        int list_id     = *pkey++;
        int block_id = *pkey;

        assert(ksize == sizeof(int)*2);
        assert(vsize > 0);

        if(val)
        {
            // Get the block from the live index, if any
            size_t lbsize = lidx.ReadBlock(list_id, block_id, lblock);

            // Calculate block header size
            size_t hsize = block_id==1 ? sizeof(PListHeader) +
                                         sizeof(PListBlockHeader)
                                       :
                                         sizeof(PListBlockHeader);

            // If block doesn't exist in live index, create new one.
            if(lbsize == 0)
               lbsize = hsize;

            PListBlock lblk = RawBlockToBlock(lblock.data(),
                                              lbsize,
                                              block_id==1);

            PListBlock dblk = RawBlockToBlock(static_cast<uint8_t*>(val),
                                              vsize,
                                              block_id==1);

            assert(block_id==1 && lblk.Body ? lblk.ListHeader->BlockCount : 1);
            assert(!IsNull(dblk));
            assert(lbsize >= sizeof(PListHeader));

            if(lbsize + dblk.BodySize > lblock.size())
               lblock.resize(lbsize + dblk.BodySize);

            // Update list header if first block
            if(block_id==1)
               *lblk.ListHeader = *dblk.ListHeader;

            // NOTE: Delta blocks may contain only the list header
            //       (this happens when we are appending a new block other
            //       than the first and update the block's list header),
            //       so we need to check whether a block is present.
            if(dblk.Header && dblk.Body)
            {
               assert(dblk.Header->BodySize == lblk.BodySize + dblk.BodySize);

               *lblk.Header = *dblk.Header;

               // Append delta body to live block
               std::copy(dblk.Body, dblk.Body + dblk.BodySize, lblock.data() + lbsize);
            }

            lidx.WriteBlock(list_id, block_id, lblock, lbsize + dblk.BodySize);

            tcfree(val);
        }

        tcfree(key);
    }
}

// ----------------------------------------------------------------------------

PListBlock TCIndex::RawBlockToBlock(uint8_t *block, size_t block_size, bool isFirst)
{
    PListBlock rblock = {};

    if(block_size==0)
       return rblock;

    assert(block);

    size_t lhdr_size = 0;

    // Special case for first blocks containing the list header
    if(isFirst){
       lhdr_size = sizeof(PListHeader);
       // The 1st block must contain at least the list header
       assert(block_size >= lhdr_size);
       rblock.ListHeader = reinterpret_cast<PListHeader*>(block);
       // If block has no other data return else continue
       // NOTE: The 1st blocks in the delta index might not contain
       //       block data but just updated list headers.
       if(block_size > sizeof(PListHeader))
          block += lhdr_size;
       else
          return rblock;
    }

    // Extract block header and body if present (see NOTE above).

    // A block must contain at least the block header
    assert(block_size - lhdr_size >= sizeof(PListBlockHeader));
    // Extract block header
    rblock.Header = reinterpret_cast<PListBlockHeader*>(block);
    // Extract body
    rblock.Body = block + sizeof(PListBlockHeader);
    rblock.BodySize = block_size - sizeof(PListBlockHeader) - lhdr_size;

    return rblock;

}

// ----------------------------------------------------------------------------

void TCIndex::FlushBlockCache()
{
    if(m_BlocksCache.buffer.empty())
       return;

    // Schedule any remaining blocks for batched insert.
    for (auto &block : m_BlocksCache.buffer)
         WriteBlock(m_BlocksCache.list_id,
                    block.first, 
                    block.second, 
                    block.second.size());

    ClearCache();
}

void TCIndex::ClearCache()
{
    m_BlocksCache.list_id = 0;
    m_BlocksCache.accum = 0;
    m_BlocksCache.buffer.clear();
}


//=============================================================================
//                              TCFingerprints
//=============================================================================



TCFingerprints::TCFingerprints(TCDataStore *dstore) :
    TCCollection (dstore)
{
}

// ----------------------------------------------------------------------------

size_t TCFingerprints::ReadFingerprintSize(uint32_t FID)
{
    int vsize = tchdbvsiz(m_DBHandle, &FID, sizeof(uint32_t));
    return vsize > 0 ? static_cast<size_t>(vsize) : 0;
}

// ----------------------------------------------------------------------------

size_t TCFingerprints::ReadFingerprint(uint32_t FID, 
                                       std::vector<uint8_t> &buffer, 
                                       size_t size, 
									   uint32_t bo)
{
    int dsize;
    void *data;

    data = tchdbget(m_DBHandle, &FID, sizeof(uint32_t), &dsize);

    if(data){
       assert(0 <= bo && bo < dsize);
       size_t gsize = size ? size : dsize - bo;
       gsize = std::min<size_t>(gsize, dsize - bo);

       if(gsize > buffer.size())
          buffer.resize(gsize);

       uint8_t *pdata = reinterpret_cast<uint8_t*>(data) + bo;
       std::copy(pdata, pdata + gsize, buffer.begin());
       tcfree(data);

       return gsize;
    }
    return 0;
}

// ----------------------------------------------------------------------------

void TCFingerprints::WriteFingerprint(uint32_t FID, const uint8_t *data, size_t size)
{
    if(m_DBHandle==nullptr)
       throw std::runtime_error("Fingerprints database not open.");

    assert(data);
    assert(size > 0);
    assert(FID > 0);

    if(!tchdbput(m_DBHandle, &FID, sizeof(uint32_t), data, size)){
        CHECK_OP(m_DBHandle);
    }
}


//=============================================================================
//                                TCMetadata
//=============================================================================



TCMetadata::TCMetadata(TCDataStore *dstore) :
    TCCollection (dstore)
{
}

// ----------------------------------------------------------------------------

std::string TCMetadata::Read(uint32_t FID)
{
    std::string str;
    if(m_DBHandle){
       str = std::to_string(FID);
       char* pstr = tchdbget2(m_DBHandle, str.c_str());
       str.assign( pstr ? pstr : "" );
       tcfree(pstr);
    }
    return str;
}

// ----------------------------------------------------------------------------

void TCMetadata::Write(uint32_t FID, const std::string &meta)
{
    if(m_DBHandle==nullptr)
       throw std::runtime_error("Metadata database not open");

    std::string str = std::to_string(FID);
    if(!tchdbput2(m_DBHandle, str.c_str(), meta.c_str())){
       CHECK_OP(m_DBHandle);
    }
}



//=============================================================================
//                                TCInfo
//=============================================================================



TCInfo::TCInfo(TCDataStore *dstore) :
    TCCollection (dstore)
{
}

// ----------------------------------------------------------------------------

DBInfo_t TCInfo::Read()
{
    if(m_DBHandle==nullptr)
       throw std::runtime_error("Info database not open");

    int dsize, key = 0;
    void *data;
    DBInfo_t dbinfo;

    data = tchdbget(m_DBHandle, &key, sizeof(int), &dsize);

    if(data){
       assert(dsize == sizeof(DBInfo_t));
       dbinfo = *reinterpret_cast<DBInfo_t*>(data);
       tcfree(data);
    }
    return dbinfo;
}

// ----------------------------------------------------------------------------

void TCInfo::Write(const DBInfo_t &info)
{
    if(m_DBHandle==nullptr)
       throw std::runtime_error("Metadata database not open");

    int key = 0;
    if(!tchdbput(m_DBHandle, &key, sizeof(int), &info, sizeof(DBInfo_t))){
        CHECK_OP(m_DBHandle);
    }
}

