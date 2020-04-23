
#include <memory>
#include <vector>
#include <algorithm>

#include "audioneex-py.h"

// ============== TESTS =============

std::ofstream logfile ("C:\\Users\\Albert\\Desktop\\git\\audioneex\\bindings\\python\\log.txt", std::ofstream::app);

// ==================================


//-------------------------------------------------------------------------------


AxPyDatabase::AxPyDatabase(const AxStr& url, const AxStr& name)
:
    _impl{ std::make_unique<TCDataStore>(url, name) }
{
    //set_url(url);
    //set_name(name);
}

AxPyDatabase::~AxPyDatabase()
{
    close();
}

void 
AxPyDatabase::open(int op)
{
    _impl->Open(_i_to_kvop(op), true, true, true);
}

void 
AxPyDatabase::close() 
{ 
    _impl->Close(); 
}

bool 
AxPyDatabase::is_empty() 
{ 
    return _impl->IsEmpty(); 
}

void
AxPyDatabase::clear() 
{  
    _impl->Clear(); 
}

bool 
AxPyDatabase::is_open() 
{ 
    return _impl->IsOpen(); 
}

int 
AxPyDatabase::get_fp_count() 
{ 
    return _assert_uint(_impl->GetFingerprintsCount());
}

void 
AxPyDatabase::put_metadata(int FID, const AxStr& meta)
{ 
    _impl->PutMetadata(_assert_uint(FID), meta);
}

AxStr
AxPyDatabase::get_metadata(int FID) 
{ 
    return _impl->GetMetadata(_assert_uint(FID)); 
}

void 
AxPyDatabase::put_info(const AxStr& info) 
{ 
    _impl->PutInfo(info); 
}

AxStr
AxPyDatabase::get_info() 
{ 
    return _impl->GetInfo(); 
}

int
AxPyDatabase::get_op_mode()
{ 
    return _impl->GetOpMode();
}

void 
AxPyDatabase::set_op_mode(int mode)
{
    _impl->SetOpMode(_i_to_kvop(mode));
}

void 
AxPyDatabase::set_url(const AxStr& url)
{
    _impl->SetDatabaseURL(url);
}

AxStr
AxPyDatabase::get_url()
{
    return _impl->GetDatabaseURL();
}

void 
AxPyDatabase::set_name(const AxStr& name)
{
    _impl->SetDatabaseName(name);
}

AxStr
AxPyDatabase::get_name()
{
    return _impl->GetDatabaseName();
}

Audioneex::KVDataStore*
AxPyDatabase::_pimpl()
{
    return _impl.get();
}


// =============================================================================


int 
AxPyCallbacks::OnAudioData(uint32_t FID, float* buffer, size_t nsamples)
{
    auto abuffer = on_audio_request(_assert_uint(FID), nsamples);
    return abuffer ? abuffer->_copy_to(buffer, nsamples) : -1;
}


// =============================================================================


AxPyIndexer::AxPyIndexer()
:
    _impl{ Audioneex::Indexer::Create() }
{
}

AxPyIndexer::AxPyIndexer(AxRef<AxPyDatabase> dstore, 
                         AxRef<AxPyCallbacks> aprovider)
:
    _impl{ Audioneex::Indexer::Create() }
{
    if(dstore)
    {
       set_datastore(dstore);
    }
    if(aprovider)
    {
       set_audio_provider(aprovider);
    }
}

void 
AxPyIndexer::start()
{
    _impl->Start();
}

void 
AxPyIndexer::index(int FID)
{
    _impl->Index(_assert_uint(FID));
}

void 
AxPyIndexer::end(bool flush)
{
    _impl->End();
}

void 
AxPyIndexer::set_match_type(int type)
{
    _impl->SetMatchType(_i_to_mtype(type));
}

int 
AxPyIndexer::get_match_type()
{
    return _impl->GetMatchType();
}

void 
AxPyIndexer::set_cache_limit(int limit)
{
    _impl->SetCacheLimit(_assert_uint(limit));
}

int 
AxPyIndexer::get_cache_limit()
{
    return _assert_uint(_impl->GetCacheLimit());
}

int 
AxPyIndexer::get_cache_used()
{
    return _assert_uint(_impl->GetCacheUsed());
}

void 
AxPyIndexer::set_datastore(AxRef<AxPyDatabase> dstore)
{
    //_assert_ptr(dstore);
    _impl->SetDataStore(dstore->_pimpl());
    _db = dstore;
}

AxRef<AxPyDatabase>
AxPyIndexer::get_datastore()
{
    return _db;
}

void 
AxPyIndexer::set_audio_provider(AxRef<AxPyCallbacks> aprovider)
{
    //_assert_ptr(aprovider);
    _impl->SetAudioProvider(aprovider.get());
    _audio_cbks = aprovider;
}

AxRef<AxPyCallbacks>
AxPyIndexer::get_audio_provider()
{
    return _audio_cbks;
}

Audioneex::Indexer*
AxPyIndexer::_pimpl()
{
    return _impl.get();
}


// =============================================================================


AxPyIdMatch
AxPyIdResults::get_item(int i)
{
    _assert_inbound(i, _results.size());
    return _results[i];
}

void
AxPyIdResults::set_item(int i, const AxPyIdMatch val)
{
    _assert_inbound(i, _results.size());
    _results[i] = val;
}

int
AxPyIdResults::get_length()
{
    return _results.size();
}

AxPyIdResults::Iterator
AxPyIdResults::iterator()
{
    return AxPyIdResults::Iterator();
}

AxVec<AxPyIdMatch>*
AxPyIdResults::_pimpl()
{
    return &_results;
}


//------------------------------------------------------------------------------


AxPyRecognizer::AxPyRecognizer()
:
    _impl{ Audioneex::Recognizer::Create() }
{
}

AxPyRecognizer::AxPyRecognizer(AxRef<AxPyDatabase> dstore)
:
    _impl{ Audioneex::Recognizer::Create() }
{
    if(dstore)
    {
       set_datastore(dstore);
    }
}

void 
AxPyRecognizer::set_match_type(int type)
{
    _impl->SetMatchType(_i_to_mtype(type));
}

void 
AxPyRecognizer::set_mms(float value)
{
    _impl->SetMMS(value);
}

void 
AxPyRecognizer::set_identification_type(int type)
{
    _impl->SetIdentificationType(_i_to_idtype(type));
}

void 
AxPyRecognizer::set_identification_mode(int mode)
{
    _impl->SetIdentificationMode(_i_to_idmode(mode));
}

void 
AxPyRecognizer::set_binary_id_threshold(float value)
{
    _impl->SetBinaryIdThreshold(value);
}

void 
AxPyRecognizer::set_binary_id_min_time(float value)
{
    _impl->SetBinaryIdMinTime(value);
}

void 
AxPyRecognizer::set_max_recording_duration(int duration)
{
    _impl->SetMaxRecordingDuration(duration);
}

int 
AxPyRecognizer::get_match_type()
{
    return _impl->GetMatchType();
}

float 
AxPyRecognizer::get_mms()
{
    return _impl->GetMMS();
}

int 
AxPyRecognizer::get_identification_type()
{
    return _impl->GetIdentificationType();
}

int 
AxPyRecognizer::get_identification_mode()
{
    return _impl->GetIdentificationMode();
}

float 
AxPyRecognizer::get_binary_id_threshold()
{
    return _impl->GetBinaryIdThreshold();
}

float 
AxPyRecognizer::get_binary_id_min_time()
{
    return _impl->GetBinaryIdMinTime();
}

AxRef<AxPyIdResults> 
AxPyRecognizer::identify(AxPyAudioBuffer* audio)
{
    _assert_ptr(audio);
    auto buf = audio->_pimpl();
    _impl->Identify(buf->data(), buf->size());
    return get_results();
}

AxRef<AxPyIdResults>
AxPyRecognizer::get_results()
{

    auto results = _impl->GetResults();
    
    if(!results) return nullptr;
    
    auto id_results = std::make_shared<AxPyIdResults>();
    auto matches = id_results->_pimpl();

    for(int i=0; !Audioneex::IsNull(results[i]); i++)
    {
        AxPyIdMatch pm = {
            results[i].FID,
            results[i].Confidence,
            results[i].Score,
            results[i].IdClass,
            results[i].CuePoint
        };
        matches->push_back(pm);
    }
    return id_results;
}

double 
AxPyRecognizer::get_identification_time()
{
    return _impl->GetIdentificationTime();
}

void 
AxPyRecognizer::flush()
{
    _impl->Flush();
}
/*
void 
AxPyRecognizer::reset()
{
    _impl->Reset();
}
*/
void 
AxPyRecognizer::set_datastore(AxRef<AxPyDatabase> dstore)
{
    //_assert_ptr(dstore);
    _impl->SetDataStore(dstore->_pimpl());
    _db = dstore;
}

AxRef<AxPyDatabase>
AxPyRecognizer::get_datastore()
{
    return _db;
}

Audioneex::Recognizer*
AxPyRecognizer::_pimpl()
{
    return _impl.get();
}


// =============================================================================


AxPyAudioBuffer::AxPyAudioBuffer(int size)
:
    _impl     ( _assert_uint(size) ),
    _srate    { _SFREQ },
    _channels { _NCHAN },
    _bps      { sizeof(float) * 8 }
{
}

float
AxPyAudioBuffer::get_item(int i)
{
    _assert_inbound(i, _impl.size());
    return _impl[i];
}

void
AxPyAudioBuffer::set_item(int i, float val)
{
    _assert_inbound(i, _impl.size());
    _impl[i] = val;
}

int
AxPyAudioBuffer::capacity()
{
    return _impl.capacity();
}

int
AxPyAudioBuffer::size()
{
    return _impl.size();
}

void
AxPyAudioBuffer::set_size(int nsamples)
{
    return _impl.resize(_assert_uint(nsamples));
}

float 
AxPyAudioBuffer::sample_rate()
{
    return _srate;
}

int
AxPyAudioBuffer::channels()
{
    return _channels;
}

int
AxPyAudioBuffer::bits_per_sample()
{
    return _bps;
}

float
AxPyAudioBuffer::duration()
{
    return size() / (float(_srate) * _channels);
}

int
AxPyAudioBuffer::copy_to(AxPyAudioBuffer* buffer)
{
    _assert_ptr(buffer);
    buffer->_impl = this->_impl;
    return size();
}

int
AxPyAudioBuffer::get_length()
{
    return size();
}

AxPyAudioBuffer::Iterator
AxPyAudioBuffer::iterator()
{
    return AxPyAudioBuffer::Iterator();
}

int
AxPyAudioBuffer::_copy_to(float* buffer, int nsamples)
{
    _assert_ptr(buffer);
    int copied = std::min(nsamples, size());
    std::copy(begin(_impl), begin(_impl) + copied, buffer);
    return copied;
}

AxVec<float>*
AxPyAudioBuffer::_pimpl()
{
    return &_impl;
}


// =============================================================================


AxPyAudioSourceFile::AxPyAudioSourceFile(const AxStr& fileURL)
:
    _impl         { std::make_unique<AudioSourceFile>() },
    _audio_buffer { std::make_shared<AxPyAudioBuffer>(_BUFSIZE) },
    _input_buffer { _BUFSIZE, _SFREQ, _NCHAN }
{
    _impl->SetSampleRate( _SFREQ );
    _impl->SetChannelCount( _NCHAN );
    _impl->SetSampleResolution( _BPS );
    
    if(fileURL != "")
    {
       open(fileURL);
    }
}

void
AxPyAudioSourceFile::open(const AxStr& fileURL)
{
    _impl->Open(fileURL);
}

void
AxPyAudioSourceFile::close()
{
    _impl->Close();
    _audio_buffer->_pimpl()->resize(0);
}

bool
AxPyAudioSourceFile::is_open()
{
    return _impl->IsOpen();
}

/*
int
AxPyAudioSourceFile::readin(float* buffer, int nsamples)
{
    _assert_ptr(buffer);
    _read_from_source(nsamples);
    return _audio_buffer->copy_to(buffer, nsamples);
}
*/

int
AxPyAudioSourceFile::readin(AxPyAudioBuffer* buffer)
{
    _assert_ptr(buffer);
    _read_from_source(buffer->size());
    return _audio_buffer->copy_to(buffer);
}

int
AxPyAudioSourceFile::read(int nsamples)
{
    return _read_from_source(nsamples < 0 ? _READSIZE : nsamples);
}

AxStr
AxPyAudioSourceFile::filename()
{
    return _impl->GetFileName();
}

void
AxPyAudioSourceFile::set_position(float time)
{
    return _impl->SetPosition(_assert_noneg(time));
}

float
AxPyAudioSourceFile::get_position()
{
    return _impl->GetPosition();
}

AxRef<AxPyAudioBuffer>
AxPyAudioSourceFile::buffer()
{
    return _audio_buffer;
}

int
AxPyAudioSourceFile::_read_from_source(int nsamples)
{
    auto audio_buffer = _audio_buffer->_pimpl();

    // AudioBuffer does not do reallocations, so clients should never 
    // request more samples than its capacity to avoid data loss.
    if(nsamples > audio_buffer->capacity())
    {
       throw Audioneex::Exception
       ("Maximum buffer capacity exceeded (" + std::to_string(_BUFDUR) + 
        "s, " + std::to_string(_BUFSIZE) + "samples)");
    }

    _input_buffer.Resize( nsamples );
    _impl->GetAudioBlock( _input_buffer );
    audio_buffer->resize(_input_buffer.Size());
    
    std::transform(_input_buffer.Data(),
                   _input_buffer.Data() + _input_buffer.Size(),
                   audio_buffer->begin(),
                   []
                   (int16_t sample) -> float {
                      return sample / 32768.f;
                   });
    
    return audio_buffer->size();
}

