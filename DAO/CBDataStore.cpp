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
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "CBDataStore.h"

using namespace Audioneex;

	
// ============================================================================
//                  C callbacks invoked by Couchbase's driver
// ============================================================================

extern "C"{

static void store_callback(lcb_t instance,
                           const void *cookie,
                           lcb_storage_t operation,
                           lcb_error_t error,
                           const lcb_store_resp_t *item)
{
    CBSetResp* sresp = (CBSetResp*)(cookie);
    assert(sresp);

    if (error != LCB_SUCCESS) {

        std::string emsg = "[Couchbase] - The store request was not executed: " +
                           std::string(lcb_strerror(instance, error)); + "\n" +
                           std::string("SENDER :") + sresp->sender;

        std::cout << emsg << std::endl;

        // Temporary failure (server unavailable). May retry.
        if (LCB_EIFTMP(error)) {
            std::string emsg = "[Couchbase] - Transient error. May retry. " +
                               std::string(lcb_strerror(instance, error));
            std::cout << emsg << std::endl;
        }
    }
    sresp->status = error;
}

// ============================================================================

static void get_callback(lcb_t instance,
                         const void *cookie,
                         lcb_error_t error,
                         const lcb_get_resp_t *item)
{
    CBGetResp* gresp = (CBGetResp*)(cookie);
    //DEBUG_MSG("get_callback(): error="<<error<<", value="<<item->v.v0.nbytes)
    assert(gresp);

    if (error == LCB_SUCCESS) {
        // Get the data from the retrieved value
        if(gresp->buf)
        {
            // Compute amount of data to get from the value.
            // If data_size is specified, get data_size bytes, else get all value
            // starting from offset.
            size_t vsize = item->v.v0.nbytes - gresp->data_offset;
            size_t gsize = gresp->data_size ? gresp->data_size : vsize;

            gsize = std::min<size_t>(gsize,vsize);

            // Reallocate the return buffer if data does not fit it
            if(gsize > gresp->buf->size())
               gresp->buf->resize(gsize);

            // Set the amount of data read.
            gresp->read_size = gsize;

            uint8_t* po = (uint8_t*)(item->v.v0.bytes) + gresp->data_offset;

            std::copy(po, po + gsize, gresp->buf->begin());
        }
        // Set the size of the retrieved value
        gresp->value_size = item->v.v0.nbytes;
    }
    // If we get a negative response for the request check whether
    // the key does not exist.
    else if (LCB_EIFDATA(error)) {

        switch (error) {
           // Key not found
           case LCB_KEY_ENOENT:
               gresp->value_size = 0;
               gresp->read_size = 0;
               //DEBUG_MSG("KEY NOT FOUND: "<<(*(int*)item->v.v0.key))
               break;
           // Some other negative response
           default:
               std::string emsg = "[Couchbase] - The get request was not executed: " +
                                  std::string(lcb_strerror(instance, error));
               std::cout << emsg << std::endl;
               break;
        }
    }
    else if (LCB_EIFTMP(error)) {
        std::string emsg = "[Couchbase] - Transient error. May retry. " +
                           std::string(lcb_strerror(instance, error));
        std::cout << emsg << std::endl;
    }

    gresp->status = error;
}

// ============================================================================

static void http_chunk_callback(lcb_http_request_t,
                                lcb_t,
                                const void *cookie,
                                lcb_error_t err,
                                const lcb_http_resp_t *resp)
{
    CBHttpResp *hresp = (CBHttpResp *)cookie;

    if (resp->v.v0.nbytes)
        hresp->response.append((const char*)resp->v.v0.bytes, resp->v.v0.nbytes);

    hresp->status = err;
}

// ============================================================================

static void http_done_callback(lcb_http_request_t,
                               lcb_t,
                               const void *cookie,
                               lcb_error_t err,
                               const lcb_http_resp_t *resp)
{
    CBHttpResp *hresp = (CBHttpResp *)cookie;
    hresp->status = err;
}

}// extern "C"


// ============================================================================
//                          CBDataStore implementation
// ============================================================================


CBDataStore::CBDataStore(const std::string &url) :
    m_MainIndex     (this),
    m_QFingerprints (this),
    m_DeltaIndex    (this),
    m_Metadata      (this),
    m_Info          (this),
    m_ReadBuffer    (32768)
{
    m_DBURL = url;
	
    m_MainIndex.SetName("data_idx");
    m_QFingerprints.SetName("data_qfp");
    m_Metadata.SetName("data_met");
    m_Info.SetName("data_inf");
    m_DeltaIndex.SetName("data_tmp");
}

// ----------------------------------------------------------------------------

void CBDataStore::Open(eOperation op,
                       bool use_fing_db,
                       bool use_meta_db, 
                       bool use_info_db)
{
    if(m_IsOpen)
       Close();

    int open_mode = op == GET ? OPEN_READ : OPEN_READ_WRITE;

    // Open the main index
    m_MainIndex.Open(open_mode);

    // NOTE: The fingerprints database is not required by the engine
    //       and the metadata and info database are optional. The delta
	//       index is only used for build&merge strategies.

    if(use_fing_db)
       m_QFingerprints.Open(open_mode);

    if(use_meta_db)
       m_Metadata.Open(open_mode);

    if(use_info_db)
       m_Info.Open(open_mode);

    m_Op = op;
    
    // All databases were opened successfully. The connection is open.
    m_IsOpen = true;
}

// ----------------------------------------------------------------------------

void CBDataStore::Close()
{
    m_MainIndex.Close();
    m_DeltaIndex.Close();
    m_QFingerprints.Close();
    m_Metadata.Close();
    m_Info.Close();

    m_IsOpen=false;
}

// ----------------------------------------------------------------------------

bool CBDataStore::Empty()
{
    return m_MainIndex.GetRecordsCount() == 0 &&
           m_QFingerprints.GetRecordsCount() == 0 &&
           m_Metadata.GetRecordsCount() == 0;
}

// ----------------------------------------------------------------------------

void CBDataStore::Clear()
{
    m_MainIndex.Drop();
    m_QFingerprints.Drop();
    m_Metadata.Drop();
    m_Info.Drop();
}

// ----------------------------------------------------------------------------

size_t CBDataStore::GetFingerprintsCount()
{
    return m_QFingerprints.GetRecordsCount();
}

// ----------------------------------------------------------------------------

const uint8_t* CBDataStore::GetPListBlock(int list_id, 
                                          int block, 
                                          size_t &data_size, 
                                          bool headers)
{
    // Read block from datastore into read buffer
    // (or get a reference to its memory location if the block is cached)
    data_size = m_MainIndex.ReadBlock(list_id, block, m_ReadBuffer, headers);
    return m_ReadBuffer.data();
}

// ----------------------------------------------------------------------------

void CBDataStore::OnIndexerStart()
{
    if(m_Op == GET)
       throw std::invalid_argument
       ("OnIndexerStart(): Invalid operation");

    if(m_Op == BUILD_MERGE)
       m_DeltaIndex.Open(OPEN_READ_WRITE);

    m_MainIndex.ResetCaches();
    m_DeltaIndex.ResetCaches();

    m_Run = 0;
}

// ----------------------------------------------------------------------------

void CBDataStore::OnIndexerEnd()
{ 
    // Here we shall merge the temporary delta index with the live index
    // if we're performing a build-merge operation.

    // NOTE: The merge operations should be performed in one single transaction
    //       across all data stores hosting the live index.

    if(m_Op == BUILD_MERGE)
    {
       // Flush the delta index block cache before merging
       m_DeltaIndex.FlushBlockCache();
	   
       std::cout << "Merging..." << std::endl;

       m_DeltaIndex.Merge(m_MainIndex);

       std::cout << "Clearing delta index..." << std::endl;
 
       m_DeltaIndex.Drop();
       m_DeltaIndex.Close();
    }
}

// ----------------------------------------------------------------------------

void CBDataStore::OnIndexerFlushStart()
{
    std::cout<<"Flushing..."<<std::endl;

    m_Run ++;
}

// ----------------------------------------------------------------------------

void CBDataStore::OnIndexerFlushEnd()
{
    // Flush the block cache to the database
    if(m_Op == BUILD)
       m_MainIndex.FlushBlockCache();
    else
       m_DeltaIndex.FlushBlockCache();
}

// ----------------------------------------------------------------------------

PListHeader CBDataStore::OnIndexerListHeader(int list_id)
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

PListBlockHeader CBDataStore::OnIndexerBlockHeader(int list_id, int block)
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

void CBDataStore::OnIndexerChunk(int list_id,
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

void CBDataStore::OnIndexerNewBlock(int list_id,
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

void CBDataStore::OnIndexerFingerprint(uint32_t FID, uint8_t *data, size_t size)
{
	if(m_QFingerprints.IsOpen())
       m_QFingerprints.WriteFingerprint(FID, data, size);
}

// ----------------------------------------------------------------------------

size_t CBDataStore::GetFingerprintSize(uint32_t FID)
{
    return m_QFingerprints.ReadFingerprintSize(FID);
}

// ----------------------------------------------------------------------------

const uint8_t* CBDataStore::GetFingerprint(uint32_t FID,
                                           size_t &read, 
                                           size_t nbytes, 
                                           uint32_t bo)
{
    read = m_QFingerprints.ReadFingerprint(FID, m_ReadBuffer, nbytes, bo);
    return m_ReadBuffer.data();
}



//=============================================================================
//                                CBCollection
//=============================================================================



CBCollection::CBCollection(CBDataStore *datastore) :
    KVCollection (datastore),
    m_DBHandle   ()
{
}

// ----------------------------------------------------------------------------

CBCollection::~CBCollection()
{
    if(m_IsOpen) Close();
}

// ----------------------------------------------------------------------------

void CBCollection::THROW_ON_FAIL(const lcb_error_t &res, 
                                 const char *msg) const
{
    if (res != LCB_SUCCESS && res!=LCB_KEY_ENOENT){
        std::string emsg = "[Couchbase] - ";
        emsg.append(msg);
        emsg.append(" Database '" + this->GetName() + "'. ");
        emsg.append(lcb_strerror((*this), res));
        throw std::runtime_error(emsg);
    }
}

// ----------------------------------------------------------------------------

void CBCollection::Open(int mode)
{
    // Close current database if open
    if(m_IsOpen)
       Close();

    struct lcb_create_st cropts;
    lcb_error_t err;
    memset(&cropts, 0, sizeof(cropts));

    cropts.version = 3;

    std::string server = m_Datastore->GetServerName();
    std::string user = m_Datastore->GetUsername();
    std::string pass = m_Datastore->GetPassword();
    
    std::string conn_str = "couchbase://" + server + "/" + m_Name;

    cropts.v.v3.connstr = conn_str.c_str();
    cropts.v.v3.username = user.empty() ? 0 : user.c_str();
    cropts.v.v3.passwd = pass.empty() ? 0 : pass.c_str();

    // Create handle for database
    err = lcb_create(&m_DBHandle, &cropts);

    THROW_ON_FAIL(err, "Couldn't create handle.");

    lcb_U32 curval = 10000000; //< us
    lcb_cntl(m_DBHandle,LCB_CNTL_SET, LCB_CNTL_OP_TIMEOUT, &curval);
//    lcb_cntl(m_DBHandle,LCB_CNTL_GET, LCB_CNTL_OP_TIMEOUT, &curval);
//    std::cout<<"Current timeout: "<<curval<<" us"<<std::endl;

    // Install the callbacks
    lcb_set_get_callback(m_DBHandle, get_callback);
    lcb_set_store_callback(m_DBHandle, store_callback);
    lcb_set_http_data_callback(m_DBHandle, http_chunk_callback);
    lcb_set_http_complete_callback(m_DBHandle, http_done_callback);

    // Initiate the connection
    err = lcb_connect(m_DBHandle);

    THROW_ON_FAIL(err, "Couldn't initiate connection.");

    // Wait for the connection to execute
    err = lcb_wait(m_DBHandle);

    THROW_ON_FAIL(err, "Couldn't connect.");

    m_IsOpen = true;
}

// ----------------------------------------------------------------------------

void CBCollection::Close()
{
    if(m_DBHandle)
       lcb_destroy(m_DBHandle);

    m_DBHandle = nullptr;
    m_IsOpen = false;
}

// ----------------------------------------------------------------------------

void CBCollection::Drop()
{
    if(!m_DBHandle) return;

    std::string uri = "/pools/default/buckets/" + 
                      m_Name + 
                      "/controller/doFlush";

    lcb_http_cmd_st cmd;
    memset(&cmd, 0, sizeof cmd);

    cmd.v.v0.method = LCB_HTTP_METHOD_POST;
    cmd.v.v0.chunked = 1;
    cmd.v.v0.path = uri.c_str();
    cmd.v.v0.npath = uri.size();
    cmd.v.v0.body = NULL;
    cmd.v.v0.nbody = 0;
    cmd.v.v0.content_type = "";

    lcb_http_request_t dummy;
    lcb_error_t err;

    CBHttpResp hresp;
    hresp.status = LCB_SUCCESS;

    err = lcb_make_http_request(m_DBHandle,
                                &hresp, 
                                LCB_HTTP_TYPE_MANAGEMENT, 
                                &cmd, 
                                &dummy);

    THROW_ON_FAIL(err, "Couldn't initiate http request.");

    // Wait for http response
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(hresp.status, "Couldn't drop database.");
}

// ----------------------------------------------------------------------------

uint64_t CBCollection::GetRecordsCount() const
{
    if(!m_DBHandle) return 0;

    size_t fp_count = 0;

    std::string uri = "/pools/default/buckets/" + m_Name;

    lcb_http_cmd_st cmd;
    memset(&cmd, 0, sizeof cmd);

    cmd.v.v0.method = LCB_HTTP_METHOD_GET;
    cmd.v.v0.chunked = 1;
    cmd.v.v0.path = uri.c_str();
    cmd.v.v0.npath = uri.size();
    cmd.v.v0.body = NULL;
    cmd.v.v0.nbody = 0;
    cmd.v.v0.content_type = "";

    lcb_http_request_t dummy;
    lcb_error_t err;

    CBHttpResp hresp;
    hresp.status = LCB_SUCCESS;

    err = lcb_make_http_request(m_DBHandle, 
                                &hresp, 
                                LCB_HTTP_TYPE_MANAGEMENT, 
                                &cmd, 
                                &dummy);

    THROW_ON_FAIL(err, "Couldn't initiate http request.");

    // Wait for http response
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(hresp.status, "Couldn't execute query.");

    std::stringstream json;
    json << hresp.response;

    boost::property_tree::ptree pt;
    boost::property_tree::read_json(json, pt);

    std::string icount = pt.get<std::string>("basicStats.itemCount");
    fp_count = std::stoull(icount);

    return fp_count;
}



//=============================================================================
//                                CBIndex
//=============================================================================



CBIndex::CBIndex(CBDataStore *dstore) :
    CBCollection (dstore)
{
}

// ----------------------------------------------------------------------------

PListHeader CBIndex::GetPListHeader(int list_id)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    PListHeader       hdr = {};
    key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, 1);

    // Create response structure
    CBGetResp gresp = {};
    gresp.buf = &m_Buffer;
    gresp.data_size = sizeof(PListHeader);

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = key.get();
    cmd.v.v0.nkey = key.size();

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    // NOTE: If the get operation fails the issue is not necessarily critical
    // as it may be a temporary failure (i.e. server busy). However the
    // indexer would receive an incorrect response (empty header for an existing
    // list), so we throw even it's a temporary issue.

    THROW_ON_FAIL(gresp.status, "Couldn't get list header.");

    // Block found. Copy header data into return struct.
    if(gresp.read_size > 0){
       assert(gresp.read_size == sizeof(PListHeader));
//       assert(gresp.value_size >= sizeof(PListHeader) + sizeof(PListBlockHeader));       
       hdr = *reinterpret_cast<PListHeader*>(m_Buffer.data());
    }

    return hdr;
}

// ----------------------------------------------------------------------------

PListBlockHeader CBIndex::GetPListBlockHeader(int list_id, int block_id)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    PListBlockHeader  hdr = {};
    key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, block_id);

    // Create response structure
    CBGetResp gresp = {};
    gresp.buf = &m_Buffer;
    gresp.data_size = sizeof(PListBlockHeader);
    gresp.data_offset = block_id == 1 ? sizeof(PListHeader) : 0;

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = key.get();
    cmd.v.v0.nkey = key.size();

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    // NOTE: If the get operation fails the issue is not necessarily critical
    // as it may be a temporary failure (i.e. server busy). However the
    // indexer would receive an incorrect response (empty header for an existing
    // block), so we throw even it's a temporary issue.

    THROW_ON_FAIL(lcb_error_t(gresp.status), "Couldn't get block header.");

    // Block found. Copy header data into return struct.
    if(gresp.read_size > 0){
       assert(gresp.read_size == sizeof(PListBlockHeader));
//       assert(gresp.value_size > sizeof(PListHeader) + sizeof(PListBlockHeader));
       hdr = *reinterpret_cast<PListBlockHeader*>(m_Buffer.data());
    }
	
    return hdr;
}

// ----------------------------------------------------------------------------

size_t CBIndex::ReadBlock(int list_id, 
                          int block_id, 
                          std::vector<uint8_t> &buffer, 
                          bool headers)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    size_t            off=0;
    key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, block_id);

    if(!headers)
       off = block_id==1 ? sizeof(PListHeader) +
                           sizeof(PListBlockHeader)
                         : sizeof(PListBlockHeader);

    // Create response structure
    CBGetResp gresp = {};
    gresp.buf = &buffer;
    gresp.data_offset = off;

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = key.get();
    cmd.v.v0.nkey = key.size();

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    // NOTE: The block data is copied into the given buffer in the
    //       get callback. If the block does not exist zero read data
    //       must be returned.
    //       The following line will throw for temporary failures as
    //       well (you may want to retry the operation).

    THROW_ON_FAIL(gresp.status, "Couldn't get block.");

	return gresp.read_size;	
}

// ----------------------------------------------------------------------------

void CBIndex::WriteBlock(int list_id, 
                         int block_id, 
                         std::vector<uint8_t> &buffer, 
                         size_t data_size)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    assert(!buffer.empty());
    assert(data_size <= buffer.size());

    key_builder<int>  key;
	
    // Make the key to the block <listID|blockID>
    // The list header is prepended to the 1st block
	key(list_id, block_id);

    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t *commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = key.get();
    cmd.v.v0.nkey = key.size();
    cmd.v.v0.bytes = buffer.data();
    cmd.v.v0.nbytes = data_size;
    cmd.v.v0.operation = LCB_SET;

    // NOTE: Write operations are just scheduled (buffered) and must be
    //       flushed to the server by calling lcb_wait(). This means that
    //       whenever a call to WriteBlock() is made we can either wait for
    //       the operation to complete or continue scheduling if a batch
    //       write is desired.
    lcb_error_t err = lcb_store(m_DBHandle, &m_StoreResp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't schedule set operation.");

    // Wait for set command to execute
    //lcb_wait(m_DBHandle[db]);
}

// ----------------------------------------------------------------------------

void CBIndex::AppendChunk(int list_id,
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
       //block_map::iterator iblock = m_BlocksCache.buffer.begin();
       //for (; iblock != m_BlocksCache.buffer.end(); ++iblock)
       for(auto &iblock : m_BlocksCache.buffer)
       {
           WriteBlock(m_BlocksCache.list_id, 
                      iblock.first, 
                      iblock.second, 
                      iblock.second.size());
           //m_BlocksCache.accum += iblock->second.size();
       }
       // Execute the batch if the threshold is reached
       if(m_BlocksCache.accum >= 4096){

          m_StoreResp.sender = __FUNCTION__;

          //DEBUG_MSG("   Executing batch..."<<float(m_BlocksCache.accum/1024.f)<<"KB")
          lcb_wait(m_DBHandle);

          lcb_error_t rstatus = static_cast<lcb_error_t>(m_StoreResp.status);

          if (rstatus != LCB_SUCCESS)
             throw std::runtime_error
		     ("Bulk load error. Indexing failed.");

          m_BlocksCache.accum = 0;
         // DEBUG_MSG("   Done.")
       }
       // Reset cache for current list
       m_BlocksCache.list_id = list_id;
       m_BlocksCache.buffer.clear();
    }

    m_BlocksCache.accum += chunk_size;

    // Get block from cache (create new one if not found)
    std::vector<uint8_t> &block = m_BlocksCache.buffer[hdr.ID];

    // Read block from database if not in cache and not new
    if(block.empty() && !new_block)
       ReadBlock(list_id, hdr.ID, block);

    // Compute block header offset and headers size
    size_t hoff = hdr.ID==1 ? sizeof(PListHeader) : 0;
    size_t hsize = hoff + sizeof(PListBlockHeader);

    // If block does not exist, create new one.
    if(block.empty()){
       block.resize(hsize);
       // Save this key for later iteration.
       CBKey cbk; cbk.k1=list_id; cbk.k2=hdr.ID;
       m_KeyCache.push_back( cbk );
    }

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
    // for the curent list, located in the first block (not necessary if
    // we're processing the first block as it's already updated above).
    if(new_block && hdr.ID!=1)
       UpdateListHeader(list_id, lhdr);
}

// ----------------------------------------------------------------------------

void CBIndex::UpdateListHeader(int list_id, PListHeader &lhdr)
{
    // Try the cache first
    std::vector<uint8_t> &block = m_BlocksCache.buffer[1];

    // Read from database if cache miss
    if(block.empty())
       ReadBlock(list_id, 1, block);

    if(block.empty()){
       block.resize(sizeof(PListHeader));
       // Save this key for later iteration.
       CBKey cbk; cbk.k1=list_id; cbk.k2=1;
       m_KeyCache.push_back( cbk );
    }

    assert(block.size() >= sizeof(PListHeader));

    // Copy list header
    PListHeader& lhdr_old = *reinterpret_cast<PListHeader*>(block.data());
    lhdr_old = lhdr;
}

// ----------------------------------------------------------------------------

void CBIndex::Merge(CBIndex &lidx)
{
    // Here we should iterate through all the keys in the delta index
    // to merge them with the corresponding ones in the main index.
    // This is efficiently done if the K-V store provides direct
    // iterators over the K-space.
    // Unfortunately, Couchbase does not provide such iterators in the
    // C API and the only way of traversing the whole K-space is to create
    // a dedicated Key index (a view) and use the REST API to query it.
    // Here we take advantage of the fact that the key values, by design,
    // are bounded within a fixed range [0, Kmax] (where Kmax depends on
    // the indexing algorithm used). So, to iterate over all the keys we
    // create batches of sequential key IDs and perform a multi-get until
    // the max key value is reached.

    //DEBUG_MSG(m_KeyCache.size()<<" cached keys")

    std::vector<uint8_t> lblock(32768), dblock(32768);

    // For each key in the delta store
    for(CBKey &key : m_KeyCache)
    {
        int list_id  = key.k1;
        int block_id = key.k2;

        // Get delta block chunk
        size_t dbsize = ReadBlock(list_id, block_id, dblock);

        assert(dbsize);

        // Get the block from the live index, if any
        size_t lbsize = lidx.ReadBlock(list_id, block_id, lblock);

        // Calculate block header size
        size_t hsize = block_id==1 ? sizeof(PListHeader) +
                                     sizeof(PListBlockHeader):
                                     sizeof(PListBlockHeader);

        // If block doesn't exist in live index, create new one.
        if(lbsize == 0)
           lbsize = hsize;

        PListBlock lblk = RawBlockToBlock(lblock.data(),
                                          lbsize,
                                          block_id==1);

        PListBlock dblk = RawBlockToBlock(dblock.data(),
                                          dbsize,
                                          block_id==1);

        assert(block_id==1 && lblk.Body ? lblk.ListHeader->BlockCount : 1);
        assert(lbsize > sizeof(PListHeader) + sizeof(PListBlockHeader));

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
           std::copy(dblk.Body,
                     dblk.Body + dblk.BodySize,
                     lblock.data() + lbsize);
        }

        lidx.WriteBlock(list_id, block_id, lblock, lbsize + dblk.BodySize);
    }
}

// ----------------------------------------------------------------------------

PListBlock CBIndex::RawBlockToBlock(uint8_t *block, 
                                    size_t block_size, 
                                    bool isFirst)
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

void CBIndex::FlushBlockCache()
{
    if(m_BlocksCache.buffer.empty())
       return;

    // Schedule any remaining blocks for batched insert.
    for (auto &block : m_BlocksCache.buffer)
    {
        WriteBlock(m_BlocksCache.list_id, 
                   block.first, 
                   block.second, 
                   block.second.size());
    }

    m_StoreResp.sender = __FUNCTION__;

    // Execute the batch
    lcb_wait(m_DBHandle);
    
    THROW_ON_FAIL(m_StoreResp.status, "Couldn't flush the data in the cache.");

    m_BlocksCache.list_id = 0;
    m_BlocksCache.accum = 0;
    m_BlocksCache.buffer.clear();
}

// ----------------------------------------------------------------------------

void CBIndex::ResetCaches()
{
    m_BlocksCache.accum = 0;
    m_BlocksCache.buffer.clear();
    m_KeyCache.clear();
    m_StoreResp.failedKeys.clear();
}



//=============================================================================
//                            CBFingerprints
//=============================================================================



CBFingerprints::CBFingerprints(CBDataStore *dstore) :
    CBCollection (dstore)
{
}

// ----------------------------------------------------------------------------

size_t CBFingerprints::ReadFingerprintSize(uint32_t FID)
{
    /// Unfortunately Couchbase's API does not provide a direct function
    /// to get the size of a stored value, so here we just do a 'get'
    /// and retrieve the size of the value from the response without
    /// copying the data.

    if(m_DBHandle==NULL)
       throw std::runtime_error
      ("Database '"+m_Name+"' not open");

    // Create response structure
    CBGetResp gresp = {};

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &FID;
    cmd.v.v0.nkey = sizeof(uint32_t);

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(gresp.status, "Couldn't execute get operation.");

    return gresp.value_size;
}

// ----------------------------------------------------------------------------

size_t CBFingerprints::ReadFingerprint(uint32_t FID, 
                                       std::vector<uint8_t> &buffer,
                                       size_t size, 
                                       uint32_t bo)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    // Create response structure
    CBGetResp gresp = {};
    gresp.buf = &buffer;
    gresp.data_size = size;
    gresp.data_offset = bo;

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &FID;
    cmd.v.v0.nkey = sizeof(uint32_t);

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(gresp.status, "Couldn't execute get operation.");

    // The fingerprint data will be copied in 'buffer' in the get callback
    
    return gresp.read_size;
}

// ----------------------------------------------------------------------------

void CBFingerprints::WriteFingerprint(uint32_t FID, 
                                      const uint8_t *data, 
                                      size_t size)
{
    assert(data);

    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    CBSetResp sresp;
    sresp.sender = __FUNCTION__;

    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t *commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &FID;
    cmd.v.v0.nkey = sizeof(uint32_t);
    cmd.v.v0.bytes = data;
    cmd.v.v0.nbytes = size;
    cmd.v.v0.operation = LCB_SET;

    lcb_error_t err = lcb_store(m_DBHandle, &sresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate set operation.");

    // Wait for set command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(sresp.status, "Couldn't store the data.");

}


//=============================================================================
//                                CBMetadata
//=============================================================================



CBMetadata::CBMetadata(CBDataStore *dstore) :
    CBCollection (dstore)
{
}

std::string CBMetadata::Read(uint32_t FID)
{   
    std::string str;

    if(m_DBHandle)
    {
        // Create response structure
        CBGetResp gresp = {};
        gresp.buf = &m_Buffer;

        // Create the get command
        lcb_get_cmd_t cmd;
        const lcb_get_cmd_t* commands[1] = { &cmd };
        memset(&cmd, 0, sizeof(cmd));

        cmd.v.v0.key = &FID;
        cmd.v.v0.nkey = sizeof(uint32_t);

        // Initiate get command
        lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

        THROW_ON_FAIL(err, "Couldn't initiate get operation.");

        // Wait for get command to execute
        lcb_wait(m_DBHandle);

        // The data will copied in 'buffer' in the get callback
        if(gresp.read_size > 0){
            char *pstr = reinterpret_cast<char*>(m_Buffer.data());
            str.assign(pstr, gresp.read_size);
        }
    }
    return str;
}

// ----------------------------------------------------------------------------

void CBMetadata::Write(uint32_t FID, const std::string &meta)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
       ("Database '"+m_Name+"' not open");

    if(meta.empty())
       return;
       
    CBSetResp sresp;
    sresp.sender = __FUNCTION__;

    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t *commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &FID;
    cmd.v.v0.nkey = sizeof(uint32_t);
    cmd.v.v0.bytes = meta.data();
    cmd.v.v0.nbytes = meta.size();
    cmd.v.v0.operation = LCB_SET;

    lcb_error_t err = lcb_store(m_DBHandle, &sresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate set operation.");

    // Wait for set command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(sresp.status, "Couldn't store the data.");
}


//=============================================================================
//                                CBInfo
//=============================================================================


CBInfo::CBInfo(CBDataStore *dstore) :
    CBCollection (dstore)
{
}

DBInfo_t CBInfo::Read()
{
    if(m_DBHandle==nullptr)
       throw std::runtime_error
       ("Info database not open");

    DBInfo_t dbinfo;
    int key = 0;

    // Create response structure
    CBGetResp gresp = {};
    gresp.buf = &m_Buffer;

    // Create the get command
    lcb_get_cmd_t cmd;
    const lcb_get_cmd_t* commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &key;
    cmd.v.v0.nkey = sizeof(int);

    // Initiate get command
    lcb_error_t err = lcb_get(m_DBHandle, &gresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate get operation.");

    // Wait for get command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(gresp.status, "Couldn't execute get operation.");

    if(gresp.read_size > 0){
       assert(gresp.read_size == sizeof(DBInfo_t));
       dbinfo = *reinterpret_cast<DBInfo_t*>(m_Buffer.data());
    }
    return dbinfo;
}

// ----------------------------------------------------------------------------

void CBInfo::Write(const DBInfo_t &info)
{
    if(m_DBHandle==NULL)
       throw std::runtime_error
      ("Database '"+m_Name+"' not open");
    
    int key = 0;

    CBSetResp sresp;
    sresp.sender = __FUNCTION__;

    lcb_store_cmd_t cmd;
    const lcb_store_cmd_t *commands[1] = { &cmd };
    memset(&cmd, 0, sizeof(cmd));

    cmd.v.v0.key = &key;
    cmd.v.v0.nkey = sizeof(int);
    cmd.v.v0.bytes = &info;
    cmd.v.v0.nbytes = sizeof(DBInfo_t);
    cmd.v.v0.operation = LCB_SET;

    lcb_error_t err = lcb_store(m_DBHandle, &sresp, 1, commands);

    THROW_ON_FAIL(err, "Couldn't initiate set operation.");

    // Wait for set command to execute
    lcb_wait(m_DBHandle);

    THROW_ON_FAIL(sresp.status, "Couldn't store the data.");

}

