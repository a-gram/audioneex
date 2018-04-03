/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

#ifndef AUDIO_ENCODER
#define AUDIO_ENCODER

#ifdef WIN32
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdint>
#include <string>

#include <libsndfile/sndfile.h>
#include <AudioBlock.h>

// DLL function pointers typedefs
typedef SNDFILE*    (*SF_OPEN)        (const char *path, int mode, SF_INFO *sfinfo) ;
typedef sf_count_t  (*SF_WRITE_FLOAT) (SNDFILE *sndfile, const float *ptr, sf_count_t items) ;
typedef sf_count_t  (*SF_WRITE_SHORT) (SNDFILE *sndfile, const short *ptr, sf_count_t items) ;
typedef int         (*SF_CLOSE)       (SNDFILE *sndfile) ;
typedef const char* (*SF_ERRORSTR)    (SNDFILE *sndfile) ;


/// This class implements a simple writer for audio data to files
/// using libsndfile.

class AudioFileWriter
{
  public:

      enum eFileType
      {
          WAV,
          AIFF,
          FLAC,
          MP3,
          UNKNOWN
      };

	  enum
	  {
		  SIGNED_8_BIT = 7,
		  UNSIGNED_8_BIT = 8,
		  SIGNED_16_BIT = 16,
		  SIGNED_24_BIT = 24,
		  SIGNED_32_BIT = 32,
		  NORMALIZED_FLOAT = 0xFFFF,
		  NORMALIZED_DOUBLE = NORMALIZED_FLOAT + 1
	  };

	  ///
      /// Encoding format structure
	  ///
      struct AudioFormat
	  {
         eFileType Format;
         uint32_t  SampleRate;
         uint32_t  SampleResolution;
         uint32_t  ChannelsCount;

         AudioFormat() : Format(WAV),
                         SampleRate(44100),
                         SampleResolution(16),
                         ChannelsCount(2)
         {}
      };

      AudioFileWriter(const AudioFormat &aformat);
      ~AudioFileWriter();

      void Open(std::string filename);
      void Close();
      std::int64_t Write(const float *buff, size_t nsamples);
      std::int64_t Write(const short *buff, size_t nsamples);

      template <typename T>
      std::int64_t Write(const AudioBlock<T> &buf);

private:

    // Encoders Plugin handles
#ifdef WIN32
    HINSTANCE hDLL_MP3;
    HINSTANCE hDLL_LIBSNDFILE;
#else
    void* hDLL_MP3;
    void* hDLL_LIBSNDFILE;
#endif

    AudioFormat mFormat;


    // ------------ LIBSNDFILE ------------

    SNDFILE*  mFile;

    SF_OPEN        sfOpen;
    SF_WRITE_FLOAT sfWriteFloat;
    SF_WRITE_SHORT sfWrite16bit;
    SF_ERRORSTR    sfError;
    SF_CLOSE       sfClose;

    // ------------------------------------

protected:

};


// --------------------------------------------------------
//                Templates implementations
// --------------------------------------------------------


template <typename T>
std::int64_t AudioFileWriter::Write(const AudioBlock<T> &buff)
{
    if(mFile==NULL)
       return 0;

    std::int64_t written = Write(buff.Data(), buff.Size());
    return written;
}


#endif


