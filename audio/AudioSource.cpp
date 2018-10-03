/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#include <vector>
#include <algorithm>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "AudioSource.h"

#ifdef WIN32
namespace {
 const char PLATFORM_BIN_NAME[]    = "ffmpeg.exe";
 const char AUDIO_SUBSYSTEM[]      = "dshow";
 const char LIST_CAP_DEVICES_CMD[] = "ffmpeg -list_devices true -f dshow -i dummy";
 const int CAPTURE_CHUNK_SIZE      = 4096;  // samples
}
#else
namespace {
 const char PLATFORM_BIN_NAME[]    = "ffmpeg";
 const char AUDIO_SUBSYSTEM[]      = "alsa";
 const char LIST_CAP_DEVICES_CMD[] = "arecord -l";
 const int CAPTURE_CHUNK_SIZE      = 4096;  // samples
}
#endif

// statics
std::string            AudioSource::m_BinPath          = PLATFORM_BIN_NAME;
std::list<std::string> AudioSource::m_SupportedFormats = {"WAV","AIFF","AU","CDA","FLAC",
                                                          "MP3","M4A","AAC","OGG"};

// ----------------------------------------------------------------------------

AudioSource::~AudioSource()
{
    if(IsOpen())
       Close();
}

// ----------------------------------------------------------------------------

void AudioSource::Close()
{
    // Terminate the capture thread if running
    if(m_CaptureThread){
       m_StopCapture=true;
       m_CaptureThread->join();
       m_CaptureThread.reset();
    }

    if(m_Pipe.IsOpen())
       m_Pipe.Close();

#ifdef ID3_TAG_SUPPORT
    m_ID3Tags = ID3Tag();
#endif
}

// ----------------------------------------------------------------------------

void AudioSource::StartCapture()
{
    // Create the capture thread
    m_CaptureThread.reset( new boost::thread( boost::bind(&CaptureThread::Run,
                                                           CaptureThread::sptr
                                                          ( new CaptureThread(this) ))) );
}

// ----------------------------------------------------------------------------

void AudioSource::StopCapture(bool wait_for_finish)
{
    m_StopCapture = true;
    if(wait_for_finish)
       m_CaptureThread->join();
}

// ----------------------------------------------------------------------------

std::string AudioSource::GetFormattedDuration()
{
    char buf[32];
    int dur = static_cast<int>(m_TotalSamples / m_Channels / m_SampleRate);
    ::sprintf(buf, "%02d:%02d:%02d", dur/3600, (dur%3600)/60, (dur%3600)%60);
    return std::string(buf);
}

// ----------------------------------------------------------------------------

void AudioSource::SetDataListener(AudioSourceDataListener *dataListener)
{
    assert(dataListener);
    m_DataListener = dataListener;
}

// ----------------------------------------------------------------------------

void AudioSource::ListCaptureDevices()
{
    std::string cmd = LIST_CAP_DEVICES_CMD;
	
#ifdef WIN32
    WindowsPipe<> pipe;
    pipe.SetUseErrorChannel(true);
	
    if(!pipe.Open(cmd, INPUT_PIPE))
       throw std::runtime_error
       ("Couldn't execute " + cmd + ". " + pipe.GetError());
   
    std::cout << pipe.ReadErr() << std::endl;
#else
    PosixPipe pipe;

    if(!pipe.Open(cmd, INPUT_PIPE))
       throw std::runtime_error
       ("Couldn't execute " + cmd + ". " + pipe.GetError());
#endif
}

// ----------------------------------------------------------------------------

bool AudioSource::IsFormatSupported(std::string fmt)
{
											  

    std::transform(fmt.begin(), fmt.end(),
                   fmt.begin(), ::toupper);

    return std::find(m_SupportedFormats.begin(),
                     m_SupportedFormats.end(),
                     fmt) != m_SupportedFormats.end();
}


// ----------------------------------------------------------------------------
//                             AudioSourceFile
// ----------------------------------------------------------------------------


void AudioSourceFile::Open(const std::string &source_name)
{
    if(IsOpen()) Close();

    m_Pipe.SetProgramPath( m_BinPath );

    m_Pipe.AddCmdArg( "-i \"" + source_name + "\"" );
    m_Pipe.AddCmdArg( m_SampleRate ? "-ar " + std::to_string(m_SampleRate) : "" );
    m_Pipe.AddCmdArg( m_Channels ? "-ac " + std::to_string(m_Channels) : "" );
    m_Pipe.AddCmdArg( m_SampleResolution ? "-f s" + std::to_string(m_SampleResolution) + "le" : "-f s16le" );
    m_Pipe.AddCmdArg( m_TimeOffset ? "-ss " + std::to_string(m_TimeOffset) : "" );
    m_Pipe.AddCmdArg( m_TimeLength ? "-t " + std::to_string(m_TimeLength) : "" );
    m_Pipe.AddCmdArg( "-");
	
#ifdef WIN32
    m_Pipe.SetUseErrorChannel(true);
#else
    m_Pipe.AddCmdArg( "2>/dev/null" );
#endif

    if(!m_Pipe.Open(INPUT_PIPE))
       throw std::runtime_error
       ("Couldn't open pipe to "+std::string(PLATFORM_BIN_NAME)+". "+m_Pipe.GetError());

    m_TotalSamples = 0;
    m_FileName = source_name;

#ifdef ID3_TAG_SUPPORT
    m_ID3Tags = ID3Tag(source_name);
#endif
}


// ----------------------------------------------------------------------------
//                            AudioSourceDevice
// ----------------------------------------------------------------------------


void AudioSourceDevice::Open(const std::string &source_name)
{
    if(m_DataListener == nullptr)
       throw std::invalid_argument
       ("No audio data consumer set");

    if(IsOpen()) Close();

    m_Pipe.SetProgramPath( m_BinPath );

    m_Pipe.AddCmdArg( "-f " + std::string(AUDIO_SUBSYSTEM) );
#ifdef WIN32
    m_Pipe.AddCmdArg( "-i audio=\"" + source_name +"\"" );
#else
    m_Pipe.AddCmdArg( "-i hw:" + source_name );
#endif
    m_Pipe.AddCmdArg( m_SampleRate ? "-ar " + std::to_string(m_SampleRate) : "" );
    m_Pipe.AddCmdArg( m_Channels ? "-ac " + std::to_string(m_Channels) : "" );
    m_Pipe.AddCmdArg( m_SampleResolution ? "-f s" + std::to_string(m_SampleResolution) + "le" : "-f s16le" );
    m_Pipe.AddCmdArg( "-" );
#ifdef WIN32
    m_Pipe.SetUseErrorChannel(true);
#else
    m_Pipe.AddCmdArg( "2>/dev/null" );
#endif

    if(!m_Pipe.Open(INPUT_PIPE))
       throw std::runtime_error
      ("Couldn't open pipe to "+std::string(PLATFORM_BIN_NAME)+". "+m_Pipe.GetError());

    m_StopCapture = false;

    m_TotalSamples = 0;
    m_FileName = source_name;
}


// ----------------------------------------------------------------------------
//                        AudioSource::CaptureThread
// ----------------------------------------------------------------------------


void AudioSource::CaptureThread::Run()
{
    // Set capture buffer
    AudioBlock<int16_t> cbuffer(CAPTURE_CHUNK_SIZE,
                                m_AudioSource->m_SampleRate,
                                m_AudioSource->m_Channels, 0);

    size_t read_samples = 0;
    size_t readBytes = sizeof(int16_t) * CAPTURE_CHUNK_SIZE;

    while(!m_AudioSource->m_StopCapture)
    {
        if(!m_AudioSource->m_Pipe.Read(cbuffer.Data(), readBytes, read_samples)){
            std::cout << "Audio thread: Reading from pipe failed." << std::endl;
            break;
        }

        if(read_samples == 0){
           std::cout << "Audio thread: No data received from pipe." << std::endl;
           break;
        }

        // Pipe::Read() returns read bytes. Convert to samples.
        read_samples /= sizeof(int16_t);

        cbuffer.Resize(read_samples);

        m_AudioSource->m_DataListener->OnAudioSourceData(cbuffer);
    }
}


// ----------------------------------------------------------------------------
//                                 ID3Tag
// ----------------------------------------------------------------------------


#ifdef ID3_TAG_SUPPORT

ID3Tag::ID3Tag(const std::string& file) :
   mFilename(file)
{
   SetFile(mFilename);
}


void ID3Tag::SetFile(std::string file)
{
   if(file == "")
      return;

   TagLib::FileRef f(file.c_str());

   if(f.isNull() || f.tag()==nullptr)
      return;

   TagLib::Tag *tag = f.tag();

   std::stringstream num;

   mTitle  = tag->title().toCString();
   mArtist = tag->artist().toCString();
   mAlbum  = tag->album().toCString();
   num     << tag->year();
   mYear   = num.str();
   mGenre  = tag->genre().toCString();

   if(f.audioProperties())
   {
      TagLib::AudioProperties *properties = f.audioProperties();
      mDuration = properties->length();
      mChannels = properties->channels();
      mSampleRate =  properties->sampleRate();
   }
}

#endif

