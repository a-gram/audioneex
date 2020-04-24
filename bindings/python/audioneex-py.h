
#ifndef AUDIONEEX_PY_H
#define AUDIONEEX_PY_H

#include <memory>
#include <limits>

#include "audioneex.h"
#include "TCDataStore.h"
#include "AudioSource.h"

// ============== TESTS =============

#if SWIG
#else
#include <fstream>
#include <iostream>

extern std::ofstream logfile;
inline void LOG(const std::string &msg) {
    logfile << msg << std::endl; logfile.flush();
}
#endif
// ==================================


#if SWIG
 // During SWIG parsing the following typedefs are already
 // defined in the .i file, so disable them to avoid annoying
 // warnings.
#else
 template<class T> using AxRef = std::shared_ptr<T>;
 template<class T> using AxPtr = std::unique_ptr<T>;
 template<class T> using AxVec = std::vector<T>;
 typedef std::string     AxStr;
#endif

enum AxPyEMatchType
{
    K_MSCALE_MATCH,
    K_XSCALE_MATCH
};

enum AxPyEIdentificationType
{
    K_FUZZY_IDENTIFICATION,
    K_BINARY_IDENTIFICATION
};

enum AxPyEIdentificationMode
{
    K_STRICT_IDENTIFICATION,
    K_EASY_IDENTIFICATION
};

enum AxPyEIdClass
{
    K_UNIDENTIFIED,
    K_SOUNDS_LIKE,
    K_IDENTIFIED
};

struct AxPyIdMatch
{
    int   FID;
    float confidence;
    float score;
    int   id_class;
    int   cue_point;
};

enum Params
{
    _SFREQ    = 11025,  // sampling freq. (Hz)
    _NCHAN    = 1,      // # of channels
    _BPS      = 16,     // bits per sample
    _BUFDUR   = 10,     // buffer length (sec)
    _BUFSIZE  = _SFREQ * _BUFDUR,
    _READSIZE = 16538   // read block size (samples)
};


// -----------------------------------------------------------------------------
// Some helpers
// -----------------------------------------------------------------------------


inline Audioneex::KVDataStore::eOperation
_i_to_kvop(int val) 
{
    return static_cast<Audioneex::KVDataStore::eOperation>(val);
}

inline Audioneex::eMatchType
_i_to_mtype(int val) 
{
    return static_cast<Audioneex::eMatchType>(val);
}

inline Audioneex::eIdentificationType
_i_to_idtype(int val) 
{
    return static_cast<Audioneex::eIdentificationType>(val);
}

inline Audioneex::eIdentificationMode
_i_to_idmode(int val) 
{
    return static_cast<Audioneex::eIdentificationMode>(val);
}

inline Audioneex::eIdClass
_i_to_idclass(int val) 
{
    return static_cast<Audioneex::eIdClass>(val);
}

inline int
_assert_uint(int n) 
{
    if(n < 0 || n > std::numeric_limits<std::uint32_t>::max()) 
       throw Audioneex::Exception("Out of bound uint32 value");
    return n;
}

template<typename T>
inline void
_assert_ptr(T* ptr) 
{
    if(!ptr) 
       throw Audioneex::Exception("Got a null pointer");
}

template<typename T>
inline T
_assert_noneg(T val) 
{
    if(val < 0) 
       throw Audioneex::Exception("Got a negative value");
    return val;
}

inline void
_assert_inbound(int i, int size)
{
    if(i < 0 || i >= size)
       throw Audioneex::Exception
      ("Array index '" + std::to_string(i) + "' out of bound");
}


// -----------------------------------------------------------------------------


template<class T_Iterable, class T_ItItem>
class AxIterator
{
    typename AxRef<T_Iterable>           _iterable;
    typename AxVec<T_ItItem>::iterator   _it;
    typename AxVec<T_ItItem>::iterator   _it_end;

public:

    void
    set(AxRef<T_Iterable> it)
    {
        _iterable = it;
        _it = _iterable->_pimpl()->begin();
        _it_end = _iterable->_pimpl()->end();
    }

    T_ItItem
    next()
    {
        if(_it == _it_end)
           throw Audioneex::Exception
           ("__AX_ITERATOR_END__");
        return (*_it++);
    }
};


// -----------------------------------------------------------------------------


class AxPyDatabase
{
    AxPtr<Audioneex::KVDataStore>
    _impl;
    
public:

    enum
    {
        FETCH,
        BUILD
    };    

    AxPyDatabase(const AxStr& url = "",
                 const AxStr& name = "",
                 const AxStr& opmode = "");
    
    ~AxPyDatabase();

    void 
    open(int op);

    void 
    close();
	    
    bool 
    is_empty();

    void 
    clear();

    bool 
    is_open();
	
    int 
    get_fp_count();
    
    void 
    put_metadata(int FID, const AxStr& meta);

    AxStr
    get_metadata(int FID);

    void 
    put_info(const AxStr& info);

    AxStr
    get_info();

    int
    get_op_mode();

    void 
    set_op_mode(int mode);

    void 
    set_url(const AxStr& url);
	
    AxStr
    get_url();
        
    void 
    set_name(const AxStr& name);
	
    AxStr
    get_name();

    
    Audioneex::KVDataStore*
    _pimpl();
    
};


// =============================================================================


class AxPyAudioBuffer;

class AxPyCallbacks : public Audioneex::AudioProvider
{
public:  // Python callbacks

    AxPyCallbacks() = default;

    virtual 
   ~AxPyCallbacks() = default;

    virtual AxRef<AxPyAudioBuffer>
    on_audio_request(int FID, int nsamples) = 0;

public:  // Internal "bridging" callbacks

    int 
    OnAudioData(uint32_t FID, float* buffer, size_t nsamples);

};


// =============================================================================


class AxPyIndexer
{
    AxPtr<Audioneex::Indexer> 
    _impl;
    
    AxRef<AxPyDatabase>
    _db;
    
    AxRef<AxPyCallbacks>
    _audio_cbks;
    
public:
    
    AxPyIndexer();
    AxPyIndexer(AxRef<AxPyDatabase> dstore, 
                AxRef<AxPyCallbacks> aprovider);
                
    ~AxPyIndexer() = default;

    void 
    start();

    void 
    index(int FID);

    void 
    end(bool flush = true);

    void 
    set_match_type(int type);

    int 
    get_match_type();

    void 
    set_cache_limit(int limit);

    int 
    get_cache_limit();

    int 
    get_cache_used();

    void 
    set_datastore(AxRef<AxPyDatabase> dstore);
    
    AxRef<AxPyDatabase>
    get_datastore();
    
    void 
    set_audio_provider(AxRef<AxPyCallbacks> aprovider);
    
    AxRef<AxPyCallbacks>
    get_audio_provider();

    Audioneex::Indexer*
    _pimpl();    

};


// =============================================================================



class AxPyAudioBuffer
{
    AxVec<float>
    _impl;
    
    int
    _srate;
    
    int
    _channels;
    
    int
    _bps;
    
public:

    typedef AxIterator<AxPyAudioBuffer, float> Iterator;

    AxPyAudioBuffer(int size=_READSIZE);
    
    ~AxPyAudioBuffer() = default;
    
    float
    get_item(int i);
    
    void
    set_item(int i, float val);
    
    int
    capacity();
    
    int
    size();
    
    void
    set_size(int nsamples);
    
    float 
    sample_rate();
    
    int
    channels();
    
    int
    bits_per_sample();
    
    float
    duration();
    
    int
    copy_to(AxPyAudioBuffer* buffer);
        
    int
    get_length();

    AxPyAudioBuffer::Iterator
    iterator();
    
    int
    _copy_to(float* buffer, int nsamples);

    AxVec<float>*
    _pimpl();
    
};


// =============================================================================


class AxPyAudioSourceFile
{
    AxPtr<AudioSourceFile>
    _impl;
    
    AxRef<AxPyAudioBuffer>
    _audio_buffer;
    
    AudioBuffer<int16_t>
    _input_buffer;

public:
    
    AxPyAudioSourceFile(const AxStr& fileURL="");
    
    ~AxPyAudioSourceFile() = default;
    
    void
    open(const AxStr& fileURL);
   
    void
    close();
    
    bool
    is_open();

    int
    read(int nsamples=-1);

    int
    readin(AxPyAudioBuffer* buffer);
   
//    int
//    readin(float* buffer, int nsamples);
    
    AxStr
    filename();
    
    void
    set_position(float time);
    
    float
    get_position();
   
    AxRef<AxPyAudioBuffer>
    buffer();

private:

    int
    _read_from_source(int nsamples);
};


// =============================================================================


class AxPyIdResults
{
    AxVec<AxPyIdMatch>
    _results;
    
public:

    typedef AxIterator<AxPyIdResults, AxPyIdMatch> Iterator;

    AxPyIdResults() = default;
    ~AxPyIdResults() = default;
    
    AxPyIdMatch
    get_item(int i);
    
    void
    set_item(int i, const AxPyIdMatch val);
    
    int
    get_length();
    
    AxPyIdResults::Iterator
    iterator();
    
    AxVec<AxPyIdMatch>*
    _pimpl();
};


//------------------------------------------------------------------------------


class AxPyRecognizer
{
    AxPtr<Audioneex::Recognizer> 
    _impl;
    
    AxRef<AxPyDatabase>
    _db;
    
public:

    AxPyRecognizer();
    AxPyRecognizer(AxRef<AxPyDatabase> dstore);
    ~AxPyRecognizer() = default;

    void 
    set_match_type(int type);

    void 
    set_mms(float value);

    void 
    set_identification_type(int type);

    void 
    set_identification_mode(int mode);

    void 
    set_binary_id_threshold(float value);

	void 
    set_binary_id_min_time(float value);

    void 
    set_max_recording_duration(int duration);

    int 
    get_match_type();

    float 
    get_mms();

    int 
    get_identification_type();

    int 
    get_identification_mode();

    float 
    get_binary_id_threshold();

	float 
    get_binary_id_min_time();

    AxRef<AxPyIdResults> 
    identify(AxPyAudioBuffer* audio);

    AxRef<AxPyIdResults>
    get_results();

    double 
    get_identification_time();

    void 
    flush();

//    void 
//    reset();

    void 
    set_datastore(AxRef<AxPyDatabase> dstore);
    
    AxRef<AxPyDatabase>
    get_datastore();
    
    Audioneex::Recognizer*
    _pimpl();

};


#endif  // AUDIONEEX_PY_H

