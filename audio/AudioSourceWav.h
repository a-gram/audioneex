/*
  Copyright (c) 2014, Alberto Gramaglia

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

*/

/// A simple audio source class to stream audio from WAV files

#include <iostream>
#include <fstream>
#include <cstdint>

#include "AudioBlock.h"

struct WavHeader {

    struct RIFF {
        char      ID[4];
        uint32_t  Size;
        char      Format[4];
    }
    RIFF;

    struct FMT {
        char     ID[4];
        uint32_t Size;
        uint16_t AudioFormat;
        uint16_t Channels;
        uint32_t SampleRate;
        uint32_t ByteRate;
        uint16_t BlockAlign;
        uint16_t BitsPerSample;
    }
    FMT;

    struct DATA {
        char     ID[4];
        uint32_t Size;
    }
    DATA;
};

class AudioSourceWavFile
{
    std::ifstream m_File;
    WavHeader     m_Header;
    size_t        m_AvailableData {0};
    size_t        m_Nsamples      {0};
    float         m_Duration      {0};
    float         m_Position      {0};

    bool IsTag(char* tag, char t1, char t2, char t3, char t4){
		
        return tag[0] == t1 &&
               tag[1] == t2 &&
               tag[2] == t3 &&
               tag[3] == t4;
    }

    bool IsValidWav(){
		
        bool isvalid = true;
		
        m_File.read(reinterpret_cast<char*>(&m_Header), sizeof(WavHeader));
		
        isvalid &= m_File.gcount() == sizeof(WavHeader);
        isvalid &= IsTag(m_Header.RIFF.ID,'R','I','F','F');
        isvalid &= IsTag(m_Header.RIFF.Format,'W','A','V','E');
        isvalid &= IsTag(m_Header.FMT.ID,'f','m','t',' ');
        isvalid &= m_Header.FMT.Size == 16;
        isvalid &= m_Header.FMT.AudioFormat == 1;
        isvalid &= m_Header.FMT.ByteRate == m_Header.FMT.SampleRate *
                                            m_Header.FMT.Channels *
                                            m_Header.FMT.BitsPerSample/8;
        isvalid &= m_Header.FMT.BlockAlign == m_Header.FMT.Channels *
                                              m_Header.FMT.BitsPerSample/8;
        isvalid &= IsTag(m_Header.DATA.ID,'d','a','t','a');

        return isvalid;
    }

 public:

    AudioSourceWavFile() = default;

    ~AudioSourceWavFile(){
         Close();
    }

    void Open(const std::string &filename){
		
         Close();
         m_File.open(filename, std::ios::in|std::ios::binary);
         if(!m_File.is_open())
            throw std::runtime_error("Couldn't open "+filename);
         if(!IsValidWav())
            throw std::runtime_error("Invalid WAV file "+filename);
         m_AvailableData = m_Header.DATA.Size;
         m_Nsamples = m_AvailableData / (m_Header.FMT.BitsPerSample/8);
         m_Duration = static_cast<float>(m_Nsamples / m_Header.FMT.Channels) /
                                         m_Header.FMT.SampleRate;
         m_Position = 0;
    }

    void Close(){
		
         if(m_File.is_open())
            m_File.close();
		
         m_AvailableData = 0;
         m_Position = 0;
         m_Duration = 0;
         m_Nsamples = 0;
    }

    void SetPosition(float time){
		
         size_t offset = static_cast<size_t>(time * m_Header.FMT.SampleRate) *
                                                   (m_Header.FMT.BitsPerSample/8) *
                                                    m_Header.FMT.Channels;
													
         offset = std::min<size_t>(offset, m_Header.DATA.Size);
         m_File.seekg (sizeof(WavHeader) + offset);
         m_AvailableData = m_Header.DATA.Size - offset;
         m_Position = offset < m_Header.DATA.Size ? time : m_Duration;
    }

    float GetPosition() const { return m_Position; }

    template <typename T>
    size_t Read(T* buffer, size_t nsamples){
		
         if(buffer && m_AvailableData && nsamples>0)
         {
            size_t nbytes = std::min(m_AvailableData, nsamples * sizeof(T));
            m_File.read(reinterpret_cast<char*>(buffer), nbytes);
            assert(m_File.gcount() == nbytes);
            m_AvailableData -= nbytes;
            m_Position = static_cast<float>( (m_Header.DATA.Size-m_AvailableData) /
                                             (m_Header.FMT.BitsPerSample/8) /
                                              m_Header.FMT.Channels) /
                                              m_Header.FMT.SampleRate;
            return nbytes / sizeof(T);
         }
         return 0;
    }

    template <class T>
    void Read(AudioBlock<T> &block){
		
         block.Resize( Read(block.Data(), block.Size()) );
    }

    int GetSampleRate() const { return m_Header.FMT.SampleRate; }
    int GetChannels()   const { return m_Header.FMT.Channels; }
    int GetSampleResolution() const { return m_Header.FMT.BitsPerSample; }
    float GetLenSeconds() const { return m_Duration; }
    size_t GetLenSamples() const { return m_Nsamples; }

};

