/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifdef WIN32
 #include <Windows.h>
 #define LIBSNDFILE_MOD_NAME "libsndfile-1.dll"
#else
 #define LIBSNDFILE_MOD_NAME "libsndfile.so"
#endif

#include <cassert>
#include <stdexcept>

#include "AudioFileWriter.h"


AudioFileWriter::AudioFileWriter(const AudioFormat &aformat)
{
   // load the sound file library

#ifdef WIN32
   hDLL_LIBSNDFILE = LoadLibrary(TEXT(LIBSNDFILE_MOD_NAME));
#else
   hDLL_LIBSNDFILE = dlopen(LIBSNDFILE_MOD_NAME, RTLD_LAZY);
#endif

   if(hDLL_LIBSNDFILE == nullptr)
      throw std::runtime_error
      ("Couldn't load library "+std::string(LIBSNDFILE_MOD_NAME));

   // Get Interface functions from the DLL
#ifdef WIN32
   sfOpen	    = (SF_OPEN)        GetProcAddress(hDLL_LIBSNDFILE, "sf_open");
   sfWriteFloat	= (SF_WRITE_FLOAT) GetProcAddress(hDLL_LIBSNDFILE, "sf_write_float");
   sfWrite16bit	= (SF_WRITE_SHORT) GetProcAddress(hDLL_LIBSNDFILE, "sf_write_short");
   sfError      = (SF_ERRORSTR)    GetProcAddress(hDLL_LIBSNDFILE, "sf_strerror");
   sfClose      = (SF_CLOSE)       GetProcAddress(hDLL_LIBSNDFILE, "sf_close");
#else
   sfOpen	    = (SF_OPEN)        dlsym(hDLL_LIBSNDFILE, "sf_open");
   sfWriteFloat	= (SF_WRITE_FLOAT) dlsym(hDLL_LIBSNDFILE, "sf_write_float");
   sfWrite16bit	= (SF_WRITE_SHORT) dlsym(hDLL_LIBSNDFILE, "sf_write_short");
   sfError      = (SF_ERRORSTR)    dlsym(hDLL_LIBSNDFILE, "sf_strerror");
   sfClose      = (SF_CLOSE)       dlsym(hDLL_LIBSNDFILE, "sf_close");
#endif

   // Check if interface is correct
   assert(sfOpen != nullptr);
   assert(sfWriteFloat != nullptr);
   assert(sfWrite16bit != nullptr);
   assert(sfError != nullptr);
   assert(sfClose != nullptr);

   mFormat = aformat;

}

//------------------------------------------------------------------------------

void AudioFileWriter::Open(std::string filename)
{
   SF_INFO sfInfo = { };

   if(mFile) Close();

   if(mFormat.Format != MP3)
   {
       // Set file format
       if(mFormat.Format == WAV)
          sfInfo.format = SF_FORMAT_WAV;
       else if(mFormat.Format == AIFF)
          sfInfo.format = SF_FORMAT_AIFF;
       else if(mFormat.Format == FLAC)
          sfInfo.format = SF_FORMAT_FLAC;
       else
          throw std::invalid_argument
          ("Unsupported audio format");

       sfInfo.samplerate = mFormat.SampleRate;
       sfInfo.channels   = mFormat.ChannelsCount;

       // set sample resolution
       if(mFormat.SampleResolution == UNSIGNED_8_BIT)
           sfInfo.format |= SF_FORMAT_PCM_U8;
       else if(mFormat.SampleResolution == SIGNED_8_BIT)
           sfInfo.format |= SF_FORMAT_PCM_S8;
       else if(mFormat.SampleResolution == SIGNED_16_BIT)
           sfInfo.format |= SF_FORMAT_PCM_16;
       else if(mFormat.SampleResolution == SIGNED_24_BIT)
           sfInfo.format |= SF_FORMAT_PCM_24;
       else if(mFormat.SampleResolution == SIGNED_32_BIT)
           // NOTE: libsndfile FLAC only supports 8/16/24 bits
           sfInfo.format |= (mFormat.Format != FLAC ? SF_FORMAT_PCM_32 : SF_FORMAT_PCM_24);
       else if(mFormat.SampleResolution == NORMALIZED_FLOAT)
           sfInfo.format |= SF_FORMAT_FLOAT;
       else if(mFormat.SampleResolution == NORMALIZED_DOUBLE)
           sfInfo.format |= SF_FORMAT_DOUBLE;
       else
           throw std::invalid_argument
           ("Unsupported sample resolution");


       mFile = sfOpen(filename.c_str(), SFM_WRITE, &sfInfo);

       if(mFile == nullptr)
          throw std::runtime_error(sfError(mFile));

   }

}

//------------------------------------------------------------------------------

void AudioFileWriter::Close()
{
   sfClose(mFile);
}

//------------------------------------------------------------------------------

int64_t AudioFileWriter::Write(const float *buff, size_t nsamples)
{
    assert(mFile != nullptr);
    assert(sfWriteFloat != nullptr);

    int64_t written = sfWriteFloat(mFile, buff, nsamples);

    return written;

}

//------------------------------------------------------------------------------

int64_t AudioFileWriter::Write(const short* buff, size_t nsamples)
{
    assert(mFile != nullptr);
    assert(sfWrite16bit != nullptr);

    int64_t written = sfWrite16bit(mFile, buff, nsamples);

    return written;
}

//------------------------------------------------------------------------------

AudioFileWriter::~AudioFileWriter()
{
   // close file
   if(mFile)  Close();

   // unload plugins
#ifdef WIN32
   if(hDLL_LIBSNDFILE) FreeLibrary(hDLL_LIBSNDFILE);
   if(hDLL_MP3)        FreeLibrary(hDLL_MP3);
#else
   if(hDLL_LIBSNDFILE) dlclose(hDLL_LIBSNDFILE);
   if(hDLL_MP3)        dlclose(hDLL_MP3);
#endif
}

//------------------------------------------------------------------------------


