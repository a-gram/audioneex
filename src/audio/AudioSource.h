/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef AUDIOSOURCEEX_H
#define AUDIOSOURCEEX_H

#include <string>
#include <memory>

#include <boost/thread.hpp>

#ifdef WITH_ID3
 #include <taglib/fileref.h>
 #include <taglib/tag.h>
#endif

#include "AudioBuffer.h"
#include "Pipes.h"

#define AUDIO_ENGINE_NAME "FFMPEG"


#ifdef WITH_ID3

/// ID3 Tag class
class ID3Tag
{
    std::string mFilename;
    std::string mTitle;
    std::string mAlbum;
    std::string mArtist;
    std::string mYear;
    std::string mComment;
    std::string mGenre;

    int         mSampleRate {0};
    int         mChannels   {0};
    int         mDuration   {0};

    void SetFile(std::string file);

 public:

    ID3Tag(const std::string &file = "");

    std::string GetTitle()   const { return mTitle; }
    std::string GetAlbum()   const { return mAlbum; }
    std::string GetArtist()  const { return mArtist; }
    std::string GetYear()    const { return mYear; }
    std::string GetComment() const { return mComment; }
    std::string GetGenre()   const { return mGenre; }
    int         GetSampleRate() const { return mSampleRate; }
    int         GetChannels() const { return mChannels; }
    int         GetDuration() const { return mDuration; }
};

#endif

/// Audio source data listener interface. All clients interested in receiving
/// data from an audio source must implement this interface.

class AudioSourceDataListener
{
 public:

    virtual void 
    OnAudioSourceData(AudioBuffer<int16_t> &buffer) = 0;
};

// ----------------------------------------------------------------------------

///
/// This class represents an audio stream reader that uses an external
/// program to supply audio data via stdout. Supports streaming from files
/// and capturing from input audio devices. It is an abstract class as the
/// implementation to open specific sources is left to derived classes, which
/// must implement the Open() method according to the program's CLI.
///

class AudioSource
{
 protected:

    /// The capture thread used to grab audio from input audio lines
    class CaptureThread
    {
      private:
      
        AudioSource* 
        m_AudioSource;
        
      public:

        typedef std::unique_ptr<AudioSource::CaptureThread>   uptr;
        typedef boost::shared_ptr<AudioSource::CaptureThread> sptr;
		
        CaptureThread(AudioSource* asource) :
            m_AudioSource (asource)
        { }
		
        CaptureThread(const CaptureThread&) = delete;
        CaptureThread& operator=(const CaptureThread&) = delete;		

        void 
        Run();
    };
    
#ifdef WIN32

    WindowsPipe<> 
    m_Pipe;
    
#else
    
    PosixPipe 
    m_Pipe;
    
#endif

    /// Path to the audio program.
    static std::string 
    m_BinPath;

    /// List of supported audio formats
    static std::list<std::string> 
    m_SupportedFormats;

    /// The read data's sample rate (in Hz)
    int
    m_SampleRate       {44100};
    
    /// The read data's sample resolution (in bits per sample)
    int
    m_SampleResolution {16};
    
    /// The read data's number of channels
    int
    m_Channels         {2};
    
    /// Offset at which to start reading data (only seekable sources)
    float 
    m_TimeOffset       {0.f};
    
    /// The length of the audio to be decoded starting from m_TimeOffset
    float 
    m_TimeLength       {0.f};
    
    /// Flag to signal the capture thread about termination
    bool
    m_StopCapture      {false};
    
    /// Total samples read from the audio source
    size_t 
    m_TotalSamples     {0};

    /// The file (or input device) being streamed.
    std::string 
    m_FileName;

    /// Listener receiving data from the audio source
    AudioSourceDataListener* 
    m_DataListener     {nullptr};

    /// The audio capture thread
    std::unique_ptr<boost::thread> 
    m_CaptureThread;

#ifdef WITH_ID3

    /// The stream's id3 tags, if present (for files only)
    ID3Tag
    m_ID3Tags;
    
#endif


 public:

    AudioSource() = default;
    virtual ~AudioSource();

    /// Open the audio source. Concrete implementations will provide
    /// the specialized code to open specific sources.
    virtual void 
    Open(const std::string& source_name) = 0;

    bool
    IsOpen() const { return m_Pipe.IsOpen(); }

    /// Close the audio source and stop any capturing in progress.
    void
    Close();

    /// Start the audio streaming thread.
    void
    StartCapture();

    /// End the audio streaming thread.
    void
    StopCapture(bool wait_for_finish=false);

    std::string
    GetFileName() const { return m_FileName; }

    int
    GetSampleRate() const { return m_SampleRate; }
    
    int
    GetSampleResolution() const { return m_SampleResolution; }
    
    int
    GetChannelsCount() const { return m_Channels; }

    void
    SetSampleRate(int rate) { m_SampleRate = rate; }
    
    void
    SetChannelCount(int chans) { m_Channels = chans; }
    
    void
    SetSampleResolution(int res) { m_SampleResolution = res; }

    float
    GetPosition() const { return m_TimeOffset; }

    /// Set the position (in seconds) within the audio source from which to start
    /// decoding data. This is only possible for seekable audio sources.
    void 
    SetPosition(float pos)  { m_TimeOffset = pos; }

    /// Get the duration of all data decoded and read from the source.
    float
    GetDuration() const { return float(m_TotalSamples) / float(m_SampleRate); }

    /// Set the duration of the audio data to be decoded starting from m_TimeOffset
    void
    SetDataLength(float len) { m_TimeLength = len; }

    std::string 
    GetFormattedDuration();

    void
    SetDataListener(AudioSourceDataListener *dataListener);

    /// Print a list of the available input lines from which is possible
    /// to capture.
    static void 
    ListCaptureDevices();

    /// Set the path to the ffmpeg executable. This method is usually called
    /// only once prior to using any AudioSource instance.
    static void
    SetBinPath(const std::string& path) { m_BinPath = path; }
    
    static std::string 
    GetBinPath() { return m_BinPath; }

    static void
    SetSupportedFormats(const std::list<std::string>& fmts) { m_SupportedFormats = fmts; }
    
    static bool
    IsFormatSupported(std::string fmt);

    /// The name of the underlying audio engine
    static std::string 
    GetAudioEngineName() { return AUDIO_ENGINE_NAME; }


    /// Read a block of audio from the open audio source.
    template <class T>
    void 
    GetAudioBlock(AudioBuffer<T> &buffer)
    {
        if(m_Pipe.IsOpen())
        {
           size_t readSamples = 0;// = fread(buffer.Data(), sizeof(T), buffer.Size(), m_Pipe);

           if(!m_Pipe.Read(buffer.Data(), sizeof(T)*buffer.Size(), readSamples))
              throw std::runtime_error
              ("Reading from pipe failed. "+m_Pipe.GetError());

           // Pipe::Read() returns read bytes. Convert to samples.
           readSamples /= sizeof(T);

           // set read buffer size
           buffer.Resize(readSamples);

           m_TotalSamples += readSamples;
        }
        else {
            throw std::runtime_error
            ("Audio source not open");
        }
    }

#ifdef WITH_ID3

    const ID3Tag&
    GetID3Tags() const { return m_ID3Tags; }

    /// Get id3 tags from the specified audio file without opening a stream.
    static ID3Tag
    GetID3TagsFromFile(const std::string &file) { return ID3Tag(file); }
    
#endif

};


// ----------------------------------------------------------------------------

/// This class specializes in providing the right command to the external
/// audio program in order to read audio from a file.
///
class AudioSourceFile : public AudioSource
{
 public:

    void 
    Open(const std::string &source_name);
					 
};

// ----------------------------------------------------------------------------

/// This class specializes in providing the right command to the external
/// audio program in order to read data from an input audio line/device.

class AudioSourceDevice : public AudioSource
{
 public:

    void 
    Open(const std::string &source_name);
								   
};

// ----------------------------------------------------------------------------


#endif // AUDIOSOURCE_H

