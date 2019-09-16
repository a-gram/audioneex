/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef AUDIONEEX_API_H
#define AUDIONEEX_API_H

#include <cstdint>
#include <cstdlib>
#include <stdexcept>

#define ENGINE_VERSION      010300L
#define ENGINE_VERSION_STR  "1.3.0"

/// API linkage management code.
/// Clients should define AUDIONEEX_DLL if they are using the DLL version
/// of the library in order to use (import) the interface under Windows.
/// DO NOT define AUDIONEEX_API_EXPORT as it is reserved for internal use.

#ifdef AUDIONEEX_API_EXPORT
 #ifdef AUDIONEEX_DLL
  #if defined(_MSC_VER) && (_MSC_VER >= 1600)
    #define AUDIONEEX_API __declspec(dllexport)
  #elif defined(__GNUC__) && (__GNUC__ >= 4)
    #define AUDIONEEX_API __attribute__((visibility("default")))
  #elif defined(__clang__) && (__clang_major__ >= 2)
    #define AUDIONEEX_API __attribute__((visibility("default")))	
  #else
    #error "Untested compiler"
  #endif
 #else
  #define AUDIONEEX_API // Static lib
 #endif
#else
 #ifdef AUDIONEEX_DLL
  #define AUDIONEEX_API __declspec(dllimport)
 #else
  #define AUDIONEEX_API
 #endif
#endif


///Audioneex namespace
namespace Audioneex
{

/// These constants determine the type of algorithm used to find a match in
/// the fingerprint database. It must be set before the index is created and 
/// must be used with all the identifications performed using that index. Trying
/// to identify audio using a match type that's different from the one
/// used to build the index will result in wrong recognitions or undefined
/// behavior.
enum eMatchType
{
     /// The original "standard" algorithm. It provides high accuracy
     /// at the expense of reduced speed at increasing scales.
     MSCALE_MATCH,
     
     /// A modified version of the standard algorithm designed
     /// to increase the search speed at large scales by trading off
     /// some accuracy. Produces a much bigger index than MSCALE_MATCH.
     XSCALE_MATCH
};


/// These constants determine the type of classification used internally
/// by the recognizer to perform the identification. The fuzzy mode uses an 
/// internal 3-class fuzzy classifier (see Audioneex::eIdClass) while the binary
/// mode is a simple thresholded classification around a value set by
/// clients using Recognizer::SetBinaryIdThreshold().
enum eIdentificationType
{
     /// Perform identification using a fuzzy logic
     /// classifier. Provides more robustness to noise
     /// by reducing false positives but requires more
     /// audio to find a match.
     FUZZY_IDENTIFICATION,
     
     /// A classifier based on simple thresholding.
     /// Yields results very quickly but may give 
     /// many false positives and thus lower accuracy,
     /// depending on the value of the threshold. Use
     /// Recognizer::SetBinaryIdThreshold() to set the
     /// threshold.
     BINARY_IDENTIFICATION
};


/// These identification modes only apply to the fuzzy identification type
/// and determine the classifier's behavior.
enum eIdentificationMode
{
     /// The classifier uses very tight requirements 
     /// in order to find the best match. Makes the
     /// recognition more robust to noise but slower.
     STRICT_IDENTIFICATION,
     
     /// The classifier uses very loose requirements
     /// to find a match. Makes the recognition faster
     /// but more sensitive to noise.
     EASY_IDENTIFICATION
};


/// These labels are attached to the results returned by the Recognizer::GetResults()
/// method to indicate the outcome of the identification.
/// When using the binary identification type, only two of these labels are
/// are used.
enum eIdClass
{
    /// There is no clear evidence of a match.
    UNIDENTIFIED,
    
    /// There are similarities between the audio and the 
    /// indicated match(es), but the confidence is low.
    SOUNDS_LIKE,
    
    /// There is a clear evidence of a match with a strong
    /// confidence value.
    IDENTIFIED
};


/// Structure for identified best matches returned by the Recognizer. Clients
/// may use this information to link the identified audio to its metadata,
/// to verify the degree of confidence of the recognition and to get the
/// time point within the identified recording at which the match occurred.
struct IdMatch
{
    /// The fingerprint's unique identifier.
    uint32_t FID;
    
    /// A measure of the confidence of match.
    float    Confidence;
    
    /// A score assigned to the match.
    float    Score;
    
    /// The identification class label (see Audioneex::eIdClass).
    eIdClass IdClass;
    
    /// Estimated time point within the recording being identified.
    ///
    /// @note The CuePoint is an estimation and may not exactly represent the 
    ///       real point. The engine works by identifying similarities between 
    ///       the given audio snippet and the reference recordings and it may 
    ///       happen that there are many parts in a recording that are perceptually 
    ///       identical. This is common in music (for example in refrains) so 
    ///       the current audio snippet may match the recording at different 
    ///       time points that cannot be easily disambiguated.
    uint32_t CuePoint;
};


/// A structure holding the header for an index list
struct PListHeader
{
    /// Number of blocks in the list
    uint32_t BlockCount;
};


/// A structure holding the header for an index list's block
struct PListBlockHeader
{
    /// The block's identifier (1-based, sequential number)
    uint32_t ID;
    
    /// Size of the block's body
    uint32_t BodySize;
    
    /// Max value of FID in the block
    uint32_t FIDmax;
};


// ----------------------------------------------------------------------------


/// Convenience functions to check for null id match results
inline bool IsNull(const IdMatch& res)
{
    return res.FID == 0 &&
           res.IdClass == 0 &&
           res.Score == 0 &&
           res.Confidence == 0 &&
           res.CuePoint == 0;
}

/// Convenience functions to check for null list headers
inline bool IsNull(const PListHeader& hdr)
{
    return hdr.BlockCount == 0;
}

/// Convenience functions to check for null block headers
inline bool IsNull(const PListBlockHeader& hdr)
{
    return hdr.ID == 0 && 
           hdr.BodySize == 0 && 
           hdr.FIDmax == 0;
}


// ----------------------------------------------------------------------------


/// Data-layer access interface. 

/// DataStore exposes a generic interface to store and read data used internally
/// by the identification engine. There are, logically, two kinds of data that
/// the engine needs to access: the fingerprints raw data and the fingerprints 
/// indexed data.
/// The DataStore interface provides an abstraction to all these data collectively, 
/// regardless of their specific implementation.
/// Clients must provide the concrete implementations in order to physically access
/// these structures. Note that how these structures are organised in the data store is
/// irrelevant to the identification engine, as long as they are returned in the
/// specified formats.

class AUDIONEEX_API DataStore
{
public:

    /// This method is called by the engine during the identification stage. It shall
    /// return the list's block for the specified list id from the fingerprints index.
    /// Although clients are free to implement their storage solutions and layouts, index
    /// blocks, along with their headers, must be returned as they have been emitted
    /// during the indexing stage, so if the datastore implementation applies some sort
    /// of transformation to the blocks data, the inverse transformation must be applied 
    /// prior to returning the block's data to the engine.
    ///
    /// @param[in]  lid        The identifier of the list from which to retrieve the block.
    /// @param[in]  bid        The identifier of the block to be retrieved.
    /// @param[out] data_size  The size in bytes of the block's data.
    /// @param[in]  headers    Flag specifying whether to include the block's header
    ///                        in the returned data. If true the block's header must be
    ///                        returned prepended to the block's body.
    /// @return                A pointer to the memory location containing the block's data.
    ///                        Clients must ensure that the returned pointer remains valid
    ///                        after the method returns, for example by storing the data
    ///                        into a session-bound buffer. See [link] for more details.
    ///                        @note A null pointer and zero size shall be returned if the
    ///                        block is not found.
    virtual const uint8_t* GetPListBlock(int lid, int bid,
                                         size_t& data_size, 
                                         bool headers=false) = 0;

    /// This method is called by the indexer to signal the data store about the start
    /// of an indexing session. It can be used to perform specific tasks in the data
    /// store before the indexer starts emitting index chunks.
    virtual void OnIndexerStart() = 0;

    /// This method is called by the indexer to signal the data store about the end
    /// of an indexing session. It can be used to perform specific tasks in the data
    /// store after the indexer has finished its job.
    virtual void OnIndexerEnd() = 0;

    /// This method is called by the indexer to signal that the data stored in the
    /// cache is being flushed, that is processed, and index (list) chunks are being
    /// emitted. It is triggered when the cache size exceeds the internal default limit
    /// (or the limit set using Indexer::SetCacheLimit()) or as a result of calling
    /// Indexer::Flush(). It can be used to perform specific tasks according to the
    /// chosen indexing strategy (see [link] for more details).
    virtual void OnIndexerFlushStart() = 0;

    /// This method is called by the indexer to signal that the data stored in the
    /// cache has been completely processed and emitted.
    virtual void OnIndexerFlushEnd() = 0;

    /// This method is called by the indexer during the indexing stage in order to
    /// build the search lists. It shall return the header of the specified list.
    /// The headers must be returned as they have been emitted by the indexer, so if
	/// some sort of transformation has been applied to them the original layout must
	/// be restored.
    ///
    /// @param[in]  lid   The identifier of the list whose header has to be retrieved.
    /// @return           The header of the specified list.
    ///                   @note If the list does not exist the method shall return a
    ///                   null header (a zero-initialized header).
    virtual PListHeader OnIndexerListHeader(int lid) = 0;

    /// This method is called by the indexer during the indexing stage in order to
    /// emit the chunked blocks. It shall return the block's header for the specified
    /// list. The headers must be returned as they have been emitted by the indexer,
	/// so if some sort of transformation has been applied to them the inverse 
	/// must be applied in order to restore the original layout.
    ///
    /// @param[in]  lid   The identifier of the list where the block can be found.
    /// @param[in]  bid   The identifier of the block whose header has to be retrieved.
    /// @return           The header of the specified block in the specified list.
    ///                   @note If the list or block does not exist the method shall return
    ///                   a null header (a zero-initialized header).
    virtual PListBlockHeader OnIndexerBlockHeader(int lid, int bid) = 0;

    /// This handler is called by the indexer whenever a new block chunk is produced.
    /// When the indexer's cache is flushed, its contents are processed and block chunks
    /// emitted for the data store to process. Logically, these chunks are appended to
    /// the specified list as part of the specified block (the last block, also referred
    /// to as the "append block"), although the data store implementation is free to organize
    /// the data as it likes. However, remember that when blocks are requested by the engine,
    /// they have to be returned exactly as they had been emitted. See [link] for more details.
    /// Also, since the list (and the block) to which we're appending are being modified,
    /// the updated headers are emitted as well. Clients should use them to update the
    /// data store accordingly.
    ///
    /// @param[in]  lid    The identifier of the list to which to append the chunk.
    /// @param[in]  lhdr   The list's header.
    /// @param[in]  hdr    The header of the block to which to append the chunk. This is
    ///                    actually always the last block in the list (the "append block").
    /// @param[in]  chunk  Pointer to the chunk's data. Clients must not delete nor
    ///                    retain this pointer.
    /// @param[in]  chunk_size  The size in bytes of the chunk's data.
    virtual void OnIndexerChunk(int lid,
                                PListHeader &lhdr,
                                PListBlockHeader &hdr,
                                uint8_t* chunk, 
                                size_t chunk_size) = 0;

    /// This handler is called by the indexer whenever a new block is produced. 
    /// This method is called to signal the data store that the last block (the 
    /// "append block") in the specified list has reached the set size limit
    /// and the chunk must be put in a new empty block. See [link] for more details.
    ///
    /// @param[in]  lid    The identifier of the list to which to append the chunk.
    /// @param[in]  lhdr   The list's header.
    /// @param[in]  hdr    The block's header.
    /// @param[in]  chunk  Pointer to the chunk's data. Clients must not delete nor
    ///                    retain this pointer.
    /// @param[in]  chunk_size  The size in bytes of the chunk's data.
    virtual void OnIndexerNewBlock (int lid,
                                    PListHeader &lhdr,
                                    PListBlockHeader &hdr,
                                    uint8_t* chunk, 
                                    size_t chunk_size) = 0;

    /// As part of the indexing process, a fingerprint for each processed audio recording
    /// is emitted. This method is called by the indexer to signal the creation of the
    /// fingerprint. Clients may choose not to implement this method if the original
    /// fingerprints are not needed (for example if MMS is always set to zero the only match
    /// level used by the algorithm will not use the fingerprints original data).
    ///
    /// @param[in] FID   The fingerprint's unique identifier.
    /// @param[in] data  Pointer to the memory location containing the fingerprint's data.
    /// @param[in] data_size  The size in bytes of the fingerprint's data.
    virtual void OnIndexerFingerprint(uint32_t FID, 
                                      uint8_t* data, 
                                      size_t data_size) = 0;

    /// Get the size of the specified fingerprint.
    ///
    /// @param[in] FID   The fingerprint's unique identifier.
    /// @return          The size in bytes of the specified fingerprint.
    virtual size_t GetFingerprintSize(uint32_t FID) = 0;

	/// Get fingerprint data from the datastore. The match algorithm does not need the entire
	/// fingerprints for recognition but only small chunks of them, so this method is flexible
	/// with respect to the amount of data that can be retrieved, being it the whole fingerprint
	/// or just portions of it. This method is never called if the MMS parameter is set to 0
    /// because the fingerprints data is not needed in such a case.
    ///
    /// @param[in]  FID    The fingerprint's unique identifier.
    /// @param[out] read   The size of the data actually read.
    /// @param[in]  nbytes The size in bytes of the data to be read. If it's set to zero, the
    ///                    whole fingerprint starting at @p bo shall be returned.
    /// @param[in]  bo     The offset in bytes within the fingerprint at which to start reading.
    /// @return            A pointer to the memory location containing the fingerprint's data.
    ///                    The engine does not take ownership of this pointer, which must remain
    ///                    valid after the method returns. This can be achieved by storing the
    ///                    data into a session-bound buffer (see [link] for more details).
    virtual const uint8_t* GetFingerprint(uint32_t FID,
                                          size_t &read,
                                          size_t nbytes = 0,
                                          uint32_t bo = 0) = 0;


    virtual ~DataStore(){}

};

// ----------------------------------------------------------------------------

/// Audio provider interface to supply audio to the engine.

/// The engine needs audio data in a specific format in order to extract the
/// fingerprints. During indexing an audio provider must be plugged in from
/// which the indexer can retrieve the required audio data. Any class that will
/// handle the supply of audio must implement this interface.

class AUDIONEEX_API AudioProvider
{
public:
    virtual ~AudioProvider(){}

    /// Called by the Indexer whenever audio data for fingerprinting is needed.
    ///
    /// @param[in]  FID      The unique identifier of the recording being indexed.
    /// @param[out] buffer   Pointer to the buffer that will receive the audio data.
    ///                      The audio must be 16 bit, mono, 11025Hz normalized in [-1,1].
    /// @param[in]  nsamples The number of requested samples.
    /// @return              The number of samples actually read or a negative value
    ///                      on error. When all the audio for a recording is consumed
    ///                      the audio provider must signal this to the engine by returing 0.
    virtual int OnAudioData(uint32_t FID, 
                            float* buffer, 
                            size_t nsamples) = 0;
};

// ----------------------------------------------------------------------------

/// Interface to access the engine's core functionality

/// The Recognizer class exposes the part of the API that deals with the audio
/// identification. It performs fingerprinting of the given audio, matching and
/// classification. Clients will use this interface to get the results of the
/// recognition.

class AUDIONEEX_API Recognizer
{
public:

    /// Create an instance of recognizer
    static Recognizer* Create();

    /// Set the type of matching algorithm to be used.
    ///
    /// @param[in]  type  The match type. This must be one of the supported
    ///                   values in Audioneex::eMatchType.
    ///
    /// @warning This value must match the value set in the Indexer to produce
    ///          the fingerprints.
    virtual void SetMatchType(eMatchType type) = 0;

    /// The engine uses a multi-level matching system (MMS) in order to find the 
    //  best candidate fingerprints. This approach allows adaptation of the accuracy 
	/// by adjusting the magnitude of the collected evidence. Clients can control 
    /// such adaptive behaviour by setting a parameter according to the specific 
    /// application being used.
    /// This parameter controls how adaptive the match algorithm should be to
    /// the current state of the recognition. The values are in the range
    /// [0,1], where a value of '0' means that the adaptive behaviour is
    /// turned off (the engine will only use the current evidence to find a
    /// match), a value of '1' means that the engine will collect the maximum
    /// possible amount of evidence, while any value in between affects the
	/// system's decision of whether to use additional evidence or not
	/// (adaptive behaviour). The closer the value to 1 the more evidence the
	/// system will collect (and the more the processing power), and viceversa.
    ///
    /// @param[in] value  The MMS parameter in the range [0,1].
    virtual void SetMMS(float value) = 0;

    /// Set the type of identification to be used.
    ///
    /// @param[in]  type  The identification type. This must be one of the supported
    ///                   values in Audioneex::eIdentificationType.
    virtual void SetIdentificationType(eIdentificationType type) = 0;

    /// Set the classification mode used by the fuzzy classifier.
    ///
    /// @param[in]  mode  The identification mode. This must be one of the supported
    ///                   values in Audioneex::eIdentificationMode.
    virtual void SetIdentificationMode(eIdentificationMode mode) = 0;

    /// Set the threshold for the binary identification mode.
    /// The value of the threshold is in the range [0.5, 1] and the
    /// optimal value is highly application dependent, so you need to
    /// experiment to find the right setting. The threshold controls
    /// an internal measure of "confidence of match" above which the
    /// current best match is considered identified.
    /// See Audioneex::eIdentificationType for more details.
    ///
    /// @param[in]  value  The value of the threshold in the range [0.5, 1].
    virtual void SetBinaryIdThreshold(float value) = 0;

	/// Set the minimum identification time for the binary identification mode. 
    /// This is the minimum period of time that shall elapse before results are 
    /// returned if an identification occurs and that can be used to increase the 
    /// confidence of match.
    /// By default it is set to zero, i.e. there is no minimum time and the 
    /// results are returned as soon as the confidence reaches the set threshold.
	///
	/// @param[in] value   The minimum identification time in seconds. Valid
    ///                    values are in the range [0, 20]
	virtual void SetBinaryIdMinTime(float value) = 0;

    /// This method can be used to set the maximum recording duration (or its 
    /// expected value) in the dataset to be fingerprinted. This value will be 
    /// used internally to optimize the efficiency of some data structures used 
    /// during the matching process. It is not mandatory to set it as the default 
    /// value may be sufficient for most applications. However, should you 
    /// experience highly recurring warning messages about reallocations occurring 
    /// in the matcher, then you can use this value to mitigate or avoid this issue
    /// (note that sporadic reallocations are normal).
    ///
    /// @param[in]  duration  The max duration in seconds.
    virtual void SetMaxRecordingDuration(size_t duration) = 0;

    /// Get the currently set match type.
	/// @return The currently set match type.
    virtual eMatchType GetMatchType() const = 0;

    /// Get the currently set MMS value.
	/// @return The currently set MMS.
    virtual float GetMMS() const = 0;

    /// Get the currently set identification type.
	/// @return The currently set identification type.
    virtual eIdentificationType GetIdentificationType() const = 0;

    /// Get the currently set identification mode.
	/// @return The currently set identification mode.
    virtual eIdentificationMode GetIdentificationMode() const = 0;

    /// Get the currently set binary id threshold.
	/// @return The currently set binary id threshold.
    virtual float GetBinaryIdThreshold() const = 0;

	/// Get the currently set binary id minimum identification time.
	/// @return The currently set binary id minimum identification time.
	virtual float GetBinaryIdMinTime() const = 0;

    /// This method is the heart of the recognition engine. Given an audio clip, 
    /// it tries to match it against the reference fingerprints in the database
    /// to find the best match. It is designed and optimized for real-time audio 
    /// identification, so it must be fed with short chunks of audio, generally 
    /// 1-2 seconds long. If longer chunks are used, a buffer overflow with data 
    /// loss will occur. Snippets shorter than 500ms won't be processed. 
    /// The audio must be 16 bit normalized in [-1,1], mono, 11025Hz. Note that
    /// this call is synchronous (i.e. blocking).
    ///
    /// @param[in]  audio  Pointer to the buffer containing the audio samples.
    ///                    The engine does not take ownership of the pointer.
    /// @param[in]  nsamples   Number of samples in the buffer.
    virtual void Identify(const float *audio, size_t nsamples) = 0;

    /// Call this method to check the current state of the identification.
    /// Usually this is done right after calling Identify().
    ///
    /// @return  A pointer to an array of Audioneex::IdMatch structures representing the
    /// best match(es). Usually there would be only one match, although ties may
    /// occur. The end of the array is marked by a 'null' element, which is an IdMatch
    /// structure set to zero (you can use Audioneex::IsNull(IdMatch) to check 
    /// for that), so the number of elements in the array is always greater than zero.
    /// If no identification could be made, the results set will contain only the null
    /// element. The recognizer will always return a results set after a set period of
    /// time has elapsed, which can be empty if no match was found or an array 
    /// with the best match(es). If the identification cannot be completed (e.g. for 
    /// insufficient audio) the returned pointer will be null, so clients should 
    /// always check its validity.
    ///
    /// @note The returned pointer is owned by the identification engine and must not be
    ///       deleted nor retained by clients.
    virtual const IdMatch* GetResults() = 0;

    /// Get the identification time. This is actually the duration of the audio being
    /// fed to the engine until a response is given (whether positive or negative).
    ///
    /// @return  The time taken to perform the identification.
    virtual double GetIdentificationTime() const = 0;

    /// Flush the internal buffers. This method is mostly useful when performing
    /// off-line identifications (i.e. on fixed streams, such as files) where the
    /// length of the audio stream is finite. In this case, if the audio data is 
    /// exhausted before the identification engine gives a response this method 
    /// can be called to flush any residual data in the internal buffers and then 
    /// check again for results. Using this method for identifications performed on
    /// indefinitely long streams (i.e. live streams) is not recommended. The method 
    /// does nothing if the recognizer has already given a response.
    virtual void Flush() = 0;

    /// Reset the recognizer's internal state. After identification results are
    /// produced the recognizer must be reset to start a new identification session
    /// using the same instance.
    ///
    /// @warning It is absolutely important to reset the recognizer if the same
    ///          instance is reused for different recognitions. Not doing so may
    ///          produce undefined behavior.
    virtual void Reset() = 0;

    /// Set the data store to be used for data I/O.
    ///
    /// @param[in] dstore  A pointer to a data store implementation.
    virtual void SetDataStore(DataStore* dstore) = 0;

    /// Get the currently set data store.
    /// @return The currently set datastore
    virtual DataStore* GetDataStore() const = 0;


    virtual ~Recognizer(){}

};

// ----------------------------------------------------------------------------

/// Interface to access the engine's indexing functionality

/// The Indexer is responsible for the initiation, maintenance and finalization
/// of an indexing session. Its job is that of extracting the fingerprints
/// from the given audio recordings, transform them into a format that is suitable
/// for fast searches and emitting the data necessary to build the required data
/// structures for storage.

class AUDIONEEX_API Indexer
{
public:

    /// Create an instance of Indexer
    static Indexer* Create();

    /// Start an indexing session. This method **must** be called prior to calling
    /// any of the Index() methods.
    virtual void Start() = 0;

    /// Do indexing of audio recordings. The client shall call this method
    /// for every audio recording to be fingerprinted by passing its unique
    /// fingerprint identifier (FID). Fingerprint Identifiers must be strict
    /// increasing positive integral numbers. Upon calling this method, the
    /// engine will call the client's registered audio provider in order
    /// to get the recording's audio data. This is accomplished by repeatedly
    /// calling AudioProvider::OnAudioData() until there is no more audio for
    /// the recording identified by FID. The fingerprint for the current
    /// audio recording is emitted by calling DataStore::OnIndexerFingerprint()
    /// and is indexed according to the currently set match type.
    ///
    /// @note An indexing session must be started by calling Indexer::Start() 
    /// prior to using this method.
    ///
    /// @param[in]  FID   The fingerprint's unique identifier. Must be a positive
    ///                   strictly monotonic increasing integer.
    virtual void Index(uint32_t FID) = 0;

    /// Do indexing of the given fingerprint. This method may be used to reindex
    /// the fingerprints database, for example in order to change the match type
    /// or other parameters.
    /// An indexing session must be started by calling Indexer::Start() prior
    /// to using this method. Note that using different Index() methods within the
    /// same session is not allowed.
    ///
    /// @param[in]  FID     The fingerprint's unique identifier.
    /// @param[in]  fpdata  A pointer to the memory location containing the fingerprint
    ///                     data. The engine does not take ownership of the pointer.
    /// @param[in]  fpsize  The size in bytes of the fingerprint data.
    virtual void Index(uint32_t FID, 
                       const uint8_t* fpdata, 
                       size_t fpsize) = 0;

    /// This method flushes the indexer's cache starting the processing and emission
    /// of list chunks. Normally the cache is automatically flushed by the indexer
    /// whenever the set memory limit is reached, but you may want to do it manually
    /// based on circumnstances. In such a case you can use this method in conjunction
    /// with GetCacheUsed().
    virtual void Flush() = 0;

    /// End an indexing session. This method **must** be called when there is nothing
    /// more to index. Failing to end an indexing session may result in data loss
    /// and/or undefined behaviour. The method also flushes any remaining data in
    /// the cache, unless otherwise specified.
    ///
    /// @param[in]  flush  Flag specifying whether to flush the indexer's cache.
    virtual void End(bool flush = true) = 0;

    /// Set the match type to be used with the index. This determines the
    /// type and size of the index.
    ///
    /// @param[in]  type  The match type. This must be one of the supported
    ///                   values in Audioneex::eMatchType.
    ///
    /// @note Once the index has been created with a specific match type it
    ///       can only be used to perform identifications using that type of
    ///       matching strategy. To set the match type for the identification
    ///       use the Recognizer::SetMatchType() method passing the same value
    ///       used to build the index. Mixing match types (that is trying to
    ///       identify audio using an index built with a different match type)
    ///       will produce incorrect results.
    virtual void SetMatchType(eMatchType type) = 0;

    /// Get the currently set match type
    virtual eMatchType GetMatchType() const = 0;

    /// Set the cache size (in MB). The indexer will flush the cache once this
    /// limit is reached.
    ///
    /// @param[in]  limit  The memory limit in MB.
    virtual void SetCacheLimit(size_t limit) = 0;

    /// Get the currently set cache limit.
    ///
    /// @return The currently set limit in MB
    virtual size_t GetCacheLimit() const = 0;

    /// Get the amount of memory currently used by the cache.
    ///
    /// @return  The current cache size in bytes.
    virtual size_t GetCacheUsed() const = 0;

    /// Set the data store to be used for data I/O.
    virtual void SetDataStore(DataStore* dstore) = 0;

    /// Get the currently set data store.
    virtual DataStore* GetDataStore() const = 0;

    /// Set the audio provider.
    virtual void SetAudioProvider(AudioProvider* aprovider)= 0;

    /// Get the currently set audio provider.
    virtual AudioProvider* GetAudioProvider() const = 0;


    virtual ~Indexer(){}

};


// ----------------------------------------------------------------------------


/// Get the engine version
AUDIONEEX_API const char* GetVersion();


// ----------------------------------------------------------------------------


/// Exception base class for all engine error conditions.
class AUDIONEEX_API Exception : public std::logic_error {
  /// Constructor
  public: explicit Exception(const std::string& msg) :
                   std::logic_error(msg) {}
};

/// This exception is raised as a consequence of processing an invalid
/// fingerprint. This may be due to errors in the data stores, issues
/// with the audio data, etc. It may or may not be a fatal error, depending
/// on the context. For instance, trying to fingerprint a corrupt audio
/// file or unsupported format during indexing may be recoverable by just
/// skipping the offending file, while an invalid fingerprint received
/// during the identification stage could be a sign of a more serious
/// problem, such as corrupt databases, etc.
class AUDIONEEX_API InvalidFingerprintException : public Audioneex::Exception {
  /// Constructor
  public: explicit InvalidFingerprintException(const std::string& msg) :
                   Audioneex::Exception(msg) {}
};

/// This exception is raised whenever invalid data is received from the index.
/// This is a sign of inconsistency in the fingerprints index (maybe due to
/// errors in its implementation) and should be considered a fatal error.
class AUDIONEEX_API InvalidIndexDataException : public Audioneex::Exception {
  /// Constructor
  public: explicit InvalidIndexDataException(const std::string& msg) :
                   Audioneex::Exception(msg) {}
};

/// This exception is raised when trying to perform some indexing operation
/// while the indexer is in an invalid state. For instance, trying to perform
/// indexing without starting a session will rise this exception.
class AUDIONEEX_API InvalidIndexerStateException : public Audioneex::Exception {
  /// Constructor
  public: explicit InvalidIndexerStateException(const std::string& msg) :
                   Audioneex::Exception(msg) {}
};

/// This exception is raised as a consequence of invalid parameters being
/// set in the engine. For example trying to perform identification without
/// setting a data store or trying to set parameters with invalid arguments.
/// It is very similar in scope to std::invalid_argument.
class AUDIONEEX_API InvalidParameterException : public Audioneex::Exception {
  /// Constructor
  public: explicit InvalidParameterException(const std::string& msg) :
                   Audioneex::Exception(msg) {}
};


}// end namespace Audioneex::

#endif // AUDIONEEX_API_H
